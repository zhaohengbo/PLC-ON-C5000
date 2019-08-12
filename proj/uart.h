//==========================================================================================
// Filename:		uart.h
//
// Description:		TLC16C550C UART driver routines.
//
// Copyright (C) 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Functions: 
//		int uart_read() - Read a buffer of data from uart
//		int uart_write() - Write a buffer of data to uart
//
// Revision History:
//==========================================================================================

#ifndef _UART_H_
#define _UART_H_


//==========================================================================================
// Defines
//==========================================================================================

//C54XX interrupt the uart is mapped to
#define UART_INTR_FLAG		INT1
#define UART_INTR_TRAP		INT1_TRAP

//uart register addresses
// Prototype is wired to select with address lines A13 & A12.
// Real board is wired to use A14 & A13.  
// These addresses will work for both boards.
volatile ioport u16 port7000;
volatile ioport u16 port7001;
volatile ioport u16 port7002;
volatile ioport u16 port7003;
volatile ioport u16 port7004;
volatile ioport u16 port7005;
volatile ioport u16 port7006;
volatile ioport u16 port7007;

//uart registers
#define UART_RBR_REG port7000
#define UART_THR_REG port7000
#define UART_IER_REG port7001
#define UART_IIR_REG port7002
#define UART_FCR_REG port7002
#define UART_LCR_REG port7003
#define UART_MCR_REG port7004
#define UART_LSR_REG port7005
#define UART_MSR_REG port7006
#define UART_SCR_REG port7007

//for these registers ensure DLAB=1 (b7 of LCR) before accessing
#define UART_DLL_REG port7000
#define UART_DLM_REG port7001


//==========================================================================================
// Register bit definitions
//==========================================================================================
//UART_IIR_REG
#define UART_IIR_RECEIVED_DATA	(0x01)
#define UART_IIR_TRANSMIT_EMPTY	(0x02)
#define UART_IIR_LINE_STATUS	(0x04)
#define UART_IIR_MODEM_STATUS	(0x08)

//UART_FCR_REG
#define UART_FCR_FIFO_ENABLE	(0x01)
#define UART_FCR_RCV_RESET		(0x02)
#define UART_FCR_XMT_RESET		(0x04)
#define UART_FCR_DMA_MODE		(0x08)
#define UART_FCR_TRIGGER		(0xC0)

//UART_LCR_REG
#define UART_STOPBIT_ENABLE     (0x04)
#define UART_PARITY_ENABLE      (0x08)
#define UART_PARITY_SELECT      (0x10)
#define DLAB_MASK               (0x80)

//UART_MCR_REG
#define UART_LOOP_ENABLE        (0x10)
#define	UART_OUT1				(0x04)	
#define	UART_OUT2				(0x08)

//UART_LSR_REG
#define UART_LSR_DR				(0x1)		// Data Ready
#define UART_LSR_THRE			(1<<5)		// Transmitter Holding Register
#define UART_LSR_TEMT			(1<<6)		// Transmitter Empty Register


#define UART_TARGET_EMETER	0
#define	UART_TARGET_HOST	1

#define	RS485_TALK			3
#define	RS485_LISTEN		0


//==========================================================================================
// Public function prototypes
//==========================================================================================
u16 uart_read(u16* pBuf, u16 index, u16 cnt);

#endif
