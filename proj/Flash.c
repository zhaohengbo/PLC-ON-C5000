//==========================================================================================
// Filename:		Flash.c
//
// Description:		Routines for managing flash access and code images
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// The flash is partitioned into distinct areas for different purposes of operation.
// 
// flash partitioning is:
//   0x000000 - 0x00007F => reserved
//   0x018000 - 0x01FFFF => active code image
//   0x028000 - 0x02FFFF => DSP code download image
//   0x038000 - 0x03FFFF => data logging space
//
// The DSP and MSP cannot have any ROM image existing at address zero so tags are placed
// there indicating status of images
// 0x010000 = DSP code tag address
//
// Tags are image status for active and erasable
//   0xFFFF => the image does not exist, set by default when an erasure occurs
//   0x1248 => the image has been downloaded and should be used to overwrite the current one
//   0x0000 => the image is unused and can be erased
// Note:  Since writing to flash can only be done when bits are changed from a 1 to a 0,
//        the tags are assigned values which allow each succesive value to be written on
//        top of the previous tag value.
//
//	The user code must have its entry point at 0x0080, NO EXCEPTIONS!
//
// Revision History:
//==========================================================================================

//==========================================================================================
// included files
//==========================================================================================
#include "ofdm_modem.h"
#include "ofdm_datatypes.h"
#include "ofdm_functions.h"
#include "intr.h"
#include "Flash.h"

//==========================================================================================
// local definitions for flash management
//==========================================================================================
#define FLASH_IMAGE_LOAD            (0x1248u)
#define FLASH_IMAGE_PROG            (0x028000ul)
#define FLASH_IMAGE_ACTIVE          (0x018000ul)
#define FLASH_IMAGE_BOOT			(0x00FF80ul)
#define FLASH_BOOT_CODE_TAG         (0x3CA5u)
#define FLASH_IMAGE_PROG_TAG_OFFSET (2)
#define FLASH_BOOT_CODE_TAG_OFFSET	(127)
#define FLASH_PROG_CODE_UPPER_LIMIT (0x7FFFul)
#define FLASH_PROG_CODE_LOWER_LIMIT (0x0080ul)
#define FLASH_LOG_BASE_ADDRESS		(0x038000ul)
#define FLASH_LOG_MAX_SIZE			(0x008000ul)
#define ERASED_FLASH_VALUE			(0xFFFFu)
#define DOWN_LOAD_TIMEOUT			(0x4000u)
#define LED_START					(0X80)
#define LED_UPDATE_RATE				(300)

//==========================================================================================
// exported global variables
//==========================================================================================
u16 FlashLEDMask = 0;										// used to output flash status to LED's
u32 FlashLogSize;											// indicates the current size of the data log in flash

//==========================================================================================
// Local function declarations
//==========================================================================================
void RunStateMachine(i16 Target, u32 Address, u16 Data, i16 Operation);
i16  GetBufferCharacter(i16 Position);
void DoEraseSector(i16 Target, u32 SectorAddress);
u16  ReadHexNumber(i16 Digits, i16* Position);
u16  Xlate(u16 Data);
u16  UnXlate(u16 Data);
i16  ValidateFlashModification(void);
void EnableFlashWriteAccess(void);
void DisableFlashWriteAccess(void);
i16  ValidateFlashComponent(void);
void WriteFlash(u32 Address, u16 Data);
u32  GetBaseAddress(i16 Target);
void UpdateStatus(i16 Target);
void ResetProcessor(void);

//==========================================================================================
// state machine function prototypes, each function is a different state
//==========================================================================================
void* SM_Idle(void);  
void* CommandPolling(void);
void* EraseCommandPolling(void);
void* EraseFlash(void);
void* EraseSector(void);
void* WriteFlashData(void);

//==========================================================================================
// globals local to this file hence the static prefix
//==========================================================================================
static u16   StateMachineStatus = FLASH_STATUS_COMPLETE;	// indicates status of the state machine
static void* CurrentState;									// holds current state of the state machine
static i16   TranslationMap[] = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};// for Xlate() and UnXlate()
static i16   EnableFlashWrites;								// a protection flag, must be set to do writes
static u16   FlashProgBuffer[16];							// buffer for DSP program image writes
static u16   FlashLogBuffer[FLASH_LOG_RECORD_MAX_SIZE + 2];	// buffer for flash log writes
static u16   FlashBootBuffer[1];							// the buffer for boot sector writes
static u16   FlashCopyBuffer[1];							// the buffer for copying flash to flash
static u16*  FlashBuffer[FLASH_TARGET_COUNT] = { FlashProgBuffer, 0/* MSP placeholder */, FlashLogBuffer, FlashBootBuffer, FlashCopyBuffer };// pointers to the buffers
static i16   FlashSize[FLASH_TARGET_COUNT];					// size of data in buffers, zero indicates buffer is not busy, negative indicates erase
static i16   WriteTag[FLASH_TARGET_COUNT];					// flags to indicate needed update of tags for images
static i16   CurrentTarget;									// points to the currently active target of the state machine
static u32   FlashLoadAddress[FLASH_TARGET_COUNT];			// current address for current buffer data
static u16	 DownLoadActive;
static u16   DisplayMode;
static u16   Dir;
static u16   TargetStatus[FLASH_TARGET_COUNT];
static struct 
{
	u16  Data;
	u32 Address;
}	StateData;

//==========================================================================================
// Function:		ResetProcessor()		
//
// Description:		Sets the PMST register similar to that of a hard reset and
//                  jumps to the reset vector at 0xFF80.  This function never returns
//
// Revision History:
//==========================================================================================
void ResetProcessor(void)
{
	INTR_GLOBAL_DISABLE();	// Disable Global Interrupts (if they were enabled)

	asm(" STM 0xFFE0, 0x1D");	// reset the PMST register
	asm(" NOP"); 			// wait for changed to take effect
	asm(" NOP"); 
	asm(" NOP"); 
	asm(" NOP"); 
	asm(" NOP"); 
	asm(" B   0xFF80");		// jump to reset vector
}

//==========================================================================================
// Function:		InitFlash()		
//
// Description:		initializes globals and initial state, checks for flash operation
//                  and updates MSP code if needed base on MSP tag value
//
// Revision History:
//==========================================================================================
void InitFlash(void)
{
	u32 j;
	i16 i;
	u16 BootCode[] = {										// the boot code, copies flash to RAM and executes
//	/* 0xFF80 */ 0xF073u,	// FF80: B FF80					; branch to self, debug bring up
//	/* 0xFF81 */ 0xFF80u,
	/* 0xFF80 */ 0xF495u,	// FF80: NOP					; non debug bring up
	/* 0xFF81 */ 0xF495u,	// FF81: NOP
	/* 0xFF82 */ 0x771Du,	// FF82: STM FFE8h, PMST		; set PMST to overlay RAM in program space
	/* 0xFF83 */ 0xFFE8u,
	/* 0xFF84 */ 0xF495u,	// FF84: NOP
	/* 0xFF85 */ 0xF495u,	// FF85: NOP
	/* 0xFF86 */ 0xF495u,	// FF86: NOP
	/* 0xFF87 */ 0xF495u,	// FF87: NOP
	/* 0xFF88 */ 0xF495u,	// FF88: NOP
	/* 0xFF89 */ 0xF495u,	// FF89: NOP
	/* 0xFF8A */ 0xF495u,	// FF8A: NOP
	/* 0xFF8B */ 0x7708u,	// FF8B: STM 	0, 8h			; point accumulator A to downloaded program image tag
	/* 0xFF8C */ 0x0000u,
	/* 0xFF8D */ 0x7709u,	// FF8D: STM 	0, 9h
	/* 0xFF8E */ 0x0000u,
	/* 0xFF8F */ 0x770Au,	// FF8F: STM 	0, ah
	/* 0xFF90 */ 0x0000u,
	/* 0xFF91 */ 0xF3C0u,	// FF91: XOR 	B, B			; clear accumulator B
	/* 0xFF92 */ 0x7E0Bu,	// FF92: READA 	bh				; read the tag value into lower word of accumulator B
	/* 0xFF93 */ 0xF310u,	// FF93: SUB 	#0, B			; check tag for active download
	/* 0xFF94 */ 0x0000u,
	/* 0xFF95 */ 0xF84Du,	// FF95: BC		LOAD_NEW, BEQ	; if an active tag, load the new image
	/* 0xFF96 */ 0xFF9Fu,
	/* 0xFF97 */ 0x7708u,	// FF97: STM 	0, 8h		 	; load address of old image
	/* 0xFF98 */ 0x0000u,
	/* 0xFF99 */ 0x7709u,	// FF99: STM 	0, 9h
	/* 0xFF9A */ 0x0000u,
	/* 0xFF9B */ 0x770Au,	// FF9B: STM 	0, ah
	/* 0xFF9C */ 0x0000u,
	/* 0xFF9D */ 0xF073u,	// FF9D: B 		DO_LOAD			; start loading the old image
	/* 0xFF9E */ 0xFFA5u,
							//      LOAD_NEW
	/* 0xFF9F */ 0x7708u,	// FF9F: STM	0, 8h			; load address of new image
	/* 0xFFA0 */ 0x0000u,
	/* 0xFFA1 */ 0x7709u,	// FFA1: STM	0, 9h
	/* 0xFFA2 */ 0x0000u,
	/* 0xFFA3 */ 0x770Au,	// FFA3: STM	0, ah
	/* 0xFFA4 */ 0x0000u,
							//      DO_LOAD
	/* 0xFFA5 */ 0x7711u,	// FFA5: STM #80h, AR1			; initialize ar1 to point to beginning of ram too
	/* 0xFFA6 */ 0x0080u,
	/* 0xFFA7 */ 0xF495u,	// FFA7: NOP					; stall for debugging
	/* 0xFFA8 */ 0xF495u,	// FFA8: NOP
	/* 0xFFA9 */ 0xF070u,	// FFA9: RPT #7F7Fh				; load number of words to copy from flash - 1
	/* 0xFFAA */ 0x0000u,
	/* 0xFFAB */ 0x7E91u,	// FFAB: READA *AR1+			; copy from flash (in acc a) to data space (in ar1)
	/* 0xFFAC */ 0xF495u,	// FFAC: NOP
	/* 0xFFAD */ 0xF495u,	// FFAD: NOP
	/* 0xFFAE */ 0xF495u,	// FFAE: NOP
	/* 0xFFAF */ 0xF495u,	// FFAF: NOP
	/* 0xFFB0 */ 0xF495u,	// FFB0: NOP
	/* 0xFFB1 */ 0x771Du,	// FFB1: STM 00E8h,PMST			; set PMST to execute from RAM also
	/* 0xFFB2 */ 0x00E8u,
	/* 0xFFB3 */ 0xF495u,	// FFB3: NOP
	/* 0xFFB4 */ 0xF495u,	// FFB4: NOP
	/* 0xFFB5 */ 0xF495u,	// FFB5: NOP
	/* 0xFFB6 */ 0xF495u,	// FFB6: NOP
	/* 0xFFB7 */ 0xF495u,	// FFB7: NOP
	/* 0xFFB8 */ 0xF495u,	// FFB8: NOP
	/* 0xFFB9 */ 0xF495u,	// FFB9: NOP
	/* 0xFFBA */ 0xF073u,	// FFBA: B 0x0080				; branch to user code at user reset vector
	/* 0xFFBB */ 0x0080u };
	u16 BootCodeLength = 0xFFBBu - 0xFF80u + 1;				// number of instruction words in boot code

	// patch the download image tag address into the assembly code
	BootCode[12] = (u16)((FLASH_IMAGE_PROG + FLASH_IMAGE_PROG_TAG_OFFSET) & 0xFFFFul);
	BootCode[14] = (u16)(((FLASH_IMAGE_PROG + FLASH_IMAGE_PROG_TAG_OFFSET) >> 16) & 0x007Ful);
	BootCode[16] = 0u;

	// patch the tag value for loading into the assembly code
	BootCode[20] = (u16)(FLASH_IMAGE_LOAD);

	// patch the active image base address into the assembly code
	BootCode[24] = (u16)((FLASH_IMAGE_ACTIVE + FLASH_PROG_CODE_LOWER_LIMIT) & 0xFFFFul);
	BootCode[26] = (u16)(((FLASH_IMAGE_ACTIVE + FLASH_PROG_CODE_LOWER_LIMIT) >> 16) & 0x007Ful);
	BootCode[28] = 0u;

	// patch the new image base address into the assembly code
	BootCode[32] = (u16)((FLASH_IMAGE_PROG + FLASH_PROG_CODE_LOWER_LIMIT) & 0xFFFFul);
	BootCode[34] = (u16)(((FLASH_IMAGE_PROG + FLASH_PROG_CODE_LOWER_LIMIT) >> 16) & 0x007Ful);
	BootCode[36] = 0u;

	// patch the download count
	BootCode[42] = (u16)(FLASH_PROG_CODE_UPPER_LIMIT - FLASH_PROG_CODE_LOWER_LIMIT);

	DownLoadActive = 0;										// clear the downloading flag
	Dir = 0;												// initialize direction of indicators
	FlashLEDMask = 0;										// initialize the indicator
	
	for(i = 0; i < FLASH_TARGET_COUNT; i++)					// loop through all targets clearing state data
	{
		FlashLoadAddress[i] = 0;
		FlashSize[i]        = 0;
		WriteTag[i]         = 0;
		TargetStatus[i]     = 0;
	}

	CurrentTarget = 0;										// initialize target counter
	CurrentState = SM_Idle;									// initialize the state machine state
  
	if(ValidateFlashComponent() != 0)						// a quick check to believe everything is OK
	{
		FlashLEDMask = 0xFF;								// indicate failure
		AssignLEDs(FlashLEDMask);							// forces output
		for(;;);											// lock up the system
	}

// FORCE ERASE -- if the following line of code is uncommented, the flash will be erased on boot
//	RunStateMachine(FLASH_TARGET_BOOT, 0, ERASED_FLASH_VALUE, -1);	// erase the flash

	for(j = 0; j < BootCodeLength; j++)						// loop through all boot code
	{
		if(BootCode[j] != ReadFlash(0xFF80u + j))			// if code word fails to compare with expected
		{
			RunStateMachine(FLASH_TARGET_BOOT, FLASH_BOOT_CODE_TAG_OFFSET, 0, 1);	// invalidate the boot tag
			break;											// exit the loop
		}
	}
															// if boot code appears corrupted or missing
	if(ReadFlash(GetBaseAddress(FLASH_TARGET_BOOT) + FLASH_BOOT_CODE_TAG_OFFSET) != FLASH_BOOT_CODE_TAG)
	{
		RunStateMachine(FLASH_TARGET_BOOT, 0, ERASED_FLASH_VALUE, -2);	// erase the boot sector
  
		// copy the boot code to flash
		for(j = 0; j < BootCodeLength; j++)					// loop through all boot code
		{													// copy this word of the boot code
			RunStateMachine(FLASH_TARGET_BOOT, j, BootCode[j], 1);
		}
    
															// update the boot code tag
		RunStateMachine(FLASH_TARGET_BOOT, FLASH_BOOT_CODE_TAG_OFFSET, FLASH_BOOT_CODE_TAG, 1);
	}
															// if need to update DSP code image
	if(ReadFlash(GetBaseAddress(FLASH_TARGET_PROG) + FLASH_IMAGE_PROG_TAG_OFFSET) == FLASH_IMAGE_LOAD)
	{
		// erase the DSP active sector
		RunStateMachine(FLASH_TARGET_COPY, 0, ERASED_FLASH_VALUE, -2);	// erase this sector
    
		// copy the DSP code to low flash
		for(j = FLASH_PROG_CODE_LOWER_LIMIT; j <= FLASH_PROG_CODE_UPPER_LIMIT; j++)	// loop through all code space
		{													// copy this word of the code image
			RunStateMachine(FLASH_TARGET_COPY, j, ReadFlash(GetBaseAddress(FLASH_TARGET_PROG) + j), 1);
		}

		// update the DSP code image tag
		RunStateMachine(FLASH_TARGET_PROG, FLASH_IMAGE_PROG_TAG_OFFSET, 0, 1);
		ResetProcessor();									// run the newly loaded code
	}
															// if need to update MSP code image
	// by definition, the download image for the DSP will have been cleared by this point  
	// initialize flash log size variable by running through the linked list
	FlashLogSize = 0;										// point to beginning of log records list
	do														// loop through log records
	{
		j = ReadFlash(FlashLogSize + FLASH_LOG_BASE_ADDRESS);	// get high word of succeeding record pointer
		j <<= 16;											// move it into the high word
		j += ReadFlash(FlashLogSize + FLASH_LOG_BASE_ADDRESS + 1);	// get low word of succeeding record pointer
 
		if(j != 0xFFFFFFFFul)								// if not at end of linked list
		{
			if(j <= FlashLogSize || j >= FLASH_LOG_MAX_SIZE)	// if the linked list appears corrupt
			{
				uFlashLogStatus = FLASH_STATUS_TIMEOUT;		// set the log status
				break;										// exit the loop
			}
			FlashLogSize = j;								// move to the next record in the list
		}

	} while(j < FLASH_LOG_MAX_SIZE);						// while log records exist
 
	return;													// exit
	}

//==========================================================================================
// Function:		RunStateMachine()		
//
// Description:		Runs the state machine in a dedicated manner until it achieves an idle
//					state.
//
// Revision History:
//==========================================================================================
void RunStateMachine(i16 Target, u32 Address, u16 Data, i16 Operation)
{
	DownLoadActive = 5;										// allow the lights to work
	if(FlashLEDMask == 0)									// if the led's are not already on
	{
		FlashLEDMask = LED_START;							// initialize them
	}

	EnableFlashWriteAccess();								// allow access to the flash
	FlashLoadAddress[Target] = Address;						// assign address to write data to
	FlashBuffer[Target][0] = Data;							// get data to copy
	FlashSize[Target] = Operation;							// indicate operation needed
	do
	{
		FlashStateMachine();								// clock the state machine
	} while(CurrentState != SM_Idle || FlashLEDMask != 0);	// until it is completely idle
	return;													// operation complete, exit
}

//==========================================================================================
// Function:		SM_Idle()		
//
// Description:		The idle state for the flash state machine.
//
//					Checks in rotation each flash target to see if there are any flash
//					commands which need to be executed and if so executes them by setting
//					the appropriate state.
//
//					Commands are recognized by a non-zero count in the FlashSize global
//                  variable.  If the value is positive then there is data in the FlashBuffer
//                  variable to write to flash.  If the value is -1 then the flash needs to
//                  be erased.  If the value is less than -1 then a sector needs to be
//                  ersed.
//
//					When no commands are pending and no error exists, this routine is
//					responsible for setting the StateMachineStatus to FLASH_STATUS_COMPLETE.
//
// Revision History:
//==========================================================================================
void *SM_Idle(void)
{
	i16 i;

	for(i = 0; i < FLASH_TARGET_COUNT; i++)					// loop through all targets at least once
	{
		CurrentTarget++;									// bump pointer to next target
		if(CurrentTarget >= FLASH_TARGET_COUNT)				// if we have overflowed the number of targets
		{
			CurrentTarget = 0;								// reset the counter
		}

		if(FlashSize[CurrentTarget] != 0)					// if this target is active
		{
			TargetStatus[CurrentTarget] = 1;				// indicate the target is active
			if(FlashSize[CurrentTarget] > 0)				// if writing to flash
			{
				StateData.Address = GetBaseAddress(CurrentTarget);	// get pointer to appropriate image
				StateData.Address += FlashLoadAddress[CurrentTarget];	// get base address of line to write to
				StateData.Address += FlashSize[CurrentTarget] - 1;	// update address to write to, we write from end of buffer forward
				FlashSize[CurrentTarget]--;					// update the pointer
				StateData.Data = FlashBuffer[CurrentTarget][FlashSize[CurrentTarget]];	// get next (from end) word to write
				return WriteFlashData;
			}
			else											// if erasing flash or sectors
			{
				if(FlashSize[CurrentTarget] == -1)			// if erasing all of flash
				{
					StateData.Data    = 0xFFFFu;			// indicate expected data after erase
					StateData.Address = GetBaseAddress(CurrentTarget);	// get pointer to appropriate image

					return EraseFlash;						// switch to erasing flash state
				}
				else										// if erasing a sector
				{
					StateData.Address = GetBaseAddress(CurrentTarget);	// get pointer to appropriate image
					StateData.Address += FlashLoadAddress[CurrentTarget];	// indicate sector to erase
					StateData.Data = 0xFFFFu;				// indicate expected data after erase

					return EraseSector;						// switch to erasing sector state
				}
			}
		}
		else												// if nothing to write
		{
			if(WriteTag[CurrentTarget] != 0)				// check to see if a tag needs writing
			{
				StateData.Address = GetBaseAddress(CurrentTarget);	// assign image tag address
				switch(CurrentTarget)
				{
					case FLASH_TARGET_PROG:					// if working with DSP code
						StateData.Address += FLASH_IMAGE_PROG_TAG_OFFSET;	// add in offset
						break;
				}
				StateData.Data = FLASH_IMAGE_LOAD;			// assign tag value
				WriteTag[CurrentTarget] = 0;				// clear the write tag flag
				return WriteFlashData;
			}
		}

		TargetStatus[CurrentTarget] = 0;					// if we made it here, this target is no longer active
	}

	if(StateMachineStatus < FLASH_STATUS_TIMEOUT)			// if no errors occurred
	{
		StateMachineStatus = FLASH_STATUS_COMPLETE;			// set status to complete 
		UpdateStatus(CurrentTarget);						// update appropriate status
		DisableFlashWriteAccess();							// disallow flash access since all work is done
	}

	if(DownLoadActive == 0)									// if not in an active download
	{
		FlashLEDMask = 0;									// make sure the LEDs are off
	}
	else													// if in an active download
	{
		DownLoadActive--;									// decrement the flag
	}
	return SM_Idle;											// stay in idle state
}

//==========================================================================================
// Function:		FlashStateMachine()		
//
// Description:		The flash state machine handler function.
//
//					Holds the current state in the CurrentState variable.  Each time a
//					state is executed, that state returns the next state to execute which
//					could be itself or another state depending upon what its inputs are.
//
//					This routine is called repeatedly from the main loop thus the state
//                  machine is always active but may simply be idle.
//
// Revision History:
//==========================================================================================
void FlashStateMachine(void)
{
	Set_XF(XF_FLASH_SM);

	if(DownLoadActive != 0 && FlashLEDMask == 0)			// if supposed to be showing status
	{
		FlashLEDMask = LED_START;							// turn on the lights
		Dir = 0;											// set the direction to start
	}
	
	if(FlashLEDMask != 0 && uFlashShow > LED_UPDATE_RATE)	// if need to update light status
	{
		uFlashShow = 0;										// reset the counter
		if(DisplayMode != 0)								// if in erasure mode
		{
			if((FlashLEDMask & 0xFF) == 0 || (FlashLEDMask & 0xFF) > 0x80)	// if an invalid display code found
			{
				FlashLEDMask = 1;							// reinitialize the display mask
				Dir = 1;									// reinitialize the direction
			}
			if((FlashLEDMask & 0xFF) == 1 && Dir == 0)		// if at limit and direction needs to change
			{
				Dir = 1;									// change direction
			}
			if((FlashLEDMask & 0xFF) == 0x80 && Dir != 0)	// if at limit and direction needs to change
			{
				Dir = 0;									// change direction
			}
			if(Dir == 0)									// if going in right to left direction
			{
				FlashLEDMask >>= 1;							// move light one bit right
			}
			else
			{												// if going in left to right direction
				FlashLEDMask <<= 1;							// move light one bit left
			}
		}
		else												// if in erasing mode
		{
			if((FlashLEDMask & 0x81) == 0x81 && (FlashLEDMask & 0xFF) != 0xFF) // if an invalid display code found
			{
				FlashLEDMask = 0xFF;						// reinitialize the display mask
			}
			if((FlashLEDMask & 1) != 1)						// if filling from the left
			{
				FlashLEDMask |= 0x100;						// add it fill bit
			}
			FlashLEDMask >>= 1;								// shift the lights left
		}
		AssignLEDs(FlashLEDMask);							// force output
	}

	CurrentState = ((void * (*)(void)) CurrentState)();		// execute the current state
	Clear_XF(XF_FLASH_SM);
	return;
}

//==========================================================================================
// Function:		Xlate()		
//
// Description:		Translates data to match pin labels on the flash part since they
//					are swapped on the board with respect to the DSP pin labels.
//
//					This is necessary when writing commands to the flash but not data.
//
// Revision History:
//==========================================================================================
u16 Xlate(u16 Data)
{
	u16 ReturnValue;
	i16 Index;
	u16 temp;

	ReturnValue = 0;										// initialize return value
	for(Index = 0; Index < 16; Index++)						// loop through all data bits
	{
		temp = (Data >> Index) & 1;							// get current bit we are working on
		temp <<= TranslationMap[Index];						// move the bit to translated position
		ReturnValue |= temp;								// add in the bit
	}
	return ReturnValue;										// return translated value
}

//==========================================================================================
// Function:		UnXlate()		
//
// Description:		Un-Translates flash data to match pin labels on the DSP since the data
//					pins are swapped on the board with respect to the DSP pin labels.
//
//					This is necessary when reading command status from the flash but not
//					for data.
//
// Revision History:
//==========================================================================================
u16 UnXlate(u16 Data)
{
	u16 ReturnValue;
	i16 Index;
	u16 temp;

	ReturnValue = 0;										// initialize return value
	for(Index = 0; Index < 16; Index++)						// loop through all data bits
	{
		temp = Data >> TranslationMap[Index];				// shift the data appropriately
		temp &= 1;											// get current bit we are working on
		ReturnValue |= temp << Index;						// add in the bit
	}
	return ReturnValue;										// return translated value
}

//==========================================================================================
// Function:		CommandPolling()		
//
// Description:		Polls the flash to see if the most recently issued write command is
//					complete.
//					
//					Returns next state in the state machine which could be itself if the
//					command is not complete.  Also checks for errors and sets
//					StateMachineStatus appropriately.
//
//					If the data being written is the last line of a file, denoted by
//					WriteTag of the current target being 1, moves to the SetTag state of
//					state machine to update image tags as necessary.
//
//					Refer to the flash specification sheet for polling algorithm.
//
// Revision History:
//==========================================================================================
void* CommandPolling(void)
{
	u16 Status;
	
	DisplayMode = 0;										// indicate we are writing flash
	
	Status = ReadFlash(StateData.Address);					// read flash status
															// if translated bit 7 matches what was written
	if((Status & Xlate(0x0080u)) == (StateData.Data & Xlate(0x0080u)))
	{
		if(ReadFlash(StateData.Address) != StateData.Data)	// if verification fails
		{
			StateMachineStatus = FLASH_STATUS_VERIFY_FAIL;	// set error
			UpdateStatus(CurrentTarget);					// update appropriate status
			FlashSize[CurrentTarget] = 0;					// clear out the status
			return SM_Idle;									// return to idle loop
		}

		if(FlashSize[CurrentTarget] > 0)					// if still writes left to do
		{
			StateData.Address--;							// move to next address to write
			FlashSize[CurrentTarget]--;						// update the pointer
			StateData.Data = FlashBuffer[CurrentTarget][FlashSize[CurrentTarget]];	// get next word
			return WriteFlashData;							// write this word
		}

		return SM_Idle;										// go to idle state, will update flash status for us
	}

	if((Status & Xlate(0x0020u)) == 0)						// if write command not finished
	{
		return CommandPolling;								// stay in command polling state
	}

	Status = ReadFlash(StateData.Address);					// read flash status
															// if translated bit 7 matches what was written
	if((Status & Xlate(0x0080u)) == (StateData.Data & Xlate(0x0080u)))
	{
		if(ReadFlash(StateData.Address) != StateData.Data)	// if verification fails
		{
			StateMachineStatus = FLASH_STATUS_VERIFY_FAIL;	// set error
			UpdateStatus(CurrentTarget);					// update appropriate status
			FlashSize[CurrentTarget] = 0;					// clear out the status
			return SM_Idle;									// return to idle loop
		}

		if(FlashSize[CurrentTarget] > 0)					// if still writes left to do
		{
			StateData.Address--;							// move to next address to write
			FlashSize[CurrentTarget]--;						// update the pointer
			StateData.Data = FlashBuffer[CurrentTarget][FlashSize[CurrentTarget]];	// get next word
			return WriteFlashData;							// write this word
		}

		return SM_Idle;										// go to idle state, will update flash status for us
	}

	WriteFlash(StateData.Address, Xlate(0x00F0));			// write reset command
	StateMachineStatus = FLASH_STATUS_WRITE_FAIL;			// indicate failed status
	UpdateStatus(CurrentTarget);							// update appropriate status
  
	return SM_Idle;											// exit
}

//==========================================================================================
// Function:		EraseCommandPolling()		
//
// Description:		Polls the flash to see if the most recently issued erase command is
//					complete.
//					
//					Returns next state in the state machine which could be itself if the
//					command is not complete.  Also checks for errors and sets
//					StateMachineStatus appropriately.
//
//					If a complete flash erase was issued, the returned state is				
//					WriteBootCode which rewrites the boot code into the reset vectors of
//					flash for proper loading of the correct image.
//
//					Refer to the flash specification sheet for polling algorithm.
//
// Revision History:
//==========================================================================================
void* EraseCommandPolling(void)
{
	u16 Status;
  
	DisplayMode = 1;										// indicate we are erasing flash

	Status = ReadFlash(StateData.Address);					// read flash status
															// if translated bit 7 matches what was written
	if((Status & Xlate(0x0080u)) == (StateData.Data & Xlate(0x0080u)))
	{
		FlashSize[CurrentTarget] = 0;						// clear our command status
		return SM_Idle;										// have the idle state update our status
	}

	if((Status & Xlate(0x0020u)) == 0)						// if erase command not completed
	{
		return EraseCommandPolling;							// stay in this state
	}

	Status = ReadFlash(StateData.Address);					// read flash status
															// if translated bit 7 matches what was written
	if((Status & Xlate(0x0080u)) == (StateData.Data & Xlate(0x0080u)))
	{
		FlashSize[CurrentTarget] = 0;						// clear our command status
		return SM_Idle;										// have the idle state update our status
	}

	WriteFlash(StateData.Address, Xlate(0x00F0));			// write reset command
	StateMachineStatus = FLASH_STATUS_ERASE_FAIL;			// update status
	UpdateStatus(CurrentTarget);							// update appropriate status

	return SM_Idle;											// let idle state update our status
}

//==========================================================================================
// Function:		EraseFlash()		
//
// Description:		Issues an erase flash command to the flash and sets the next state
//					to EraseCommandPolling to monitor the progress.
//
// Revision History:
//==========================================================================================
void* EraseFlash(void)
{
	// begin chip erase procedure
	WriteFlash(0x8555ul, Xlate(0x00AAu));					// write first unlock command
	WriteFlash(0x82AAul, Xlate(0x0055u));					// write second unlock command
	WriteFlash(0x8555ul, Xlate(0x0080u));					// write setup command
	WriteFlash(0x8555ul, Xlate(0x00AAu));					// write third unlock command
	WriteFlash(0x82AAul, Xlate(0x0055u));					// write fourth unlock command
	WriteFlash(0x8555ul, Xlate(0x0010u));					// write chip erase command

	return EraseCommandPolling;								// move to erase command polling state
}

//==========================================================================================
// Function:		EraseSector()		
//
// Description:		Issues an erase sector command to the flash and sets the next state
//					to EraseCommandPolling to monitor the progress.
//
// Revision History:
//==========================================================================================
void* EraseSector(void)
{
	// begin sector erase procedure
	WriteFlash(0x8555ul, Xlate(0x00AAu));					// write first unlock command
	WriteFlash(0x82AAul, Xlate(0x0055u));					// write second unlock command
	WriteFlash(0x8555ul, Xlate(0x0080u));					// write setup command
	WriteFlash(0x8555ul, Xlate(0x00AAu));					// write third unlock command
	WriteFlash(0x82AAul, Xlate(0x0055u));					// write fourth unlock command
	WriteFlash(StateData.Address, Xlate(0x0030u));			// write sector erase command

	return EraseCommandPolling;								// move to erase command polling state
}

//==========================================================================================
// Function:		GetBaseAddress()		
//
// Description:		returns the base address of the appropriate code image or the beginning
//					of flash if not working on a code type of target
//
// Revision History:
//==========================================================================================
u32 GetBaseAddress(i16 Target)
{
	u32 rtn;
	switch(Target)											// select target
	{
		case FLASH_TARGET_PROG:								// if working with DSP code
			rtn = FLASH_IMAGE_PROG;							// return pointer to DSP image
			break;
		case FLASH_TARGET_LOG:								// if working with log data
			rtn = FLASH_LOG_BASE_ADDRESS;					// return pointer tolog space
			break;
		case FLASH_TARGET_BOOT:								// if working with the boot code
			rtn = FLASH_IMAGE_BOOT;							// return pointer to boot space
			break;
		case FLASH_TARGET_COPY:								// if working on flash copy
			rtn = FLASH_IMAGE_ACTIVE;						// return pointer to copy space
			break;
	}

	return rtn;												// give'm what they wanted
}

//==========================================================================================
// Function:		WriteFlashData()		
//
// Description:		Issues a single write command to the flash.  The data to be written and
//					the address to write it at is located in the StateData global structure.
//
//					Before the write command is issued, the flash is validated to make sure
//					the data pattern can actually be written since bits can only be cleared
//					during a flash write.
//
// Revision History:
//==========================================================================================
void* WriteFlashData(void)
{
	// if pattern cannot be written to flash
	if((ReadFlash(StateData.Address) & StateData.Data) != StateData.Data)
	{
		StateMachineStatus = FLASH_STATUS_NOT_ERASED;		// indicate data pattern invalid
		UpdateStatus(CurrentTarget);						// update appropriate status
		return SM_Idle;
	}

	WriteFlash(0x8555ul, Xlate(0x00AAu));					// write first unlock command
	WriteFlash(0x82AAul, Xlate(0x0055u));					// write second unlock command
	WriteFlash(0x8555ul, Xlate(0x00A0u));					// write program word command
	WriteFlash(StateData.Address, StateData.Data);			// write the word
	return CommandPolling;									// move to command polling state
}

//==========================================================================================
// Function:		ValidateFlashModification()		
//
// Description:		validates the EnableFlashWrites global variable has been written with
//                  the appropriate authorization code to allow flash writes to succeed.
//
// Revision History:
//==========================================================================================
i16 ValidateFlashModification(void)
{
	if(EnableFlashWrites != 0x27B3)							// if somebody did not enable the flash
	{
		StateMachineStatus = FLASH_STATUS_NO_MODIFY;
		UpdateStatus(CurrentTarget);						// update appropriate target status
		return StateMachineStatus;							// indicate the error
	}
	return 0;												// return status OK
}

//==========================================================================================
// Function:		EnableFlashWriteAccess()		
//
// Description:		Writes the authorization code to EnableFlashWrites global variable
//                  allowing flash write to succeed.
//
// Revision History:
//==========================================================================================
void EnableFlashWriteAccess(void)
{
	EnableFlashWrites = 0x27B3;
	return;
}

//==========================================================================================
// Function:		DisableFlashWriteAccess()		
//
// Description:		Clears the EnableFlashWrites global variable so flash writes will
//                  not succeed.
//
// Revision History:
//==========================================================================================
void DisableFlashWriteAccess(void)
{
	EnableFlashWrites = 0;
	return;
}

//==========================================================================================
// Function:		ValidateFlashComponent()		
//
// Description:		Checks to see the flash is a recognizable part which guarantees these
//                  routines will work with it properly.
//
// Revision History:
//==========================================================================================
i16 ValidateFlashComponent(void)
{
	u16 ManufacturerID;
	u16 DeviceID;

	EnableFlashWriteAccess();								// enable access to the flash

	WriteFlash(0x8100ul, Xlate(0x00F0));					// write reset command
	WriteFlash(0x8555ul, Xlate(0x00AAu));					// write first unlock command
	WriteFlash(0x82AAul, Xlate(0x0055u));					// write second unlock command
	WriteFlash(0x8555ul, Xlate(0x0090u));					// write autoselect command

	ManufacturerID = UnXlate(ReadFlash(0x8100));			// read who made it
	DeviceID       = UnXlate(ReadFlash(0x8101));			// read what it is
	WriteFlash(0x8100, Xlate(0x00F0));						// write reset command

	DisableFlashWriteAccess();								// disable access to the flash

	if((ManufacturerID & 0xFF) != 0x01)						// if an invalid manufacturing ID
	{
		return FLASH_STATUS_MANUFACTURER;					// return failed status
	}
	else													// if an AMD flash
	{
		if(DeviceID != 0x22BA)								// if incorrect part number for AMD flash
		{
			return FLASH_STATUS_PART_NO;					// return failed status
		}
	}

	return 0;												// return good status
}

//==========================================================================================
// Function:		WriteFlash()		
//
// Description:		Writes Data to Address in flash.
//                  The write is accomplished by setting up a single DMA transfer.
//					The DMA can access flash memory space independent of how the memory
//					spaces are defined by the OVLY and MP/MC bits thus eliminating the need
//					to manage swapping RAM in and out.
//
// Revision History:
//==========================================================================================
void WriteFlash(u32 Address, u16 Data)
	{
	if(ValidateFlashModification() != 0)					// if access not allowed
	{
		return;
	}

	FarWrite(Address, Data);

	return;
}

//==========================================================================================
// Function:		ReadFlash()		
//
// Description:		Reads from flash at Address.
//                  The read is accomplished by setting up a single DMA transfer.
//					The DMA can access flash memory space independent of how the memory
//					spaces are defined by the OVLY and MP/MC bits thus eliminating the need
//					to manage swapping RAM in and out.
//
// Revision History:
//==========================================================================================
u16 ReadFlash(u32 Address)
{
	return FarRead(Address);
}

//==========================================================================================
// Function:		GetBufferCharacter()		
//
// Description:		Reads a nibble from the input buffer at the nibble offset specified
//					by Position and returns it
//
// Revision History:
//==========================================================================================
i16 GetBufferCharacter(i16 Position)
{
	u16 Character = uUartDataArray[Position >> 2];			// get word position in buffer
	i16 Shift = (3 - (Position & 0x03)) * 4;				// calculate shift distance

	return (Character >> Shift) & 0xFu;						// return value of this nibble
}

//==========================================================================================
// Function:		FlashErase()		
//
// Description:		Called to initiate an erasure of some part of the flash.
//
//					If the target is a code image, the code sector image is erased.
//					If the target is in user space, only the sector containing
//					the address specified in FlashLoadAddress of the target is erased.
//
// Revision History:
//==========================================================================================
u16 FlashErase(i16 Target)
{
	u16 i;

	if(FlashLogSize != 0)									// if area currently used by flash log
	{
		return FLASH_STATUS_IN_USE;							// tell the user
	}
  
  	DownLoadActive = DOWN_LOAD_TIMEOUT;						// indicate the lights can come on
															// if the boot code has not been loaded or appears corrupt
	if(ReadFlash(GetBaseAddress(FLASH_TARGET_BOOT) + FLASH_BOOT_CODE_TAG_OFFSET) != FLASH_BOOT_CODE_TAG)
	{
		// by indicating flash needs to be erased in all targets we lockout any possibility of other
		// commands being issued to other targets and halt any commands in progress
		for(i = 0; i < FLASH_TARGET_COUNT; i++)// loop through all targets
		{
			FlashSize[i] = -1;								// indicate the flash needs to be erased
		}
	}
	else													// if erasing a target image
	{
		switch(Target)										// select target
		{
        	case FLASH_TARGET_PROG:
				FlashLoadAddress[Target] = FLASH_PROG_CODE_LOWER_LIMIT;	// point to beginning of code space
                break;
        	case FLASH_TARGET_LOG:
				FlashLoadAddress[Target] = 0;				// point to beginning of log space
                break;
		}

		FlashSize[Target] = -2;								// indicate to target to erase old image
	}

	EnableFlashWriteAccess();								// enable the erase to take place
	return StateMachineStatus = FLASH_STATUS_PENDING;		// return change of status
}

//==========================================================================================
// Function:		ParseRecord()		
//
// Description:		Called to parse and write to flash a line of data from an intelHex
//					file.
//
//					If any errors are detected during parsing of the line, the failure mode
//					is returned and the write does not take place.
//
//					If the parse completes withouth error, the data is put in the
//					FlashBuffer for the target specified and the write is initiated for the
//					state machine.  A pending status is returned to indicate successful
//					parsing.
//
// Revision History:
//==========================================================================================
u16 ParseRecord(i16 Target)
{
	i16 i;
	u16 Size = FlashSize[Target];
	u16* Buffer = FlashBuffer[Target];
	u16 CheckSum;
	u16 Address;
	u16 Type;
	u16 DataValue;
    u32 tmp;
	i16 UartBuffPos = 0;									// index into buffer uart data buffer

	DownLoadActive = DOWN_LOAD_TIMEOUT;						// indicate we are actively downloading data

	Size = ReadHexNumber(2, &UartBuffPos);					// get record size
	CheckSum = Size;										// initialize the checksum
	Address = ReadHexNumber(4, &UartBuffPos);				// get address for this record
	CheckSum += (Address >> 8) & 0xFF;
	CheckSum += Address & 0xFF;								// add address into checksum 
	Type = ReadHexNumber(2, &UartBuffPos);					// get record type
	CheckSum += Type;										// add type into checksum

	for(i = 0; i < (Size >> 1); i++)						// loop through record data field
	{
		DataValue = ReadHexNumber(4, &UartBuffPos);			// get this data word
		CheckSum += DataValue & 0xFF;						// add low byte to checksum
		CheckSum += DataValue >> 8;							// add high byte to checksum
		Buffer[i] = DataValue;								// save the data word
	}

	if((Size & 1) != 0)										// if record size was an odd number of bytes
	{
		DataValue = ReadHexNumber(2, &UartBuffPos);			// read in last byte
		CheckSum += DataValue;								// add it to the checksum
		DataValue = DataValue << 8;							// move data to the high byte
		DataValue |= 0xFF;									// set unused bits
		Buffer[(Size >> 1) + 1] = DataValue;				// assign last byte of data
		Size++;												// adjust size to include unused bits
	}

	CheckSum = (~CheckSum + 1) & 0xFF;						// calculate final checksum

	if(CheckSum != ReadHexNumber(2, &UartBuffPos))			// if checksum fails
	{
		return StateMachineStatus = FLASH_STATUS_CHECKSUM;	// bail with error
	}

	if(Type == 4)											// if this is a high address field
	{
		if(Target > FLASH_TARGET_CODE_LIMIT)				// if working on general data
		{
			FlashLoadAddress[Target] = Buffer[0] * 0x10000ul;	// set high address bits
		}
		else												// if working in a code image
		{
			if(Buffer[0] != 0)								// and out of address range
			{
				return StateMachineStatus = FLASH_STATUS_ADDR_RANGE;	// bail with error
			}
		}
	}

	if(Type == 0 || Type == 1)								// if this is a data record
	{
		if(Type != 0 && Target <= FLASH_TARGET_CODE_LIMIT)	// check to see if the tag needs to be updated, only for code images
		{
			DownLoadActive = 5;								// allow the delay to be short
			WriteTag[Target] = 1;							// indicate the tag needs to be updated
		}
		else												// if this is not the end of the file
		{
			WriteTag[Target] = 0;							// don't modify the tag yet
		}
      
		FlashLoadAddress[Target] &= ~0xFFFFul;				// remove lower address bits
		FlashLoadAddress[Target] += Address;				// add in low address value

		if(Size > 0)										// if this record contains data
		{
           	tmp = FlashLoadAddress[Target];					// get working address
			if(Target == FLASH_TARGET_PROG)					// if working in DSP image
			{
				if(tmp < FLASH_PROG_CODE_LOWER_LIMIT
                   || tmp > FLASH_PROG_CODE_UPPER_LIMIT)  	// and address is out of range
				{
					return StateMachineStatus = FLASH_STATUS_ADDR_RANGE;	// bail on error
				}
			}
 		}
        
		FlashSize[Target] = Size >> 1;						// update the size of the buffer to write in words
		EnableFlashWriteAccess();							// allow flash to be written
		return StateMachineStatus = FLASH_STATUS_PENDING;	// tell user we have the data
	}

	return StateMachineStatus = FLASH_STATUS_INVALID_REC;	// indicate the error
}

//==========================================================================================
// Function:		ReadHexNumber()		
//
// Description:		Reads a hexadecimal number of Digits starting at nibble Position in the
//					input buffer and returns the number read.
//
// Revision History:
//==========================================================================================
u16 ReadHexNumber(i16 Digits, i16* Position)
{
	i16 i;
	i16 Character = -1;
	u16 Value = 0;
	for(i = 0; i < Digits; i++)								// loop through all digits
	{
		Character = GetBufferCharacter((*Position)++);		// get character, bump pointer
		Value <<= 4;										// shift by one hex digit
		Value += Character;									// add in this digit
	}
	return Value;											// return the value retrieved
}

//==========================================================================================
// Function:		GetTargetStatus()		
//
// Description:		Returns the status of a specific target from the view point of the
//					state machine.  If the target is busy a nonzero value is returned.
//
// Revision History:
//==========================================================================================
i16 GetTargetStatus(i16 Target)
{
	return TargetStatus[Target];							// return info requested
}

//==========================================================================================
// Function:		FlashLog()
//
// Description:		Writes records to the current log heap located in flash memory.
//
//					Records can be any size up to FLASH_LOG_RECORD_MAX_SIZE words.
//
//					Returns status codes indicate:
//					SUCCESS				The state machine has been handed the data.
//					FLASH_LOG_NO_MEM	The log heap in flash is full, data not stored.
//					FLASH_LOG_TOO_BIG   The data size passed is too large for a log record,
//										data not stored.
//					FLASH_LOG_BUSY      The state machine is currently busy with a previous
//										record write and cannot handle this request,
//										data not stored.
//                  FLASH_LOG_CORRUPT   The log appears to be corrupted and must be erased
//										before any more writes can occur.
//
//					uFlashLogStatus will contain the status of the state machine with respect
//					to the flash log.  If uFlashLogStatus >= FLASH_STATUS_TIMEOUT then a call
//					to FlashLogErase() must be made to clear the log file and uFlashLogStatus.
//					This is because an error occured at sometime during state machine
//					operation and the log file is now suspected to be corrupt.  Erasure is
//					the only way to guarantee its status.
//					
//
// Revision History:
//==========================================================================================
i16 FlashLog(void* Source, u16 Size)
{
	u16 i;

	if(uFlashLogStatus >= FLASH_STATUS_TIMEOUT)				// if the flash log appears corrupted
	{
		return FLASH_LOG_CORRUPT;
	}
    
	// if adding this record will cause the log buffer to grow larger than the end of flash memory
	if(FlashLogSize + Size + 2 >= FLASH_LOG_MAX_SIZE)
	{
		return FLASH_LOG_NO_MEM;							// let the user know about it
	}
  
	if(Size > FLASH_LOG_RECORD_MAX_SIZE)					// if data record and succeeding link too big
	{
		return FLASH_LOG_TOO_BIG;							// let the user know about it
	}
  
	if(GetTargetStatus(FLASH_TARGET_LOG))					// if the buffer is not available
	{
		return FLASH_LOG_BUSY;								// let the user know about it
	}

	DownLoadActive = DOWN_LOAD_TIMEOUT;						// indicate the flash log is active

	EnableFlashWriteAccess();								// enable access to flash
															// set address to write data at
	FlashLoadAddress[FLASH_TARGET_LOG] = FlashLogSize;

	FlashLogSize += Size + 2;								// update size of the flash log
															// write the new succeeding link to the buffer
	FlashBuffer[FLASH_TARGET_LOG][0] = (u16)((FlashLogSize >> 16) & 0xFFFFul);
	FlashBuffer[FLASH_TARGET_LOG][1] = (u16)(FlashLogSize & 0xFFFFul);
  
	for(i = 0; i < Size; i++)								// loop through all data to write
	{														// copy it to the code buffer
		FlashBuffer[FLASH_TARGET_LOG][i + 2] = *(((u16*)Source) + i);
	}
  
	FlashSize[FLASH_TARGET_LOG] = Size + 2;					// set size of data to write so state machine will do it
  
	return 0;												// return success
}

//==========================================================================================
// Function:		FlashLogErase()
//
// Description:		Erases the entire flash log
//
//					Returns status codes indicate:
//					SUCCESS				The state machine has been handed the data.
//					FLASH_LOG_BUSY      The state machine is currently busy with a previous
//										record write and cannot handle this request.
//                  FLASH_LOG_CORRUPT   The log appears to be corrupted and must be erased
//										before any more writes can occur.
//
//					FlashLogErase() will clear any error condition in uFlashLogStatus if
//					the flash erase completes without error itself.
//
// Revision History:
//==========================================================================================
i16 FlashLogErase(void)
{
	if(FlashLogSize == 0)									// if nothing to erase
	{
		return 0;											// return success
	}
	
	if(GetTargetStatus(FLASH_TARGET_LOG))					// if the is not available
	{
		return FLASH_LOG_BUSY;								// let the user know
	}

	DownLoadActive = DOWN_LOAD_TIMEOUT;						// indicate flash log active

	EnableFlashWriteAccess();								// enable access to the flash
	FlashLoadAddress[FLASH_TARGET_LOG] =0;					// set address to erase
	FlashSize[FLASH_TARGET_LOG] = -2;						// set erase sector command

	FlashLogSize = 0;										// reflect the new status
	uFlashLogStatus = FLASH_STATUS_COMPLETE;				// clear the log status
  
	return 0;												// return success
}

//==========================================================================================
// Function:		UpdateStatus()
//
// Description:		Updates the appropriate status flag depending on the target passed
//
// Revision History:
//==========================================================================================
void UpdateStatus(i16 Target)
{
	if(Target == FLASH_TARGET_LOG)							// if working on the flash log
	{
		uFlashLogStatus = StateMachineStatus;				// update logging status
	}
	else													// if working on a program update
	{
		uFlashWriteStatus = StateMachineStatus;				// update program update status
	}
	return;
}
