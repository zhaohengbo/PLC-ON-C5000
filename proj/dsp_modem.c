//==========================================================================================
// Filename:		dsp_modem.c
//
// Description:		Main loop for ofdm power line modem.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================
#include "intr.h"
#include "ofdm_modem.h"
#include "uart.h"
#include "Flash.h"

//==========================================================================================
// Local function declarations
//==========================================================================================
void InitEverything(void);
void InitVars(void);
u16	 InitHardware(void);
void InitTimer0(void);
u16  ReceivePacket(void);
void TestInterruptFlag(u16 uMarker);
void SetTxBias(u16 uBias);

// Transmitter Amplifier Current Bias Setting Options
#define	TX_BIAS_OFF		3
#define	TX_BIAS_LOW		2
#define	TX_BIAS_MID		1
#define	TX_BIAS_FULL	0
#define	TX_BIAS_ON		TX_BIAS_MID		// Bias setting to enable transmit amplifier (choose from LOW, MID, or FULL)


//==========================================================================================
// Set global vars
//==========================================================================================
u16		packetCntr = 0;

// Local error code definitions.
#define	ERR_RX_FRAME_ALIGN		(0xBBD1)
#define	ERR_RX_PARITY			(0xBADF)
#define ERR_RX_ADDR_MISMATCH	(0x0001)
#define ERR_CMD_BLOCKED			(0x0002)
#define	RX_TIMEOUT				(PACKET_TIME*5)		// About five packet times - used for LED toggling.


//==========================================================================================
// Function:		PulseXF()
//
// Description: 	Pulses the XF pin a desired number of times to aid in debug.  
//
// Revision History:
//==========================================================================================
void PulseXF(u16 uN)
{
	u16 i;		// Loop counter

	for (i= 0; i<uN; i++)
	{
		SetXF();				// Turn on XF flag for debug
		DelayNus(1);
		ClearXF();
		DelayNus(1);
	}
	DelayNus(3);

	return;
}


//==========================================================================================
// Function:		main()
//
// Description: 	Main function.  
//					Used for testing the OFDM encode/decode functions and the AFE hardware.
//					Eventually this will be replaced with a fuller version with task 
//					swapping.
//					Initializes variables and hardware in idle transmission mode.
//					Constructs and transmits a data packet and returns to idle.
//					Captures the transmitted waveform and decodes it.
//
// Revision History:
//==========================================================================================
void main()
{ 
	volatile u16	BailOut = 0;

//================ CONFIGURE THE SYSTEM ==================================
	INTR_GLOBAL_DISABLE();	// Disable Global Interrupts (if they were enabled)

	InitEverything();		// Set up DSP memory config registers, program variables,
							// MCBSP ports and DMA engines, and periodic timer.
							// Enable interrupts.

	SetXF();				// Turn on XF flag for debug

	//====== MAIN LOOP ========
	BailOut = 0;
		INTR_GLOBAL_DISABLE();	//###TEMP Disable Global Interrupts (if they were enabled)
	while(BailOut == 0)		// Loop forever, unless BailOut is changed using an emulator
	{
		PulseXF(2);			//### Pulse XF line to aid debug

		TurnLEDsOff(LED_LOOP_ACTIVE | LED_RX_AGC_HOLD | LED_RX_FRAME_ALIGN | LED_RX_BUFF_ERR | LED_RX_SEARCH );// Turn off bottom 5 LEDs	
		TurnLEDsOn(LED_LOOP_ACTIVE);	// Turn on green LED0	
		DebugDelay();		// Flush C54x if in debug mode.
		ClearXF();			//### Turn on XF flag for debug

		CheckPLC();			// Monitors for PL activity.
		
		// If we're waiting for the last bit of a RS485 packet to go out, don't start receiving
		// a PLC packet.  The outgoing RS485 bit will only take about 10 usec which won't affect
		// the PLC, but if we start receiving a PLC packet, we'll miss the e-meter response on
		// the 485 since we'll still be in 485 talk mode.
		// This can also be set through the Host using the SetParm command.
		if (uHoldOffPLC == 0)
		{
			ReceivePLC();		// Check for and handle incoming PL data.
		}

		ReceiveUART();		// Check for and handle incoming UART data.

		SendPLC();			// Manage outgoing PL data.
		
		SendUART();			// Manage outgoing UART data.

		BERTest();			// Handles BER testing

		AutoPoll();			// Handles periodic idle polling of SLAVE when appropriate.

		CheckTimeouts();	// See if we timed out while waiting for data.

		CheckFlash();		// execute current flash state machine state and return

		if(uSlaveFound == 0)	// Has a closest slave been identified yet?
		{
			CheckForClosestSlave();
		}
		else if(uFindSlaves == 1)	// Command to find slaves is running.
		{
			FindSlaves();
		}
	}

	DebugDelay();				// Flush C54x if in debug mode.

	//This is for debug use only:  Halt the MCBSP so we don't lose the data in the receive buffer.
	// Shut off frame sync generation (FS)
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	DebugDelay();				// Flush C54x if in debug mode.
}	
//=============================== END OF MAIN()  ===========================================


//==========================================================================================
// Function:		CheckPLC()		
//
// Description:		This function now is used to turn off the parity error and packet good
//					LEDs and to toggle the XF pin.
//
//					It also checks to see if Global Interrupts have been disabled during
//					the loop and re-enables them.  (This shouldn't happen.  It looks like
//					it is related to the reading of the DMA source pointer...)
//
// Revision History:
//==========================================================================================
void CheckPLC()
{
	volatile u16	ST1Shadow;			

	SetXF();							// Turn on XF flag for debug
	DebugDelay();						// Flush C54x if in debug mode.

	ST1Shadow = ST1 & 0x0800; 
	
	if (ST1Shadow != 0)
	{
		PostErrorCode(0xBAE5, "CheckPLC", "dsp_modem.c", "Re-enabling Global interrupts");
		#if SAVETRACE == TRUE
		{
			volatile u16	uRxDMA;
			uRxDMA = (u16)ReadRxDMAPointer();	// Read DMA destination pointer in RxBuffer
			SaveTraceData(0xBAE5);				//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)recSignal); 		//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(uRxDMA);				//!!!DEBUG  Put a marker in the trace buffer
		}
		#endif
		INTR_GLOBAL_ENABLE();		// Enable Global Interrupts (not supposed to be disabled, but turn them on again anyway
		DebugDelay();				// Flush C54x if in debug mode.
		DebugDelay();				// Flush C54x if in debug mode.
	}

	DebugDelay();				// Flush C54x if in debug mode.
	ClearXF();					// Turn off XF flag for debug

	if (ulLastRxCounter > RX_TIMEOUT)	// Turn off Parity Error and Good packet LEDs after RX_TIMEOUT
	{
		ulLastRxCounter = 0;				// Reset the Rx timeout counter
		TurnLEDsOff(LED_RX_PARITY_ERR);		// Turn off red LED7
		TurnLEDsOff(LED_RX_PACKET_GOOD);  	// Turn off green LED6	
	}			

	DebugDelay();				// Flush C54x if in debug mode.
}


//==========================================================================================
// Function:		ReceivePLC()		
//
// Description:		Check for and handle incoming PL data.
//
// Revision History:
//==========================================================================================
void ReceivePLC(void)
{
	u16		uStatus = ~SUCCESS;
	// Next declaration needed if the LCD update code below is restored.
	// u16		emeterMsgString[20];	
	
	if(agcState == AgcHold)		// Data is coming in via PLC
	{
		ulLastRxCounter = 0;			// Reset the Rx timeout counter
		uStatus = ReceivePacket();		// Data will be read into rxUserDataCopy[]

		if (uStatus == SUCCESS) 	// Good packet addressed to me was received.
		{
			// MASTER was expecting SLAVE RESPONSE
			if(uPlcState == PLC_RECEIVE_RESPONSE)	
			{
				// Send received PLC data on to host.
				WriteUART(UART_TARGET_HOST, uHostResponseLength, &rxUserDataCopy[uHostResponseStart] ); // count, pointer
				uPlcState = PLC_IGNORE;
				ulAutoPollCounter = 0L;
				uAutoPollPause=0;		// Release AutoPoll.
			}
		
			// MASTER was expecting SLAVE RESPONSE to periodic polling.
			else if(uPlcState == PLC_RECEIVE_POLLING)
			{
				uPlcState = PLC_IGNORE;
			}

			// MASTER is looking for closest slave (and found one).
			else if(uPlcState == PLC_FIND_CLOSEST_SLAVE)
			{
				ulClosestSlaveAddr = (u32)rxUserDataCopy[2]*65536L + (u32)rxUserDataCopy[3];
				uSlaveFound = 1;
				uPlcState = PLC_IGNORE;
				uUartState =  UART_RECEIVE_COMMAND;

				// May want to do something here in the future, so don't delete...
				// !!!!!	// Update LCD display
				// !!!!!	emeterMsgString[0] = (u16) 'W';		// Write command.
				// !!!!!	emeterMsgString[1] = (u16) 'R';
				// !!!!!	emeterMsgString[2] = (u16) 'e';
				// !!!!!	emeterMsgString[3] = (u16) 'a';
				// !!!!!	emeterMsgString[4] = (u16) 'd';		// "Ready"
				// !!!!!	emeterMsgString[5] = (u16) 'y';
				// !!!!!	emeterMsgString[6] = (u16) ' ';
				// !!!!!	emeterMsgString[7] = (u16) ' ';
				// !!!!!	emeterMsgString[8] = (u16) 0xd;		// \r
				// !!!!!	emeterMsgString[9] = 0;				// String terminator.
				// !!!!!	WriteUARTString(emeterMsgString);
				// !!!!!	uUartState = UART_RECEIVE_COMMAND;
				// !!!!!	uAckAfter485 = 0;
				// !!!!!
				// !!!!!	// Update Slave LCD display.
				// !!!!!	memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
				// !!!!!	*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;	// Destination address default
				// !!!!!	*(u32*)&txUserDataArray[2] = ulMyAddr;
				// !!!!!	txUserDataArray[4] = 0x10;			// Command number = slave emeter command.
				// !!!!!	txUserDataArray[5] = (((u16) 'W')<<8) + ((u16) 'R');		// Write command.
				// !!!!!	txUserDataArray[6] = (((u16) 'e')<<8) + ((u16) 'a');
				// !!!!!	txUserDataArray[7] = (((u16) 'd')<<8) + ((u16) 'y');
				// !!!!!	txUserDataArray[8] = (((u16) 0xd)<<8) + 0;				// \r, string terminator
				// !!!!!	uTransmitPacketReady = 1;			// Send completed command to the SLAVE.
				// !!!!!	uPlcState = PLC_IGNORE;				// Don't care about response
			}

			// MASTER is looking for all slaves (and found one).
			else if(uPlcState == PLC_FIND_ALL_SLAVES)
			{
				ulFindSlaveAddr = (u32)rxUserDataCopy[2]*65536L + (u32)rxUserDataCopy[3];
				uFindSlaves = 0;		// Stop looking.
				uPlcState = PLC_IGNORE;
				ulAutoPollCounter = 0L;
				uAutoPollPause = 0;
				WriteUART(UART_TARGET_HOST, 2, (u16*) &ulFindSlaveAddr);

				// If we'd never found a slave before, might as well initialize this in case we
				// get commands to slave 0 (closest).
				if (ulClosestSlaveAddr == 0)
				{
					ulClosestSlaveAddr = ulFindSlaveAddr;
				}
			}
														
			// SLAVE was waiting for COMMAND
			else if(uPlcState == PLC_RECEIVE_COMMAND)	
			{
				// Keep most recent received address to use for MASTER addr in BER
				ulMasterAddr = (u32)rxUserDataCopy[2]*65536 + (u32)rxUserDataCopy[3];	
													
				ProcessSlaveCommand();
			}
		}
	}
}


//==========================================================================================
// Function:		ReceiveUART()	
//
// Description:		Check for and handle incoming UART data.
//
// Revision History:
// 06/06/03 EGO		New function.
//==========================================================================================
void ReceiveUART(void)
{
	u16		i;				// Loop index.
	u16		uReturnChar;	// character from uart_read.

	if(uUartBufferPointerIn != uUartBufferPointerOut)	// UART data is available.
	{
		ulUartCounter = 0L;			// Reset counter for use as timeout.
		uAutoPollPause = 1;			// Used to temporarily suspend auto poll.
									// (this has no effect on a Slave)

		// MASTER is receiving a command from the host.
		if(uUartState == UART_RECEIVE_COMMAND)		// MASTER is receiving command
		{
			// If we run into problems reading only one character at a time, this while loop instead of the
			// proceding 'if' will read in all available UART characters.
			// while( (UART_LSR_REG & UART_LSR_DR)  &&  ( (uPlcState!=PLC_RECEIVE_POLLING) || (uUartCommandIndex < ((COMMAND_SIZE*2)-2)) ) )
			// Don't receive last parm of command until any AutoPoll requests have been completed
			if( ( (uPlcState!=PLC_RECEIVE_POLLING)&&(ulBerDelayCounter>ulBerDelay) ) || (uUartCommandIndex < ((COMMAND_SIZE*2)-2)) )
			{
				uart_read((u16*)&uUartCommand, uUartCommandIndex, 1);	// read in a single character
				uUartCommandIndex++;					// Index in bytes.
				if(uUartCommandIndex == COMMAND_SIZE*2)	// MASTER has command ready to process. (COMMAND_SIZE*2 bytes)
				{
					uUartCommandIndex = 0;			// Reset for next time.
					ulAutoPollCounter = 0;			// Delay periodic polling while executing commands from host.
					ProcessMasterCommand();
				}
			}
		}

		// MASTER is receiving data. Don't make this a simple "if", we want to process
		// the command between receiving command and receiving data.
		else if(uUartState == UART_RECEIVE_DATA)		
		{
			// Currently only data coming is Flash data to be programmed, but another
			// possiblility would be sending generic data out the modem.  A flag for
			// different cases should be set in the commnand and tested here.
			// For now just assume it's flash data and call Jim's routine.

			// Don't receive last byte of data until the previous flash command has finished (or an error occurs).
			if( ((uUartDataIndex+1) < uUartDataLength) || (uFlashWriteStatus != FLASH_STATUS_WRITING) )
			{
				if(uFlashWriteStatus >= FLASH_STATUS_TIMEOUT)	// An error occurred.
				{
					WriteUARTValue(UART_TARGET_HOST, FLASH_ERROR + uFlashWriteStatus);
					uUartState = UART_RECEIVE_COMMAND;
					uFlashWriteStatus = FLASH_STATUS_COMPLETE;
				}
				else		// read in the next character.
				{
					ulAutoPollCounter = 0;		// Delay periodic polling while receiving data.

					uart_read((u16*)&uUartDataArray, uUartDataIndex++, 1); // read in a single character
					if(uUartDataIndex == uUartDataLength)	// A full line received.
					{
						uUartState = UART_RECEIVE_COMMAND;

						// Respond to host.  This allows the host to send the next line of data.
						WriteUARTValue(UART_TARGET_HOST, SUCCESS);

						// This will result in the WriteFlash routine running.
						uFlashWriteStatus = FLASH_STATUS_WRITING;

						ulFlashWriteCounter = 0L;		// Reset timeout counter
					}
				}
			}
		}

		// MASTER or SLAVE is receiving data from eMeter.
		else if(uUartState == UART_RECEIVE_RESPONSE)
		{
			if(uBoard == BOARD_MASTER)
			{
				// Data from the emeter has to go to the host.
				// The host receives a fixed number of characters every read, and since the
				// emeter return size is variable, we'll pad the response and send the maximum
				// that could fit in a PLC packet, or 122 bytes (we'll be reading the same
				// kind of data from the slave, so this is reasonable).
				//
				// We can't send response data directly to the host, because the single
				// 16C550 handles both data streams and we need to reconfigure it first.
				// So we'll buffer the characters in upUartResponseBuffer[] and send the
				// buffer when we receive an end of line character.  The global pointer,
				// uUartResponseIndex, will be initialized by the command function.

				// read in a single character
				uReturnChar = uart_read((u16*)upUartResponseBuffer, uUartResponseIndex++, 1);

				// Test for end of response.
				if( (uReturnChar == 0xD)				// Check for '\r'
				 || (uUartResponseIndex >= 122) )		// No more room - something went wrong...
				{
					// Fill in remaining bytes with 0.
					// This loop operates on a word-by-word basis.  The (index+1)/2 will move us
					// to the next word if we ended on an odd byte.
					for(i=(uUartResponseIndex+1)/2; i<122/2; i++)
					{
						upUartResponseBuffer[i] = 0;
					}
					WriteUART(UART_TARGET_HOST, 122/2, (u16*)upUartResponseBuffer);
					ulAutoPollCounter = 0L;
					uAutoPollPause=0;			// Release AutoPoll.
					uUartState = UART_RECEIVE_COMMAND;
				}
			}
			else	// uBoard == BOARD_SLAVE
			{
				// read in a single character
				uReturnChar = uart_read((u16*)txUserDataArray, uUartResponseIndex++, 1);

				// Test for end of response.
				if( (uReturnChar == 0xD)				// Check for '\r'
				 || (uUartResponseIndex >= 126) )		// No more room - something went wrong...
				{
					// Fill in the rest of the buffer with zeros
					for(i=(uUartResponseIndex+1)/2; i<126/2; i++)
					{
						txUserDataArray[i] = 0;
					}
					uTransmitPacketReady = 1;
					uPlcState = PLC_RECEIVE_COMMAND;
					uUartState = UART_IDLE;
				}
			}
		}
	}
}


//==========================================================================================
// Function:		SendPLC()		
//
// Description:		Manage outgoing PL data.
//
// Revision History:
//==========================================================================================
void SendPLC(void)
{
	if(   (uTransmitPacketReady==1) 		// Outgoing packet ready
	   && (agcState!=AgcHold)				// & not receiving data
	   && (preambleDetCount == 0)  ) 		// & not receiving preamble 
	{
		uCommandBlock = 0;					// Re-enable packet reception.
											// Almost all slave commands come through this
											// point eventually so re-enable here instead of in
											// each command.  The two exceptions are BER and
											// broadcast e-meter commands which are handled
											// separately.

		TransmitPacket();					// Send out packet txUserDataArray[]
		uTransmitPacketReady = 0;			// Reset flag.
		agcState = AgcIdle;					// Turn off AGC hold in case it got turned on by our transmit signal.
	}
}


//==========================================================================================
// Function:		SendUART()		
//
// Description:		Manage outgoing UART data.
//					In order to keep all data on a word boundary, it is always sent in two
//					byte pairs.  Because there is no way to see exactly how much room there
//					is in the FIFO (can only check TEMT for empty or THRE for full), this
//					function waits for a completely empty FIFO before sending any characters
//					to prevent idle waiting for characters to be transmitted (which would
//					occur if there was only room for one byte and we had to wait for THRE
//					before sending the second half of a word).  When the FIFO is empty,
//					eight words are sent (if there are that many to send).
//					
// Revision History:
//==========================================================================================
void SendUART(void)
{
	static u16 	index = 0;
	u16			uSentWords = 0;			// Keep track of bytes sent out each pass
	u16			uStartingTarget;		// Local flag - Used to prevent switching targets
										// 		before all data is sent.

	// Check if the FIFO is empty.
	if(UART_LSR_REG & UART_LSR_TEMT)
	{
	  	UART_IER_REG &= 0xFFFD;		// disable transmit empty interrupt.  This is only
	  								// enabled for the end of an emeter transmission.

		// If there's data to go out, set the appropriate target.
		if(UARTDataOut[index].count != 0)
		{
			if(UARTDataOut[index].target == UART_TARGET_EMETER)
			{
				SelectUARTTarget(UART_TARGET_EMETER);
				SelectRS485Direction(RS485_TALK);
			}
			else
			{
				SelectUARTTarget(UART_TARGET_HOST);
			}
			uStartingTarget = uUARTTarget;		// Note the target - can only send to one
												// target in amy given pass through the loop.
		}

		// Send out up to 8 words (FIFO is 16 bytes).  Must be to same target.
		// Further, if the target is the emeter, can only send a single message - even
		// if there's room in the fifo for more characters.
		while(  (UARTDataOut[index].count > 0)
			 && (uSentWords++ < 8)
			 && (uStartingTarget == UARTDataOut[index].target) )
		{
			// For the RS485 we're sending ASCII, which is stored with one char in each
			// word.  So if the Target is EMETER, do not send the high byte.
			if(UARTDataOut[index].target == UART_TARGET_HOST)
			{
				// Send high byte to 16C550
				UART_THR_REG = (UARTDataOut[index].dataPtr[0] & 0xFF00) >> 8;
			}
			// Send low byte (regardless of target).
			UART_THR_REG = UARTDataOut[index].dataPtr[0] & 0xFF;

			UARTDataOut[index].dataPtr++;		// Increment pointer to next word
			if(--UARTDataOut[index].count == 0)	// Decrement count and test for end
			{
				// At the end of an emeter packet enable the interrupt so we can switch
				// to listen right away in order to not miss a response.
				if(uStartingTarget == UART_TARGET_EMETER)
				{
  					UART_IER_REG |= 0x02;		// Allow transmit empty interrupt.
					uSentWords = 10;			// Don't allow any more to go out.  As soon as the
												// last word of the string we just sent  is 
												// transmitted, the RS485 will be switched
												// to LISTEN (so additional bytes sent
												// wouldn't go out anyway).
					uHoldOffPLC = 1;
					uLastByteFlag = 1;
				}

				if(++index == SIZE_UART_OUT_ARRAY)	// Check for end of array
				{
					index = 0;
				}
			}
		}
	}

	DebugDelay();		// Flush C54x if in debug mode.
}


//==========================================================================================
// Function:		BERTest()		
//
// Description:		Handles sending BER packets from master to slave for BER testing.
//
// Revision History:
//==========================================================================================
void BERTest(void)
{
	static u16 uFirstPass = 1;

	if( (uBerTestInProgress == 1)
	 && (uBoard == BOARD_MASTER)
	 && (uPlcState != PLC_RECEIVE_RESPONSE)	
	 && (ulBerCounter < ulNumBerPackets)
	 && (uTransmitPacketReady == 0) )			// Make sure the last one went out.
	{
		if(uFirstPass == 1)						// Only do this once per packet
		{
			uFirstPass = 0;
			ulBerDelayCounter = 0L;
		}

		// Now wait an addition time for the slave to process the last packet.
		else if(ulBerDelayCounter > ulBerDelay)		// This many Timer0 interrupts.  (62 usec/int)
		{
			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
			*(u32*)&txUserDataArray[0] = ulBERSlaveAddr;	// Dest address
			*(u32*)&txUserDataArray[2] = ulMyAddr;			// Source address
			txUserDataArray[4] = SC_BER;					// This is a BER packet.
			txUserDataArray[5] = (u16) (ulBerCounter>>16);		// Send count used to check for
			txUserDataArray[6] = (u16) (ulBerCounter & 0xFFFF);	//		missed packets.
			ulBerCounter++;
			uTransmitPacketReady = 1;
			ulAutoPollCounter = 0;		// Delay periodic polling while performing BER test.
			uFirstPass = 1;
			SendPLC();					// Get it out here before ReceiveUART has a chance to corrupt.
		}
	}

	if( (ulBerCounter == ulNumBerPackets)		// We've completed the test.
	 && (uBerTestInProgress == 1)
	 && (uTransmitPacketReady == 0) )			// Make sure the last one went out.
	{
		memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
		uBerTestInProgress = 0;					// Stop sending packets.
		ulAutoPollCounter = 0L;
		uAutoPollPause = 0;						// Release AutoPoll.

		// Send command to slave signalling end of test so it stops counting CRC errors
		*(u32*)&txUserDataArray[0] = ulBERSlaveAddr;	// Dest address
		*(u32*)&txUserDataArray[2] = ulMyAddr;
		txUserDataArray[4] = SC_ABORT_BER;		// Command number = slave abort BER.
		uTransmitPacketReady = 1;				// Send completed command to the SLAVE.
		SendPLC();								// Get it out here before ReceiveUART has a chance to corrupt.
	}
}


//==========================================================================================
// Function:		AutoPoll()		
//
// Description:		Handles idle polling of SLAVE when appropriate.
//
// Revision History:
//==========================================================================================
void AutoPoll(void)
{
	if( (uAutoPoll==1)
	 && (uAutoPollPause==0)
	 && (uBoard==BOARD_MASTER)
	 && (ulAutoPollCounter>AUTO_POLL_COUNT)
	 && (uTransmitPacketReady == 0)				// Make sure the last one went out.
	 && (uPlcState==PLC_IGNORE) )
	{
		// Issue periodic polling command.
		memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
		*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr; // Slave addr
		*(u32*)&txUserDataArray[2] = ulMyAddr;			// Master addr
		txUserDataArray[4] = SC_AUTOPOLL;		// Polling command number.
		uTransmitPacketReady = 1;				// Signal packet is ready to go.
		uPlcState = PLC_RECEIVE_POLLING;		// Set state to receive polling response.
		ulAutoPollCounter = 0L;					// Reset counter for periodic polling.
		ulAutoPollResponseTimeoutCounter = 0L;	// Used to detect missing response.
	}
}


//==========================================================================================
// Function:		CheckTimeout()		
//
// Description:		See if we timed out while waiting for data.
//
// Revision History:
//==========================================================================================
void CheckTimeouts(void)
{
	u16		i;				// loop index

	// MASTER Check command response timeout.  Slave never responded to command.
	if( (uPlcState == PLC_RECEIVE_RESPONSE)
		&& (ulPlcResponseTimeoutCounter >= ((T0_INTS_PER_SEC>>2)*3)) ) 	// 3/4 second. 
	{																// Less than matlab timeout.
		WriteUARTValue(UART_TARGET_HOST, ERR_SLAVE_RESPONSE_TIMEOUT);
		uPlcState = PLC_IGNORE;
		uUartState = UART_RECEIVE_COMMAND;
		uUartCommandIndex = 0;
		ulPlcResponseTimeoutCounter = 0L;
		ulAutoPollCounter = 0L;
		uAutoPollPause=0;		// Release AutoPoll.
	}

	// MASTER Check autopoll response timeout.  Slave never responded to autopoll command.
	else if( (uPlcState == PLC_RECEIVE_POLLING)
	    && (ulAutoPollResponseTimeoutCounter >= (PACKET_TIME*4)) ) 			// Shouldn't be much more than 2 packet
	{														// times as there's not much processing
		uPlcState = PLC_IGNORE;								// required.
		uUartState = UART_RECEIVE_COMMAND;
		uUartCommandIndex = 0;
		ulLastRxCounter = 0L;
		ulAutoPollCounter = 0L;
		ulAutoPollResponseTimeoutCounter = 0L;
	}

	// MASTER Check UART command timeout.  Started receiving command characters from the host
	// but didn't receive an entire packet.
	else if ( (uUartState == UART_RECEIVE_COMMAND) 
		   && (ulUartCounter >= (T0_INTS_PER_SEC >> 2)) 	// 1/4 second. 16 bytes @ 9600 baud = ~0.02 seconds
		   && (uUartCommandIndex > 0) )
	{
		if(uFindSlaves == 1) // if not in find slaves mode
		{
			uPlcState = PLC_IGNORE;
		}
		uUartState = UART_RECEIVE_COMMAND;
		uUartCommandIndex = 0;
		ulUartCounter = 0L;
		ulAutoPollCounter = 0L;
		uAutoPollPause = 0;		// Release AutoPoll.
	}

	// MASTER Check UART data timeout.  Started receiving characters that are meant to be part of
	// a packet for the slave, but didn't receive an entire packet.
	else if ( (uUartState == UART_RECEIVE_DATA) 
		   && (ulUartCounter >= (T0_INTS_PER_SEC >> 1)) 	// 1/2 seconds. 128 bytes @ 9600 baud = ~0.13 seconds
		   && (uUartDataIndex > 0) )
	{
		WriteUARTValue(UART_TARGET_HOST, ERR_RECV_DATA_TIMEOUT);
		uPlcState = PLC_IGNORE;
		uUartState = UART_RECEIVE_COMMAND;
		uUartDataIndex = 0;
		ulUartCounter = 0L;
		ulAutoPollCounter = 0L;
		uAutoPollPause=0;		// Release AutoPoll.
	}

	// MASTER Check UART response timeout.  Should be receiving characters from the emeter, but
	// never received an end of string char signalling the end of response.
	else if( (uBoard == BOARD_MASTER)
		  && (uUartState == UART_RECEIVE_RESPONSE)
		  && (ulUartCounter >= (T0_INTS_PER_SEC >> 1)) )	 	// Give the emeter 1/2 second to respond.
	{														// Should be less than matlab timeout.
		WriteUARTValue(UART_TARGET_HOST, ERR_MASTER_EMETER_TIMEOUT);
		uPlcState = PLC_IGNORE;
		uUartState = UART_RECEIVE_COMMAND;
		uUartCommandIndex = 0;
		ulUartCounter = 0L;
		ulAutoPollCounter = 0L;
		uAutoPollPause=0;		// Release AutoPoll.
	}

	// MASTER check flash write timeout.  Sent command to write flash.  uFlashWriteComplete
	// never got set.
	else if( (uBoard == BOARD_MASTER)
	      && (uFlashWriteStatus == FLASH_STATUS_WRITING)
	      && (ulFlashWriteCounter >= (2L*T0_INTS_PER_SEC)) )
	{
		uFlashWriteStatus = FLASH_STATUS_TIMEOUT;	// Timeout error occurred.
		ulFlashWriteCounter = 0L;					// Reset timeout counter
	}

	// SLAVE Check UART response timeout.  Should be receiving characters from the emeter, but
	// never received an end of string char signalling the end of response.
	else if( (uBoard == BOARD_SLAVE)
		  && (uUartState == UART_RECEIVE_RESPONSE)
		  && (ulUartCounter >= (T0_INTS_PER_SEC >> 1)) ) 	 // Give the emeter 2 seconds to respond.
	{
		txUserDataArray[0] = ERR_SLAVE_EMETER_TIMEOUT;
		for(i=1; i<126/2; i++)
		{
			txUserDataArray[i] = 0;
		}
		uTransmitPacketReady = 1;

		uPlcState = PLC_RECEIVE_COMMAND;
		uUartState = UART_IDLE;
		uUartCommandIndex = 0;
		ulUartCounter = 0L;
	}

	return;
}


//==========================================================================================
// Function:		CheckForClosestSlave() 		
//
// Description:		This function is used to find the SLAVE with the closest address to the
//					MASTER's.  It is called from the main loop every pass until a slave is
//					found.  It calculated the next address to try and issues the polling
//					command to that address.  If a response is received, the proper
//					variables and flags will be updated in ReceivePLC(), and this function
//					will no longer be called.
//
// Revision History:
//==========================================================================================
void CheckForClosestSlave(void)
{
	static u16		uFirstPass = 1;
	static s32 		slNextAddr;
	static u32		i = 0L;
	static s16		sSign=-1;
	static s32		slMaxSlaveAddr;
	static s32		slMinSlaveAddr;

	if (uFirstPass == 1)
	{
		uFirstPass = 0;
		slNextAddr = (s32)ulMyAddr;
		ulAutoPollCounter = (PACKET_TIME*3);		// Make sure we go right into next conditional.
		slMinSlaveAddr = ulMyAddr - MAX_SLAVE_ADDR_RANGE;

		// Check for valid boundaries [0000:0003 - 7FFF:FFFF]

		// Not going to test addresses 0-2.
		//   0 used for closest slave
		//   1 used for broadcast to slaves
		//   2 used for broadcast to masters
		if(slMinSlaveAddr < 3)
		{
			slMinSlaveAddr = 3;
		}
		slMaxSlaveAddr = ulMyAddr + MAX_SLAVE_ADDR_RANGE;
		if(slMaxSlaveAddr < 0)		// Address rolled with addition.
		{
			slMaxSlaveAddr = 0x7fffffffL;
		}
	}
	
	if( (uPlcState == PLC_FIND_CLOSEST_SLAVE)
	 && (uTransmitPacketReady == 0)				// Make sure the last one went out.
	 && (ulAutoPollCounter>=(PACKET_TIME*3)) )	// Wait 3 packet times for response.
	{
		// Not going to test addresses 0-2, or ulMyAddr.
		//   0 used for closest slave
		//   1 used for broadcast to slaves
		//   2 used for broadcast to masters
		if (++i > (MAX_SLAVE_ADDR_RANGE*2))			// If we hit this, we've gone through all
		{										// the addresses.  Might as well keep going
			i = 1;								// though, since there's nothing else to do...
			sSign = -1;
			slNextAddr = (s32)ulMyAddr;
		}
		sSign *= -1;

		slNextAddr += i*sSign;

		// Don't issue polling command if test address is out of range.
		if((slNextAddr >= slMinSlaveAddr) && (slNextAddr <= slMaxSlaveAddr))
		{
			// Issue polling command
			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
			*(u32*)&txUserDataArray[0] = slNextAddr;	// Test slave addr
			*(u32*)&txUserDataArray[2] = ulMyAddr;		// Master addr
			txUserDataArray[4] = 6;						// Polling command number.
			uTransmitPacketReady = 1;					// Signal packet is ready to go.
			ulAutoPollCounter = 0L;						// Reset counter for use as timeout.
		}
	}

	return;
}


//==========================================================================================
// Function:		FindSlaves() 		
//
// Description:		This function is used to find to SLAVES.  It is initiated by a host
//					command which sets the flag uFindSlaves to 1 which allows this routine
//					to be called and sets the boundaries for the search using two global
//					variabes ulFindSlaveAddr and ulFindSlaveAddrMax.
//
//					Three things will cause this finding of slaves to stop:
//						1) Finding a slave - the receive PLC function will then respond
//							with the address of the found slave.
//						2) Passing ulFindSlaveAddrMax without finding a slave.
//						3) Receiving an AbortFindSlaves command.
//
// Revision History:
//==========================================================================================
void FindSlaves(void)
{
	if( (uPlcState == PLC_FIND_ALL_SLAVES)
	 && (uTransmitPacketReady == 0)				// Make sure the last one went out.
	 && (ulAutoPollCounter>=(PACKET_TIME*3)) )	// Wait 3 packet times for response.
	{
		if (ulFindSlaveAddr > ulFindSlaveAddrMax)
		{
			uFindSlaves = 0;
			uPlcState = PLC_IGNORE;
			ulAutoPollCounter = 0L;
			uAutoPollPause = 0;
			WriteUART(UART_TARGET_HOST, 2, (u16*) &ulFindSlaveAddr);
		}
		else
		{
			// Issue polling command
			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
			*(u32*)&txUserDataArray[0] = ulFindSlaveAddr++;	// Slave addr - increment here for next pass.
			*(u32*)&txUserDataArray[2] = ulMyAddr;		// Master addr
			txUserDataArray[4] = 6;						// Polling command number.
			uTransmitPacketReady = 1;					// Signal packet is ready to go.
			ulAutoPollCounter = 0L;						// Reset counter for use as timeout.
		}
	}

	return;
}


//==========================================================================================
// Function:		CheckFlash()		
//
// Description:		Handles flash writing.
//
//					The variable uFlashWriteStatus is the main indicator that flash data is
//						ready.  When a complete line has been received, ReceiveUART()
//						sets uFlashWriteStatus to FLASH_STATUS_WRITING.
//					When flash writing is complete this routine will change uFlashWriteStatus 
//						to FLASH_STATUS_COMPLETE, or FLASH_STATUS_XXX if an error occurred.
//
//					The global variable uFlashTarget determines where the data is 
//						intended to go.  ProcessMasterCommand() sets it to FLASH_TARGET_XXX
//						based on the command received from the host.
//							XXX = PROG  Flash data intended for DSP program space
//							XXX = DATA	Flash data intended for DSP data space
//							XXX = MSP Flash data intended for the MSP430
//
//					The data to be written is located at uUartDataArray and in a packed
//						byte format (two ascii bytes per word) has the Intel form:
//							10204E0029D00229D6022A1402287D02284302270B
//							^ ^   ^ ^                               ^
//							| |   | |                               |
//							| |   | |       checksum----------------+
//							| |   | +-------data bytes
//							| |   +---------record type (00=data, 01=end of file) 
//							| +-------------address for this line of data
//							+---------------number of bytes of data in this line
//
// Revision History:
//==========================================================================================
void CheckFlash(void)
{
	if( uBoard == BOARD_MASTER )                       // if this board is a master
    {
		if( uFlashWriteStatus == FLASH_STATUS_WRITING )
		{
	    	if( !GetTargetStatus( uFlashTarget ) )     // if intended target is open for access
	        {                                          // get state machine rolling on this target
	        	uFlashWriteStatus = ParseRecord( uFlashTarget );
	        }
		}

		if( uFlashWriteStatus == FLASH_STATUS_ERASING )
		{
	    	if( !GetTargetStatus( uFlashTarget ) )     // if intended target is open for access
	        {                                          // get state machine rolling on this target
	        	uFlashWriteStatus = FlashErase( uFlashTarget );
	        }
		}
    }

    FlashStateMachine( );   // execute current state of flash management state machine
    
	return;
}


//==========================================================================================
// Function:		InitEverything()
//
// Description: 	This function sets up DSP memory config registers, program variables,
// 					MCBSP ports and DMA engines, and periodic timer.
// 					Enable interrupts.
//
// Revision History:
//==========================================================================================
void InitEverything(void)
{
	// delcaration needed if update LCD code is re-enabled.
	// u16		emeterMsgString[20];

	PMST |= 0x0020;			// Turn on OVLY bit to force internal memory use
	PMST |= 0x0008;			// Turn on DROM bit to enable high memory 
	SWWSR = 0xFFFF;			// Max out all wait states
	IFR = 0xFFFF;			// Clear all pending interrupt flags
	IMR = 0;				// Disable all interrupts until we specifically enable them
	
	// Shut off MCBSP frame sync generation (FS)
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	MCBSP_SUBREG_BITWRITE(UART_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	MCBSP_SUBREG_BITWRITE(LED_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	DebugDelay();			// Flush C54x if in debug mode.
	
	INTR_INIT;				// Initialize the interrupt vector table pointer

	CLKMD = 0x0000;			// Turn off PLL	
	RPTNOP(10);				// Wait for PLL to stabilize at new speed			
	CLKMD =  (15u<<12)		// PLLMUL = 15
			|(1<<11)		// PLLNDIV = 1
			|(0xFF<<3)		// PLLCOUNT = 255
			|(1<<2)			// PLLONOFF = 1
			|(1<<1)			// PLLNDIV = 1
			|(1<<0);			// PLLSTATUS = 1
	RPTNOP(255);			// Wait for PLL to stabilize at new speed			

	InitVars();				// Initialize variables

	// GPIO register
	GPIOCR =  (0<<7)		// In	Master/~Slave
			 |(1<<6)		// Out 	+MSP_Reset  
			 |(1<<5)		// Out	~EN485
			 |(1<<4)		// Out	~485RE
			 |(1<<3)		// Out	485DE
			 |(0<<2)		// In	~Low5V
			 |(1<<1)		// Out	DRV Bias2
			 |(1<<0);		// Out 	DRV Bias1

	InitHardware();			// Initialize AFE hardware.  Start DMA sequencers.
	DebugDelay();			// Flush C54x if in debug mode.

	SetTxBias(TX_BIAS_OFF);	// Turn off the bias to the transmitter amplifier
	DebugDelay();			// Flush C54x if in debug mode.

	ConfigureLEDMCBSP ();	// Configure MCBSP to control status LEDs
	DebugDelay();			// Flush C54x if in debug mode.

	ConfigureUART();		// Initialize the 16C550 and UART buffers.
	DebugDelay();			// Flush C54x if in debug mode.

	// Read jumper input to determine if board is a master or slave.
	if (TestBit(GPIOSR, 1<<7))	// GPIO-7 tied to +Master/-Slave jumper.
	{
		uBoard = BOARD_MASTER; 
		ulMyAddr = 0x3;					// Will be overwritten with read from MSP flash below.
		uPlcState = PLC_FIND_CLOSEST_SLAVE;
		uUartState = UART_RECEIVE_COMMAND;
		uUartCommandIndex = 0;		// Index into command buffer for the next char received.
		uUartDataIndex = 0;			// Index into data buffer for the next char received.
		uSlaveFound = 0;			// No closest slave identified yet.
		SelectUARTTarget(UART_TARGET_HOST);		// Make sure we're ready to talk.
	}
	else
	{
		uBoard = BOARD_SLAVE;
		ulMyAddr = 0x4;					// Will be overwritten with read from MSP flash below.
		uPlcState = PLC_RECEIVE_COMMAND;
		uUartState = UART_IDLE;
		uSlaveFound = 1;				// Setting 1, will avoid looking in SLAVE case.
	}

	ulMyAddr = (u32)(*((u32*)(0X100)));		// Get address from DSP address 0x100.

	InitTesterParms();					// Initialize the variable address arrays used by the tester.

	uTransmitPacketReady = 0;			// Nothing to send yet.

	ClearXF();				// Turn off XF flag for debug
	DelayNus(7000);			// Wait 7 ms (a little over 2 frame times) before we start trying to look for signal
	SetXF();				// Turn on XF flag for debug

	prevSnrSample = ReadRxDMAPointer() - recSignalArray;	// This MUST be done just before enabling interrupts.
	InitTimer0();			// Configure and start Timer0 to generate periodic interrupts
	INTR_GLOBAL_ENABLE();	// Enable Global Interrupts (KEEP THIS RIGHT AFTER InitTimer0();

	InitFlash();            // initialize flash manager routines


	// MSP430 needs time to come out of reset.  If we don't wait, the first
	// few emeter commands often don't work.  Use the AutoPollCounter for a
	// timer, rather than create a new variable for this purpose.
	ulAutoPollCounter = 0L;
	while(ulAutoPollCounter < ((u32)T0_INTS_PER_SEC*3))
	{
		// Wait.
	}

	//====== Display startup text on LCDs ======
	// !!!!! This works, but we've opted to let the meter values be displayed instead...
	// !!!!! Don't delete this, as it's a good example of how the DSP can write to the
	// !!!!! LCD display if we ever want to use it again...
	// !!!!!if(uBoard == BOARD_MASTER)
	// !!!!!{
	// !!!!!	emeterMsgString[0] = (u16) 'W';		// Write command.
	// !!!!!	emeterMsgString[1] = (u16) 'L';
	// !!!!!	emeterMsgString[2] = (u16) 'o';
	// !!!!!	emeterMsgString[3] = (u16) 'o';
	// !!!!!	emeterMsgString[4] = (u16) 'k';		// "Looking"
	// !!!!!	emeterMsgString[5] = (u16) 'i';
	// !!!!!	emeterMsgString[6] = (u16) 'n';
	// !!!!!	emeterMsgString[7] = (u16) 'g';
	// !!!!!	emeterMsgString[8] = (u16) 0xd;		// \r
	// !!!!!	emeterMsgString[9] = 0;				// String terminator.
	// !!!!!	WriteUARTString(emeterMsgString);
	// !!!!!	uUartState = UART_RECEIVE_COMMAND;
	// !!!!!	uAckAfter485 = 0;
	// !!!!!}
	// !!!!!else	// Slave.
	// !!!!!{
	// !!!!!	emeterMsgString[0] = (u16) 'W';		// Write command.
	// !!!!!	emeterMsgString[1] = (u16) 'W';
	// !!!!!	emeterMsgString[2] = (u16) 'a';
	// !!!!!	emeterMsgString[3] = (u16) 'i';
	// !!!!!	emeterMsgString[4] = (u16) 't';		// "Waiting"
	// !!!!!	emeterMsgString[5] = (u16) 'i';
	// !!!!!	emeterMsgString[6] = (u16) 'n';
	// !!!!!	emeterMsgString[7] = (u16) 'g';
	// !!!!!	emeterMsgString[8] = (u16) 0xd;		// \r
	// !!!!!	emeterMsgString[9] = 0;				// String terminator.
	// !!!!!	WriteUARTString(emeterMsgString);
	// !!!!!	uAckAfter485 = 0;
	// !!!!!}
}


//==========================================================================================
// Function:		InitVars()
//
// Description: 	This function initializes some global variables.
//
// Revision History:
//==========================================================================================
void InitVars(void)
{
	u16		i;		// Loop index

	// DSP code version reported in GUI. 1st byte = integer portion, 2nd byte = fraction
	uCodeVersion = 0x0214;			// Code version xx.xx  0x0214=2.20

	for(i=0; i<UART_BUFFER_SIZE; i++)
	{
		upUartBufferFifo[i] = 0;	// Clear FIFO buffer (not required, but nice for debug).
	}

	//---- initialize stuff ------------------------------
	memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
	memset(rxUserDataArray, 0, DATA_BUFFER_LEN*sizeof(rxUserDataArray[0]));	// Fill User Data buffer with zeroes
	memset(rxUserDataCopy, 0, DATA_BUFFER_LEN*sizeof(rxUserDataCopy[0]));	// Fill reference buffer with zeroes

	memset(uErrCnt, 0, 32*sizeof(uErrCnt[0]));								// Zero the error count array

	initCRCtable( CRCtableArray );		// Initialize CRC table

	agcState = AgcIdle;
	prevSnrSample = 0;					// used to time SNR calc
	SNRflag = 0;						
	recSignal = recSignalArray;	
	uBerTestInProgress = 0;

	// Clear all timeout counters
	ulAutoPollCounter = 0;
	ulUartCounter = 0;
 	ulLastRxCounter = 0;

	// Optional: Useful for debug
	memset(recSignalArray, 0xFFFE, RX_CIRC_BUFFER_LEN*sizeof(recSignalArray[0]));	// Fill receive buffer with FFFE
	memset(txSignalArray,  0xBBBB, TX_BUFFER_LEN*sizeof(txSignalArray[0]));			// Fill tx buffer with BBBB

	#if (SAVETRACE == TRUE) || (SAVESYMBOLS == TRUE)
		memset((u16*)uTraceData, 0xABCD, TRACE_BUFF_SIZE*sizeof(uTraceData[0]));	// Fill with known characters
	#endif

	DebugDelay();				// Flush C54x if in debug mode.
}


//==========================================================================================
// Function:		InitTimer0()
//
// Description: 	Configure and start Timer0 to generate periodic interrupts
//
// Revision History:
//==========================================================================================
void InitTimer0(void)
{
	#define TIMER0	0					//!!!TEMP	- Move somewhere else
	#define	SER_CLK_DIV		25			//!!!TEMP  - Eliminate
	#define	TIMER0_PERIOD	((AGC_INTERVAL * SER_CLK_DIV * 16 * AFE_SIZE)-1)		//!!!TEMP Move to ofdm_defines.xls 
	#define	TDDR_INIT	(1-1)	
	TIM(TIMER0) = TIMER0_PERIOD-1;	
	PRD(TIMER0) = TIMER0_PERIOD-1;
	SetBits(TCR(TIMER0), (1<<TSS || 1<<TRB));	// Stop Timer and Reset
	TCR(TIMER0) = (   (0<<TIMSOFT) 			// 0 = freeze timer at breakpoint, 1= let timer expire
	               || (0<<TIMFREE) 			// 0 = TIMSOFT bit selects timer action @ breakpoint, 1= timer runs free
	               || (TDDR_INIT<<PSC)		// Timer Prescale counter
	               || (0<<TRB) 				// 0 = Normal, 1= Reset Timer
	               || (0<<TSS)				// 0 = Start timer, 1 = Stop timer
	               || (TDDR_INIT<TDDR) );	// Timer Divide-Down Ratio
	INTR_CLR_FLAG(TINT0); 	// Clear Timer0 Interrupt Flags
	INTR_ENABLE(TINT0); 	// Enable Timer0 Interrupt

	return;
}

//==========================================================================================
// Function:		InitHardware()
//
// Description: 	This function configures hardware I/O devices such as the AFE (Analog
//					 Front End).
//
// Revision History:
//==========================================================================================
u16 InitHardware(void)
{
	u16	uStatus = SUCCESS;

	AFEctrl1 =    (1<<7)	// Power Control: 0=Normal Power, 1=Low Power,Low Speed 
				+ (0<<6) 	// Reserved: 0
				+ (1<<4) 	// Tx Cutoff Freq: 0=.25x, 1=.38x, 2=.5x word rate
				+ (1<<3)	// Tx Filter Order: 0= 5th order, 1= 7th order
				+ (0<<1)	// Tx Power Backoff: 0=Normal, 1=-6dB, 2=-12dB, 3=-18dB
				+ (1<<0);	// Rx Cutoff Freq: 0=.25x, 1=.5x word rate
	AFEctrl2 =    (0<<6)	// Reserved: 0
				+ (5<<3)	// Rx Gain: 0=0 dB, 1=3dB, 2=6dB,..., 7=21dB
				+ (0<<1)	// Loop-Back: 0=Normal, 1=Digital Loopback, 2=Hybrid LB, 3=Line LB
				+ (0<<0);	// Reserved: 0

	FillIdleBuffer();		// Write new control setting to idle buffer if it has changed

	uStatus = ConfigureAFE ();
	if (uStatus != SUCCESS)
	{
		postError("InitHardware failed doing ConfigureAFE./n");
		return(uStatus);
	}
	
		// Use DMA for both Rx and Tx
	ConfigureMCBSPDMA(AFE_MCBSP);
	

	uStatus = ConfigureAFE ();		// Reset all MCBSP and DMA
	if (uStatus != SUCCESS)
	{
		postError("InitHardware failed doing ConfigureAFE./n");
	}
  

	DMAReadAFE((u16)&recSignalArray[0], RX_CIRC_BUFFER_LEN);	//!!!TEMP

	// Arm and start the Idle DMA sequence.  	
	DMAWriteAFE(IdleBuffArray, IDLE_BUFFER_LEN*AFE_SIZE);

	DebugDelay();				// Flush C54x if in debug mode.

	
	return(uStatus);
}


//==========================================================================================
// Function:		SetTxBias()
//
// Description: 	This function sets the bias current in the transmit amplifier.
//
// Revision History:
//==========================================================================================
void	SetTxBias(u16 uBias)
{
	#define	TX_BIAS_MASK	3
	AssignBits(GPIOSR,  TX_BIAS_MASK<<0, uBias<<0);	 // Send tx bias settings to bits 0 and 1 of HPI GPIO pins
}


//==========================================================================================
// Function:		TransmitPacket()
//
// Description: 	This function takes a message from the txUserDataArray and transmits
//					it to another station as a power-line communication packet.
//
// Revision History:
//==========================================================================================
void TransmitPacket(void)
{
	TXDATA			*pTxSignal;
	i16				winSignal[WINDOW_LEN];	// Smoothed transition from preamble to data
	u16				SNRflagBackup;
	
	DebugDelay();				// Flush C54x if in debug mode.
	SNRflagBackup = SNRflag;	// Make a backup copy of the SNRflag
	SNRflag = -2;				// Tell receiver not to look for preambles in my Tx signal
	TurnLEDsOn(LED_TX_PACKET);	// Turn on green Tx LED1
	SetTxBias(TX_BIAS_ON);		// Turn on the bias to the transmitter amplifier

	DebugDelay();				// Flush C54x if in debug mode.
	PostErrorCode(0xBAE0, "TransmitPacket", "dsp_modem.c", "Transmitting Packet");
	#if SAVETRACE == TRUE
		SaveTraceData(0xBAE0);			//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)recSignal); 	//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData(SNRflagBackup);	//!!!DEBUG  Put a marker in the trace buffer
	#endif

	DebugDelay();				// Flush C54x if in debug mode.
	pTxSignal = makePreamble((TXDATA*)txSignalArray, winSignal); // start at beginning of tx buffer and build preamble

	DebugDelay();				// Flush C54x if in debug mode.
	pTxSignal = makeDataFrames(pTxSignal, winSignal, txUserDataArray);	// start at current ptr and build all data frames

	DelayNus(1500);				// delay 1.5 mSec to allow 7V power supply to settle after TX
	SetTxBias(TX_BIAS_OFF);		// Turn off the bias to the transmitter amplifier

	DebugDelay();				// Flush C54x if in debug mode.
	SNRflag = SNRflagBackup;	// Restore the SNRflag
	TurnLEDsOff(LED_TX_PACKET);	// Turn off green Tx LED1

	return;
}

//==========================================================================================
// Function:		ReceivePacket()
//
// Description: 	This function receives a packet from the power-line communication
//					interface, decodes it, checks for errors, and stores the resulting
//					message in rxUserDataArray, then copies into rxUserDataCopy to
//					prevent overwriting.
//
// Revision History:
//==========================================================================================
u16 ReceivePacket(void)
{
	u16				errCount = 0;
	volatile i16	uNonZeroBits = 0;	// Count of non-zero bits in received message
	volatile u16	uRxDMA;
	u16	uStatus;

	TurnLEDsOn(LED_RX_AGC_HOLD);		//Turn on yellow LED6
	
	#if SAVETRACE == TRUE
		uRxDMA = (u16)ReadRxDMAPointer();	// Read DMA destination pointer in RxBuffer
		SaveTraceData(0xACCA);			//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)recSignal);	//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData(uRxDMA);			//!!!DEBUG  Put a marker in the trace buffer
	#endif

	//====================================================================
	//	read data and do detection
	//=====================================================================
	recSignal = frameAlign(phaseEqArray, recSignal );
	DebugDelay();				// Flush C54x if in debug mode.

	if (frameAligned == 0)
	{
		agcState = AgcIdle;	
		preambleDetCount = 0;			//	start over
		uStatus = ERR_RX_FRAME_ALIGN;
	}
	else	// Preamble Frame Align successful.  Read the data frames.
	{			
		TurnLEDsOn(LED_RX_FRAME_ALIGN);	//Turn on yellow LED5	
		packetCntr++;
		ulLastRxCounter = 0;				// Reset the Rx timeout counter

		#if SAVETRACE == TRUE
			SaveTraceData(0xACCB);				//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)ulLastRxCounter);//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)0000);			//!!!DEBUG  Put a marker in the trace buffer

			SaveTraceData(0xACCC);				//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)recSignal);		//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(packetCntr);			//!!!DEBUG  Put a marker in the trace buffer
		#endif

		DebugDelay();						// Flush C54x if in debug mode.
		errCount = readDataFrames(rxUserDataArray, recSignal, phaseEqArray );
		if( errCount != 0 )
		{
			uStatus = ERR_RX_PARITY;	// Parity Error
			DebugDelay();				// Flush C54x if in debug mode.
			PostErrorCode(0xBADF, "main", "dsp_modem.c", "Parity errors");
			TurnLEDsOn(LED_RX_PARITY_ERR);	// Turn on red LED3
			TurnLEDsOff(LED_RX_PACKET_GOOD);// Turn off green LED4	

			// Increment BER error counter for parity errors
			if(uBerTestInProgress == 1)
			{
				uBerErrorCounter++;
			}

			#if SAVETRACE == TRUE
				uNonZeroBits = countErrors(rxUserDataArray);
				DebugDelay();						// Flush C54x if in debug mode.

				SaveTraceData(0xBADF);			//!!!DEBUG  Put a marker in the trace buffer
				SaveTraceData((u16)recSignal); 	//!!!DEBUG  Put a marker in the trace buffer
				SaveTraceData(uNonZeroBits);		//!!!DEBUG  Put a marker in the trace buffer
			#endif
			#if SAVESYMBOLS == TRUE
			{
				u16		n;							// append the freqEq coeficients to the tracebuffer 
				iCplx	*feqPtr = phaseEqArray;
				for(n = 0; n < CARRIER_LEN; n++ )
				{
					SaveTraceData((u16)(feqPtr->re));
					SaveTraceData((u16)(feqPtr->im));
					feqPtr++;
				}
				uTraceEnable = FALSE;				// halt saving to tracebuffer
			}
			#endif
			DebugDelay();				// Flush C54x if in debug mode.
			DebugDelay();				// Flush C54x if in debug mode.

		}
		else
		{
			// Good packet received.  Turn off parity error LED.
			DebugDelay();				// Flush C54x if in debug mode.
			PostErrorCode(0xBAD0, "main", "dsp_modem.c", "Good Packet Received!");
			TurnLEDsOff(LED_RX_PARITY_ERR);	// Turn off red LED3
			TurnLEDsOn(LED_RX_PACKET_GOOD);	//Turn on green LED4

			#if SAVETRACE == TRUE
				SaveTraceData(0xBAD0);			//!!!DEBUG  Put a marker in the trace buffer
				SaveTraceData((u16)recSignal); 	//!!!DEBUG  Put a marker in the trace buffer
				SaveTraceData(errCount);		//!!!DEBUG  Put a marker in the trace buffer
			#endif

			// Only overwrite rxUserDataCopy if the destination adddress matches myAddr.
 			if( (*(u32*)&rxUserDataArray[0] == ulMyAddr)	// Is this packet addressed to me?
			 || ( (*(u32*)&rxUserDataArray[0] == 1) && (uBoard == BOARD_SLAVE) )		// Check for slave broadcast.
			 ||	( (*(u32*)&rxUserDataArray[0] == 2) && (uBoard == BOARD_MASTER) ) )		// Check for master broadcast.
			{
				// Check if we're already processing a command.  If command blocking is enabled
				// do not receive another packet!
				// This test was added for the case when another MASTER on the same powerline
				// comes up and starts trying to find slaves.  It may eventually issue a
				// find slave command to our slave - which looks like a valid command so
				// without this test we'll overwrite rxUserDataCopy, which results in the
				// response to the currently active command being sent to the wrong MASTER.
				if((uBoard==BOARD_SLAVE) && (uCommandBlock==1))
				{
					uStatus = ERR_CMD_BLOCKED;
				}
				else
				{
					// Block all further incoming traffic until we're finished with this command.
					// NOTE: This must be turned off when the command is finished or we'll never
					// receive another packet.
					// NOTE2: This has no effect on a MASTER.
					uCommandBlock = 1;

					// Copy from rxUserDataArray to rxUserDataCopy in case a new message
					// is received while interpretting the last one (only need data portion of buffer).
					memcpy(rxUserDataCopy, rxUserDataArray, NUM_USER_BYTES/2*sizeof(u16));
					uStatus = SUCCESS;			//Good

					#if SAVESYMBOLS == TRUE
					{
						u16		n;							// append the freqEq coeficients to the tracebuffer 
						iCplx	*feqPtr = phaseEqArray;
						for(n = 0; n < CARRIER_LEN; n++ )
						{
							SaveTraceData((u16)(feqPtr->re));
							SaveTraceData((u16)(feqPtr->im));
							feqPtr++;
						}
					}
					#endif
					DebugDelay();				// Flush C54x if in debug mode.
					DebugDelay();				// Flush C54x if in debug mode.
				}
			}
			else
			{
				uStatus = ERR_RX_ADDR_MISMATCH;
				TurnLEDsOff(LED_RX_PACKET_GOOD);	//Turn off green LED4 - packet was not for me.
			}
		}
	}

	TurnLEDsOff(LED_RX_FRAME_ALIGN | LED_RX_AGC_HOLD);	// Turn off yellow LED5 and yellow LED6		
	DebugDelay();						// Flush C54x if in debug mode.

	return(uStatus);
}


//==========================================================================================
// Function:		TestInterruptFlag()
//
// Description: 	This function reads the interrupt mask flag and posts a trace message
//					if it is non-zero (interrupts disabled).
//
// Revision History:
//==========================================================================================
void TestInterruptFlag(u16 uMarker)
{
	volatile u16	ST1Shadow;			
	volatile u16	ST1Shadow2;	
	#if SAVETRACE == TRUE
		u16	uRxDMA;
	#endif
			
	NOP(); 					// Wait for pipeline to clear.
	ST1Shadow = ST1;		// Read the ST1 reg which contains the interrupt mask
	RPTNOP(4); 				// Wait for pipeline to clear.
	ST1Shadow2 = ST1; 		// Read ST1 again.  Was seeing false triggers when they differed.
	RPTNOP(4); 				// Wait for pipeline to clear.
				
	#if SAVETRACE == TRUE
		if ( ST1Shadow != ST1Shadow2 )		// Post a trace message if these don't match
		{
			SaveTraceData(0x1333);			//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(ST1Shadow); 		//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(ST1Shadow2);		//!!!DEBUG  Put a marker in the trace buffer
			DebugDelay();					// Flush C54x if in debug mode.
			DebugDelay();					// Flush C54x if in debug mode.
		}
	#endif

	if ( ((ST1Shadow & 0x0800) != 0) && ((ST1Shadow2 & 0x0800) != 0) )
	{
		INTR_GLOBAL_ENABLE();		// Re-Enable Global Interrupts 
		#if SAVETRACE == TRUE
			uRxDMA = (u16)ReadRxDMAPointer();	// Read DMA destination pointer in RxBuffer
			SaveTraceData(uMarker);				//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)recSignal); 		//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(uRxDMA);				//!!!DEBUG  Put a marker in the trace buffer
			DebugDelay();						// Flush C54x if in debug mode.
		#endif
		DebugDelay();				// Flush C54x if in debug mode.
	}

	return;
}


//==========================================================================================
// Function:		INVALID_ISR()
//
// Description: 	Handler for Invalid Interrupts.
//
// Revision History:
//==========================================================================================
interrupt void	INVALID_ISR(void)
{
	volatile u16	hang;
	
	DebugDelay();				// Flush C54x if in debug mode.
	// Shut off MCBSP frame sync generation (FS)
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	MCBSP_SUBREG_BITWRITE(LED_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	DebugDelay();				// Flush C54x if in debug mode.

	#ifdef	SHOW_FLOW
 		printf("Invalid Interrupt:  IMR=%X IFR=%X \n", IMR, IFR);
	#endif
	DebugDelay();				// Flush C54x if in debug mode.
	
	hang = 1;					// 0= return immediately, 1= hang until user intervention
	while( hang )
	{
		DebugDelay();			// Flush C54x if in debug mode.
	}
}


//==========================================================================================
// Function:		TINT0_ISR()
//
// Description: 	Timer 0 Interrupt Handler
//
// Revision History:
//==========================================================================================
interrupt void	TINT0_ISR(void)
{
	#define DMSBA_REG	(*(volatile unsigned int *) DMSBA_ADDR)

	u16		DMAsubAddrShadow;			// Backup copy of the DMA base address register
	
	DMAsubAddrShadow = DMSBA_REG;		// Make a backup copy of the DMA base address register
										// IMPORTANT! This must be done before any other operations that
										// might access the DMA registers.

	if( agcState != AgcHold )
	{
		sample_interupt();		// Look at recent received values.  Update AGC.  
								// Set SNRflag when time to look for preamble.
	
		FillIdleBuffer();		// Write new control setting to Tx idle buffer if it has changed
	
		if( SNRflag == 1 )
		{
			SNRflag = -1;		// In Progress
			recSignal = lookForPacket();	// Look for preamble pattern in recently received buffer
			SNRflag = 0;		// Completed
		}
	}

	// Increment one counter to time auto-polling, and two used to keep track of time since
	// a PLC packet was sent or received.
	ulAutoPollCounter++;		// Used for autopoll and finding closest slave
	ulUartCounter++;
 	ulLastRxCounter++;
	ulPlcResponseTimeoutCounter++;
	ulAutoPollResponseTimeoutCounter++;
	ulBerDelayCounter++;
	ulFlashWriteCounter++;
	uFlashShow++;

	// Move any pending characters from the 16C550.
	// This is done in the ISR instead of ReceiveUART() so that if we receive more
	// characters than fit in the FIFO we won't loose them.
	while (UART_LSR_REG & 0x1)		// As long as there is data.
	{
		// Store one character per word.
		upUartBufferFifo[uUartBufferPointerIn] = (u16) UART_RBR_REG;
		if(++uUartBufferPointerIn >= UART_BUFFER_SIZE)
		{
			uUartBufferPointerIn = 0;
		}
	}

	// ----- Clean up and restore context ------
	DMSBA_REG = DMAsubAddrShadow;		// Restore the DMA base address register in case we messed it up		
	IFR = 1<<TINT0;						// Clear the Timer0 Interrupt Flag
}

