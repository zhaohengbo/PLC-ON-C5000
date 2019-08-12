/*=================================================================================
Filename:			ofdm_fft.c

 Description:		function calls for ifft and fft to Matlab

 Copyright (C) 2002 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
========================================================================*/
#include "ofdm_modem.h"

// IGNORE_RX_DMA:  When defined, the circ_fft() function will ignore the hardware DMA 
//  pointer and will copy directly from memory.	 Useful for debugging.
// Comment out next line for normal operation.
//#define	IGNORE_RX_DMA

/*
#if FFT_LEN == 256
	#define	FFT_LEN_SHIFT		8
#elif FFT_LEN == 128
	#define	FFT_LEN_SHIFT		7
#else
	#error unexpected FFT_LEN
#endif
*/

//==========================================================================================
// Function:		fillCarriers()
//
// Description: 	This function fills in the carrier array.
//
// Revision History:
//==========================================================================================
void fillCarriers( iCplx  *fftBuf, const iCplx *carriers, i16 scale )

{
	i16				n;					// loop counter
	DATA			*pfftBuf;			// working pointer
	i16				scaleDn;			// shift right
	
	//-------------------------------------------------------------
	pfftBuf = (DATA *)(fftBuf);	// point to start of fft buffer

	for( n = 0; n < CARRIER_LOW; n++ )	
	{
		*pfftBuf++ = 0;		// zero out the non-carrier bins, real part
		*pfftBuf++ = 0;		// imag part
	}

	if( scale < 0 )
	{
		scaleDn  = -scale;
		for( n = 0; n < CARRIER_LEN; n++ )	
		{	
			*pfftBuf++ = carriers[n].re >> scaleDn;	// put the carriers in the ifft bins
			*pfftBuf++ = carriers[n].im >> scaleDn;
		}
	}
	else
	{
		for( n = 0; n < CARRIER_LEN; n++ )	
		{	
			*pfftBuf++ = carriers[n].re << scale;	// put the carriers in the ifft bins
			*pfftBuf++ = carriers[n].im << scale;
		}
	}
	
	for( n = CARRIER_HIGH+1; n < FFT_LEN-CARRIER_HIGH; n++ )
	{
		*pfftBuf++ = 0;					// zero out middle bins
		*pfftBuf++ = 0;
	}

	if( scale < 0 )
	{
		for( n = CARRIER_LEN-1; n >= 0; n-- )	
		{	
			*pfftBuf++ =  carriers[n].re >> scaleDn; 	// calc the conj of the carriers
			*pfftBuf++ = -carriers[n].im >> scaleDn; 	// and put in the ifft bins
		}
	}
	else
	{
		for( n = CARRIER_LEN-1; n >= 0; n-- )	
		{	
			*pfftBuf++ =  carriers[n].re << scale; 		// calc the conj of the carriers
			*pfftBuf++ = -carriers[n].im << scale; 		// and put in the ifft bins
		}
	}
	
	for( n = FFT_LEN-CARRIER_LOW+1; n < FFT_LEN; n++ )
	{
		*pfftBuf++ = 0;					// zero out the rest of the bins
		*pfftBuf++ = 0;
	}

	DebugDelay();						// Flush C54x if in debug mode.

	#if COMPILE_MODE == DSP_COMPILE
		pfftBuf = (DATA *)(fftBuf);	// point to start of fft buffer
		cbrev(pfftBuf, pfftBuf, FFT_LEN);
	#endif

	DebugDelay();						// Flush C54x if in debug mode.

}


//==========================================================================================
// Function:		WrapRecPointer()
//
// Description: 	This function handles wraparound at the beginning and end of the
//					receive buffer.
//
// Revision History:
//==========================================================================================
i16* WrapRecPtr(i16* recPtr, i16 offset) 
{
	i32	tempPtr;
	tempPtr = recPtr - recSignalArray;
	tempPtr += offset;
	if (tempPtr < 0)
	{
		tempPtr += RX_CIRC_BUFFER_LEN;
	}
	if (tempPtr > RX_CIRC_BUFFER_LEN)
	{
		tempPtr -= RX_CIRC_BUFFER_LEN;
	}

	return(recSignalArray + tempPtr);
}


#if 1
//==========================================================================================
// Function:		FillFFTBuff2()
//
// Description: 	This function copies a block of data from the receive DMA buffer to 
//					FFT buffer, properly handling wraparound at the end of the rx buffer.
//
// Revision History:
//==========================================================================================
i16* FillFFTBuff2(DATA* dest, i16* recStart, i16* recEnd)
{
	u16 		segmentLen;
	#if	(AFE_RX_FORMAT == 24)
		u16		i;				// Generic loop counter
	#endif

	if (recEnd > recStart)	// Copy in one contigous block
	{
		segmentLen = recEnd - recStart;
		#ifndef	IGNORE_RX_DMA
			WaitForRxBufferFree(recStart, segmentLen);
		#endif

		#if	(AFE_RX_FORMAT == 16)
			memcpy(dest, recStart, segmentLen*sizeof(DATA) );
			dest += segmentLen;
		#else
			for (i=0; i<segmentLen/RX_SRC_INC; i++)
			{
				*dest++ = *recStart++;		// Copy source to destination
				recStart++;					// Increment the source pointer one extra time
			}
		#endif

	}
	else				  	// Wrapped around end of buffer: handle in two pieces
	{
		// First piece: Copy from near end of rec buffer to beginning of fft buffer
		segmentLen = (recSignalArray+RX_CIRC_BUFFER_LEN) - recStart;
		#ifndef	IGNORE_RX_DMA
			WaitForRxBufferFree(recStart, segmentLen);
		#endif

		#if	(AFE_RX_FORMAT == 16)
			memcpy(dest, recStart, segmentLen*sizeof(DATA) );
			dest += segmentLen;
		#else
			for (i=0; i<segmentLen/RX_SRC_INC; i++)
			{
				*dest++ = *recStart++;		// Copy source to destination
				recStart++;					// Increment the source pointer one extra time
			}
		#endif


		// Second piece: Copy from beginning of rec buffer to middle of fft buffer
		recStart = recSignalArray;
		segmentLen = recEnd - recSignalArray;
		#ifndef	IGNORE_RX_DMA
			WaitForRxBufferFree(recStart, segmentLen);
		#endif

		#if	(AFE_RX_FORMAT == 16)
			memcpy(dest, recStart, segmentLen*sizeof(DATA) );
			dest += segmentLen;
		#else
			for (i=0; i<segmentLen/RX_SRC_INC; i++)
			{
				*dest++ = *recStart++;		// Copy source to destination
				recStart++;					// Increment the source pointer one extra time
			}
		#endif

	}

	return(recEnd);
}



#else

/*FILL_CNT*///==========================================================================================
/*FILL_CNT*/// Function:		FillFFTBuff2()
/*FILL_CNT*///
/*FILL_CNT*/// Description: 	This function copies a block of data from the receive DMA buffer to 
/*FILL_CNT*///					FFT buffer, properly handling wraparound at the end of the rx buffer.
/*FILL_CNT*///
/*FILL_CNT*/// Revision History:
/*FILL_CNT*/// 04/14/03	HEM		New function.
/*FILL_CNT*/// 05/14/03	HEM		New arguments: Use start and count instead of start and end.
/*FILL_CNT*///==========================================================================================
/*FILL_CNT*///i16* FillFFTBuff2(DATA* dest, i16* recStart, i16* recEnd)
/*FILL_CNT*/i16* FillFFTBuff2(DATA* pDest, i16* pSrc, i16 iCnt)
/*FILL_CNT*/{
/*FILL_CNT*/	i16*	pStart;
/*FILL_CNT*/	i16*	pEnd;	
/*FILL_CNT*/	u16		segmentLen;
/*FILL_CNT*/	u16		i;				// Generic loop counter
/*FILL_CNT*/
/*FILL_CNT*/
/*FILL_CNT*///	#define ST1	*(volatile unsigned int*)0x07	// Needed for XF()
/*FILL_CNT*///	SetXF();					// Turn on XF flag for debug
/*FILL_CNT*///	Delay1us();
/*FILL_CNT*///	ClearXF();					// Turn off XF flag for debug
/*FILL_CNT*///	Delay1us();
/*FILL_CNT*///	SetXF();					// Turn on XF flag for debug
/*FILL_CNT*/
/*FILL_CNT*/	// Calculate the start and end location
/*FILL_CNT*/	if (iCnt < 0)
/*FILL_CNT*/	{
/*FILL_CNT*/		pStart = WrapRecPtr(pSrc, iCnt);
/*FILL_CNT*/		pEnd = pSrc;
/*FILL_CNT*/	}
/*FILL_CNT*/	else
/*FILL_CNT*/	{
/*FILL_CNT*/		pStart = pSrc;
/*FILL_CNT*/		pEnd = WrapRecPtr(pSrc, iCnt);
/*FILL_CNT*/	}
/*FILL_CNT*/
/*FILL_CNT*/	// Copy the data from source to destination
/*FILL_CNT*/	if (pEnd > pStart)	// Copy in one contigous block
/*FILL_CNT*/	{
/*FILL_CNT*/		segmentLen = pEnd - pStart;
/*FILL_CNT*/		#ifndef	IGNORE_RX_DMA
/*FILL_CNT*/			WaitForRxBufferFree(pStart, segmentLen);
/*FILL_CNT*/		#endif
/*FILL_CNT*///DUAL		memcpy(dest, recStart, segmentLen*sizeof(DATA) );
/*FILL_CNT*/		for (i=0; i<segmentLen; i++)
/*FILL_CNT*/		{
/*FILL_CNT*/			*pDest++ = *pStart;		// Copy source to destination
/*FILL_CNT*/			pStart += SRC_INC;		// Increment the source pointer by the required amount
/*FILL_CNT*/		}
/*FILL_CNT*/
/*FILL_CNT*/		#if SAVETRACE == TRUE
/*FILL_CNT*///			SaveTraceData((u16)dest);
/*FILL_CNT*///			SaveTraceData((u16)recStart);
/*FILL_CNT*///			SaveTraceData((u16)segmentLen);
/*FILL_CNT*/		#endif
/*FILL_CNT*/	}
/*FILL_CNT*/	else				  	// Wrapped around end of buffer: handle in two pieces
/*FILL_CNT*/	{
/*FILL_CNT*/		// First piece: Copy from near end of rec buffer to beginning of fft buffer
/*FILL_CNT*/		segmentLen = (recSignalArray+RX_CIRC_BUFFER_LEN) - pStart;
/*FILL_CNT*/		#ifndef	IGNORE_RX_DMA
/*FILL_CNT*/			WaitForRxBufferFree(pStart, segmentLen);
/*FILL_CNT*/		#endif
/*FILL_CNT*///DUAL		memcpy(dest, recStart, segmentLen*sizeof(DATA) );
/*FILL_CNT*/		for (i=0; i<segmentLen; i++)
/*FILL_CNT*/		{
/*FILL_CNT*/			*pDest++ = *pStart;		// Copy source to destination
/*FILL_CNT*/			pStart += SRC_INC;		// Increment the source pointer by the required amount
/*FILL_CNT*/		}
/*FILL_CNT*///WRAP		#if SAVETRACE == TRUE
/*FILL_CNT*///WRAP			SaveTraceData((u16)dest);
/*FILL_CNT*///WRAP			SaveTraceData((u16)recStart);
/*FILL_CNT*///WRAP			SaveTraceData((u16)segmentLen);
/*FILL_CNT*///WRAP		#endif
/*FILL_CNT*/
/*FILL_CNT*/		// Second piece: Copy from beginning of rec buffer to middle of fft buffer
/*FILL_CNT*/		pDest += segmentLen;
/*FILL_CNT*/		segmentLen = pEnd - recSignalArray;
/*FILL_CNT*/		#ifndef	IGNORE_RX_DMA
/*FILL_CNT*/			WaitForRxBufferFree(recSignalArray, segmentLen);
/*FILL_CNT*/		#endif
/*FILL_CNT*///DUAL		memcpy(dest, recStart, segmentLen*sizeof(DATA) );
/*FILL_CNT*/		for (i=0; i<segmentLen; i++)
/*FILL_CNT*/		{
/*FILL_CNT*/			*pDest++ = *pStart;		// Copy source to destination
/*FILL_CNT*/			pStart += SRC_INC;		// Increment the source pointer by the required amount
/*FILL_CNT*/		}
/*FILL_CNT*///WRAP		#if SAVETRACE == TRUE
/*FILL_CNT*///WRAP			SaveTraceData((u16)dest);
/*FILL_CNT*///WRAP			SaveTraceData((u16)recSignalArray);
/*FILL_CNT*///WRAP			SaveTraceData((u16)segmentLen);
/*FILL_CNT*///WRAP		#endif
/*FILL_CNT*/	}
/*FILL_CNT*/
/*FILL_CNT*///	ClearXF();					// Turn off XF flag for debug
/*FILL_CNT*///	Delay1us();
/*FILL_CNT*///	SetXF();					// Turn on XF flag for debug
/*FILL_CNT*///	Delay1us();
/*FILL_CNT*///	ClearXF();					// Turn off XF flag for debug
/*FILL_CNT*/
/*FILL_CNT*/	return(pEnd);
/*FILL_CNT*/}

#endif


//==========================================================================================
// Function:		circFFT()
//
// Description: Fills the FFT buffer and calls the FFT library function.
//				Starts at the sample in the buffer pointed to by recSignal .
//				FFT data is calculated "in place" in the fftBuffer.
//				Returns a pointer to the next data in recSignal (which may have wrapped
//				 to the beginning of the buffer).
//
// Revision History:
//==========================================================================================
i16 *circFFT( iCplx *fftBuffer, i16 *recSignal )
{
	DATA		*fftBuf;
	DATA		*fftBuf2;

	fftBuf = (DATA *)fftBuffer;		// first half of the buffer
	fftBuf2 = fftBuf + FFT_LEN;		// second half

	// Copy from the receive buffer to the second half of the FFT buffer.
	// Handle wraparound at ends of rec buffer if necessary.
	recSignal = FillFFTBuff2(fftBuf2, recSignal, WrapRecPtr(recSignal,FFT_LEN*RX_SRC_INC));

	// ----- Copy from the second half of the FFT buffer to the first half,  -----
	//       with bit reversal if necessary. 
	#if COMPILE_MODE == DSP_COMPILE
		cbrev( fftBuf2, fftBuf, FFT_LEN/2 );	// copy data in bit 
	#else										// reversed format
		memcpy(fftBuf, fftBuf2, FFT_LEN*sizeof(DATA) );
	#endif

	//---- call the fft library funciton ------------------------

	#if DEBUGIT == DEBUG_CIRCFFT
	{
		int			n;
		DATA		*fBuf;
		if( diag.r != NULL )
		{
		 	fBuf = fftBuf;
			for(n = 0; n < FFT_LEN; n++ )
			{
				*diag.r++ = (double)(*fBuf++);
				*diag.i++ = (double)(n);
			}
		}
	}
	#endif


	rfft( fftBuf, FFT_LEN, FFT_SCALE );


	#if DEBUGIT == DEBUG_CIRCFFT
	{
		int			n;
		DATA		*fBuf;
		if( diag.r != NULL )
		{
			fBuf = fftBuf;
			for(n = 0; n < FFT_LEN; n++ )
			{
				*diag.r++ = (double)(*fBuf++);
				*diag.i++ = (double)(*fBuf++);
			}
		}
	}
	#endif



	return recSignal;
}


//==========================================================================================
// Function:		backFFT()
//
// Description: Fills the FFT buffer and calls the FFT library function.
//				Ends at the sample in the buffer pointed to by recSignal.
//				FFT data is calculated "in place" in the fftBuffer.
//				Returns a pointer to the next data in recSignal (which may have wrapped
//				 to the beginning of the buffer).
//
// Revision History:
//==========================================================================================
void backFFT( iCplx *fftBuffer, i16 *recSignal )
{
	DATA		*fftBuf;
	DATA		*fftBuf2;

	fftBuf = (DATA *)fftBuffer;		// first half of the buffer
	fftBuf2 = fftBuf + FFT_LEN;		// second half
	
	// Copy from the receive buffer to the second half of the FFT buffer.
	// Handle wraparound at ends of rec buffer if necessary.
	recSignal = FillFFTBuff2(fftBuf2, WrapRecPtr(recSignal,-FFT_LEN*RX_SRC_INC), recSignal);

	// ----- Copy from the second half of the FFT buffer to the first half,  -----
	//       with bit reversal if necessary.
	#if COMPILE_MODE == DSP_COMPILE
		cbrev( fftBuf2, fftBuf, FFT_LEN/2 );	// copy data in bit-reversed format
	#else									
		memcpy(fftBuf, fftBuf2, FFT_LEN*sizeof(DATA) );	//copy data
	#endif

	//---- call the fft library funciton ------------------------
	rfft( fftBuf, FFT_LEN, FFT_SCALE );

	return;
}


#if COMPILE_MODE == MEX_COMPILE
/*====================================================================
	do ifft and fft by calling Matlab

=====================================================================*/
void mexfft(  i16 *fftBuffer, i16 len, i16 scale, char cmdCh, char typeCh )
{

	int				n, rc;				// loop counter
	int				task;

	i32				acc;
	DATA			*fBuf;

	dCplxPtr		dFftBuf;			// buffer pointer
	mxArray			*fftInArray[1];		// fft/ifft input buffer
	mxArray			*fftOutArray[1];	// fft/ifft output buffer
										//    Note that this is allocated by
										//    mexCallMatlab and must be destroyed

	if( scale != 0 )
	{
		switch( len )
		{
		case 64:
			scale = 6;
			break;
		case 128:
			scale = 7;
			break;
		case 256:
			scale = 8;
			break;
		case 512:
			scale = 9;
			break;
		default:
			mexErrMsgTxt("unsupported FFT buffer length."); 
		}	
	}

	//---- allocate mxArray for call to Matlab iFFT/FFT routine --------
	//	   note that this initializes both arrays to all zeros
	if( typeCh == 'C' )
	{
		fftInArray[0]  = mxCreateDoubleMatrix(FFT_LEN, 1, mxCOMPLEX);
		dFftBuf.r = mxGetPr(fftInArray[0]);
		dFftBuf.i = mxGetPi(fftInArray[0]);
		dFftBuf.len = FFT_LEN;

		//---- load the Matlab input array -------------------------
		fBuf = fftBuffer;
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			*dFftBuf.r++ = (double)(*fBuf++);
			*dFftBuf.i++ = (double)(*fBuf++);
		}
	}
	else   // typeCh == 'R'
	{
		fftInArray[0]  = mxCreateDoubleMatrix(FFT_LEN, 1, mxREAL);
		dFftBuf.r = mxGetPr(fftInArray[0]);
		dFftBuf.i = NULL;
		dFftBuf.len = FFT_LEN;

		//---- load the Matlab input array -------------------------
		fBuf = fftBuffer;
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			*dFftBuf.r++ = (double)(*fBuf++);
		}
	}

	//---- do the fft/ifft by calling Matlab -------------------
	if( cmdCh == 'I' )
	{
		rc = mexCallMATLAB(1, fftOutArray, 1, fftInArray, "ifft");
		if( rc != 0 )
			mexErrMsgTxt("IFFT call failed."); 
	}
	else
	{
		rc = mexCallMATLAB(1, fftOutArray, 1, fftInArray, "fft");
		if( rc != 0 )
			mexErrMsgTxt("FFT call failed."); 
	}

	dFftBuf.r = mxGetPr(fftOutArray[0]);
	dFftBuf.i = mxGetPi(fftOutArray[0]);
	fBuf = fftBuffer;

	//---- copy the Matlab output array -------------------------
	task = (typeCh == 'C')*4 + (cmdCh == 'I')*2 + (dFftBuf.i == NULL);
	switch( task )
	{
	case 0:	//---- RFFT with complex output from matlab -----------------
		for( n = 0;  n<FFT_LEN/2;  n++ )
		{
			//acc = (i32)(*dFftBuf.r++);
			//*fBuf++ = (DATA)(acc >> scale);		// divide by len for FFT
			//acc = (i32)(*dFftBuf.i++);
			//*fBuf++ = (DATA)(acc >> scale);
			*fBuf++ = (DATA)(*dFftBuf.r++ / (1<<scale));		// do floating point divide
			*fBuf++ = (DATA)(*dFftBuf.i++ / (1<<scale));		// divide by len for FFT
		}
		break;
	case 1:	//---- RFFT with only real output from matlab -----------------
		for( n = 0;  n<FFT_LEN/2;  n++ )
		{
			acc = (i32)(*dFftBuf.r++);
			*fBuf++ = (DATA)(acc >> scale);		// divide by len for FFT
			*fBuf++ = 0;
		}
		break;
	case 2:	//---- RIFFT. only care about read output from matlab -----------------
	case 3:
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			acc = (i32)(*dFftBuf.r++);
			*fBuf++ = (DATA)(acc << scale);		// mult by len for IFFT
			*fBuf++ = 0;
		}
		break;
	case 4:	//---- CFFT with complex output from matlab -----------------
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			acc = (i32)(*dFftBuf.r++);
			*fBuf++ = (DATA)(acc >> scale);		// divide by len for FFT
			acc = (i32)(*dFftBuf.i++);
			*fBuf++ = (DATA)(acc >> scale);
		}
		break;
	case 5:	//---- CFFT with only real output from matlab -----------------
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			acc = (i32)(*dFftBuf.r++);
			*fBuf++ = (DATA)(acc >> scale);		// divide by len for FFT
			*fBuf++ = 0;
		}
		break;
	case 6:	//---- CIFFT with complex output from matlab -----------------
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			acc = (i32)(*dFftBuf.r++);
			*fBuf++ = (DATA)(acc << scale);		// mult by len for IFFT
			acc = (i32)(*dFftBuf.i++);
			*fBuf++ = (DATA)(acc >> scale);
		}
		break;
	case 7:	//---- CIFFT with only real output from matlab -----------------
		for( n = 0;  n<FFT_LEN;  n++ )
		{
			acc = (i32)(*dFftBuf.r++);
			*fBuf++ = (DATA)(acc << scale);		// mult by len for FFT
			*fBuf++ = 0;
		}
		break;
	default:
		mexErrMsgTxt("unsupported FFT output task."); 
	}	


	mxDestroyArray(fftInArray[0]);	// clean up mex array we created
	mxDestroyArray(fftOutArray[0]);	// clean up array mexCallMatlab created

	return;	
}
#endif










