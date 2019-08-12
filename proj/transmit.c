//==========================================================================================
// Filename:		transmit.c
//
// Description:		Functions related to the data transmition of a 	power line modem.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"

#define WINDOW_SCALE	8


// When RAMP_TEST is defined, we replace the real preamble and data with a ramp
// waveform to make DMA debug easier
//#define		RAMP_TEST	

#if COMPILE_MODE == DSP_COMPILE
	#include "intr.h"		// Needed for interrupt enable/disable macros
#else						// MEX:  Fake macros for interrupt enable/disable
	#define	INTR_ENABLE(x)	 
	#define INTR_DISABLE(x)
#endif


// When PEAK_DEBUG is TRUE, we spit out trace messages with the peak amplitude in each transmit frame
#define	PEAK_DEBUG	TRUE
//#define	PEAK_DEBUG	FALSE

// When PEAK_DEBUG is TRUE, scale the transmitted waveform to have constant peaks, boosting average
#define	ADJUST_TX_PEAKS	TRUE
//#define	ADJUST_TX_PEAKS	FALSE


//==========================================================================================
// global var for this file
//==========================================================================================
	static const iCplx 	complexMap[4] = 
					{	1,  0,
						0,  1,
					   -1,  0,
						0, -1  };

//--- Matlab code to generate FrameWindow values: -----------------------
// wLen = 16; wScale=8; win = round((.5 - .5*cos([0:wLen-1]/wLen*pi))*2^wScale); sprintf('%5.0f,',win)
static const i16 FrameWindow[WINDOW_LEN] =
//		{ 256,	256,  256,  256,  256,  256,  256,  256,  256,  256,  256,  256,  256,  256,  256,  256 }; // Unity gain throughout. Zero postfix.
		{   0,    2,   10,   22,   37,   57,   79,  103,  128,  153,  177,  199,  219,  234,  246,  254 }; // Raised cosine window
//		{   0,    0,    0,    0,    0,    0,    0,    0,  256,  256,  256,  256,  256,  256,  256,  256 }; // Abrupt cutoff


//==========================================================================================
// Function:		initCRCtable()
//
// Description: 	Init CRC-16 table
//
// Revision History:
//==========================================================================================
void initCRCtable(u16 *CRCtable)
{
	u16			reg;		// working register
	i16			n, b;		// counters

	#if COMPILE_MODE == MEX_COMPILE
		mexPrintf("CRC table init.\n");
	#endif
	reg = 0;
    for( n = 0; n < 256; n++ )
	{
        reg = n << 8;
        for( b = 0; b < 8; b++ )
		{
            if( reg & CRC_TOPBIT )
				reg = (reg << 1) ^ CRC_POLYNOMIAL;
			else
				reg <<= 1;
        }
        *CRCtable++ = reg;
    }    
	return;
}


//==========================================================================================
// Function:		viterbiZero()
//
// Description: 	Force the end of the user data buffer to zero so that the receiving
//					trelis returnes to zero at the end of the message.
//
// Revision History:
//==========================================================================================
void viterbiZero(u16 *pUserData)
{						   // zeros for Viterbi	     zero rest of buffer
	i16			zeroBits =  (2*VITERBI_K + 1) + ( DATA_BUFFER_LEN*WORD_LEN - NUM_SYMBOLS);

	pUserData += (DATA_BUFFER_LEN - 1);			// point to last word in data buffer
	while( zeroBits >= WORD_LEN )
	{
		*pUserData = 0;							// zero this word
		pUserData--;							// point to previous word
		zeroBits -= WORD_LEN;
		//mexPrintf("zeroBits loop = %d\n", zeroBits);
	}

	*pUserData >>= zeroBits;					// zero final bits
	*pUserData <<= zeroBits;
	return;
}


//==========================================================================================
// Function:		appendParityCheckBytes()
//
// Description: 	Calculate parity check bytes for outer RS code and append.
//					Data come in as 16 bit words. Take each byte and calc parity bytes
//					Calc CRC on whole words.
//
// Revision History:
//==========================================================================================
void appendParityCheckBytes(u16 *pUserData, i16 numWords)
{
	i16				wordCount = 0;
	u16				dataByte;
	u16				reg;
	u16				*parity;

	//---- init parity table ----------------------
	if( CRCtableArray[1] == 0 )
		initCRCtable( CRCtableArray );

	//---- point to appended parity part of data buffer and zero out ---
	parity = pUserData + numWords;
	memset(parity, 0, (DATA_BUFFER_LEN-numWords)*sizeof(u16));	
	
	/*--------------------------------------------------------------
	Read through the data to generate the CRC word.
	Append the CRC word at the end of the user data in the data buffer
	--------------------------------------------------------------*/
	reg = CRC_REG_INIT;
	for( wordCount = 0; wordCount < (numWords+1); wordCount++ )
	{
		dataByte = (*pUserData) >> BYTE_LEN;// high data byte
		reg = (reg << BYTE_LEN) 
			^ CRCtableArray[ (reg >> (CRC_LEN-BYTE_LEN)) ] ^ dataByte;

		dataByte = (*pUserData++) & 0x00FF;	// low data byte
		reg = (reg << BYTE_LEN) 
			^ CRCtableArray[ (reg >> (CRC_LEN-BYTE_LEN)) ] ^ dataByte;
	}
	*parity = reg;	
	return;
}

//==========================================================================================
// Function:		scramble()
//
// Description: 	Scrambler using linear shift register with polynomial 	
//					S(x) x^7 + x^4 + 1
//					The default inital state is 127 (all ones).
//
//					                             input data |
//					                  |--------------|      |
//					                  |              V      V
//					  ->D-->D-->D-->D-.>D-->D-->D--> + -.-> + 
//					  |  1   2   3   4   5   6   7      |   |    
//					  |---------------------------------|   |
//					                            output data V
//
// Revision History:
//==========================================================================================
void scramble(u16 *pUserData, u16 userDataLen)
{
	i16				n;
	u16				wordCount = 0;

	while( wordCount < userDataLen )
		for( n = 0; n < SCRAMBLER_LEN; n++ )
		{	
			*pUserData++ ^= scramblerLookup[n];
			wordCount++;
			if( wordCount >= userDataLen )
				break;
		}
}

//==========================================================================================
// Function:		CalcTxStartAddr()
//
// Description: 	Calculate the starting address in the TxSignal array, given the
// 					present pointer position and the requested block size.
//
// Revision History:
//==========================================================================================
#if COMPILE_MODE == DSP_COMPILE
TXDATA* CalcTxStartAddr(TXDATA* uCurrAddr, u16 uCnt)
{
	TXDATA*	uStart;

	if ( (uCurrAddr + (uCnt * AFE_SIZE)) > (txSignalArray + TX_BUFFER_LEN))
	//if ( ((uCurrAddr-txSignalArray) + (uCnt * AFE_SIZE)) > TX_BUFFER_LEN )		// fishing here
	{
		uStart = txSignalArray;		// Too close to end; re-start at beginning
	}
	else
	{
		uStart = uCurrAddr;			// Plenty of room; start transfer from present pointer
	}

	return (uStart);
}
#else
	#define	CalcTxStartAddr(x,y)	x	// MEX: Make a fake function that returns its own argument
#endif


#define	TX_RAW_CLIP			(12000)			// 12000 = Crest factor of 3.59  (13.5 bits)
#define	TX_PEAK_SETPOINT	(32767L)		// Max values allowed by transmit DAC

#define	TX_GAIN_SCALE		11				// that is: 2048
#define	TX_GAIN		 		2400			// (i16)(1.17 * (1<<TXGAIN_SCALE) )
#define	TX_PEAK_CLIP		(12000L)		// Truncate at this level digitally
		 									// instead of saturating the driver
//==========================================================================================
// Function:		setTxPower()
//
// Description: 	Adjust the gain of the transmitted signal to get the desired power
//					out of the codec.
//
//	This function provides fine control of the amplitude of the signal generated by the IFFT.
//	It also clips the signal at the level set by TX_PEAK_CLIP.  This is useful in preventing the
//	analog line driver from going into saturation since it takes the line driver amplifier a while
//	to come out of saturation.  That is, it's better to saturate the digital signal then the 
//	analog signal.
//
// Revision History:
//==========================================================================================
void setTxPower(iCplx* ifftBuf)
{
#if (ADJUST_TX_PEAKS == TRUE)
	u16			n;				   	// loop counter
	i16			sample;
	u16 		uSample;
	u16			magMax = 0;			
	u16			magMaxCnt = 0;			

	for(n=0; n<FFT_LEN; n++)
	{
		#if COMPILE_MODE == MEX_COMPILE
			sample = (i16)(((i32)ifftBuf->re * TX_GAIN)>>TX_GAIN_SCALE);
		#else			//  DSP_COMPILE
			sample = (ifftBuf->re * TX_GAIN)>>TX_GAIN_SCALE; 
		#endif
		
		uSample = abs(sample);
		if( uSample > TX_PEAK_CLIP )   
		{
			magMaxCnt++;
			if( uSample > magMax )
			{
				magMax = uSample;
			}
			ifftBuf->re = (i16)Saturate( (i32)sample, -TX_PEAK_CLIP, TX_PEAK_CLIP); 	
		}
		else
		{
			ifftBuf->re = sample;
		}
		ifftBuf++;
	}

	#if SAVETRACE == TRUE
		if( magMax > 0 )
		{
			PostErrorCode(0xBAE1, "setTxPower", "transmit.c", "Clipped transmit waveform");
			SaveTraceData(0xDF00);	
			SaveTraceData(magMax);	 
			SaveTraceData(magMaxCnt); 
		}
		DebugDelay();
	#endif

#endif
	return;
}



//==========================================================================================
// Function:		AdjustTxPeaks()
//
// Description: 	Adjust the gain of the transmitted signal to maintain constant peaks.
//
//	---------- CALCULATION OF RMS POWER AND CREST FACTOR ----------------------------------
//	RMS value for each entire frame (except cyclic prefix) is 
//		RMSideal = FFT_LEN * sqrt(2 * CARRIER_LEN)  =  256 * sqrt( 2 * 60)	= 2804
//	Because the cyclic prefix repeats just a portion of the frame, the actual RMS could be 
//	as little as 
//	  RMSmin = (FFT_LEN / (FFT_LEN + CYCLIC_PREFIX_LEN)) = (256/(256+26)) = 0.91 * RMSideal
//	or as high as 
//	  RMSmax = 2 * RMSideal.
//	However, for most waveforms, the RMS of the packet with cyclic prefix will be pretty 
//	close to RMSideal. 
//
//	The crest factor is defined as the ratio of the peak value divided by the RMS value.
//	A sine wave has a crest factor of sqrt(2) = 1.414.  An OFDM waveform could have a crest 
//	factor as high as CARRIER_LEN*sqrt(2) = 60*1.414 = 84 worst case.  In general, however
//	the crest factor looks like a gaussian distributed variable, with crest factors above 3.5
//	occurring only about once per 1000 frames.
//
//	In this function, we want to boost the RMS as high as possible without truncating the 
//	peak values.  On the occassional rare frame with a huge crest factor that exceeds the 
//	limit, we will adjust the RMS gain as if the crest factor were right at the upper limit
//	and then clip the peak that exceeds that limit.
//
//
// Revision History:
//==========================================================================================
void AdjustTxPeaks(iCplx* ifftBuf, u16 block, u16 frame)
{
	u16		n;				   	// loop counter
	u16		magSample;			// magnitude (abs) of waveform sample
	u16		magMax = 0;			// Peak magnitude
	i16		txGain;				// Gain to multiply tx signal
	iCplx* 	fftBuf;				// fft buffer pointer

	//---- Find biggest sample.  Its index indicates offset from beginning of frame --------
	fftBuf = ifftBuf;
	for(n=0; n<FFT_LEN; n++)
	{
		magSample = abs((fftBuf++)->re);
		if( magSample > magMax )	// find the max real sample
		{
			magMax = magSample;
		}
	}
	DebugDelay();

	//----------------------------------------------------------------------------
	// Calculate the gain that we will use to multiply the entire signal.
	// The saturate term in the denominator limits the range of gains.
	// Lower limit sets upper limit of gain to keep it from rolling over.
	// Upper limit keeps us from squelching frames with really large peaks; 
	// on those, we will clip the peaks.
	//----------------------------------------------------------------------------
	txGain = (TX_PEAK_SETPOINT<<TX_GAIN_SCALE) / Saturate(magMax, 1<<TX_GAIN_SCALE, TX_RAW_CLIP);
	//mexPrintf("  %d   %8.4f\n", magMax, ((double)txGain)/((double)(1<<TX_GAIN_SCALE)) );
	DebugDelay();

#if (ADJUST_TX_PEAKS == TRUE)
	fftBuf = ifftBuf;
	if (magMax < TX_RAW_CLIP)
	{	// Reasonable input signal (common).  No saturation needed, so just multiply gain.
		for(n=0; n<FFT_LEN; n++)
		{
			#if COMPILE_MODE == MEX_COMPILE
				(fftBuf++)->re = (i16)( ((i32)(fftBuf)->re * (i32)txGain)>>TX_GAIN_SCALE );
			#else			//  DSP_COMPILE
				(fftBuf++)->re = ((fftBuf)->re * txGain)>>TX_GAIN_SCALE; 	// Takes 1802 ticks. Uses MPY instruction.
				//(fftBuf++)->re = ((i32)(fftBuf)->re * (i32)txGain)>>13; 	// Takes 10003 ticks. Uses L$$MPY function.
			#endif
		}
	}
	else	// Big input signal (rare).  Saturate each entry.  Use 32-bit for intermediate results.
	{
		//mexPrintf("saturating..." );
		for(n=0; n<FFT_LEN; n++)
		{
			(fftBuf++)->re = (i16)Saturate( ((i32)(fftBuf)->re * (i32)txGain)>>TX_GAIN_SCALE,
			                                -TX_PEAK_SETPOINT, 
			                                 TX_PEAK_SETPOINT); 	
		}

		// Keep track of every time we need to clip a frame
		PostErrorCode(0xBAE1, "AdjustTxPeaks", "transmit.c", "Clipped transmit waveform");
		#if SAVETRACE == TRUE
			SaveTraceData(0xDF00 + (block<<4) + frame);	//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)magMax);					//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)txGain);					//!!!DEBUG  Put a marker in the trace buffer
		#endif
		DebugDelay();
	}
#endif

#if (PEAK_DEBUG==TRUE)
	#if SAVETRACE == TRUE
		SaveTraceData(0xDE00 + (block<<4) + frame);	//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)magMax);				//!!!DEBUG  Put a marker in the trace buffer
		SaveTraceData((u16)txGain);					//!!!DEBUG  Put a marker in the trace buffer
	#endif
#endif

	DebugDelay();
	return;
}


//==========================================================================================
// Function:		makePreamble()
//
// Description: 	Make the preamble and send it to the AFE.
//					This function transmits data in several chunks.  Each chunk consists of
//					one or more calls to these functions:
//						CalcTxStartAddr - Finds location in txBuff big enough to hold chunk
//						FillAFETxBuffI16 - Fills values into tx buffer.
//						JamDMAReloadRegs - Jams the DMA registers to point to next chunk.
//						                   Handles timing to avoid conflicts.
//
//		logical frame   |   0   |    1  |   2   |   3   |   4   |   5   |   sync    | data  |
//					| 2 | 1 | 2 | 1 | 2 | 1 | 2 | 1 | 2 | 1 | 2 | 1 | 2 |-1 |-2 |-1 |   D0  |
//		windowing    <                                                   x           x
//		xmit frame	|  1st  | loop  | loop  | loop  | loop  | loop  | last  | sync  |       | 
//
// Return:			Pointer to the next position in txSignalArray
//
// Revision History:
//==========================================================================================
TXDATA* makePreamble(TXDATA* pTxSignal, i16 *prefix )
{
	i16				frame;
	u16				n;						// loop counter
	TXDATA*			TxStart;
 	iCplx			*ifftBuf;
	i16				preSignal[FFT_LEN];		// local copy of frame waveform


	//---- put carriers in correct fft bins and call the ifft routine ----
	getPrePhases( phaseEqArray, PreambleArray );
	//diagData(phaseEqArray, CARRIER_LEN, "prePhase", diagICPLX);

	// We must shut off timer interrupts here to prevent the receive process from corrupting the FFT buffer
	INTR_DISABLE(TINT0); 	// Disable Timer0 Interrupt

	#if SAVETRACE == TRUE
		SaveTraceData(0x5000);				// Put a marker in the trace buffer
		SaveTraceData((u16)recSignal);		// at the start of a transmit opperation
		SaveTraceData((u16)pTxSignal);		// 
	#endif

	//---- put carriers in correct fft bins and call the ifft routine ----
	fillCarriers( fftArray, phaseEqArray, PRE_IFFT_SCALE );
	cifft( (DATA *)(fftArray), FFT_LEN, IFFT_SCALE );

	//AdjustTxPeaks(fftArray, 0xF, 0xF);	// Adjust gain of tx signal to maintain constant peaks.
	setTxPower(fftArray);					// 2nd & 3rd arguments are markers for trace buffer.

	// Copy preamble from FFT buffer to a local variable so that when the receive 
	// interrupt changes the FFT buffer it doesn't affect the preamble.
	ifftBuf = fftArray;
	for( n = 0; n < FFT_LEN; n++ )
	{
		#ifdef RAMP_TEST							
			preSignal[n] = (n-128)<<5;			// Make a fake preamble ramp.
		#else
			preSignal[n] = (ifftBuf++)->re;		// Make a local copy from the FFT buffer
		#endif
	}
	//diagData(preSignal, FFT_LEN, "preSignal", diagI16);

	INTR_ENABLE(TINT0); 			// Re-Enable Timer0 Interrupt

	getSyncPhases( phaseEqArray, PreambleArray );	// get ready for sync frame
	//diagData(phaseEqArray, CARRIER_LEN, "syncPhase", diagICPLX);


	//---- get the modulated time domain signal and repeat for each frame 

	//---- send the first frame which is windowed, starting with the 2nd half of the waveform ----------
	for( n = 0; n < WINDOW_LEN ; n++ )
	{
		prefix[n]  = ( (i32)(preSignal[FFT_LEN/2 + n]) * FrameWindow[n] ) >> WINDOW_SCALE;
	}
	TxStart   = CalcTxStartAddr (pTxSignal, FFT_LEN);				// Find a free space in Tx buffer
	pTxSignal = FillAFETxBuffI16 (TxStart,   prefix, WINDOW_LEN);	// windowed part of 2nd half of waveform
	pTxSignal = FillAFETxBuffI16 (pTxSignal, preSignal+FFT_LEN/2+WINDOW_LEN, FFT_LEN/2-WINDOW_LEN); // rest of 2nd half
	pTxSignal = FillAFETxBuffI16 (pTxSignal, preSignal, 				     FFT_LEN/2); // 1st half of waveform
	JamDMAReloadRegs(TxStart, FFT_LEN);		// Load the AFE registers to start the transmit

	//---- send middle frames with normal phase, starting in the second half of the waveform ---------
	for( frame = 1; frame < NUM_PRE_FRAMES-2; frame++ )  
	{
		TxStart   = CalcTxStartAddr(pTxSignal, FFT_LEN);	// Find a free space in Tx buffer
		pTxSignal = FillAFETxBuffI16 (TxStart,   preSignal+FFT_LEN/2, FFT_LEN/2);	// 2nd half of waveform
		pTxSignal = FillAFETxBuffI16 (pTxSignal, preSignal,           FFT_LEN/2);	// 1st half of waveform
		JamDMAReloadRegs(TxStart, FFT_LEN);		// Load the AFE registers to start the transmit
	}
	
	//---- send the last half preamble frame ----------------------------------------------
	TxStart   = CalcTxStartAddr(pTxSignal, FFT_LEN);	// Find a free space in Tx buffer
	pTxSignal = FillAFETxBuffI16 (TxStart,   preSignal+FFT_LEN/2, FFT_LEN/2); // rest of 2nd half

	// //---- Ramp down the positive preamble and ramp up the negative.
	// //     Formula looks different from windowing done elsewhere because we are combining
	// //	   the ramp-down of a signal with a ramp-up of a negative version of same signal.
	// for( n = 0; n < WINDOW_LEN ; n++ )
	// {									  
	// 	prefix[n] = (256-FrameWindow[n]) - FrameWindow[n];	// prefix = (windowDn - windowUp) * preSignal
	// 	prefix[n]  = ( (i32)(preSignal[n]) * prefix[n] ) >> WINDOW_SCALE;
	// }
	// pTxSignal = FillAFETxBuffI16 (pTxSignal, prefix, WINDOW_LEN);	// windowed part of sync field

	//---- calc prefix for transition between preamble and sync frame ------------
	for( n = 0; n < WINDOW_LEN ; n++ )
	{									  
		prefix[n]  = ( (i32)(preSignal[n]) * (256-FrameWindow[n]) ) >> WINDOW_SCALE;	// Window down
	}

	//---- Now calc sync frame waveform----------------------------------------
	INTR_DISABLE(TINT0); 	// Disable Timer0 Interrupt
	
	fillCarriers( fftArray, phaseEqArray, PRE_IFFT_SCALE );
	cifft( (DATA *)(fftArray), FFT_LEN, IFFT_SCALE );

	//AdjustTxPeaks(fftArray, 0xF, 0xF);	// Adjust gain of tx signal to maintain constant peaks.
	setTxPower(fftArray);		

	ifftBuf = fftArray;
	for( n = 0; n < FFT_LEN; n++ )
	{
		#ifdef RAMP_TEST							
			preSignal[n] = -preSignal[n];		// reviewse test ramp
		#else
			preSignal[n] = (ifftBuf++)->re;		// Make a local copy from the FFT buffer
		#endif
	}
	//diagData(preSignal, FFT_LEN, "syncSignal", diagI16);
	
	INTR_ENABLE(TINT0); 			// Re-Enable Timer0 Interrupt

	//---- calc prefix for transition between preamble and sync frame ------------
	for( n = 0; n < WINDOW_LEN ; n++ )
	{									  
		prefix[n]  += ( (i32)(preSignal[n]) * FrameWindow[n] ) >> WINDOW_SCALE;	// Window up
	}

	pTxSignal = FillAFETxBuffI16 (pTxSignal, prefix, WINDOW_LEN);	// windowed part of sync field
	pTxSignal = FillAFETxBuffI16 (pTxSignal, preSignal+WINDOW_LEN, FFT_LEN/2-WINDOW_LEN); // rest of 1st half of sync
	JamDMAReloadRegs(TxStart, FFT_LEN);		// Load the AFE registers to start the transmit

	//---- Calculate the "postfix" to blend with the first data frame ----------------------
	//     The "prefix" calculated here will become "postfix" used in the first frame of data.
	for( n = 0; n < WINDOW_LEN ; n++ )
	{
		prefix[n]  = ( (i32)(preSignal[FFT_LEN/2 + n]) * (256-FrameWindow[n]) ) >> WINDOW_SCALE;	// Window down
	}

	//---- Finally, send a full frame with sync phase ----------------------------------
	TxStart = CalcTxStartAddr(pTxSignal, FFT_LEN);	// Find a free space in Tx buffer
	pTxSignal = FillAFETxBuffI16 (TxStart,   preSignal+FFT_LEN/2, FFT_LEN/2);	// 2nd half of inverted waveform
	pTxSignal = FillAFETxBuffI16 (pTxSignal, preSignal,           FFT_LEN/2);	// 1st half of inverted waveform
	JamDMAReloadRegs(TxStart, FFT_LEN);		// Load the AFE registers to start the transmit

	return pTxSignal;		// return pointer at current location
}


//==========================================================================================
// Function:		makeDataFrames()
//
// Description: 	Make the data frames and send them to the AFE.
//					Uses global buffer FFTArray, symbolArray and phaseEqArray.
//					Here phaseEqArray is used to hold the phase.
//
// 					windowing= (repeat of 1st part of previous frame) * (falling window function)
//			  				  + (1st part of cyclic prefix) * (rising window function)	
//
//					This function transmits data in several chunks.  Each chunk consists of
//					one or more calls to these functions:
//						CalcTxStartAddr - Finds location in txBuff big enough to hold chunk
//						FillAFETxBuffI16 - Fills values into tx buffer.
//						JamDMAReloadRegs - Jams the DMA registers to point to next chunk.
//						                   Handles timing to avoid conflicts.
//
// Return:			Pointer to the next position in txSignalArray
//
// Revision History:
//==========================================================================================
TXDATA* makeDataFrames(TXDATA* pTxSignal, i16 *postfix, u16 *pUserData )
{
	iCplx			*symbols;
	iCplx			*phase;
	TXDATA*			TxStart;

	u16				n, frame, block;
	i16				prevCarrier1;			// phase of previous 1st carrier
	i16				cumPhase;	  			// cumulative phase

	iCplx			*ifftBuf;				// working pointers for raised cos
	i16				prefix[CYCLIC_PREFIX_LEN];


	#if DEBUGIT == DEBUG_DISTANCE2
		diagArray = mxCreateDoubleMatrix(1, NUM_SYMBOLS*DATA_FRAMES_PER_BLOCK, mxREAL);
		mxSetName(diagArray, "txPhaseSyms");
		diag.r = mxGetPr(diagArray);
	#endif

	//---- work on the user data -----------------------
	appendParityCheckBytes(pUserData, (NUM_USER_BYTES>>1));
	//diagData(pUserData, DATA_BUFFER_LEN, "txData", diagU16);

	scramble(pUserData, DATA_BUFFER_LEN);// scramble the whole thing

	viterbiZero(pUserData);				// force end of buffer to zero
	//diagData(pUserData, DATA_BUFFER_LEN, "txScramble", diagU16);

	//---- encode and modulate the data ----------------
	viterbiEncodeInit(pUserData);
	prevCarrier1 = 0;

	#if SAVETRACE == TRUE
		SaveTraceData(0x5001);				// Put a marker in the trace buffer
		SaveTraceData((u16)recSignal);		// at the start of a data portion of 
		SaveTraceData((u16)pTxSignal);		// a transmit opperation
	#endif

	for( block = 0;  block < NUM_DATA_BLOCKS; block++ )
	{
		viterbiEncodeFrame(symbolArray);		// get data from user data buffer and put encoded 
		//if( block == 0 )				 		// result in real part of symbol buffer
		//	diagData(symbolArray, CARRIER_LEN, "txSymbols", diagICPLX);

		//---- build the data frames -----------------------
		for( frame = 0; frame < DATA_FRAMES_PER_BLOCK; frame++ )  
		{	
			symbols = symbolArray + (frame*SYMBOL_OFFSET);
			if( symbols >= (symbolArray + CARRIER_LEN) )
					symbols = symbols - CARRIER_LEN;
			phase = phaseEqArray;

			//---- assign phase based on 2-bit encoded data --------------------
			cumPhase  = prevCarrier1 + symbols->re;		// calc cumulative phase
			cumPhase &= 0x03;							// truncate to two bits
			prevCarrier1 = cumPhase;					// save this phase for the next frame	

			for(n = 0; n<CARRIER_LEN; n++ )				// start loop at 2nd carrier
			{
				#if DEBUGIT == DEBUG_DISTANCE2
					*diag.r++ = (double)(symbols->re);
				#endif

				phase->re = complexMap[cumPhase].re;	// map to complex quantity
				phase->im = complexMap[cumPhase].im;		
				phase++;
				
				symbols++;
				if( symbols >= (symbolArray + CARRIER_LEN) )
					symbols = symbolArray;

				cumPhase += symbols->re;				// calc cumulative phase
				cumPhase &= 0x03;						// truncate to two bits
			}	// end carrier loop		


			//---- put carriers in correct fft bins and call the ifft routine ----
			// We must shut off timer interrupts here to prevent the receive process from corrupting the FFT buffer
			INTR_DISABLE(TINT0); 	// Disable Timer0 Interrupt
 			fillCarriers( fftArray, phaseEqArray, DATA_IFFT_SCALE );
			cifft( (DATA *)(fftArray), FFT_LEN, IFFT_SCALE );

			//AdjustTxPeaks(fftArray, block, frame);	// Adjust the gain of the transmitted signal to maintain constant peaks.
			setTxPower(fftArray);		

			#ifdef RAMP_TEST
			{
				i16			sample;
				iCplx		*ifftBuf;

				ifftBuf = fftArray;
				for(sample = 0; sample < (FFT_LEN); sample++ )
				{
					(ifftBuf++)->re = ((sample-128)<<4)+(frame<<9)-(block<<10);
				}
			}
			#endif						

			//---- do windowing on the signal ----------------------------------------------
			ifftBuf = fftArray + (FFT_LEN - CYCLIC_PREFIX_LEN);	// Calc windowed part of prefix
			for( n = 0; n < WINDOW_LEN ; n++ )
			{
				prefix[n] = ( (i32)((ifftBuf++)->re) * FrameWindow[n] ) >> WINDOW_SCALE;
				prefix[n] += postfix[n];
			}
			for( n = WINDOW_LEN; n < CYCLIC_PREFIX_LEN ; n++ )	// Copy remaining part of prefix
			{
				prefix[n] = (ifftBuf++)->re;
			}
			ifftBuf = fftArray;									// Get overlap for next frame
			for( n = 0; n < WINDOW_LEN ; n++ )
			{
				postfix[n] = ( (i32)((ifftBuf++)->re) * (256-FrameWindow[n]) ) >> WINDOW_SCALE;
			}

			//---- construct the cyclic prefix from the last chunk of the data frame-----
			TxStart = CalcTxStartAddr(pTxSignal, FFT_LEN + CYCLIC_PREFIX_LEN);	// Find a free space in Tx buffer
			pTxSignal = FillAFETxBuffI16 (TxStart, prefix, CYCLIC_PREFIX_LEN);

			//---- construct the data frame ---------------
			pTxSignal = FillAFETxBuffCplx (pTxSignal, fftArray, FFT_LEN );

			JamDMAReloadRegs(TxStart, FFT_LEN + CYCLIC_PREFIX_LEN);
			INTR_ENABLE(TINT0); 			// Re-Enable Timer0 Interrupt	//???

		}	// end frame loop
	}	// end block loop

	prevSnrSample = ReadRxDMAPointer() - recSignalArray;	// Let SNR calc re-start here					

	//---- append postfix to end of packet ---------------
	TxStart = CalcTxStartAddr(pTxSignal, WINDOW_LEN);	// Find a free space in Tx buffer
	pTxSignal = FillAFETxBuffI16 (TxStart, postfix, WINDOW_LEN);
	JamDMAReloadRegs(TxStart, WINDOW_LEN);

	//--- Set the DMA to switch back to idle pattern when the last block finishes -------
	JamDMAReloadRegs((TXDATA*)IdleBuffArray, IDLE_BUFFER_LEN);
	

	#if DEBUGIT == DEBUG_DISTANCE2
		mexPutArray(diagArray, "caller");
	#endif
	
	#if SAVETRACE == TRUE
		SaveTraceData(0x5002);				// Put a marker in the trace buffer
		SaveTraceData((u16)recSignal);		// at the end of a transmit opperation
		SaveTraceData((u16)pTxSignal);		// 
	#endif

	return pTxSignal;
}


//==========================================================================================
// Function:		getPrePhases()
//
// Description: 	Calculate symbols to fill carriers for preamble part of packet.
//					Non-differential phase is passed in *preamble.
// Return:			Double-summed phase is returned in *phase.
//
// Revision History:
//==========================================================================================
void getPrePhases( iCplx *phase, const i16 *preamble )
{
	i16			n;
	i16			cPhase  = 0;
	i16			ccPhase = 0;

	for( n = 0; n < CARRIER_LEN;  n++ )
	{
	    cPhase  += *preamble++;		// 1st cumulative sum
	    ccPhase += cPhase;			// 2nd cum sum
	    ccPhase &= 3;				// mod 4

		phase->re = complexMap[ccPhase].re;	// map to complex quantity
		phase->im = complexMap[ccPhase].im;		
		phase++;
	}
	
	return;
}


//==========================================================================================
// Function:		getSyncPhases()
//
// Description: 	Calculate symbols to fill carriers for sync frame part of packet.
//					Non-differential phase is passed in *preamble.
// Return:			Double-summed phase is returned in *phase.
//
// Revision History:
//==========================================================================================
void getSyncPhases( iCplx *phase, const i16 *preamble )
{
	i16			n;
	i16			sync;
	i16			cPhase  = 0;
	i16			ccPhase = 0;

	static const i16	syncTable[] = {2, 3, 0, 1};

	for( n = 0; n < CARRIER_LEN;  n++ )
	{
		sync = syncTable[*preamble++];
	    cPhase  += sync;			// 1st cumulative sum
	    ccPhase += cPhase;			// 2nd cum sum
	    ccPhase &= 3;				// mod 4

		phase->re = complexMap[ccPhase].re;	// map to complex quantity
		phase->im = complexMap[ccPhase].im;		
		phase++;
	}
	
	return;
}
