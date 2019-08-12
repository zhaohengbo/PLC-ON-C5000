//==========================================================================================
// Filename:		command.c
//
// Description:		Routines to interpret commands.  On Master these are from UART,
//					on slave from PLC.
//
// Copyright (C) 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"
#include "uart.h"
#include "intr.h"

static u16	emeterMsgString[20];	// Holds command string to be sent to emeter.
									// Do not pack two bytes into each word.  This makes the 
									// transmitting much more straightforward.
									// 20 bytes is long enough to hold the longest string that
									// the HOST/Master protocol allows.
									// (global/static for debug)
//==========================================================================================
// Function:		ProcessMasterCommand()
//
// Description: 	This function interprets and executes a command from the UART for the
//					MASTER.  Commands must either be executed quickly, or set up state
//					variables which allow additional processing later.  They may not
//					tie up the processor for extended processing.
//
//					The command from the UART is received in 8 16-bit parms.
//					parm[0] contains the command number
//					parms[1-7] are optional depending on the command and described for each
//						command below.
//
//
// Revision History:
//==========================================================================================
void ProcessMasterCommand(void)
{
/*Master*/	u16		i;					// Loop index
/*Master*/  u16		emeterStringIndex;
/*Master*/  u32		ulAddr;				// 32-bit address formed from received uart data
/*Master*/
/*Master*/	switch(uUartCommand[0])		// command number
/*Master*/	{
/*Master*/		case MC_GET_PARM:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Get master parm.
/*Master*/			// 	uUartCommand[1] = index into address table of requested value.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		If the requested configuration parameter is valid, SUCCESS is returned
/*Master*/			//		for an acknowledge followed by the requested value(s).
/*Master*/			//		If an unrecognized parameter is requested an error code is returned.
/*Master*/			//==============================================================================
/*Master*/			// ***** Error checking for parm number < 40
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);	// command acknowlege
/*Master*/			if(uUartCommand[1] < 30)	// request for 16-bit parm
/*Master*/			{
/*Master*/				WriteUART(UART_TARGET_HOST, 1, (u16*) uppParms16[uUartCommand[1]]);
/*Master*/			}
/*Master*/			else
/*Master*/			{
/*Master*/				WriteUART(UART_TARGET_HOST, 2, (u16*) uppParms32[uUartCommand[1]-30]);
/*Master*/			}
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/			break;
/*Master*/
/*Master*/		case MC_SET_PARM:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Set parm.
/*Master*/			// 	uUartCommand[1] = index into address table of requested value.
/*Master*/			// 	uUartCommand[2] = value. (high word if 32-bit parm)
/*Master*/			// 	uUartCommand[3] = (low word of value if 32-bit parm)
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		If the configuration parmeters are valid, SUCCESS is returned,
/*Master*/			//		otherwise an error code is returned.
/*Master*/			//==============================================================================
/*Master*/			// ***** Error checking for parm number < 40
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);
/*Master*/			if(uUartCommand[1] < 30)	// writing 16-bit parm
/*Master*/			{
/*Master*/				*uppParms16[uUartCommand[1]] = uUartCommand[2];
/*Master*/			}
/*Master*/			else
/*Master*/			{
/*Master*/				*uppParms32[uUartCommand[1]-30] = (u32)(uUartCommand[2])*65536 + (u32)(uUartCommand[3]);
/*Master*/			}
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/			break;
/*Master*/
/*Master*/		case MC_GET_STATUS:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Get status.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		If the requested configuration parameter is valid, SUCCESS is returned
/*Master*/			//		for an acknowledge followed by the status array values.
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);
/*Master*/			for(i=0; i<30; i++)		// 16-bit parms
/*Master*/			{
/*Master*/				WriteUART(UART_TARGET_HOST, 1, (u16*) uppParms16[i]);
/*Master*/			}
/*Master*/			for(i=0; i<10; i++)		// 32-bit parms
/*Master*/			{
/*Master*/				WriteUART(UART_TARGET_HOST, 2, (u16*) uppParms32[i]);
/*Master*/			}
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/			break;
/*Master*/
/*Master*/		case MC_START_BER:
/*Master*/			//==============================================================================
/*Master*/			// Start BER test.
/*Master*/			// 	uUartCommand[1] = slave address - high (0 = use uClosestSlave).
/*Master*/			// 	uUartCommand[2] = slave address -low   (0 = use uClosestSlave).
/*Master*/			// 	uUartCommand[3] = nuber of BER packets requested (high).
/*Master*/			// 	uUartCommand[4] = nuber of BER packets requested (low). 
/*Master*/			//
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		SUCCESS/ERROR is returned here.  Details of the BER can be polled using
/*Master*/			//		the Get Master Configuration command.
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);		// Acknowlege command.
/*Master*/			uUartState = UART_RECEIVE_COMMAND;	// Next thing coming should be another cmd.
/*Master*/			
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			ulNumBerPackets = (u32)uUartCommand[3]*65536L + (u32)uUartCommand[4];
/*Master*/			ulBerCounter = 0L;		// Reset number sent counter.
/*Master*/			
/*Master*/			// Set flag to start sending BER packets
/*Master*/			uBerTestInProgress = 1;
/*Master*/	
/*Master*/			if(ulAddr == 0L)
/*Master*/			{
/*Master*/				ulBERSlaveAddr = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else
/*Master*/			{
/*Master*/				ulBERSlaveAddr = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			break;
/*Master*/	
/*Master*/		case MC_ABORT_BER:
/*Master*/			//==============================================================================
/*Master*/			// Abort BER test.
/*Master*/			//		This command will halt the BER test if running.
/*Master*/			// 	uUartCommand[1] = slave address - high (0 = use uClosestSlave).
/*Master*/			// 	uUartCommand[2] = slave address - low  (0 = use uClosestSlave).
/*Master*/			//
/*Master*/			// The slave gets notified also so it will stop adding to stats.
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		SUCCESS/ERROR is returned here.  Details of the BER can be polled using
/*Master*/			//		the Get Master Configuration command.
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);			// Acknowlege command.
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_ABORT_BER;	// Command number = SLAVE abort BER
/*Master*/			uTransmitPacketReady = 1;			// Send completed command to the SLAVE.
/*Master*/	
/*Master*/			uBerTestInProgress = 0;				// Stop sending packets.
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;					// Release AutoPoll.
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_WRITE_FLASH:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Write Flash
/*Master*/			// 	uUartCommand[1] = Target device FLASH_TARGET_XXX (XXX=PROG, DATA, or MSP).
/*Master*/			// 	uUartCommand[2] = Number of BYTES to follow this command.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		Nothing here.  The return is send in dsp_modem.c
/*Master*/			//==============================================================================
/*Master*/			uUartState = UART_RECEIVE_DATA;
/*Master*/			uFlashTarget = uUartCommand[1];
/*Master*/			uUartDataLength = uUartCommand[2];
/*Master*/			uUartDataIndex = 0;
/*Master*/			ulUartCounter = 0L;		// Reset timeout counter
/*Master*/			break;
/*Master*/
/*Master*/		case MC_READ_BLOCK:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Read Block
/*Master*/			// 	uUartCommand[1] = Starting address.
/*Master*/			// 	uUartCommand[2] = Count (words) to read.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		If the requested configuration parameter is valid, SUCCESS is returned
/*Master*/			//		for an acknowledge followed by the requested data
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);
/*Master*/
/*Master*/			WriteUART(UART_TARGET_HOST, uUartCommand[2], (u16*) uUartCommand[1]);
/*Master*/
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_ERASE_FLASH:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Erase Flash
/*Master*/			// 	uUartCommand[1] = Target device FLASH_TARGET_XXX (XXX=PROG, MSP).
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		Nothing here.  The return is send in dsp_modem.c
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);
/*Master*/			uFlashTarget = uUartCommand[1];
/*Master*/			uFlashWriteStatus = FLASH_STATUS_ERASING;
/*Master*/			break;
/*Master*/
/*Master*/		case MC_READ_FLASH:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Read Flash
/*Master*/			// 	uUartCommand[1] = Starting address - high.
/*Master*/			// 	uUartCommand[2] = Starting address - low.
/*Master*/			// 	uUartCommand[3] = Count (words) to read.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		If the requested configuration parameter is valid, SUCCESS is returned
/*Master*/			//		for an acknowledge followed by the requested data
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);
/*Master*/
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			for(i=0; i<uUartCommand[3]; i++)
/*Master*/			{
/*Master*/				WriteUARTValue(UART_TARGET_HOST, ReadFlash(ulAddr++));
/*Master*/			}
/*Master*/
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_FIND_SLAVES:
/*Master*/			//==============================================================================
/*Master*/			// Start searching for all slaves.
/*Master*/			//
/*Master*/			// 	uUartCommand[1] = starting slave address - high
/*Master*/			// 	uUartCommand[2] = starting slave address - low
/*Master*/			// 	uUartCommand[3] = max slave address - high
/*Master*/			// 	uUartCommand[4] = max slave address - low
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		No return value is sent until a slave is found, or all addresses are
/*Master*/			//		tried without any success.
/*Master*/			//==============================================================================
/*Master*/			ulFindSlaveAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			ulFindSlaveAddrMax = (u32)uUartCommand[3]*65536L + (u32)uUartCommand[4];
/*Master*/			uPlcState = PLC_FIND_ALL_SLAVES;	// Get ready for response
/*Master*/			uFindSlaves = 1;		// Start searching.
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_ABORT_FIND:
/*Master*/			//==============================================================================
/*Master*/			// Abort searching for all slaves.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		Command acknowledge.
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);	// Command acknowledge.
/*Master*/			uFindSlaves = 0;		// Stop searching.
/*Master*/			uPlcState = PLC_IGNORE;	// back to normal.
/*Master*/
/*Master*/			// Abort Find closest slave if it's running as well.
/*Master*/			// Initialize uClosestSlaveAddr to an invalid address.
/*Master*/			if (uSlaveFound == 0)
/*Master*/			{
/*Master*/				uSlaveFound = 1;
/*Master*/				ulClosestSlaveAddr = 0;
/*Master*/			}
/*Master*/			
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_RESET_MSP:
/*Master*/			//==============================================================================
/*Master*/			// Reset the MSP.
/*Master*/			//
/*Master*/			//	NOTE: There is a fairly long time lag before the MSP comes back!
/*Master*/			//        The host code must wait, or poll for completion.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		Command acknowledge.
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, SUCCESS);	// Command acknowledge.
/*Master*/			ResetEMeter();
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_EMETER_CMD:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Emeter command 
/*Master*/			//
/*Master*/			//  This is a command to the emeter which does NOT expect a reply.
/*Master*/			//  	ex:
/*Master*/			//		'W' Write string to LCD
/*Master*/			//		'D' Display mode
/*Master*/			//		+/- Adjust calibration values
/*Master*/			//		'S' Set time/date
/*Master*/			//
/*Master*/			//  uUartCommand[1-7] = Variable length string to send to emeter
/*Master*/			//		This string must include the command letter prefix and a training \r
/*Master*/			//		if required for the command.  Any unused trailing command parms should
/*Master*/			//		be set to zero.  When the command is sent, one additional parm of 0
/*Master*/			//		will be tacked on.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 	NOTHING. Command acknowledge handled when emeter communication is done by
/*Master*/			//			setting uAckAfter485 flag.
/*Master*/			//
/*Master*/			// NOTE: Most changes to this function should have corresponding changes made
/*Master*/			//			in the SC_EMETER_CMD case below.
/*Master*/			//==============================================================================
/*Master*/			emeterStringIndex = 0;
/*Master*/			for(i=1; i<=7; i++)
/*Master*/			{
/*Master*/				emeterMsgString[emeterStringIndex++] = (uUartCommand[i]&0xFF00)>>8;
/*Master*/				emeterMsgString[emeterStringIndex++] = (uUartCommand[i]&0x00FF);
/*Master*/			}
/*Master*/			emeterMsgString[emeterStringIndex] = 0;	// Add a null in case the received string didn't have one.
/*Master*/			WriteUARTString(emeterMsgString);		// Send command to emeter.
/*Master*/			
/*Master*/			uAckAfter485 = 1;		// Notify host after 485 communication is complete.
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_EMETER_REQ:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Emeter request 
/*Master*/			//
/*Master*/			//  This is a command to the emeter which DOES expect a reply.
/*Master*/			//
/*Master*/			//  uUartCommand[1-7] = Variable length string to send to emeter
/*Master*/			//		This string must include the command letter prefix and a training \r
/*Master*/			//		if required for the command.  Any unused trailing command parms should
/*Master*/			//		be set to zero.  When the command is sent, one additional parm of 0
/*Master*/			//		will be tacked on.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 	NOTHING.  Command response to host will be sent with emeter data.
/*Master*/			//
/*Master*/			// NOTE: Most changes to this function should have corresponding changes made
/*Master*/			//			in the SC_EMETER_CMD case below.
/*Master*/			//==============================================================================
/*Master*/			emeterStringIndex = 0;
/*Master*/			for(i=1; i<=7; i++)
/*Master*/			{
/*Master*/				emeterMsgString[emeterStringIndex++] = (uUartCommand[i]&0xFF00)>>8;
/*Master*/				emeterMsgString[emeterStringIndex++] = (uUartCommand[i]&0x00FF);
/*Master*/			}
/*Master*/			emeterMsgString[emeterStringIndex] = 0;	// Add a null in case the received string didn't have one.
/*Master*/			WriteUARTString(emeterMsgString);
/*Master*/			
/*Master*/			uUartState = UART_RECEIVE_RESPONSE;
/*Master*/			memset(upUartResponseBuffer, 0, 61*sizeof(upUartResponseBuffer[0]));	
/*Master*/			upUartResponseBuffer[0] = SUCCESS;
/*Master*/			uUartResponseIndex = 2;		// offset in bytes to start reading into receive buffer.
/*Master*/			ulUartCounter = 0L;			// Reset timeout counter
/*Master*/			break;
/*Master*/			
/*Master*/		case MC_GET_SLAVE_PARM:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Get slave parm.
/*Master*/			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/			// 	uUartCommand[3] = index into address table of requested value.
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//  txUserDataArray[5] = index into configuration table.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		uPlcState is set to generate the return code for the response.
/*Master*/			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_GET_PARM;	// Command number = Get parm.
/*Master*/			txUserDataArray[5] = uUartCommand[3];	// parm index that we're requesting.
/*Master*/			uHostResponseStart = 4;				// Slaves response data will follow two address parms.
/*Master*/			if(uUartCommand[3] <= 30)			// Requesting 16-bit parm
/*Master*/			{
/*Master*/				uHostResponseLength = 2;		// Acknowlege and single value.
/*Master*/			}
/*Master*/			else								// Requesting 32-bit parm
/*Master*/			{
/*Master*/				uHostResponseLength = 3;		// Acknowlege and double value.
/*Master*/			}
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;	// Get ready for response
/*Master*/			ulPlcResponseTimeoutCounter = 0;	// Reset timeout counter.
/*Master*/			uTransmitPacketReady = 1;			// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/	
/*Master*/		case MC_SET_SLAVE_PARM:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Get slave parm.
/*Master*/			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/			// 	uUartCommand[3] = index into address table of requested value.
/*Master*/			//  uUartCommand[4] = value (high word for 32-bit parms)
/*Master*/			//  uUartCommand[5] = (low word for 32-bit parms)
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//  txUserDataArray[5] = index into configuration table.
/*Master*/			//  txUserDataArray[6] = value (high word).
/*Master*/			//  txUserDataArray[7] = value (low word).
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		uPlcState is set to generate the return code for the response.
/*Master*/			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_SET_PARM;		// Command number = Set parm.
/*Master*/			txUserDataArray[5] = uUartCommand[3];	// parm index that we're setting.
/*Master*/			txUserDataArray[6] = uUartCommand[4];	// value (high word)
/*Master*/			txUserDataArray[7] = uUartCommand[5];	// value (low word).
/*Master*/			uHostResponseStart = 4;					// Slaves response data will follow two address parms.
/*Master*/			uHostResponseLength = 1;				// Acknowledge.
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;		// Get ready for response
/*Master*/			ulPlcResponseTimeoutCounter = 0;		// Reset timeout counter.
/*Master*/			uTransmitPacketReady = 1;				// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/
/*Master*/		case MC_GET_SLAVE_STATUS:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Get slave status.
/*Master*/			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		uPlcState is set to generate the return code for the response.
/*Master*/			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_GET_STATUS;	// Command number = Get status
/*Master*/			uHostResponseStart = 4;				// Slaves response data will follow two address parms.
/*Master*/			uHostResponseLength = 1+50;			// Acknowledge and 50 words of status data.
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;	// Get ready for response
/*Master*/			ulPlcResponseTimeoutCounter = 0L;	// Reset timeout counter.
/*Master*/			uTransmitPacketReady = 1;			// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/
/*Master*/
/*Master*/		case MC_READ_SLAVE_BLOCK:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Read memory block from slave.
/*Master*/			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/			// 	uUartCommand[3] = starting address
/*Master*/			// 	uUartCommand[4] = word count (<=60)
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//  txUserDataArray[5] = starting address
/*Master*/			//  txUserDataArray[6] = block length (words).  <= 60
/*Master*/			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_READ_BLOCK;		// Command number = Get status
/*Master*/			txUserDataArray[5] = uUartCommand[3]; 	// Starting Address.
/*Master*/			txUserDataArray[6] = uUartCommand[4]; 	// Block Length
/*Master*/			uHostResponseStart = 4;					// Slaves response data will follow two address parms.
/*Master*/			uHostResponseLength = 1+uUartCommand[4]; // Acknowledge plus requested data.
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;		// Get ready for response
/*Master*/			ulPlcResponseTimeoutCounter = 0L; 				// Reset timeout counter.
/*Master*/			uTransmitPacketReady = 1;				// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/			
/*Master*/ 		case MC_ERASE_SLAVE_FLASH:
/*Master*/ 			//==============================================================================
/*Master*/ 			// Command:  Erase Slave Flash
/*Master*/ 			//           Only able to erase log area on slave. (not program spaces)
/*Master*/ 			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/ 			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/ 			//
/*Master*/ 			//  txUserDataArray[0] = dest addr - high
/*Master*/ 			//  txUserDataArray[1] = dest addr - low
/*Master*/ 			//  txUserDataArray[2] = source addr - high
/*Master*/ 			//  txUserDataArray[3] = source addr - low
/*Master*/ 			//  txUserDataArray[4] = command number
/*Master*/ 			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_ERASE_FLASH;	// Command number
/*Master*/			uHostResponseStart = 4;					// Slaves response data will follow two address parms.
/*Master*/			uHostResponseLength = 1; 				// Command acknowledge.
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;		// Get ready for response
/*Master*/			ulPlcResponseTimeoutCounter = 0L;		// Reset timeout counter.
/*Master*/			uTransmitPacketReady = 1;				// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/ 
/*Master*/ 		case MC_SLAVE_READ_FLASH:
/*Master*/ 			//==============================================================================
/*Master*/ 			// Command:  Read Slave Flash
/*Master*/ 			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/ 			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/ 			// 	uUartCommand[3] = Starting address - high.
/*Master*/ 			// 	uUartCommand[4] = Starting address - low.
/*Master*/ 			// 	uUartCommand[5] = Count (words) to read. <= 60.
/*Master*/ 			//
/*Master*/ 			//  txUserDataArray[0] = dest addr - high
/*Master*/ 			//  txUserDataArray[1] = dest addr - low
/*Master*/ 			//  txUserDataArray[2] = source addr - high
/*Master*/ 			//  txUserDataArray[3] = source addr - low
/*Master*/ 			//  txUserDataArray[4] = command number
/*Master*/ 			//  txUserDataArray[5] = starting address - high
/*Master*/ 			//  txUserDataArray[6] = starting address - low
/*Master*/ 			//  txUserDataArray[7] = block length (words).  <= 60
/*Master*/ 			//==============================================================================
/*Master*/ 			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/ 
/*Master*/ 			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/ 			{								// 0L --> use "uClosestSlave"
/*Master*/ 				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/ 			}
/*Master*/ 			else								// else used passed arguement for slave addr
/*Master*/ 			{
/*Master*/ 				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/ 			}
/*Master*/ 	
/*Master*/ 			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/ 			txUserDataArray[4] = SC_READ_FLASH;		// Command number = Get status
/*Master*/ 			txUserDataArray[5] = uUartCommand[3]; 	// Starting Address - high.
/*Master*/ 			txUserDataArray[6] = uUartCommand[4]; 	// Starting Address - low.
/*Master*/ 			txUserDataArray[7] = uUartCommand[5]; 	// Block Length
/*Master*/ 			uHostResponseStart = 4;					// Slaves response data will follow two address parms.
/*Master*/ 			uHostResponseLength = 1+uUartCommand[4]; // Acknowledge plus requested data.
/*Master*/ 			uPlcState = PLC_RECEIVE_RESPONSE;		// Get ready for response
/*Master*/ 			ulPlcResponseTimeoutCounter = 0L;					// Reset timeout counter.
/*Master*/ 			uTransmitPacketReady = 1;				// Send completed command to the SLAVE.
/*Master*/ 			break;
/*Master*/			
/*Master*/ 		case MC_SLAVE_RESET_MSP:
/*Master*/ 			//==============================================================================
/*Master*/ 			// Command:  Reset Slave MSP430
/*Master*/ 			//
/*Master*/ 			//  txUserDataArray[0] = dest addr - high
/*Master*/ 			//  txUserDataArray[1] = dest addr - low
/*Master*/ 			//  txUserDataArray[2] = source addr - high
/*Master*/ 			//  txUserDataArray[3] = source addr - low
/*Master*/ 			//  txUserDataArray[4] = command number
/*Master*/			//
/*Master*/			//	NOTE: There is a fairly long time lag before the MSP comes back!
/*Master*/			//        The host code must wait, or poll for completion.
/*Master*/ 			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_RESET_MSP;		// Command number
/*Master*/			uHostResponseStart = 4;					// Slaves response data will follow two address parms.
/*Master*/			uHostResponseLength = 1; 				// Command acknowledge.
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;		// Get ready for response
/*Master*/			ulPlcResponseTimeoutCounter = 0L;		// Reset timeout counter.
/*Master*/			uTransmitPacketReady = 1;				// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/ 
/*Master*/		case MC_SLAVE_EMETER_CMD:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Send a command to the slave emeter
/*Master*/			//
/*Master*/			//  This is a command to the slave emeter which doesn't expect a reply.  The
/*Master*/			//	slave will still respond when it's 485 communication is finished, but only
/*Master*/			//	the one word status is valid in it's response, and only that one word is
/*Master*/			//	sent back to the host.
/*Master*/			//
/*Master*/			// Unless the address is broadcast - then this command responds to the host
/*Master*/			// and the slave will not respond to the Master.
/*Master*/			//
/*Master*/			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/			//	uUartCommand[3-7] = Command string.
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//  txUserDataArray[5-9] = Command string.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		uPlcState is set to receive the response.
/*Master*/			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_EMETER_CMD;		// Command number = Issue slave emeter command.
/*Master*/			txUserDataArray[5] = uUartCommand[3];
/*Master*/			txUserDataArray[6] = uUartCommand[4];
/*Master*/			txUserDataArray[7] = uUartCommand[5];
/*Master*/			txUserDataArray[8] = uUartCommand[6];
/*Master*/			txUserDataArray[9] = uUartCommand[7];
/*Master*/
/*Master*/			// Test for broadcast case - if so, the Master sends the acknowledge.
/*Master*/			if( ulAddr == 1L)		// broadcast.
/*Master*/			{
/*Master*/				WriteUARTValue(UART_TARGET_HOST, SUCCESS);	// Command acknowledge.
/*Master*/				uPlcState = PLC_IGNORE;						// back to normal.
/*Master*/				ulAutoPollCounter = 0L;
/*Master*/				uAutoPollPause=0;							// Release AutoPoll.
/*Master*/			}
/*Master*/			else
/*Master*/			{
/*Master*/				uHostResponseStart = 4;				// Slaves response data will follow two address parms.
/*Master*/				uHostResponseLength = 1;			// Return only the status word.
/*Master*/				uPlcState = PLC_RECEIVE_RESPONSE;	// Get ready for response
/*Master*/				ulPlcResponseTimeoutCounter = 0L;	// Reset timeout counter.
/*Master*/			}
/*Master*/			uTransmitPacketReady = 1;			// Send completed command to the SLAVE.
/*Master*/
/*Master*/			break;
/*Master*/
/*Master*/		case MC_SLAVE_EMETER_REQ:
/*Master*/			//==============================================================================
/*Master*/			// Command:  Send a request to the slave emeter
/*Master*/			//
/*Master*/			//  This is a command to the slave which expect a meaningful reply which will
/*Master*/			//	be relayed to the host (the entire PLC packet).
/*Master*/			//
/*Master*/			// 	uUartCommand[1] = Slave address - high (0 for closest).
/*Master*/			// 	uUartCommand[2] = Slave address - low  (0 for closest).
/*Master*/			//	uUartCommand[3-7] = Command string.
/*Master*/			//
/*Master*/			//  txUserDataArray[0] = dest addr - high
/*Master*/			//  txUserDataArray[1] = dest addr - low
/*Master*/			//  txUserDataArray[2] = source addr - high
/*Master*/			//  txUserDataArray[3] = source addr - low
/*Master*/			//  txUserDataArray[4] = command number
/*Master*/			//  txUserDataArray[5-9] = Command string.
/*Master*/			//
/*Master*/			// return:
/*Master*/			// 		uPlcState is set to receive the response.
/*Master*/			//==============================================================================
/*Master*/			ulAddr = (u32)uUartCommand[1]*65536L + (u32)uUartCommand[2];
/*Master*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Master*/
/*Master*/			if(ulAddr == 0L)				// Command[1] specifies slave address
/*Master*/			{								// 0L --> use "uClosestSlave"
/*Master*/				*(u32*)&txUserDataArray[0] = ulClosestSlaveAddr;
/*Master*/			}
/*Master*/			else								// else used passed arguement for slave addr
/*Master*/			{
/*Master*/				*(u32*)&txUserDataArray[0] = ulAddr;
/*Master*/			}
/*Master*/	
/*Master*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Master*/			txUserDataArray[4] = SC_EMETER_REQ;		// Command number = Issue slave emeter request.
/*Master*/			txUserDataArray[5] = uUartCommand[3];
/*Master*/			txUserDataArray[6] = uUartCommand[4];
/*Master*/			txUserDataArray[7] = uUartCommand[5];
/*Master*/			txUserDataArray[8] = uUartCommand[6];
/*Master*/			txUserDataArray[9] = uUartCommand[7];
/*Master*/			uHostResponseStart = 4;				// Slaves response data will follow two address parms.
/*Master*/			uHostResponseLength = 61;			// Return all emeter data.
/*Master*/			ulPlcResponseTimeoutCounter = 0L;			// Reset timeout counter.
/*Master*/			uPlcState = PLC_RECEIVE_RESPONSE;	// Get ready for response
/*Master*/			uTransmitPacketReady = 1;			// Send completed command to the SLAVE.
/*Master*/			break;
/*Master*/
/*Master*/		default:
/*Master*/			//==============================================================================
/*Master*/			// Unrecognized command
/*Master*/			// Respond with unknown master command error code...
/*Master*/			//==============================================================================
/*Master*/			WriteUARTValue(UART_TARGET_HOST, ERR_M_UNKNOWN_COMMAND);
/*Master*/			ulAutoPollCounter = 0L;
/*Master*/			uAutoPollPause=0;		// Release AutoPoll.
/*Master*/	}
/*Master*/
/*Master*/	return;
}


//==========================================================================================
// Function:		ProcessSlaveCommand()
//
// Description: 	This function interprets and executes a command from the PL for the
//					SLAVE.  Commands must either be executed quickly, or set up state
//					variables which allow additional processing later.  They may not
//					tie up the processor for extended processing.
//
//					A command is received in a 128 byte PL packet with the following format:
//						parm	description
//						0		destination address - high
//						1		destination address - low
//						2		souce address - high
//						3		souce address - low
//						4		command number
//						5-63	optional command parameters (described for each cmd below)
//
//					Each command is responded to with a 128 byte response.
//					The first four parms of the response are the standard packet header
//					containing the source and destination addresses.  The remaining
//					60 parms are dependent on the command received.
//
//
// Revision History:
//==========================================================================================
void ProcessSlaveCommand(void)
{
/*Slave*/	u16		i;			// Loop index
/*Slave*/	u32		ulBerSequence;
/*Slave*/	u16		emeterStringIndex;
/*Slave*/	u32		ulAddr;				// 32-bit address formed from received data
/*Slave*/
/*Slave*/	switch( rxUserDataCopy[4] )			// Action depends on command received
/*Slave*/	{
/*Slave*/		case SC_GET_PARM:
/*Slave*/			//==============================================================================
/*Slave*/			// Command:  Get slave parm.
/*Slave*/			//
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5] = index into configuration table.
/*Slave*/			//
/*Slave*/			// Return:
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = Status / Error
/*Slave*/			// txUserDataArray[5] = Data (high word if 32-bit parm).
/*Slave*/			// txUserDataArray[6] = Data (low word if 32-bit parm).
/*Slave*/			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/
/*Slave*/			// ***** Error checking for parm number < 40
/*Slave*/			txUserDataArray[4] = SUCCESS;	// command status
/*Slave*/			if(rxUserDataCopy[5] < 30)	// request for 16-bit parm
/*Slave*/			{
/*Slave*/				txUserDataArray[5] = *(uppParms16[rxUserDataCopy[5]]);
/*Slave*/			}
/*Slave*/			else		// request for 32-bit parm
/*Slave*/			{
/*Slave*/				txUserDataArray[5] = *((u16*)(uppParms32[rxUserDataCopy[5]-30]));		// high word
/*Slave*/				txUserDataArray[6] = *((u16*)(uppParms32[rxUserDataCopy[5]-30]) + 1);	// low word
/*Slave*/			}
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/
/*Slave*/		case SC_SET_PARM:
/*Slave*/			//==============================================================================
/*Slave*/			// Set slave parm.
/*Slave*/			//
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5] = index into configuration table.
/*Slave*/			// rxUserDataCopy[6] = Data (high word if 32-bit parm).
/*Slave*/			// rxUserDataCopy[7] = Data (low word if 32-bit parm).
/*Slave*/			//
/*Slave*/			// Return:
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = Status / Error
/*Slave*/			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/
/*Slave*/			// ***** Error checking for parm number < 40
/*Slave*/			txUserDataArray[4] = SUCCESS;	// command status
/*Slave*/			if(rxUserDataCopy[5] < 30)	// request for 16-bit parm
/*Slave*/			{
/*Slave*/				*(uppParms16[rxUserDataCopy[5]]) = rxUserDataCopy[6];
/*Slave*/			}
/*Slave*/			else		// request for 32-bit parm
/*Slave*/			{
/*Slave*/				*((u16*)(uppParms32[rxUserDataCopy[5]-30])) = rxUserDataCopy[6];		// high word
/*Slave*/				*((u16*)(uppParms32[rxUserDataCopy[5]-30]) + 1) = rxUserDataCopy[7];	// low word
/*Slave*/			}
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/
/*Slave*/		case SC_GET_STATUS:
/*Slave*/			//==============================================================================
/*Slave*/			// Command:  Get status.
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			//
/*Slave*/			// Return:
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = Status / Error
/*Slave*/			//
/*Slave*/			// return:
/*Slave*/			// 		If the requested configuration parameter is valid, SUCCESS is returned
/*Slave*/			//		for an acknowledge followed by the status array values.
/*Slave*/			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = SUCCESS;	// command status
/*Slave*/
/*Slave*/			// return parms 5-34
/*Slave*/			for(i=0; i<30; i++)		// 16-bit parms
/*Slave*/			{
/*Slave*/				txUserDataArray[5+i] = *(uppParms16[i]);
/*Slave*/			}
/*Slave*/			// return parms 35-54
/*Slave*/			for(i=0; i<10; i++)		// 32-bit parms
/*Slave*/			{
/*Slave*/				txUserDataArray[35+i*2] = *((u16*)(uppParms32[i]));		// high word
/*Slave*/				txUserDataArray[35+i*2+1] = *((u16*)(uppParms32[i]) + 1);	// low word
/*Slave*/			}
/*Slave*/
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/
/*Slave*/	 	case SC_BER:
/*Slave*/	 		//==============================================================================
/*Slave*/	 		// BER test.
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5] = BER count (high word).
/*Slave*/			// rxUserDataCopy[6] = BER count (low word).
/*Slave*/			// 
/*Slave*/			// When a Bit Error Rate (BER) packet is received, the slave keeps track of
/*Slave*/			// received packets, missed packets and parity errors.
/*Slave*/			// 
/*Slave*/			// Additionally if the BER count (an incrementing sequence from the Master)
/*Slave*/			// is 0, the slave resets all of its counters.
/*Slave*/	 		//==============================================================================
/*Slave*/			// Get the passed sequence number.
/*Slave*/	 		ulBerSequence = (u32) rxUserDataCopy[5]*65536L + (u32) rxUserDataCopy[6];
/*Slave*/	 		uBerTestInProgress = 1;
/*Slave*/				
/*Slave*/			// If the sequence number = 0, reset the counters.
/*Slave*/	 		if(ulBerSequence == 0L)
/*Slave*/			{
/*Slave*/				uBerMissedPackets = 0;
/*Slave*/				uBerErrorCounter = 0;
/*Slave*/				ulBerCounter = 0;
/*Slave*/			}
/*Slave*/			// The received 'ulBerSequence' should be equal to our local counter,
/*Slave*/			// 'ulBerCounter' plus the number of packets we missed due to CRC 
/*Slave*/			// errors, 'uBerErrorCounter', plus the number of packets we completely
/*Slave*/			// missed, 'uBerMissedPackets.
/*Slave*/			// If the received sequence number is greater than the expected sum,
/*Slave*/			// we missed the last packet, so increment the missed counter.
/*Slave*/			if( ulBerCounter++ + uBerMissedPackets + uBerErrorCounter < ulBerSequence)
/*Slave*/			{
/*Slave*/				uBerMissedPackets++;
/*Slave*/			}
/*Slave*/			// If the calculated sum is greater than the sequence number, somehow
/*Slave*/			// we must have incremented the CRC counter due to something other than
/*Slave*/			// an expected BER packet - most likely an error receiving the status
/*Slave*/			// request packet.  (Possibly traffic from another platform??)
/*Slave*/			// That CRC error shouldn't be included in the bit error rate, so take
/*Slave*/			// it back out (it's not included in the total, so don't include it in
/*Slave*/			// the errors).
/*Slave*/			// -1 becuse we incremented ulBerCounter in the previous conditional.
/*Slave*/			else if( (ulBerCounter + uBerMissedPackets + uBerErrorCounter -1) > ulBerSequence)
/*Slave*/			{
/*Slave*/				uBerErrorCounter--;
/*Slave*/			}
/*Slave*/			
/*Slave*/			uCommandBlock = 0;		// Re-enable packet reception.
/*Slave*/									// This gets re-enabled in SendPLC() for most other
/*Slave*/									// commands.
/*Slave*/	 		break;
/*Slave*/	 
/*Slave*/	 	case SC_ABORT_BER:
/*Slave*/	 		//==============================================================================
/*Slave*/	 		// Abort BER test.
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/	 		//==============================================================================
/*Slave*/	 		uBerTestInProgress = 0;
/*Slave*/			uCommandBlock = 0;		// Re-enable packet reception.
/*Slave*/									// This gets re-enabled in SendPLC() for most other
/*Slave*/									// commands.
/*Slave*/	 		break;
/*Slave*/
/*Slave*/		case SC_AUTOPOLL:
/*Slave*/			//==============================================================================
/*Slave*/			// Poll for slave - Someone was looking for us.
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			//
/*Slave*/			// Return:
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = SUCCESS
/*Slave*/			//==============================================================================
/*Slave*/			// No command to execute.  Just respond if we get to this point.
/*Slave*/
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = SUCCESS;			// command status
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/
/*Slave*/		case SC_READ_BLOCK:
/*Slave*/			//==============================================================================
/*Slave*/			// Command:  Read Block.
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5] = starting address
/*Slave*/			// rxUserDataCopy[6] = block length (words).  <= 60
/*Slave*/			//
/*Slave*/			// Return:
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = Status / Error
/*Slave*/			// txUserDataArray[5-] = data
/*Slave*/			//
/*Slave*/			// return:
/*Slave*/			// 		If the requested configuration parameter is valid, SUCCESS is returned
/*Slave*/			//		for an acknowledge followed by the status array values.
/*Slave*/			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = SUCCESS;	// command status
/*Slave*/
/*Slave*/			// return parms 5-34
/*Slave*/			for(i=0; i<rxUserDataCopy[6]; i++)		// 16-bit parms
/*Slave*/			{
/*Slave*/				txUserDataArray[5+i] = *(u16*)(rxUserDataCopy[5]+i);
/*Slave*/			}
/*Slave*/
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/			
/*Slave*/ 		case SC_ERASE_FLASH:
/*Slave*/ 			//==============================================================================
/*Slave*/ 			// Command:  Erase Flash
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/ 			//
/*Slave*/ 			// Only option for target on slave is log space.
/*Slave*/ 			//
/*Slave*/ 			// return:
/*Slave*/ 			// 		Nothing here.  The return is send in dsp_modem.c
/*Slave*/ 			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = SUCCESS;	// command status
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/ 			uFlashTarget = 2;		// 2 = log space
/*Slave*/ 			uFlashWriteStatus = FLASH_STATUS_ERASING;
/*Slave*/ 			break;
/*Slave*/ 
/*Slave*/ 		case SC_READ_FLASH:
/*Slave*/ 			//==============================================================================
/*Slave*/ 			// Command:  Read Flash
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5] = starting address - high
/*Slave*/			// rxUserDataCopy[6] = starting address - low
/*Slave*/			// rxUserDataCopy[7] = block length (words).  <= 60
/*Slave*/			//
/*Slave*/			// Return:
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = Status / Error
/*Slave*/			// txUserDataArray[5-] = data
/*Slave*/ 			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = SUCCESS;	// command status
/*Slave*/
/*Slave*/			// return parms 5-34
/*Slave*/ 			ulAddr = (u32)rxUserDataCopy[5]*65536L + (u32)rxUserDataCopy[6];
/*Slave*/ 			for(i=0; i<rxUserDataCopy[7]; i++)
/*Slave*/ 			{
/*Slave*/ 				txUserDataArray[5+i] = ReadFlash(ulAddr++);
/*Slave*/ 			}
/*Slave*/
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/
/*Slave*/		case SC_RESET_MSP:
/*Slave*/			//==============================================================================
/*Slave*/			// Reset the MSP.
/*Slave*/			//
/*Slave*/			//	NOTE: There is a fairly long time lag before the MSP comes back!
/*Slave*/			//        The host code must wait, or poll for completion.
/*Slave*/			//
/*Slave*/			// return:
/*Slave*/			// 		Command acknowledge.
/*Slave*/			//==============================================================================
/*Slave*/			ResetEMeter();
/*Slave*/
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = SUCCESS;			// command status
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/			break;
/*Slave*/			
/*Slave*/		case SC_EMETER_CMD:
/*Slave*/			//==============================================================================
/*Slave*/			// Command:  Emeter command
/*Slave*/			//
/*Slave*/			//  This is a command to the emeter which does NOT expect a reply.
/*Slave*/			//  	ex:
/*Slave*/			//		'W' Write string to LCD
/*Slave*/			//		'D' Display mode
/*Slave*/			//		+/- Adjust calibration values
/*Slave*/			//		'S' Set time/date
/*Slave*/			//
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5-9] = String containing command.
/*Slave*/			//
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = SUCCESS
/*Slave*/			// txUserDataArray[5-61] = Return data.
/*Slave*/			//
/*Slave*/			// NOTE: Most changes to this function should have corresponding changes made
/*Slave*/			//			in the MC_EMETER_CMD case above.
/*Slave*/			//==============================================================================
/*Slave*/			emeterStringIndex = 0;
/*Slave*/			for(i=5; i<=9; i++)
/*Slave*/			{
/*Slave*/				emeterMsgString[emeterStringIndex++] = (rxUserDataCopy[i]&0xFF00)>>8;
/*Slave*/				emeterMsgString[emeterStringIndex++] = (rxUserDataCopy[i]&0x00FF);
/*Slave*/			}
/*Slave*/			emeterMsgString[emeterStringIndex] = 0;	// Add a null in case the received string didn't have one.
/*Slave*/			WriteUARTString(emeterMsgString);		// send command to emeter.
/*Slave*/			
/*Slave*/			// This is an emeter command which does not return anything.
/*Slave*/			// A fake return of all zeros will sent after the 485 communication is done
/*Slave*/			// if this is sent to a specific slave.  If it is a broadcast message the
/*Slave*/			// Master will have already taken care of the acknowledge, so don't return
/*Slave*/			// anything.
/*Slave*/			if(*(u32*)&rxUserDataArray[0] == 1L)		// This was a broadcast command. Don't respond.
/*Slave*/			{
/*Slave*/				uAckAfter485 = 0;		// Don't send ack.
/*Slave*/				uCommandBlock = 0;		// Re-enable packet reception.
/*Slave*/										// This gets re-enabled in SendPLC() for most other
/*Slave*/										// commands.
/*Slave*/			}
/*Slave*/			else
/*Slave*/			{
/*Slave*/				memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
/*Slave*/				txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/				txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/				*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/	  			txUserDataArray[4] = SUCCESS;
/*Slave*/				uAckAfter485 = 1;		// Set flag to send ack after 485 traffic.
/*Slave*/			}
/*Slave*/			break;
/*Slave*/
/*Slave*/		case SC_EMETER_REQ:
/*Slave*/			//==============================================================================
/*Slave*/			// Command:  Emeter request
/*Slave*/			//
/*Slave*/			//  This is a command to the emeter which DOES expect a reply.
/*Slave*/			//
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			// rxUserDataCopy[5-9] = String containing command.
/*Slave*/			//
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = SUCCESS
/*Slave*/			// txUserDataArray[5-61] = Return data.
/*Slave*/			//
/*Slave*/			// NOTE: Most changes to this function should have corresponding changes made
/*Slave*/			//			in the MC_EMETER_CMD case above.
/*Slave*/			//==============================================================================
/*Slave*/			emeterStringIndex = 0;
/*Slave*/			for(i=5; i<=9; i++)
/*Slave*/			{
/*Slave*/				emeterMsgString[emeterStringIndex++] = (rxUserDataCopy[i]&0xFF00)>>8;
/*Slave*/				emeterMsgString[emeterStringIndex++] = (rxUserDataCopy[i]&0x00FF);
/*Slave*/			}
/*Slave*/			emeterMsgString[emeterStringIndex] = 0;	// Add a null in case the received string didn't have one.
/*Slave*/			WriteUARTString(emeterMsgString);
/*Slave*/			
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	// Fill User Data buffer with zeroes
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/	  		txUserDataArray[4] = SUCCESS;
/*Slave*/			
/*Slave*/			uUartResponseIndex = 10;		// Offset in bytes!
/*Slave*/			uUartState = UART_RECEIVE_RESPONSE;
/*Slave*/			memset(upUartResponseBuffer, 0, 61*sizeof(upUartResponseBuffer[0]));	
/*Slave*/			ulUartCounter = 0L;				// Reset counter used for timeout detection.
/*Slave*/			break;
/*Slave*/
/*Slave*/		default:
/*Slave*/			//==============================================================================
/*Slave*/			// Unrecognized command
/*Slave*/			// rxUserDataCopy[0] = dest addr - high
/*Slave*/			// rxUserDataCopy[1] = dest addr - low
/*Slave*/			// rxUserDataCopy[2] = source addr - high
/*Slave*/			// rxUserDataCopy[3] = source addr - low
/*Slave*/			// rxUserDataCopy[4] = command number
/*Slave*/			//
/*Slave*/			// Respond with unknown slave command error code...
/*Slave*/			// txUserDataArray[0] = Master addr - high
/*Slave*/			// txUserDataArray[1] = Master addr - low
/*Slave*/			// txUserDataArray[2] = MyAddr - high
/*Slave*/			// txUserDataArray[3] = MyAddr - low
/*Slave*/			// txUserDataArray[4] = Error
/*Slave*/			//==============================================================================
/*Slave*/			// Fill User Data buffer with zeroes
/*Slave*/			memset(txUserDataArray, 0, DATA_BUFFER_LEN*sizeof(txUserDataArray[0]));	
/*Slave*/			txUserDataArray[0] = rxUserDataCopy[2];	// Master address
/*Slave*/			txUserDataArray[1] = rxUserDataCopy[3];	// Master address
/*Slave*/			*(u32*)&txUserDataArray[2] = ulMyAddr;
/*Slave*/			txUserDataArray[4] = ERR_S_UNKNOWN_COMMAND;
/*Slave*/			uTransmitPacketReady = 1;
/*Slave*/			uPlcState = PLC_RECEIVE_COMMAND;
/*Slave*/	}
/*Slave*/
/*Slave*/	return;
}


//==========================================================================================
// Function:		ResetEMeter()
//
// Description: 	This function pulses a GPIO output line from the DSP that resets the 
//					E-Meter's processor.
//
// Revision History:
//==========================================================================================
void	ResetEMeter(void)
{
	#define	EMETER_RESET_MASK	(1<<6)
	#define EMETER_RESET		(1<<6)
	#define EMETER_RUN			(0<<6)
	AssignBits(GPIOSR, EMETER_RESET_MASK, EMETER_RESET);	// Turn on E-Meter reset line
	DelayNus(10);											// Wait 10 us	
	AssignBits(GPIOSR, EMETER_RESET_MASK, EMETER_RUN);		// Turn off E-Meter reset line 	
}
	
	
	

