/*========================================================================
 Filename:			ofdm_modem.c

 Description:		This is the main header file for the OFDM powerline modem.
					it calls several other header files.

 Copyright (C) 2002 - 2003 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
==========================================================================*/
#ifndef OFDM_MODEM_H			// Header file guard
#define OFDM_MODEM_H

#define FALSE			(0==1)
#define	TRUE			(1==1)


#define	LOC_OFFICE			1
#define	LOC_LAB				2
#define	MEX_COMPILE			3
#define	DSP_COMPILE			4

#ifdef  VCPP
	#define	COMPILE_MODE	MEX_COMPILE
	#define COMPILE_LOC		LOC_LAB
	#define SAVETRACE		FALSE
#endif

#ifndef COMPILE_MODE
	#define	COMPILE_MODE	DSP_COMPILE
#endif
#ifndef COMPILE_LOC
	#define	COMPILE_LOC		LOC_LAB
#endif

//---- Matlab library declarations --------------------------------
#if	COMPILE_LOC == LOC_LAB
//	#define MEX_HEADER	".\mex\mex.h"
	#define MEX_HEADER	"C:\NetMatlab12\extern\include\mex.h"
#endif
#if	COMPILE_LOC == LOC_OFFICE
	#define MEX_HEADER	"C:\MATLAB6p1\extern\include\mex.h"
#endif

#if	COMPILE_MODE != DSP_COMPILE
	#include MEX_HEADER
#endif



#define SUCCESS			(0)			// Return code for success.


// PROCESSOR	Which DSP is the code being built for.
#define PROCESSOR	5410


//----- Constants related to the Analog Front End (AFE) ------
#define	AFE1230		1230
#define	AFE240		240

#define AFE_TYPE	AFE1230

//#define DOUBLE_AFE_TX 	TRUE		// When TRUE, send Tx data at double the Rx rate

#define	UART_MCBSP	1		// MCBSP port used for UART to host PC or e-meter
#define	LED_MCBSP	2		// MCBSP port used to talk to status LEDs
#define	AFE_MCBSP	0		// MCBSP port used for AFE

#define	AFE_RX_DMA	4		// DMA channel used for receive from AFE
#define	AFE_TX_DMA	5		// DMA channel used for transmit to AFE


#if	COMPILE_MODE == DSP_COMPILE	
	// AFE1230 MCBSP/DMA Communication Format.  
	// Choices:  16 = Three 16-bit words
	//			 24 = Two 24-bit words
	#define	AFE_TX_FORMAT	16		
	#define	AFE_RX_FORMAT	24 
#else		//MEX always uses 16 for both Rx and Tx
	#define	AFE_TX_FORMAT	16		
	#define	AFE_RX_FORMAT	16 
#endif

#if	(AFE_RX_FORMAT == 16)
	#define	RX_SRC_INC	1		// Step through source array in one-word steps
#else
	#define	RX_SRC_INC	2		// Step through source array in two-word steps
#endif

#define	SEND_TWICE FALSE	// Send each transmitted value to AFE once
//#define	SEND_TWICE TRUE		// Send each transmitted value to AFE twice

#if (SEND_TWICE == TRUE) 
	#define	AFE_TX_WPF		1		// 1 unique input word per MCBSP frame
#else
	#define	AFE_TX_WPF		2		// 2 unique input words per MCBSP frame
#endif

#define  AFE_SIZE			3/AFE_TX_WPF	// DO NOT PUT PARENTHESES AROUND THIS TERM!!!

#define	IDLE_BUFFER_LEN		2	

//---- #choose Viterbi decoding method ------------------------------
#define	DECODING_HARD			1
#define	DECODING_SOFT			2
#ifndef DECODING_MODE
	#define	DECODING_MODE		DECODING_SOFT
#endif

//==========================================================================================
// serial.c constants and variables which need more global access.
//==========================================================================================
// Number of elements allowed in UARTDataOut array
#define SIZE_UART_OUT_ARRAY		(50)

// Error returned when trying to allocate past SIZE_UART_OUT_ARRAY
#define ERR_LIST_FULL			(1)

//---- transmit debug definitions -------
#define DEBUG_NONE					-1

#define DEBUG_CARRIER_PHASE			1		// after TX phaseAssign & after RX FFT det 
#define DEBUG_VITERBI				2
#define DEBUG_INTERL				3
#define DEBUG_SCRAM					4	
#define DEBUG_PUT_MAT				5
//---- agc debug definitions -------
#define DEBUG_AGC					11		// MEX version
#define DEBUG_SNR					12
#define	DEBUG_DDPH					13
//---- preamble debug definitions -------
#define DEBUG_AVEFFT				20		// aveFFT
#define DEBUG_PHASE				   	21		// AlignPhaseCompare
#define DEBUG_FRAME					22		// getFrameStartFromImpulse
#define DEBUG_GETFEQ				23		// getFreqEq
//---- receive debug definitions -------
#define DEBUG_DATAFFT				31		// dataFFT
#define DEBUG_SYMBOLS				32		// freqEqualization
#define DEBUG_PHCMP					33		// dataPhaseCompare
#define DEBUG_VITERBI_STATES		34		// viterbiDecodeSymbol
#define DEBUG_PARITY				35		// compareParityCheckBytes
#define DEBUG_DISTANCE				36		// distance metric
#define DEBUG_DISTANCE2				37		// more distandce metric
#define DEBUG_CIRCFFT				38
//---- FFT debug definitions -------
#define DEBUG_IFFT					41
#define DEBUG_FFT					42
#define DEBUG_COUNT					43

#if COMPILE_MODE == MEX_COMPILE
	#define DEBUGIT				DEBUG_DISTANCE2
	//#define DEBUGIT				DEBUG_NONE
#else
	#define DEBUGIT					DEBUG_NONE
#endif


#define	SAVESYMBOLS		FALSE
//#define	SAVESYMBOLS		TRUE

#if (COMPILE_MODE == DSP_COMPILE)
//	#define SAVETRACE		TRUE
	#define SAVETRACE		!(SAVESYMBOLS)
#endif

//---- standard library declarations --------------------------------
#if	COMPILE_MODE == DSP_COMPILE
	#include <stddef.h>				// contains NULL
	#include <stdio.h>
	#include "dsplib.h"				// contains fft  
#else			//  MEX_COMPILE
	#include <math.h>
#endif

#include <string.h>					// contains memcpy

#include "error.h"					// error code defines

#include "ofdm_defines_carr60.h" 	// Constant definitions

#include "ofdm_datatypes.h"			// Data types and macro definitions

#include "ofdm_functions.h"			// Declare function prototypes

#include "DspTesterConst.h"			// Constants which require duplication for tester.

//----Global vars----------------------------------------------------
#ifndef I_AM_D
	#include "ofdm_global.h"
#endif


// This is used to define the maximum ranged used by CheckForClosestSlave().
// It is not the max allowed by the system.
// The range tested will be ulMyAddr-MAX_SLAVE_ADDR_RANGE to ulMyAddr+MAX_SLAVE_ADDR_RANGE
#define MAX_SLAVE_ADDR_RANGE			(10)


//---- LED definitions ----------------------------------------------------
#define	LED_LOOP_ACTIVE		(1<<7)
#define	LED_TX_PACKET		(1<<6)
#define	LED_RX_BUFF_ERR		(1<<5)
#define	LED_RX_PARITY_ERR	(1<<4)
#define	LED_RX_PACKET_GOOD	(1<<3)
#define	LED_RX_FRAME_ALIGN	(1<<2)
#define	LED_RX_AGC_HOLD		(1<<1)
#define	LED_RX_SEARCH		(1<<0)

#endif 	// OFDM_MODEM_H       Header file guard
