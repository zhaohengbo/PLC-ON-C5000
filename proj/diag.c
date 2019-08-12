//==========================================================================================
// Filename:		diag.c
//
// Description:		Diagnostic functions ofdm power line modem.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
//	---- list of error codes as of 23Sep03 -------------------------------------------
//	code	 function			file			desc
//	0xBAD0	"main"	 			"dsp_modem.c"	"Good Packet Received!"
//	0xBAD1	"getFreqEq"	 		"preDet.c"	 	"Failed doing power scale factor"
//	0xBAD2	"frameAlign"	 	"preDet.c"	 	"Signal power too low to equalize"
//	0xBAD3	"aveDistance"	 	"dataDet.c"	 	"Overran distance metric"
//	0xBAD4	"aveFFT"	 		"preDet.c"	 	"Overflow of real part of average carrier in preamble"
//	0xBAD5	"aveFFT"	 		"preDet.c"	 	"Overflow of imag part of average carrier in preamble"
//	0xBAD6	"aveFFT"	 		"preDet.c"	 	"Failed to find Sync frame"
//	0xBAD7	"frameAlign"	 	"preDet.c"	 	"Insufficient number of frames averaged"
//	0xBAD8	"ReadRxDMAPointer"	"AFE.c"	 		"DMA Counter corrupted"
//	0xBAD9	"sample_interrupt"	"agc.c"	 		"RxDMAPointer outside Rx buffer "
//	0xBADA	"sample_interrupt"	"agc.c"	 		"RxDMAPointer miscompare "
//	0xBADB	"sample_interrupt"	"agc.c"	 		"Long delay between SNR Flags"
//	0xBADC	"main"	 			"dsp_modem.c"	"Timeout waiting for receive packet"
//	0xBADD	"sample_interrupt"	"agc.c"	 		"Odd value for recSignal"
//	0xBADE	"sample_interrupt"	"agc.c"	 		"Odd/Even misalignment in recSignal buffer"
//	0xBADF	"main"	 			"dsp_modem.c"   "Parity errors"
//	0xBAE0	"main"	 			"dsp_modem.c"	"Transmitting Packet"
//	0xBAE1	"AdjustTxPeaks"		"transmit.c"	"Clipped transmit waveform"
//	0xBAE2	"getFreqEq"	 		"preDet.c"	 	"Overflowed carrier power"
//	0xBAE3 	"viterbiDecodeFrame""viterbi.c"		"Viterbi PathMetric Overflow"
//
//	0xBAE5 	"CheckPLC" 			"dsp_modem.c" 	"Re-enabling Global interrupts"

//SaveTrace data format:
//	ReceivePacket			0x1151		ulLastRxCounter	ulLastTxCounter
//	TINT0_ISR				0x1180+agcState	recSignal	uRxDMA
//	TestInterruptFlag		0x1333		ST1Shadow		ST1Shadow2
//	WaitForRxBufferFree		0x2000+uCnt	uStart			ReadRxDMAPointer()
//	WaitForLastFrame		0x2112		uStart			uFirstSrc
//	FillIdleBuffer			0x3333		recSignal		(AFEctrl2>>3) & 7
//	makePreamble			0x5000		recSignal		pTxSignal
//	makeDataFrames			0x5001,2	recSignal		pTxSignal
//	CalcTxStartAddr			0x55nn		uStart			ReadRxDMAPointer()-txSignalArray
//								nn=uCnt-FFT_LEN;
//	lookForPacket			0x8gac 		recSignal		phError	 
//							  p=pgaGain a=agcState 		c=preambleDetCount
//	ddphase					0x9999		cc0Max			cc1Max
//							  0x0000  	xMax		   	yMax

//	ReceivePacket			0xACCA		recSignal		uRxDMA
//	ReceivePacket			0xACCB		recSignal		uRxDMA
//	ReceivePacket			0xACCC		recSignal		errCount

//	ReceivePacket			0xBAD0		recSignal		errCount
//	getFreqEq (BIN)			0xBAD1		carrierNum		carrierPwr	
//										b=block			f=frame
//	frameAlign				0xBAD2		recSignal		worstPwr
//	frameAlign				0xBAD7		recSignal		numFramesAve
//	ReadRxDMAPointer		0xBAD8		pRxDest			uRxCtr
//	sample_interupt			0xBADB		recSignal		dmaPtr		
//							  prevSnrSample	sample		SNRflag
//	CheckPLC				0xBADC		recSignal		uRxDMA
//	sample_interupt			0xBADD		recSignal		dmaPtr
//	sample_interupt			0xBADE		recSignal		dmaPtr
//	ReceivePacket			0xBADF		recSignal		uNonZeroBits
//	TransmitPacket			0xBAE0		recSignal		SNRflagBackup
//	CheckPLC				0xBAE5		recSignal		uRxDMA
//	frameAlign				0xCnff		recSignal		worstPwr
//										n=numFramesAve	ff=frameStartFirst 
//	viterbiDecodeFrame		0xD888		pathMetric0		maxPathMetric
//	viterbiDecodeFrame		0xD99A		state			pathMetricA[state]
//	viterbiDecodeFrame		0xD99B		state			pathMetricB[state]
//	dataFFT					0xDADA		recPtr			rxDMA
//	getFrameStartFromImpulse0xDDDD		maxSample		maxValue
//	AdjustTxPeaks			0xDFbf		maxReal			txGain
//	compareParityCheckBytes	0xEEEE		parity			reg
//
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"
#if COMPILE_MODE == DSP_COMPILE
	#include "intr.h"
#endif


//==========================================================================================
// Set global vars
//==========================================================================================
volatile void*	voidptr;	// Used to suppress compiler remarks only. No real function.



 //==========================================================================================
// Function:		SaveTraceData()
//
// Description: 	Save a value into the trace data array, if there is space available.
//
// Revision History:
//==========================================================================================
#if (SAVETRACE == TRUE)
void SaveTraceData(u16 uDat)
{
	if (uTraceIndex < TRACE_BUFF_SIZE)
	{
		uTraceData[uTraceIndex++] = uDat;
	}
	else
	{
		DebugDelay();			// Flush C54x if in debug mode.
		uTraceIndex = 0;		// Restart the trace 
		memset((u16*)uTraceData, 0xABCD, TRACE_BUFF_SIZE*sizeof(uTraceData[0]));	// Fill with known characters
		uTraceData[uTraceIndex++] = uDat;
		DebugDelay();			// Flush C54x if in debug mode.
	}
}
#endif
#if( SAVESYMBOLS == TRUE )
void SaveTraceData(u16 uDat)
{
	if( uTraceEnable == TRUE )
	{
		if (uTraceIndex >= TRACE_BUFF_SIZE)
		{
			uTraceIndex = 0;		// Restart the trace 
		}
		uTraceData[uTraceIndex++] = uDat;
	}
}
#endif


//==========================================================================================
// Function:		postError()
//
// Description: 	Error Handler:  Stop and report an error
//
// Revision History:
//==========================================================================================
void postError( char *errText ) 
{
#if COMPILE_MODE == MEX_COMPILE
	mexErrMsgTxt(errText);
#else
	volatile i16	hang;
	
	ClearXF();				// Turn off XF flag for debug

	//!!!TEMPORARY FOR BRING-UP ONLY:  Halt the MCBSP so we don't lose the data in the receive buffer.
	// Shut off frame sync generation (FS)
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
	DebugDelay();			// Flush C54x if in debug mode.
	DebugDelay();			// Flush C54x if in debug mode.

	#ifdef	SHOW_FLOW
		printf("ERROR: %s\n", errText);
	#else
		voidptr = (void*) errText;	// Used to suppress compiler remarks only. No real function.
	#endif

	hang = 0;		// 0= return immediately, 1= hang until user intervention
	while( hang )
	{
		DebugDelay();				// Flush C54x if in debug mode.
	}
#endif
}

//==========================================================================================
// Function:		PostErrorCode()
//
// Description: 	Error Handler:  Stop and report an error
//
// Revision History:
//==========================================================================================
void PostErrorCode(u16	uErrCode, char *errFunc, char* errFile, char* errText ) 
{
	volatile i16	hang;

	uErrCnt[uErrCode-0xBAD0]++;
		
	#if COMPILE_MODE == MEX_COMPILE
		mexPrintf("ERROR #%X in %s (%s): %s.\n", uErrCode, errFunc, errFile, errText);
		hang = 0;		// 0= return immediately, 1= hang until user intervention
		while( hang )
		{
			NOP();				// Hang here.
		}
		//mexErrMsgTxt(errText);

	#else			// DSP_COMPILE
		#ifdef	SHOW_FLOW
			printf("ERROR #%X in %s (%s): %s.\n", uErrCode, errFunc, errFile, errText);
		#else
			voidptr = (void*)errText;	// Used to suppress compiler remarks only. No real function.
			voidptr = (void*)errFunc;	// Used to suppress compiler remarks only. No real function.
			voidptr = (void*)errFile;	// Used to suppress compiler remarks only. No real function.
		#endif

		hang = 0;		// 0= return immediately, 1= hang until user intervention
		if (hang)
		{	
			ClearXF();				// Turn off XF flag for debug
		
			//!!!TEMPORARY FOR BRING-UP ONLY:  Halt the MCBSP so we don't lose the data in the receive buffer.
			// Shut off frame sync generation (FS)
			MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 0);
			DebugDelay();				// Flush C54x if in debug mode.

			while( hang )
			{
				NOP();
				NOP();				// Hang here.
			}
		}
		DebugDelay();				// Flush C54x if in debug mode.
	#endif
}



