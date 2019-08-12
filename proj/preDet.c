//==========================================================================================
// Filename:		predet.c
//
// Description:		Functions related to preamble detection for power line modem.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"

#define		FEQ_NORM_BIN		1					// use binary intervals to normalize in FEQ
#define		FEQ_NORM_DIV		2	 				// divide by power to normalize in FEQ
#define		FEQ_NORM_LOOKUP		3	 				// use lookup table for normalization in FEQ
#define		FEQ_NORM_NONE		4	 				// don't normalize in FEQ
#define		FEQ_NORM_TYPE		FEQ_NORM_DIV

//==========================================================================================
// Function:		frameAlign()
//
// Description: 	Main frame align and equalization function.
//
// Revision History:
//==========================================================================================
i16 *frameAlign(iCplx *freqEq, i16 *recSignal)
{
	i16				numFramesAve;		// count of frames averaged
	i16				frameStart;
	i16				frameStartFirst;
	i16				worstPwr;

	frameAligned = 1;			// Assume frameAlignment will turn out good
	//---- get average freq response until sync frame is received ---------
	numFramesAve = ddAveFFT( symbolArray, recSignal );
	//diagData(symbolArray, CARRIER_LEN, "avecar", diagICPLX);

	// If we didn't see enough frames with reasonable-looking phase error, 
	//  we're probably looking in the middle of a data packet, not a preamble,
	//  so treat this as a failed frame alignment.
	if (numFramesAve < 3)		//??? Make this into a defined constant
	{
		PostErrorCode(0xBAD7, "frameAlign", "preDet.c", "Insufficient number of frames averaged");
		#if SAVETRACE == TRUE
			SaveTraceData(0xBAD7);				//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)recSignal);		//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(numFramesAve);		//!!!DEBUG  Put a marker in the trace buffer
		#endif
		frameAligned = 0;			// Frame alignment failed
		preambleDetCount = 0;		// Restart the frame detection process
		agcState = AgcIdle;			// Reset the AGC mode
	}
	else
	{
	
		//---- Calculate the impulse response from the averaged frequency values.  
		frameStart = getFrameStartFromImpulse( symbolArray );	// The maximum point in the 
																// impulse response corresponds
																// to the beginning of the frame.
		//#if COMPILE_MODE == MEX_COMPILE
			//frameStart += 1;      // DEBUG!!!!
			//mexPrintf("frameStart = %d\n",frameStart);
		//#endif
		frameStartFirst = frameStart;

		//---- re-do FFT averages at correct starting point and find sync frame -------
		recSignal = WrapRecPtr(recSignal, +frameStart*RX_SRC_INC); // Point to true start of a frame

		numFramesAve = ddAveFFT( symbolArray, recSignal );
		//diagData(symbolArray, CARRIER_LEN, "avecar2", diagICPLX);
	
		frameStart = numFramesAve*FFT_LEN
				   + (int)SYNC_FIELD_LEN 
				   + CYCLIC_PREFIX_LEN;
		recSignal = WrapRecPtr(recSignal, +frameStart*RX_SRC_INC); // set first data frame starting point

		worstPwr = getFreqEq( freqEq, symbolArray, numFramesAve );
		//diagData(freqEq, CARRIER_LEN, "freqEq", diagICPLX);
	
		#if SAVETRACE == TRUE
			SaveTraceData(0xC000 + (numFramesAve<<8) + (frameStartFirst&0x00FF));	//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)recSignal);		//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(worstPwr);			//!!!DEBUG  Put a marker in the trace buffer
		#endif

		if( worstPwr < WORST_PWR_LIMIT )
		{
			#if COMPILE_MODE == MEX_COMPILE
				char	prtStr[80];
				diagData(symbolArray, CARRIER_LEN, "feqSymbols", diagICPLX);
				diagData(recSignalArray, RX_CIRC_BUFFER_LEN, "rxArray", diagI16);
				sprintf(prtStr, "signal power too low to equalize : %d\n", worstPwr );
				postError(prtStr);
			#else
				PostErrorCode(0xBAD2, "frameAlign", "preDet.c", "Signal power too low to equalize");
				#if SAVETRACE == TRUE
					SaveTraceData(0xBAD2);				//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData((u16)recSignal);		//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData(worstPwr);			//!!!DEBUG  Put a marker in the trace buffer
				#endif
			#endif
	
			frameAligned = 0;			// Frame alignment failed
			preambleDetCount = 0;		// Restart the frame detection process
			agcState = AgcIdle;			// Reset the AGC mode
		}
	}

	return recSignal;
}

	
//==========================================================================================
// Function:		aveFFT()
//
// Description: 	Get average FFT of the preamble frames.
//
// Revision History:
//==========================================================================================
i16 aveFFT( iCplx *aveCarrier, i16 *recSignal )
{
	u16				n, frame;			// loop counters
	u16				phaseError	= 0;	// sum(abs(phase)) for first n subcarriers,
										// used to determine end of preamble
	iCplx			*fftBuf;			// pointer to fft array
	iCplx			*aveCar;			// working pointer to ave data

  
	//---- initialize array used to average frequency response ------
	memset(aveCarrier, 0, CARRIER_LEN*sizeof(iCplx));	// set average to zero


	//---- get average freq response for N frames -----------------------
	for( frame = 0; frame < NUM_PRE_FRAMES+1; frame++ )  
	{	
		recSignal = circFFT( fftArray, recSignal );

		//--------------------------------------------------------------------
		//	Compare the phase of this frame with the phase of the previous frame.
		//	If the phase difference is greater that criteria, we must have hit
		//	the sync frame.  So don't incude this frame in the average and quit.
		//
		//	Note that we want to use the lowest freq frames so that any phase 
		//	shift due to sample rate error is as low as possible.
		//---------------------------------------------------------------------
		fftBuf = fftArray + CARRIER_LOW; // point to the carrier bins
		aveCar = aveCarrier;

		if( frame > 0 )
			phaseError = AlignPhaseCompare(PHASE_MEAS_LEN, aveCar, fftBuf );

		#if SAVETRACE == TRUE
			SaveTraceData((u16)(0xAA00 + (frame<<4) + phaseError));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
			SaveTraceData((u16)recSignal);							//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)ReadRxDMAPointer());					//!!!DEBUG  Put a marker in the trace 
		#endif
		
		if( phaseError >= ALIGN_DONE_CRITERIA )
			break;
		
		//---- otherwise continue processing fft data -----------------
		for( n = 0; n < CARRIER_LEN; n++ )
		{
			aveCar->re += fftBuf->re;
			if( abs(aveCar->re) > 0x7000)
			{
				PostErrorCode(0xBAD4, "aveFFT", "preDet.c", "Overflow of real part of average carrier in preamble");
			}

			aveCar->im += fftBuf->im;
			if( abs(aveCar->im) > 0x7000)
			{
				PostErrorCode(0xBAD5, "aveFFT", "preDet.c", "Overflow of imag part of average carrier in preamble");
			}

			fftBuf++;
			aveCar++;
		}

//AVEFFT		#if SAVETRACE == TRUE
//AVEFFT			SaveTraceData((u16)(0x9800 + (frame<<4) + phaseError));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
//AVEFFT			SaveTraceData((u16)symbolArray[5].re);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT			SaveTraceData((u16)symbolArray[5].im);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT			SaveTraceData((u16)(0x9900 + (frame<<4) + phaseError));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
//AVEFFT			SaveTraceData((u16)fftArray[5+CARRIER_LOW].re);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT			SaveTraceData((u16)fftArray[5+CARRIER_LOW].im);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT		#endif

	}	// end of fft block
	
	if( frame == NUM_PRE_FRAMES+1 )
	{
		PostErrorCode(0xBAD6, "aveFFT", "preDet.c", "Failed to find Sync frame");
	}

	return frame;
}


//==========================================================================================
// Function:		ddAveFFT()
//
// Description: 	Get average FFT of the preamble frames.
//
// Revision History:
//==========================================================================================
i16 ddAveFFT( iCplx *aveCarrier, i16 *recSignal )
{
	u16				n, frame;			// loop counters
	u16				phaseError	= 0;	// sum(abs(phase)) for first n subcarriers,
										// used to determine end of preamble
	iCplx			*fftBuf;			// pointer to fft array
	iCplx			*aveCar;			// working pointer to ave data

  
	//---- initialize array used to average frequency response ------
	memset(aveCarrier, 0, CARRIER_LEN*sizeof(iCplx));	// set average to zero

	memset(fftArray, 0, FFT_LEN*sizeof(iCplx));	// DEBUG!!! 

	//---- get average freq response for N frames -----------------------
	for( frame = 0; frame < NUM_PRE_FRAMES+1; frame++ )  
	{	
		recSignal = circFFT( fftArray, recSignal );

		//--------------------------------------------------------------------
		//	Compare the phase of this frame with the phase of the previous frame.
		//	If the phase difference is greater that criteria, we must have hit
		//	the sync frame.  So don't incude this frame in the average and quit.
		//
		//	Note that we want to use the lowest freq frames so that any phase 
		//	shift due to sample rate error is as low as possible.
		//---------------------------------------------------------------------
		fftBuf = fftArray + CARRIER_LOW;
		phaseError = ddphase(PreambleArray, fftBuf, CARRIER_LEN );
		#if COMPILE_MODE == MEX_COMPILE  	
		{
			//char	vnameStr[20];
			//sprintf(vnameStr,"ddAve%d",frame);
			//diagData(fftArray, FFT_LEN, vnameStr, diagICPLX);
			mexPrintf("%d: phaseError %d\n", frame, phaseError );
		}  	
		#endif

		//if( frame > 0 )
		//	phaseError = AlignPhaseCompare(PHASE_MEAS_LEN, aveCar, fftBuf );

		#if SAVETRACE == TRUE
			SaveTraceData((u16)(0xAA00 +  phaseError));			//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
			SaveTraceData((u16)recSignal);						//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)ReadRxDMAPointer());				//!!!DEBUG  Put a marker in the trace 
		#endif
		
		if( phaseError >= ALIGN_DONE_CRITERIA )
			break;
		
		//---- otherwise continue processing fft data -----------------
		fftBuf = fftArray + CARRIER_LOW;
		aveCar = aveCarrier;
		for( n = 0; n < CARRIER_LEN; n++ )
		{
			aveCar->re += fftBuf->re;
			if( abs(aveCar->re) > 0x7000)
			{
				PostErrorCode(0xBAD4, "aveFFT", "preDet.c", "Overflow of real part of average carrier in preamble");
			}

			aveCar->im += fftBuf->im;
			if( abs(aveCar->im) > 0x7000)
			{
				PostErrorCode(0xBAD5, "aveFFT", "preDet.c", "Overflow of imag part of average carrier in preamble");
			}

			fftBuf++;
			aveCar++;
		}

//AVEFFT		#if SAVETRACE == TRUE
//AVEFFT			SaveTraceData((u16)(0x9800 + (frame<<4) + phaseError));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
//AVEFFT			SaveTraceData((u16)symbolArray[5].re);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT			SaveTraceData((u16)symbolArray[5].im);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT			SaveTraceData((u16)(0x9900 + (frame<<4) + phaseError));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
//AVEFFT			SaveTraceData((u16)fftArray[5+CARRIER_LOW].re);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT			SaveTraceData((u16)fftArray[5+CARRIER_LOW].im);			//!!!DEBUG  Put a marker in the trace buffer
//AVEFFT		#endif

	}	// end of fft block
	
	if( frame == NUM_PRE_FRAMES+1 )
	{
		PostErrorCode(0xBAD6, "aveFFT", "preDet.c", "Failed to find Sync frame");
	}

	return frame;
}


//==========================================================================================
// Function:		AlignPhaseCompare()
//
// Description: 	Compare phase of carriers from the previous frame with the phase 
//					of carriers from the current frame.  Decode to one of four phases 
//					(just like data decode) and sum the abs of the phase differences.
//
//					When the sum of phase differences is sufficiently large we can say that
//					we've found the sync frame since it is coded 180 degrees different from
//					the preamble frames.
//
// Revision History:
//==========================================================================================
i16 AlignPhaseCompare(i16 carLen, iCplx *prev, iCplx *curr )
{
	int 		n;
	i16 		prevdif, prevsum;
	i16			bit1, bit0;
	i16 		dPhase;
	i16			phaseError = 0;
	i32			acc;

	static i16	phasemap[] = {2, -1, 1, 0 };


	#if DEBUGIT == DEBUG_PHASE
		char	prtStr[255];
		char	numStr[50];
		strcpy(prtStr,"phase diff:");
	#endif

	for( n = 0 ; n < carLen ; n++ )
 	{
		#if DEBUGIT == DEBUG_PHASE
			sprintf(numStr, "%5d%5d%5d%5d ", prev->re, prev->im, curr->re, curr->im );
			strcat(prtStr, numStr);
		#endif

		prevdif = prev->re - prev->im;
		prevsum = prev->re + prev->im;

		acc  =  (i32)(prevdif) * (i32)(curr->re);
		acc +=  (i32)(prevsum) * (i32)(curr->im);
		bit1 = acc > 0;

		acc  =  (i32)(prevsum) * (i32)(curr->re);
		acc -=  (i32)(prevdif) * (i32)(curr->im);
		bit0 = acc > 0;

		dPhase = phasemap[ (bit1<<1) + bit0 ];	// decoded phase

		phaseError += abs(dPhase);	// sum of differences

		prev++;						// point to next carriers
		curr++;

		#if DEBUGIT == DEBUG_PHASE
			sprintf(numStr, "%3d   ", dPhase);
			strcat(prtStr, numStr);
		#endif
	}

	#if DEBUGIT == DEBUG_PHASE
		strcat(prtStr, "\n");
		mexPrintf("%s",prtStr);
	#endif

	return phaseError;
}


//==========================================================================================
// Function:		getFrameStartFromImpulse()
//
// Description: 	Calculate the impulse response from the averaged frequency values.  
//					The maximum point in the impulse response corresponds to the beginning
//					of the frame.
//
//					impulse	= received/transmitted = Y/X = [y0 y1...yN] ./ [x0 x1...xN]
//				   		    = [y0*conj(x0)...yN*conj(xN)] ./ [x0*conj(x0)...xN*conj(xN)]  
//				           	= C*[y0*conj(x0)...yN*conj(xN)]
//	
//					Uses symbolArray (data passed in) and phaseEqArray as the impulse array 
//
// Revision History:
//==========================================================================================
i16 getFrameStartFromImpulse( iCplx *aveCarrier)
{
	i16				n;					// loop counter, return code
	i16				sample;				// sample number in frame
	u16				impulseSample;		// these three variables are used to 
	u16				maxValue;			// find the max in the impulse response
	i16				maxSample;			// true frame start that is returned

	iCplx			*fftBuf;			// pointer to fft array
	iCplx			*aveCar;			// pointer current and average complex data
	iCplx			*preamble;			// pointer to preamble pattern
	iCplx			*impulse;			// working pointer to complex data


	//---- init pointers ---------------------
	aveCar = aveCarrier;
	fftBuf = fftArray;
	preamble = (iCplx *)distanceArray;	// get complex phase from txPhase data 
	impulse = phaseEqArray;


	getPrePhases( preamble, PreambleArray );

	//---- calc the impulse response and put it in the ifft bins -------
	for( n = 0; n < CARRIER_LEN; n++ )
	{				
		impulse->re		= (aveCar->re * preamble->re)
						+ (aveCar->im * preamble->im);

		(impulse++)->im	= (aveCar->im * preamble->re)
					  	- (aveCar->re * preamble->im);
		aveCar++;
		preamble++;
	}

	//---- Fill the carriers for an IFFT, then invert.  Result is a time signal that
	//---- looks like a correlation, with a big spike where the frame offset is best.	
	// Note: When doing IFFT operations, there is scaling on both the fillCarrier() function
	// and the cifft() function.  Here we will scale up by +2 in fillCarrier(), then down
	// by -8 in cifft().  This gives us the least  quantization on the input terms without
	// rolling over the cifft output.
	// ( Shift of +2 works well.  +3 seems to work but values are close to rolling over.)
	// Contrast this with the transmit function.  There the phase terms only have four
	// unique, known values, so there we shift down before doing cifft().

	fillCarriers( fftArray, phaseEqArray, +2 );		// Fill the carriers in freq domain
	DebugDelay();									// Flush C54x if in debug mode.
	//diagData(fftArray, FFT_LEN, "impulseF", diagICPLX);

	#if COMPILE_MODE == MEX_COMPILE
		cifft( (DATA *)(fftArray), FFT_LEN, 0 );		// Invert to get a time-domain "correlation"
	#else
		cifft( (DATA *)(fftArray), FFT_LEN, 1 );		// Invert to get a time-domain "correlation"
	#endif
	DebugDelay();									// Flush C54x if in debug mode.
	
	#if DEBUGIT == DEBUG_FRAME
		diagData(aveCarrier, CARRIER_LEN, "avecar", diagICPLX);
		diagData((iCplx *)distanceArray, CARRIER_LEN, "impulseP", diagICPLX);	// preamble
		diagData(phaseEqArray, CARRIER_LEN, "impulseF", diagICPLX);
		diagData(fftBuf, FFT_LEN, "impulseT", diagICPLX);
	#endif
	
	//---- Find biggest sample.  Its index indicates offset from beginning of frame --------
	fftBuf = fftArray;		// point to the real samples
	maxValue  = 0;			
	for( sample = 0 ; sample < FFT_LEN ; sample++ )
	{
		impulseSample = abs(fftBuf->re);
		fftBuf++;

		if( impulseSample > maxValue )	// find the max sample
		{
			maxSample = sample;
			maxValue  = impulseSample;
		}
	}

	#if SAVETRACE == TRUE
		SaveTraceData(0xDDDD);					//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)maxSample);			//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)maxValue);			//!!!DEBUG  Put a marker in the trace buffer
	#endif

 	return maxSample;
}
	 

#if FEQ_NORM_TYPE == FEQ_NORM_DIV		// Use division to calculate freq equalization
//==========================================================================================
// Function:		getFreqEq()
//
// Description: 	Frequency equalization calculation 
//					EQ = tx/rx = preamble / receive = Preamble / (aveCarrier/N) = N*P/A
//
//					complex math:
//						EQ = N/(ar*ar + ai*ai) * ((ar*pr + ai*pi) + j*(ar*pi - ai*pr))
//							where N is the number of FFT averages
//
// Revision History:
//==========================================================================================
#define 	FEQ_SHIFT_DIV		9
//#define 	FEQ_SHIFT_DIV		7
i16 getFreqEq( iCplx *freqEq, iCplx *aveCarrier, i16 numFramesAve )
{
	u16				n;					// loop counter, return code
	iCplx			*preamble;

	i32				carrierPwr;		
	static i32				feqNum;

	i32				minPwr = 0x7FFFFFFF;	// init to a big number

	
	//---- allocate a diagnostic array to pass to Matlab ----------
	#if DEBUGIT == DEBUG_GETFEQ
		diagArray = mxCreateDoubleMatrix(5, CARRIER_LEN, mxCOMPLEX);
		mxSetName(diagArray, "feq_diag");
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
	#endif

	//-----------------------------------------------------------------
	//	Calc the complex freq equalization value that is 
	//	multiplied with each carrier in the data frames.
	//  Use distance array as a container for cmplex preamble symbols.
	//-----------------------------------------------------------------
	preamble = (iCplx *)distanceArray;	// get complex phase from txPhase data 
	getPrePhases( preamble, PreambleArray );
		
	for( n = 0; n < CARRIER_LEN; n++ )
	{								
		//------------------------------------------------
		//	(1.15 * 1.15) >> 12 = 2.30 >> 12 = 14.18 ==> -2.18
		//		so carrierPwr is 15-PWR_SHIFT bigger than a 1.15 number
		//-------------------------------------------------
		carrierPwr = iSquare(aveCarrier->re, PWR_SHIFT) 
				   + iSquare(aveCarrier->im, PWR_SHIFT)
				   + 1;										// make it at least 1
		if( carrierPwr < 0 )
		{
			PostErrorCode(0xBAE2, "getFreqEq", "preDet.c", "Overflowed carrier power");
		}
		
		if( carrierPwr < minPwr )
			minPwr = carrierPwr;

		if( carrierPwr < WORST_PWR_LIMIT )
		{
			DebugDelay();
			DebugDelay();
		}

				
		#if DEBUGIT == DEBUG_GETFEQ
			*diag.r++ = (double)preamble->re;
			*diag.i++ = (double)preamble->im;

			*diag.r++ = (double)aveCarrier->re;
			*diag.i++ = (double)aveCarrier->im;

			*diag.r++ = (double)carrierPwr;
			*diag.i++ = (double)numFramesAve;
		#endif

		//---------------------------------------------------------
		//  now multiply FEQ by numFramesAve/carrierPwr.
        //---------------------------------------------------------
		DebugDelay();
		feqNum      = ((i32)(aveCarrier->re * preamble->re)) << FEQ_SHIFT_DIV;
		DebugDelay();
		feqNum     += ((i32)(aveCarrier->im * preamble->im)) << FEQ_SHIFT_DIV;
		DebugDelay();
		freqEq->re  = (i16)(numFramesAve * feqNum  / carrierPwr);
		#if DEBUGIT == DEBUG_GETFEQ
			*diag.r++ = (double)freqEq->re;
			*diag.r++ = (double)feqNum;
		#endif

		DebugDelay();
		feqNum      = ((i32)(aveCarrier->re * preamble->im)) << FEQ_SHIFT_DIV;
		DebugDelay();
		feqNum     -= ((i32)(aveCarrier->im * preamble->re)) << FEQ_SHIFT_DIV;
		DebugDelay();
		freqEq->im  = (i16)(numFramesAve * feqNum  / carrierPwr);
		#if DEBUGIT == DEBUG_GETFEQ
			*diag.i++ = (double)freqEq->im;
			*diag.i++ = (double)feqNum;
		#endif

		aveCarrier++;
		freqEq++;
		preamble++;
 	}

	#if DEBUGIT == DEBUG_GETFEQ
		mexPutArray(diagArray, "caller");
	#endif
	
	return (i16)minPwr;
}
#endif

#if FEQ_NORM_TYPE == FEQ_NORM_BIN		// Use power-of-2 shifting to approximate freq equalization
//==========================================================================================
// Function:		getFreqEq()
//
// Description: 	Frequency equalization calculation 
//					EQ = tx/rx = preamble / receive = Preamble / (aveCarrier/N) = N*P/A
//
//					complex math:
//						EQ = N/(ar*ar + ai*ai) * ((ar*pr + ai*pi) + j*(ar*pi - ai*pr))
//							where N is the number of FFT averages
//
// Revision History:
//==========================================================================================
i16 getFreqEq( iCplx *freqEq, iCplx *aveCarrier, i16 numFramesAve )
{
	u16				n;					// loop counter, return code
	iCplx			*preamble;

	i32				carrierPwr;		
	i16				minPwr = 0x7FFF;	// init to a big number
	i16				pwrScaleFactor;
	i16				k, shift;
	
	
	//---- allocate a diagnostic array to pass to Matlab ----------
	#if DEBUGIT == DEBUG_GETFEQ
		diagArray = mxCreateDoubleMatrix(4, CARRIER_LEN, mxCOMPLEX);
		mxSetName(diagArray, "feq_diag");
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
	#endif

	//-----------------------------------------------------------------
	//	calc the complex freq equalization value that is 
	//	multiplied with each carrier in the data frames
	//-----------------------------------------------------------------
	preamble = (iCplx *)distanceArray;	// get complex phase from txPhase data 
	getPrePhases( preamble, PreambleArray );
	pwrScaleFactor = numFramesAve << PWR_SCALE_SHIFT;
		
	for( n = 0; n < CARRIER_LEN; n++ )
	{								
		//------------------------------------------------
		//	(1.15 * 1.15) >> 12 = 2.30 >> 12 = 14.18 ==> -2.18
		//		so carrierPwr is 15-PWR_SHIFT bigger than a 1.15 number
		//-------------------------------------------------
		carrierPwr = iSquare(aveCarrier->re, PWR_SHIFT) 
				   + iSquare(aveCarrier->im, PWR_SHIFT)
				   + 1;										// make it at least 1
		if( carrierPwr < 0 )
		{
			PostErrorCode(0xBAE2, "getFreqEq", "preDet.c", "Overflowed carrier power");
		}
		if( carrierPwr < minPwr )
			minPwr = carrierPwr;

		#if DEBUGIT == DEBUG_GETFEQ
			*diag.r++ = (double)preamble->re;
			*diag.i++ = (double)preamble->im;

			*diag.r++ = (double)aveCarrier->re;
			*diag.i++ = (double)aveCarrier->im;

			*diag.r++ = (double)carrierPwr;
		#endif

		//---------------------------------------------------------
		//  Ideally we would multiply FEQ by numFramesAve/carrierPwr.
		//  But this means dividing by carrierPwr. Instead, find the 
		//  2^k closest to numFramesAve/carrierPwr and do a shift
		//  instead of the divide.
        //---------------------------------------------------------
		for( k = PWR_SCALE_SHIFT;  k > 1;  k-- )
		{
			if( carrierPwr > pwrScaleFactor )
                break;
            carrierPwr <<= 1;
		}

		if( k <= 1 )
		{
			#if COMPILE_MODE == MEX_COMPILE
				char	prtStr[80];
				//mexPrintf("aveCarrier = %d %+dj\ncarrierPwr = %d\npwrScaleFactor = %d\n",
				//		aveCarrier->re, aveCarrier->im, carrierPwr, pwrScaleFactor );
				diagData(symbolArray, CARRIER_LEN, "feqSymbols", diagICPLX);
				diagData(recSignalArray, RX_CIRC_BUFFER_LEN, "rxArray", diagI16);
				sprintf(prtStr, "Failed doing power scale factor.  numFramesAve = %d  aveCarrier = %d, %d",
						numFramesAve, aveCarrier->re, aveCarrier->im );
				PostErrorCode(0xBAD1, "getFreqEq", "preDet.c", prtStr);
			#endif
			//uStatus = 0xBAD1;
			PostErrorCode(0xBAD1, "getFreqEq", "preDet.c", "Failed doing power scale factor");
			#if SAVETRACE == TRUE
				SaveTraceData(0xBAD1);			//!!!DEBUG  Put a marker in the trace buffer
				SaveTraceData(n);				//!!!DEBUG  Put a marker in the trace buffer
				SaveTraceData((u16)carrierPwr);		//!!!DEBUG  Put a marker in the trace buffer
			#endif
		}

		shift = CARRIER_SCALE - (15-PWR_SHIFT) - k;		// 13-15+12-k = 10-k
		
		if( shift >= 0 )
		{
			carrierPwr  = aveCarrier->re * preamble->re;
			carrierPwr += aveCarrier->im * preamble->im;
			freqEq->re  = (i16)(carrierPwr << shift);
			carrierPwr  = aveCarrier->re * preamble->im;
			carrierPwr -= aveCarrier->im * preamble->re;
			freqEq->im  = (i16)(carrierPwr << shift);
		}
		else
		{
			shift = -shift;
			carrierPwr  = aveCarrier->re * preamble->re;
			carrierPwr += aveCarrier->im * preamble->im;
			freqEq->re  = (i16)(carrierPwr >> shift);
			carrierPwr  = aveCarrier->re * preamble->im;
			carrierPwr -= aveCarrier->im * preamble->re;
			freqEq->im  = (i16)(carrierPwr >> shift);
		}

		#if DEBUGIT == DEBUG_GETFEQ
			*diag.i++ = (double)shift;

			*diag.r++ = (double)freqEq->re;
			*diag.i++ = (double)freqEq->im;
		#endif

		aveCarrier++;
		freqEq++;
		preamble++;
 	}

	#if DEBUGIT == DEBUG_GETFEQ
		mexPutArray(diagArray, "caller");
	#endif
	
	return minPwr;
}
#endif


#if FEQ_NORM_TYPE == FEQ_NORM_LOOKUP		// Use lookup table to normalize
//==========================================================================================
// Function:		getFreqEq()
//
// Description: 	Frequency equalization calculation 
//					EQ = tx/rx = preamble / receive = Preamble / (aveCarrier/N) = N*P/A
//							where N is the number of FFT averages
//
//					complex math:
//						EQ = N/(ar*ar + ai*ai) * ((ar*pr + ai*pi) + j*(ar*pi - ai*pr))
//						   = invPwr * ((ar*pr + ai*pi) + j*(ar*pi - ai*pr))
//
// Revision History:
//==========================================================================================
#define	RES_LO		8
#define	RES_HI		11
#define	KSM_LO		9
#define	KSM_HI		15
#define INV_SHIFT	10
i16 getFreqEq( iCplx *freqEq, iCplx *aveCarrier, i16 numFramesAve )
{
	u16				n, c;					// loop counter, return code
	iCplx			*preamble;

	i32				carrierPwr, invPwr;		
	i16				minPwr = 0x7FFF;	// init to a big number

	static const i32		invLookupSlope[] = {
				  //-1024,  -256,  -85,  -43,  -26,  -17,  -12,   -9 };
					-8192, -2048, -683, -341, -205, -137,  -98,  -73 };
		static const i32		invLookupOffLo[] = {
				  //1536,  768,  427,  299,  230,  188,  158,  137 };
					6144, 3072, 1707, 1195,  922,  751,  634,  549 };
	static const i32		invLookupOffHi[] = {
				  //192,   96,   53,   37,   29,   23,   20,   17 };
					768,  384,  213,  149,  115,   94,   79,   69 };

	
	//---- allocate a diagnostic array to pass to Matlab ----------
	#if DEBUGIT == DEBUG_GETFEQ
		diagArray = mxCreateDoubleMatrix(4, CARRIER_LEN, mxCOMPLEX);
		mxSetName(diagArray, "feq_diag");
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
	#endif

	//-----------------------------------------------------------------
	//	calc the complex freq equalization value that is 
	//	multiplied with each carrier in the data frames
	//-----------------------------------------------------------------
	preamble = (iCplx *)distanceArray;	// get complex phase from txPhase data 
	getPrePhases( preamble, PreambleArray );

	//---- calc frequency equalization for each carrier ---------------
	for( n = 0; n < CARRIER_LEN; n++ )
	{								
		carrierPwr = iSquare(aveCarrier->re, PWR_SHIFT)			// get the signal power
				   + iSquare(aveCarrier->im, PWR_SHIFT);
		if( carrierPwr < 0 )
		{
			PostErrorCode(0xBAE2, "getFreqEq", "preDet.c", "Overflowed carrier power");
		}
		if( carrierPwr < minPwr )
			minPwr = carrierPwr;

		//---- calculate 1/carrierPwr using a table lookup -----------
		if( carrierPwr < (1<<RES_HI) )
		{
			c = carrierPwr >> RES_LO;
			invPwr = (invLookupSlope[c]*carrierPwr) >> KSM_LO;
			invPwr += invLookupOffLo[c];
		}
		else
		{
			c = carrierPwr >> RES_HI;
			invPwr = (invLookupSlope[c]*carrierPwr) >> KSM_HI;
			invPwr += invLookupOffHi[c];
		}
		invPwr *= numFramesAve;

		//---- now to the complex math to get FEQ values -------------
		freqEq->re  = aveCarrier->re * preamble->re;
		freqEq->re += aveCarrier->im * preamble->im;
		freqEq->re  = (i16)(( invPwr * (i32)freqEq->re ) >> INV_SHIFT );

		freqEq->im  = aveCarrier->re * preamble->im;
		freqEq->im -= aveCarrier->im * preamble->re;
		freqEq->im  = (i16)(( invPwr * (i32)freqEq->im ) >> INV_SHIFT );
		
		#if DEBUGIT == DEBUG_GETFEQ
			*diag.r++ = (double)preamble->re;
			*diag.i++ = (double)preamble->im;

			*diag.r++ = (double)aveCarrier->re;
			*diag.i++ = (double)aveCarrier->im;

			*diag.r++ = (double)carrierPwr;
			*diag.i++ = (double)invPwr;

			*diag.r++ = (double)freqEq->re;
			*diag.i++ = (double)freqEq->im;
		#endif

		aveCarrier++;
		freqEq++;
		preamble++;
 	}

	#if DEBUGIT == DEBUG_GETFEQ
		mexPutArray(diagArray, "caller");
	#endif
	
	return minPwr;
}
#endif


#if FEQ_NORM_TYPE == FEQ_NORM_NONE		// don't normalize
//==========================================================================================
// Function:		getFreqEq()
//
// Description: 	Frequency equalization calculation 
//
// Revision History:
//==========================================================================================
i16 getFreqEq( iCplx *freqEq, iCplx *aveCarrier, i16 numFramesAve )
{
	u16				n;					// loop counter, return code
	iCplx			*preamble;
	
	
	//---- allocate a diagnostic array to pass to Matlab ----------
	#if DEBUGIT == DEBUG_GETFEQ
		diagArray = mxCreateDoubleMatrix(4, CARRIER_LEN, mxCOMPLEX);
		mxSetName(diagArray, "feq_diag");
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
	#endif

	//-----------------------------------------------------------------
	//	calc the complex freq equalization value that is 
	//	multiplied with each carrier in the data frames
	//-----------------------------------------------------------------
	preamble = (iCplx *)distanceArray;	// get complex phase from txPhase data 
	getPrePhases( preamble, PreambleArray );
	//pwrScaleFactor = numFramesAve << PWR_SCALE_SHIFT;
		
	for( n = 0; n < CARRIER_LEN; n++ )
	{	
		freqEq->re = 1024;
		freqEq->im = 0;

		#if DEBUGIT == DEBUG_GETFEQ
		{
			i32				carrierPwr;		

			*diag.r++ = (double)preamble->re;
			*diag.i++ = (double)preamble->im;

			*diag.r++ = (double)aveCarrier->re;
			*diag.i++ = (double)aveCarrier->im;

			carrierPwr = iSquare(aveCarrier->re, PWR_SHIFT) 
					   + iSquare(aveCarrier->im, PWR_SHIFT);

			*diag.r++ = (double)carrierPwr;
			*diag.i++ = (double)FEQ_SHIFT;

			*diag.r++ = (double)freqEq->re;
			*diag.i++ = (double)freqEq->im;
		}
		#endif

		aveCarrier++;
		freqEq++;
		preamble++;
 	}

	#if DEBUGIT == DEBUG_GETFEQ
		mexPutArray(diagArray, "caller");
	#endif
	
	return 0x2000;
}
#endif
