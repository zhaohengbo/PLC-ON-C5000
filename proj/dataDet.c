//==========================================================================================
// Filename:		dataDet.c
//
// Description:		Functions for data detection in an OFDM power line modem.
//
// Copyright (C) 2000 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//========================================================================
#include "ofdm_modem.h"
#if COMPILE_MODE == DSP_COMPILE
	#include "intr.h"		// Needed for interrupt enable/disable macros
#endif

//==============================================================
//	global vars for this module
//===============================================================
iCplx			prevCarrier1;  	// symbol from previous frame first carrier


//==============================================================
//	function prototypes for this module
//===============================================================
void aveDistance( MetricType *distance, iCplx *symbols );


//==========================================================================================
// Function:		readDataFrames()
//
// Description:		Main data detection function. 	
//
// Revision History:
//==========================================================================================
u16 readDataFrames(	u16 *pUserData, i16 *recPtr, iCplx *freqEq )
{
	u16				block, frame;
	u16				parityGood;
	MetricType		*distance;

	//---- allocate a diagnostic array to pass to Matlab ----------
	#if DEBUGIT == DEBUG_VITERBI_STATES
		diagArray = mxCreateDoubleMatrix(2, NUM_SYMBOLS*VITERBI_NUM_STATES, mxREAL);
		mxSetName(diagArray, "vStates");
		diag.r = mxGetPr(diagArray);
	#endif
	#if DEBUGIT == DEBUG_DISTANCE
		diagArray = mxCreateDoubleMatrix(2, NUM_SYMBOLS, mxREAL);
		mxSetName(diagArray, "vDist");
		diag.r = mxGetPr(diagArray);
	#endif
	#if DEBUGIT == DEBUG_DISTANCE2
		diagArray = mxCreateDoubleMatrix(8, NUM_SYMBOLS*DATA_FRAMES_PER_BLOCK, mxREAL);
		mxSetName(diagArray, "vDist2");
		diag.r = mxGetPr(diagArray);
	#endif
	#if DEBUGIT == DEBUG_DATAFFT
		diagArray = mxCreateDoubleMatrix(2, NUM_SYMBOLS*DATA_FRAMES_PER_BLOCK, mxCOMPLEX);
		mxSetName(diagArray, "dataFFT");
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
	#endif
	#if DEBUGIT == DEBUG_CIRCFFT
		diagArray = mxCreateDoubleMatrix(2*FFT_LEN, DATA_FRAMES_PER_BLOCK*NUM_DATA_BLOCKS, mxCOMPLEX);
		mxSetName(diagArray, "dataFFT");
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
	#endif

	distance = distanceArray;

	//prevCarrier1.re = VITERBI_CAR1_RE*DATA_FRAMES_PER_BLOCK;	// init for first carrier
	//prevCarrier1.im = VITERBI_CAR1_IM*DATA_FRAMES_PER_BLOCK;
	prevCarrier1.re = VITERBI_CAR1_RE;	// init for first carrier
	prevCarrier1.im = VITERBI_CAR1_IM;

	initPathMemory(pUserData); 			// init path memory and path metrics

	#if (SAVESYMBOLS == TRUE)			// Clear the symbol debug array
		if( uTraceEnable == TRUE )
		{
			uTraceIndex = 0;
		}
	#endif

	//---- process each data frame ----------------------------
	for( block = 0; block < NUM_DATA_BLOCKS;  block++ )
	{
		memset( distance, 0, 2*CARRIER_LEN*sizeof(MetricType) );	// reset distance array
		
		for( frame = 0; frame < DATA_FRAMES_PER_BLOCK;  frame++ )
		{	
			recPtr = dataFFT( symbolArray, recPtr, freqEq ); 	// read a frame and adv the pointer
			aveDistance( distance + frame*2*SYMBOL_OFFSET, symbolArray );

			// If this is the last frame, set up the variables so the periodic interrupt 
			// functions can start doing AGC operations and looking for the next packet,
			// while we finish processing this packet.
			if (  (block == (NUM_DATA_BLOCKS-1)) 
			    &&(frame == (DATA_FRAMES_PER_BLOCK-1)) )
			{
				agcState = AgcIdle;			// change back to idle mode
				SNRflag = -3;				// Do not look for preambles yet
				recSignal = recPtr;			// Set the global recSignal pointer to the end of the data we have just received

				#if SAVETRACE == TRUE
					SaveTraceData((u16)(0xDB00 + (block<<4) + frame));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
					SaveTraceData((u16)recPtr);							//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData((u16)ReadRxDMAPointer());				//!!!DEBUG  Put a marker in the trace 
				#endif
			}
		}

		viterbiDecodeFrame( distance, CARRIER_LEN );
	}

	agcState = AgcIdle;			// change back to idle mode
	preambleDetCount = 0;		// restart counter
	prevSnrSample = ReadRxDMAPointer() - recSignalArray;	// Let SNR calc re-start here					
	SNRflag = 0;				// Allow looking for preambles
	#if SAVETRACE == TRUE
		SaveTraceData((u16)(0xDC00 + (block<<4) + frame));	//!!!DEBUG  Put a marker in the trace buffer (Shouldn't need to cast this, but Visual C++ complains)
		SaveTraceData((u16)recPtr);							//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)ReadRxDMAPointer());				//!!!DEBUG  Put a marker in the trace 
	#endif

	flushPathMemoryV();
	//diagData(pUserData, DATA_BUFFER_LEN, "rxScramble", diagU16);

	scramble(pUserData, DATA_BUFFER_LEN);			// unscramble the whole buffer
	//diagData(pUserData, DATA_BUFFER_LEN, "rxData", diagU16);

	parityGood = compareParityCheckBytes(pUserData, NUM_USER_BYTES);

	#if ( ( DEBUGIT == DEBUG_VITERBI_STATES ) \
		|| ( DEBUGIT == DEBUG_DISTANCE ) \
		|| ( DEBUGIT == DEBUG_DISTANCE2 ) \
		|| ( DEBUGIT == DEBUG_DATAFFT ) \
		|| ( DEBUGIT == DEBUG_CIRCFFT ) )
		mexPutArray(diagArray, "caller");
	#endif

	return parityGood;
}


//==========================================================================================
// Function:		aveDistance(N)
//
// Description: 	Calculate the geometric distance from the received complex symbol to each
//					of the 4 possible differential phase constelation values.  Since it turns 
//					out that the second order parts of the distance equations are the same 
//					for each case, the distance metric is a linear equation.  Since this equation 
//					is negative, the branch with the closest distance is the most positive one.
//						distance[0] = p.re * c.re + p.im * c.im;		// 0 deg		
//						distance[1] = p.re * c.im - p.im * c.re;		// 90 deg
//						distance[2] = -distance[0];						// 180 deg
//						distance[3] = -distance[1]; 					// -90 deg
//					Only the first two values are saved.  The distance meteic is summed for
//					each repeated symbol in a block.
//
//					Distance can also be described as:
//						distance[0] = AmpPrev*AmpCurr*cos(dPhase) / 2^shift
//						distance[1] = AmpPrev*AmpCurr*sin(dPhase) / 2^shift
//
// Revision History:
//==========================================================================================
void aveDistance( MetricType *distance, iCplx *symbols )
{
	#if METRIC_OFFSET == 0
		#define	DIST_LIMIT	0x3FFF
	#else
		#define	DIST_LIMIT	0x7FFF
	#endif

	i16				ncar;
	iCplx			prevSymbol;		// working previous symbol value
	iCplx			currSymbol;		// working current symbol value
	i16				dist0, dist1;
	MetricType		*bufEnd;

	bufEnd = distanceArray;
	bufEnd += 2*CARRIER_LEN;

	//---- handle the first subcarrier different ---------------------------------
	prevSymbol.re = prevCarrier1.re;
	prevSymbol.im = prevCarrier1.im;
	currSymbol.re = symbols->re;
	currSymbol.im = (symbols++)->im;

	prevCarrier1.re = currSymbol.re;
	prevCarrier1.im = currSymbol.im;

	//---- loop thourgh the rest of the subcarriers ---------------------------------
	for( ncar = 0; ncar < CARRIER_LEN; ncar++ )
	{
		dist0   = ( ((i32)currSymbol.re * (i32)prevSymbol.re) 	
	          	+ ((i32)currSymbol.im * (i32)prevSymbol.im) )	
	           	>> VITERBI_DISTANCE_SHIFT;				// 0 deg

		dist1   = ( ((i32)currSymbol.im * (i32)prevSymbol.re)	
		 		- ((i32)currSymbol.re * (i32)prevSymbol.im) )
	  			>> VITERBI_DISTANCE_SHIFT;				// 90 deg

		//---- diagnostic -----------------------
		#if DEBUGIT == DEBUG_DISTANCE2
			*diag.r++ = (double)prevSymbol.re;
			*diag.r++ = (double)prevSymbol.im;
			*diag.r++ = (double)currSymbol.re;
			*diag.r++ = (double)currSymbol.im;
			*diag.r++ = (double)dist0;
			*diag.r++ = (double)dist1;
		#endif

		*distance += dist0;							// for debug!	
		if( abs(*distance) > DIST_LIMIT )				//
		{
			PostErrorCode(0xBAD3, "aveDistance", "dataDet.c", "Overran distance metric");
			#if COMPILE_MODE == MEX_COMPILE
				mexPrintf("Distance = %d\n", *distance );
			#endif
		}

 		#if DEBUGIT == DEBUG_DISTANCE2
			*diag.r++ = (double)*distance;
		#endif
		distance++;									//

		*distance += dist1;							//
		if( abs(*distance) > DIST_LIMIT )				//
		{
			PostErrorCode(0xBAD3, "aveDistance", "dataDet.c", "Overran distance metric");
			#if COMPILE_MODE == MEX_COMPILE
				mexPrintf("Distance = %d\n", *distance );
			#endif
		}

		#if DEBUGIT == DEBUG_DISTANCE2
			*diag.r++ = (double)*distance;
		#endif
		distance++;									//

		if( distance >= bufEnd )
			distance = distanceArray;

		prevSymbol.re = currSymbol.re;		// get next complex symbol
		prevSymbol.im = currSymbol.im;
		currSymbol.re = symbols->re;
		currSymbol.im = (symbols++)->im;

	}
	return;
}

//==========================================================================================
// Function:		dataFFT()
//
// Description:		Calculate the FFT of the data frame. 	
//
// Revision History:
//==========================================================================================
i16 *dataFFT( iCplx *symbols, i16 *recPtr, iCplx *freqEq )
{
	i16				n;					// loop counter, return code
	i32				acc;				// temp 32 bit value
	iCplx			*fftBuf;			// pointer to fft buffer

#if COMPILE_MODE == DSP_COMPILE
	i16				delta;				// difference between recPtr and rxDMA
	i16*			rxDMA;
#endif



	#if COMPILE_MODE == DSP_COMPILE
		//---------------------------------------------------------------------------------
		// The frameAlign() function and the cyclic prefix calculation both move recPtr
		// ahead of the rx DMA pointer.  Wait here for the DMA pointer to start filling 
		// data before we try reading it.  Several cases, depending on whether recPtr or
		// DMA pointer have wrapped, and if DMA pointer is already past recPtr or not.
		SetXF();	
		rxDMA = ReadRxDMAPointer();
		
		delta = recPtr - rxDMA;
		if (delta > (RX_CIRC_BUFFER_LEN/2))
		{	// Huge positive value due to DMA wrap: No delay needed
			DebugDelay();				// Flush C54x if in debug mode.
		}
		else if (delta > 0)
		{
			// Small positive value:  Wait a little bit
			WaitForRxBufferFree(rxDMA, delta); 
			DebugDelay();				// Flush C54x if in debug mode.
		}
		else if (delta > -(RX_CIRC_BUFFER_LEN/2) )
		{	// Small negative  or Zero value: No delay needed
			DebugDelay();				// Flush C54x if in debug mode.
		}
		else
		{	// Huge negative value due to recPtr wrap: Two short delays needed
			WaitForRxBufferFree(rxDMA, (u16)(recSignalArray + RX_CIRC_BUFFER_LEN-rxDMA));			
			DebugDelay();				// Flush C54x if in debug mode.
			WaitForRxBufferFree(recSignalArray, (u16)(recPtr-recSignalArray));
			DebugDelay();				// Flush C54x if in debug mode.
		}

		DebugDelay();				// Flush C54x if in debug mode.
		ClearXF();

	#endif

	//----get the FFT of the frame samples -------------------------------------  
	recPtr = circFFT( fftArray, recPtr );
	
	recPtr = WrapRecPtr(recPtr, +CYCLIC_PREFIX_LEN*RX_SRC_INC);	// Move recPtr ahead to skip cyclic prefix
 		
	//-------------------------------------------------------
	//	Frequency Equalization
	//		symbol = carrier*equal = (cr+jci)*(er+jei)
	//		sr = cr*er - ci*ei
	//		si = cr*ei + ci*er
	//--------------------------------------------------------
	fftBuf = fftArray + CARRIER_LOW; // point to the carrier bins

	for( n = 0; n < CARRIER_LEN; n++ )
	{
		acc = (i32)fftBuf->re * (i32)freqEq->re 
			- (i32)fftBuf->im * (i32)freqEq->im;
		symbols->re = (i16)(acc >> FEQ_SHIFT);

		acc = (i32)fftBuf->re * (i32)freqEq->im 
			+ (i32)fftBuf->im * (i32)freqEq->re;
		symbols->im = (i16)(acc >> FEQ_SHIFT);

		#if	(SAVESYMBOLS == TRUE)			// Save the symbol array for debug
			SaveTraceData(symbols->re);
			SaveTraceData(symbols->im);
		#endif

		#if DEBUGIT == DEBUG_DATAFFT
			*diag.r++ = (double)fftBuf->re;
			*diag.i++ = (double)fftBuf->im;
			*diag.r++ = (double)symbols->re;
			*diag.i++ = (double)symbols->im;
		#endif

		fftBuf++;
		freqEq++;
		symbols++;
	}

	return recPtr;
}


//==========================================================================================
// Function:		compareParityCheckBytes()   
//
// Description: 	Calculate parity check bytes for outer RS code and append
//
// Revision History:
//==========================================================================================
u16 compareParityCheckBytes(u16 *pUserData, i16 numBytes)
{
	i16				byteCount = 0;
	u16				dataByte;
	u16				reg;

	//---- init parity table ----------------------
	if( CRCtableArray[1] == 0 )
		initCRCtable( CRCtableArray );

	//--------------------------------------------------------------
	// Read through the data to calculate the CRC word.  Compare it 
	// to the CRC appended at transmit.
	//--------------------------------------------------------------
	reg = CRC_REG_INIT;
	for( byteCount = 0; byteCount < (numBytes+2); byteCount +=2 )
	{
		dataByte = (*pUserData) >> BYTE_LEN;// high data byte
		reg = (reg << BYTE_LEN) 
			^ CRCtableArray[ (reg >> (CRC_LEN-BYTE_LEN)) ] ^ dataByte;

		dataByte = (*pUserData++) & 0x00FF;	// low data byte
		reg = (reg << BYTE_LEN) 
			^ CRCtableArray[ (reg >> (CRC_LEN-BYTE_LEN)) ] ^ dataByte;
	}

	return reg;
}


//==========================================================================================
// Function:		countErrors()   
//
// Description: 	Count number of error bits
//
// Revision History:
//==========================================================================================
i16 countErrors(u16 *pUserData)
{
	i16				n, b;
	i16				errorCount = 0;
	u16				word;

	for( n = 0;  n < NUM_USER_BYTES/2;  n++ )
	{
		if( *pUserData++ > 0 )
		{
			word = *(pUserData-1);
			for( b = 0; b < WORD_LEN; b++ )
			{
				errorCount += word & 0x0001;
				word >>= 1;
			}
		}
	}
	return errorCount;				
}


#if COMPILE_MODE == MEX_COMPILE
//==========================================================================================
// Function:		readDataFramesFrom Matlab()
//
// Description:		Diagnostic data detection function.
//					Here symbol data has been collected somewhere else and is feed into the
//					distance metric and viterbi algorithms
//
// Revision History:
//==========================================================================================
u16 readDataFramesFromMatlab( u16 *pUserData, dCplxPtr drec )
{
	u16				block, frame, n;
	u16				parityGood;
	MetricType		*distance;
	iCplx			*sym;

	//---- allocate a diagnostic array to pass to Matlab ----------
	#if DEBUGIT == DEBUG_VITERBI_STATES
		diagArray = mxCreateDoubleMatrix(2, NUM_SYMBOLS*VITERBI_NUM_STATES, 1, mxREAL);
		mxSetName(diagArray, "vStates");
		diag.r = mxGetPr(diagArray);
	#endif
	#if DEBUGIT == DEBUG_DISTANCE2
		diagArray = mxCreateDoubleMatrix(8, NUM_SYMBOLS*DATA_FRAMES_PER_BLOCK, mxREAL);
		mxSetName(diagArray, "vDist2");
		diag.r = mxGetPr(diagArray);
	#endif

	distance = distanceArray;

	prevCarrier1.re = VITERBI_CAR1_RE*DATA_FRAMES_PER_BLOCK;	// init for first carrier
	prevCarrier1.im = VITERBI_CAR1_IM*DATA_FRAMES_PER_BLOCK;

	initPathMemory(pUserData); 			// init path memory and path metrics

	//---- process each data frame ----------------------------
	for( block = 0; block < NUM_DATA_BLOCKS;  block++ )
	{
		memset( distance, 0, 2*CARRIER_LEN*sizeof(MetricType) );	// reset distance array
		
		for( frame = 0; frame < DATA_FRAMES_PER_BLOCK;  frame++ )
		{	
			//recPtr = dataFFT( symbolArray, recPtr, freqEq ); 	// read a frame and adv the pointer
			sym = symbolArray;
			for( n = 0; n < CARRIER_LEN; n++ )
			{
				sym->re = *drec.r++;
				sym->im = *drec.r++;
				sym++;
			}

			aveDistance( distance + frame*2*SYMBOL_OFFSET, symbolArray );    //store Matlab diag in here
		}

		viterbiDecodeFrame( distance, CARRIER_LEN );
	}

	flushPathMemory();

	scramble(pUserData, DATA_BUFFER_LEN);			// unscramble the whole buffer

	parityGood = compareParityCheckBytes(pUserData, NUM_USER_BYTES);

	#if ( ( DEBUGIT == DEBUG_VITERBI_STATES ) \
		|| ( DEBUGIT == DEBUG_DISTANCE2 ) )
		mexPutArray(diagArray, "caller");
	#endif

	return parityGood;
}
#endif

