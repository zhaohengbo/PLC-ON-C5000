//==========================================================================================
// Filename:		Flash.h
//
// Description:		shared globals and prototypes
//
// Copyright (C) 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================
extern u16 FlashLEDMask;
extern u32 FlashLogSize;

#if COMPILE_MODE == DSP_COMPILE
	void FlashStateMachine(void);
#else  // if MEX_COMPILE  do nothing
	#define	FlashStateMachine()		
#endif
i16  GetTargetStatus(i16 Target);
u16  ParseRecord(i16 Target);
u16  FlashErase(i16 Target);
void InitFlash( void );
i16  FlashLog( void* Source, u16 Size );
i16  FlashLogErase( void );
u16  ReadFlash(u32 Address);
void FarWrite(const u32 Address, const u16 Data);
u16  FarRead(const u32 Address);

