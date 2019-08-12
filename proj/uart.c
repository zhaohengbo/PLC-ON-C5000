//==========================================================================================
// Filename:		uart.c
//
// Description:		Serial port rountines to handle the interface to the host (either
//					RS232 or USB) and communications with the meter (either RS485 or SPI).
//
// Copyright (C) 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"		// Platform's main header file.
#include "uart.h"			// 5402DSK UART library header file.
#include "intr.h"



//==========================================================================================
// Function:		ConfigureUART()
//
// Description: 	This function initializes and configures the UART to talk to either the
//					host PC or the emeter.
//					It should be called once in the beginning of code.
//
// Revision History:
//==========================================================================================
void ConfigureUART(void)
{
	volatile u16	temp;		// volatile prevents warnings about unused variable...

	// Disable all uart interrupts.
	UART_IER_REG &= 0xF0;		// Clear interrupt enable
	// Note that the transmitter empty interrupt is used, but only enabled when needed.
	   
	// Enable CPU INT1 for uart interrupts from 16C550
	INTR_CLR_FLAG(UART_INTR_FLAG); 	// Clear Interrupt Flags
	INTR_ENABLE(UART_INTR_FLAG);

	// Make sure DLAB = 0.
	UART_LCR_REG &= ~DLAB_MASK;			// DLAB = 0

	// This was part of the uart_reset function of the library uart routine.
	// (I'm not sure it's all required, but doesn't look like it'll hurt anything...)
	temp = UART_RBR_REG;	// dummy read of rx buffer register
	UART_THR_REG = 0;		// dummy write of tx holding register
	UART_FCR_REG = 0x07; 	// reset tx and rx FIFOs
	UART_FCR_REG = 0;		// disable FIFO
	UART_MCR_REG = 0x26; 	// enable autoflow control.  enable RS232.
	UART_LSR_REG = 0x60; 	// THRE & TEMT set
	UART_MSR_REG = 0;		// all bits cleared
	DebugDelay();				// Flush C54x if in debug mode.


	// ---------- Baud Rate Setup ----------	
#define	UART_BAUD_RATE	115200

	UART_LCR_REG |= DLAB_MASK;		// DLAB = 1  Unlock the baud rate registers
	DebugDelay();				// Flush C54x if in debug mode.


#if UART_BAUD_RATE == 9600
	// BAUD rate: Target = 9600 bps, Actual = 9566 bps(0.3% slow)
	// Set by a divisor for the input clock, which is an output from McBSP CLKX2.
	// CLKX2 is set above to be CPUCLK (150 MHz) / 10 = 15.0 MHz
	// 15.0 MHz / (9600 * 16) = 97.65625
	UART_DLL_REG = 98;				// LSB
	UART_DLM_REG = 0;				// MSB

#elif UART_BAUD_RATE == 57600	
	// BAUD rate: Target = 57600 bps, Actual = 58,593 bps(1.7% fast)
	// Set by a divisor for the input clock, which is an output from McBSP CLKX2.
	// CLKX2 is set above to be CPUCLK (150 MHz) / 10 = 15.0 MHz
	// 15.0 MHz / (57600 * 16) = 16.276
	UART_DLL_REG = 16;				// LSB
	UART_DLM_REG = 0;				// MSB

#elif UART_BAUD_RATE == 115200
	// BAUD rate: Target = 115.200kbps, Actual = 117.187kbps (1.7% fast)
	// Set by a divisor for the input clock, which is an output from McBSP CLKX2.
	// CLKX2 is set above to be CPUCLK (150 MHz) / 10 = 15.0 MHz
	// 15.0 MHz / (115200 * 16) = 8.13
	UART_DLL_REG = 8;				// LSB
	UART_DLM_REG = 0;				// MSB
#endif

	DebugDelay();				// Flush C54x if in debug mode.
	UART_LCR_REG &= ~DLAB_MASK;		// DLAB = 0  Lock the baud rate registers
	DebugDelay();					// Flush C54x if in debug mode.
	
	// Set word size to 8 bits.
	UART_LCR_REG |= 0x3;
	DebugDelay();				// Flush C54x if in debug mode.

	// Set 2 stop bits.
	UART_LCR_REG |= UART_STOPBIT_ENABLE;	// set bit 2 
	DebugDelay();				// Flush C54x if in debug mode.

	// Parity control - disable.
	UART_LCR_REG &= ~UART_PARITY_ENABLE;
	DebugDelay();				// Flush C54x if in debug mode.

	// Break bit clear.
	UART_LCR_REG &= 0xBF;
	DebugDelay();				// Flush C54x if in debug mode.
	
	// FIFO control - enable.
	UART_FCR_REG = 0x7; 	// Flush rx and tx FIFOs
	DebugDelay();				// Flush C54x if in debug mode.
	UART_FCR_REG |= 1;
	DebugDelay();				// Flush C54x if in debug mode.

	// Loopback control - disable.
	UART_MCR_REG &= ~UART_LOOP_ENABLE;
	DebugDelay();				// Flush C54x if in debug mode.

	// These arrarys are used to send data - clear to start.
	InitializeUARTArray();

	DebugDelay();				// Flush C54x if in debug mode.
	return;
}


//==========================================================================================
// Function:		InitializeUARTArray()
//
// Description:  	This function is used to initialize the UART, so that it comes up with
//					nothing in the buffer to send out.
//
// Revision History:
//==========================================================================================
void InitializeUARTArray(void)
{
	u16		i;

	// Zero out the entire buffer used to send data to the UART
	for(i=0; i<SIZE_UART_OUT_ARRAY; i++)
	{
		UARTDataOut[i].count = 0;

		// Next lines not needed.  Cleaner for debug & no harm...
		UARTDataOut[i].dataPtr = (u16*) 0;
		UARTDataOut[i].target = 0xdead;		// invalid value
	}
}


//==========================================================================================
// Function:		WriteUARTValue()
//
// Description:  	This function is used to send a single value to the UART.
//					The calling function doesn't have to worry about memory management.
//					
//					This function takes the passed value, assigns it to a variable, then
//					sends a pointer to that variable and a count of one, to the
//					WriteUART() function.
//
//					The "variable" used, is an element in a circular buffer.  The buffer
//					is the same length as the UARTDataOut array, so this shouldn't ever
//					have values overwritten (we should get an error from WriteUART() first).
//
// Input:			A single u16 value to be sent to the UART and it's target destination.
//
// Return:			SUCCESS if all went as expected, or the error code from WriteUART().
//
// Revision History:
//==========================================================================================
s16 WriteUARTValue(u16 target, u16 value)
{
	static u16		UARTDataValues[SIZE_UART_OUT_ARRAY];
	static s16		index = 0;
	s16				returnValue;


	// Get the passed value and put it in the local array.
	UARTDataValues[index] = value;
	
	// Now use WriteUART() to add a pointer to this value to the global array that
	// taskHandleUART uses.
	returnValue = WriteUART(target, 1, (u16*) &UARTDataValues[index]);		// target,count,pointer

	if (returnValue == SUCCESS)	// Value was successfully added to array
	{
		if (++index == SIZE_UART_OUT_ARRAY)	// increment index into local array, check
		{									// for end of buffer.
			index = 0;						// Back to start of circular buffer.
		}
	}

	if(returnValue != 0)
	{
	   // Do something...
	}

	return (returnValue);
}


//==========================================================================================
// Function:		WriteUART()
//
// Description: 	This function is used to interface between the code and the function
//					which sends data to the UART (SendUART()).
//
//					This routine accepts a count, and a pointer to the start of data.  It
//					then inserts an entry into the array UARTDataOut which TaskWriteUART
//					uses to send out data.
//					Note that it will store the count and pointer, not buffer the actual
//					data, so the calling function must keep the variable around (static or
//					global), or else use the function WriteUARTValue().
//
// Input:			u16		target;     Destination Host or Emeter.
//					s16		count;		Number of words to transfer.
//					u16*	dataPtr;	Pointer to start of data.
//
// Return:  		SUCCESS  		Added to list. (not necesarily sent yet)
//					ERR_LIST_FULL   No buffer space.
//
// Revision History:
//==========================================================================================
s16 WriteUART(u16 target, s16 count, u16* dataPtr)
{
	static s16		index = 0;		// Index into UARTDatatOut array for passed data.
	s16				maxIndex;		// Where we'll stop looking for an available space.
	s16				added = 0;		// Set when data has been added to the array.
	s16				returnValue;	// Function return value


	maxIndex = index - 1;			// We'll look through the entire array (once).
	if (maxIndex == -1)				// Check for case at 0.
	{
		maxIndex = SIZE_UART_OUT_ARRAY - 1;
	}

	// I know it's negative, but assume we'll fail, and change later when we succede.
	returnValue = ERR_LIST_FULL;

	// Keep looking for an available entry until we find one, or go through all of them.
	while ( (index != maxIndex) && (added == 0) )
	{
		if (UARTDataOut[index].count == 0)	// If count==0, this entry is free.
		{
			UARTDataOut[index].target = target;		// Add target.
			UARTDataOut[index].dataPtr = dataPtr;	// Add data pointer
			UARTDataOut[index++].count = count;		// Add count (in words).
			added = 1;								// Flag signalling done
			returnValue = SUCCESS;					// Be positive.
		}
		else	// That entry was being used, try the next one.
		{
			index++;							// increment index
		}
		if (index == SIZE_UART_OUT_ARRAY)		// If we go past the end ...
		{
			index = 0;							// ... start over at the beginning
		}
	}

	if(returnValue != 0)
	{
		// Do something...
	}

	return (returnValue);
}


//==========================================================================================
// Function:		WriteUARTString()
//
// Description: 	This function is used to interface between the code and the function
//					which sends data to the UART (SendUART()).
//
//					This routine accepts a count, and a pointer to the start of data.  It
//					then inserts an entry into the array UARTDataOut which TaskWriteUART
//					uses to send out data.
//					Note that it will store the count and pointer, not buffer the actual
//					data, so the calling function must keep the variable around (static or
//					global), or else use the function WriteUARTValue().
//
// Input:			s16		count;		Number of words to transfer.
//					u16*	dataPtr;	Pointer to start of data.
//
// Return:  		SUCCESS  		Added to list. (not necesarily sent yet)
//					ERR_LIST_FULL   No buffer space.
//
// Revision History:
//==========================================================================================
s16 WriteUARTString(u16* msgString)
{
	#define	UART_OUT_BUFF_SIZE	256
	static u16		UARTOutBuff[UART_OUT_BUFF_SIZE];
	static s16		indexS = 0;		// Index into UARTOutBuff
	s16				returnValue;	// Function return value
	s16				msgLen=-1;		// Length of message string
	u16				uLengthFound=0;	// Flag used to find string length.
	u16				i;				// Loop index

	// We don't want to sent a trailing NULL, so start the count at -1, so it doesn't show up in the count
	while( (uLengthFound==0) && (msgLen<20) )
	{
		if(msgString[++msgLen] == 0)	// Found string terminator
		{
			uLengthFound = 1;
		}
	}
			
	// Append the msgString to the end of UARTOutBuff if there is room, otherwise start back at the beginning
	if (indexS + msgLen + 1 >= UART_OUT_BUFF_SIZE)
	{
		indexS = 0;
	}

	// Copy received string into UARTOutBuff.
	for(i=0; i<msgLen; i++)
	{
		UARTOutBuff[indexS + i] = msgString[i];
	}

	returnValue = WriteUART(UART_TARGET_EMETER, msgLen, &UARTOutBuff[indexS]);		// target,count,pointer
	
	indexS += msgLen;			// Point to next available location for next pass.

	if(returnValue != 0)
	{
		// Do something...
	}

	return (returnValue);
}



//==========================================================================================
// Function:		uart_read()
//
// Description: 	This routine recieves data into the supplied buffer.
//
// Parameters:	  	buf - buffer to read data into.
//					offset - byte offset in buf to begin receiving chars.
//					cnt - number of data samples to capture into buffer.
//
// Return:			Last character read.
//
// Revision History:
//==========================================================================================
u16 uart_read(u16* pBuf, u16 offset, u16 cnt)
{
	u16		uReturnChar;

INTR_GLOBAL_DISABLE();
	while (cnt--)
	{
		while (uUartBufferPointerIn == uUartBufferPointerOut)
		{
			// Wait for RX data ready.
		}

		uReturnChar = upUartBufferFifo[uUartBufferPointerOut];
		if(++uUartBufferPointerOut >= UART_BUFFER_SIZE)
		{
			uUartBufferPointerOut = 0;
		}

		// even byte? (first half goes in high nibble)
		if(offset/2 == (offset+1)/2)
		{
			pBuf[offset/2] &= 0x00FF;	// Clear bits
			pBuf[offset/2] |= (u8) (uReturnChar & 0x00FF)<<8;
		}
		else	// odd.
		{
			pBuf[offset/2] &= 0xFF00;
			pBuf[offset/2] |= (u8) (uReturnChar & 0x00FF);
		}
		offset++;
	}
INTR_GLOBAL_ENABLE();

	return(uReturnChar);
}


//==========================================================================================
// Function:		SelectUARTTarget()
//
// Description: 	This function selects the target for UART communication.
//					Choices are UART_TARGET_HOST (1) or UART_TARGET_EMETER (0).
//
// Revision History:
//==========================================================================================
void SelectUARTTarget(u16 UARTTarget)
{
	uUARTTarget = UARTTarget;		// Set global variable
	AssignBits(GPIOSR,       1<<5, UARTTarget<<5);	// Send target receive select signal via DSP GPIO pin 	

	if(UARTTarget == UART_TARGET_HOST) 		// 1
	{
		AssignBits(UART_MCR_REG, 1<<3, 0);	// Send target transmit select signal via OUT2 pin on 16C550
	}
	else
	{
		AssignBits(UART_MCR_REG, 1<<3, 1<<3);	// Send target transmit select signal via OUT2 pin on 16C550
	}

	AssignBits(UART_MCR_REG, 1<<2, UARTTarget<<2);	// Send target transmit select signal via OUT2 pin on 16C550
	DebugDelay();									// Flush C54x if in debug mode.

	DelayNus(100);		// Improves errors related to RS232/RS485 switching.
}



//==========================================================================================
// Function:		SelectRS485Direction()
//
// Description: 	This function selects the data flow direction of the RS485 connection.
//					Choices are RS485_TALK (3) or RS485_LISTEN (0).
//
//					Note that the default direction is LISTEN.  Code that changes it to
//					TALK, MUST restore it.
//
// Revision History:
//==========================================================================================
void SelectRS485Direction(u16 RS485Dir)
{
	AssignBits(GPIOSR,  3<<3, RS485Dir<<3);		// Send direction control signal via GPIO pins 
	DebugDelay();								// Flush C54x if in debug mode.

	// Give drives time to get up.
	DelayNus(25);
}


//==========================================================================================
// Function:		UartISR()
//
// Description: 	This is the Interrupt Service Routine for INT1 which is tied to the
//					16C550.  It handles switching RS485 direction between talk and listen
//					after the transmit message has been sent, along with source switching
//					between the host and emeter if required.
//
// Revision History:
//==========================================================================================
interrupt void UartISR(void)
{
	// Check reason for interrupt.
	if(UART_IIR_REG & UART_IIR_TRANSMIT_EMPTY == UART_IIR_TRANSMIT_EMPTY)	// Data has gone out
	{
		// To get to this point, means we've just finished sending a string to the emeter.
		// Now reconfigure the 485 to listen, and if we're not expecting a return value from the
		// emeter, set the 16C550 to listen to the host, not the meter.   If we are expecting an
		// emter reply, the switch back to the host will be handled after receiving the emeter data.

		DelayNus(10);		// Time for last bit to get transfered.  Probably not needed, but 10 usec won't hurt anything.

		// We turn off PLC traffic while the last byte is transferred to make sure that the plc
		// traffic doesn't hold off this interrupt and prevent us from switching back to LISTEN.
		// The same flag (uHoldOffPLC) is also used to disable PLC traffic during an flash
		// update, so only Re-enable it here if we were stopped for the last byte.
		if(uLastByteFlag == 1)
		{
			uHoldOffPLC = 0;	// Allow PLC to continue.
			uLastByteFlag = 0;	// Reset for next time.
		}

		SelectRS485Direction(RS485_LISTEN);		// Set RS485 to listen to e-meter.

		if (uBoard == BOARD_MASTER)
		{
			if(uUartState == UART_RECEIVE_COMMAND)	// This gets set by ProcessMasterCommand() before command is issued
			{										// It covers the problem of the EMeter not sending anything back
													// but the host still needing a response.
				SelectUARTTarget(UART_TARGET_HOST);

				if(uAckAfter485 == 1)
				{
					// No real status to send back, this just lets the host know that we're ready
					// for the next command.
					WriteUARTValue(UART_TARGET_HOST, SUCCESS);
					uAckAfter485 = 0;
					ulAutoPollCounter = 0L;
					uAutoPollPause = 0;
				}
			}
			//else	// uUartState == UART_RECEIVE_RESPONSE.  We're expecting to get data back
			//{		//  from the emeter.  Nothing else to do other than switch to RS485_LISTEN.
			//}		//  which was done above.
		}
		else	// board == slave.
		{
			if(uAckAfter485 == 1)
			{
				// Note that this response isn't actually from the emeter.
				uAckAfter485 = 0;
				uTransmitPacketReady = 1;
			}
		}
	}
}



