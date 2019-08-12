//==========================================================================================
// Filename:		ofdm_agc.c
//
// Description:		Functions related to the AGC action and preamble detection in 
//					an OFDM power line modem.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"


//====================================================================
//	Global var for this module
//=====================================================================
#if COMPILE_MODE == MEX_COMPILE
	u16	  		fakeInteruptCount;		// used to generate a sample_interupt

	u16			pgaGainReg;
#endif


#if COMPILE_MODE == MEX_COMPILE
//==========================================================================================
// Function:		fakeCodecAndDMA()
//
// Description: 	Model getting an ADC sample into the recSignal Buffer.
//
// Revision History:
//==========================================================================================
i16 fakeCodecAndDMA( double **agcrec, double **rec, i16 pgaGainReg )
{
	i32				adcSample;
	i16				overflow = 0;

	//---- amplify current sample ---------------------------
	//		ctrl	gain_dB	gain
	//		 0		 0		 1.0
	//		 1		 3		 1.4
	//		...		...		...
	//		10		30		31.6
	
	//---- get data from Matlab, apply PGA gain to data -------
	#if !defined(DEBUG_ARBWAVE)
		adcSample = (i32)((**rec) * AFE_RXPGA[pgaGainReg]);		// 32bit value
		(*rec)++;
	
		if( adcSample >= ADC_CLIP_LEVEL )	// model clipping in channel ADC
		{
			adcSample = ADC_CLIP_LEVEL-1;
			overflow = 1;
		}
		if( adcSample < -ADC_CLIP_LEVEL )
		{
			adcSample = -ADC_CLIP_LEVEL;
			overflow = 1;
		}

	#else
		adcSample = **rec;					// <=== DEBUG accept data from bench 
		(*rec)++;
	#endif

	if( rxDMApointer == NULL )
		rxDMApointer = recSignalArray;

	*rxDMApointer++ = (i16)adcSample;		// put it in the rx buffer
	
	if( rxDMApointer >= recSignalArray + RX_CIRC_BUFFER_LEN )
		rxDMApointer = recSignalArray;

	if( *agcrec != NULL )
	{
		**agcrec = (double)adcSample;		// put data in Matlab output array 
		(*agcrec)++;
	}

	fakeInteruptCount++;					// used to generate a sample_interupt

	return overflow;
}
#endif


//==========================================================================================
// Function:		sample_interupt()
//
// Description: 	Main sample interrupt routine
//					Called by the DMA engine about every 16 samples.
//					Calculates the AGC control register value and checks AGC related flags.
//
// Modifies:		SNRflag, recSignal, AFEctrl2.
//
// Revision History:
//==========================================================================================
void sample_interupt()
{
	i16					sample;							// compare this to recSignal
	i16*				dmaPtr;							// compare this to recSignal

	i16					adcSample;
	i16					pk;								// indicates thrs crossings
	static i16			agcGaindB = AGC_INIT_GAIN;		// gain ctrl state var
	i16					pgaGain;

	// Fast decay gain (signal is way too high)-------------|
	// Decay gains (signal too high, decr gain)--------|	|
	// Attach gains ( signal too low, incr gain)---|   |	|
	//											   v   v	v
	static const i16	AgcAttackGains[3] =      { 9, -65, -207 };	
//	static const i16	AgcAttackGains[3] =      { 9, -65, -157 };	

#define  AGC_THRS_LO		0x3000		// 37.5% of full-scale
#define  AGC_THRS_HI		0x5000		// 62.5% of full-scale


	if( agcState != AgcHold )
	{
		dmaPtr = ReadRxDMAPointer();					// Read current Rx DMA location.
		DebugDelay();									// Flush C54x if in debug mode.

		// If DMA pointer has wrapped around, ignore little piece at end of buffer.  
		if( recSignal > dmaPtr )						 
			recSignal = recSignalArray;					// Restart at beginning of buffer.

 		#if (AFE_RX_FORMAT == 24) 			
 			// Align the pointer so that we are looking at the real data, not the blank value between them
			if ( ((u16)recSignal & 1) != 0)			// Look for even numbers
			{
				DebugDelay();						// Flush C54x if in debug mode.
				PostErrorCode(0xBADD, "sample_interrupt", "agc.c", "Odd value for recSignal");
				TurnLEDsOn(LED_RX_BUFF_ERR);	// Turn on red LED2
				#if SAVETRACE == TRUE
					SaveTraceData(0xBADD);			//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData((u16)recSignal); 	//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData((u16)dmaPtr);		//!!!DEBUG  Put a marker in the trace buffer
				#endif
				DebugDelay();				// Flush C54x if in debug mode.
			}
			else
			{
				TurnLEDsOff(LED_RX_BUFF_ERR); 	//Turn off red LED2
			}

 			// Align the pointer so that we are looking at the real data, not the blank value between them
			if ( *(i16*)( ((u16)recSignal&0xFFFE) | 1) != 0 )
			{
				DebugDelay();				// Flush C54x if in debug mode.
				PostErrorCode(0xBADE, "sample_interrupt", "agc.c", "Odd/Even misalignment in recSignal buffer");
				#if SAVETRACE == TRUE
					SaveTraceData(0xBADE);			//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData((u16)recSignal); 	//!!!DEBUG  Put a marker in the trace buffer
					SaveTraceData((u16)dmaPtr);		//!!!DEBUG  Put a marker in the trace buffer
				#endif
				DebugDelay();				// Flush C54x if in debug mode.
			}
		#endif

		while( ((u32)recSignal>>1) < ((u32)dmaPtr>>1) )					// AGC update
		{
			adcSample = abs(*recSignal++);			// compare sample
			#if (AFE_RX_FORMAT == 24)
				recSignal++;						// Increment rec pointer to skip blank value
			#endif

			if( ( agcState == AgcIdle ) || (agcState == AgcPreamble) )
			{
				pk =  adcSample >= AGC_THRS_LO;			// compare sample to AGC thresholds
				pk += adcSample >= AGC_THRS_HI;
				agcGaindB += AgcAttackGains[pk];		// update gain
			}
		}			

		#if DEBUGIT == DEBUG_AGC
			*diag.r++ = (double)ReadRxDMAPointer() - recSignalArray;
			*diag.r++ = (double)agcGaindB;
		#endif

		//---- decimate and saturate gain value -------------
		pgaGain  = (agcGaindB + AGC_CTRL_HALF) >> AGC_CTRL_SHIFT;
		if( pgaGain > AGC_CTRL_MAX )
		{
			pgaGain = AGC_CTRL_MAX;
			agcGaindB = pgaGain << AGC_CTRL_SHIFT;
		}
		if( pgaGain < AGC_CTRL_MIN )
		{
			pgaGain = AGC_CTRL_MIN;
			agcGaindB = pgaGain << AGC_CTRL_SHIFT;
		}

		#if COMPILE_MODE == MEX_COMPILE		
			if( pgaGain != pgaGainReg )
			{
				pgaGainReg = pgaGain;
				//mexPrintf("PGA gain set to %d\n",pgaGain);
			}
		#else
			// Calculate new value for AFE control variable
			AFEctrl2 = (AFEctrl2 & (~((0x07)<<3))) | (pgaGain<<3);	// Stuff Rx Gain into hardware control variable
		#endif


		#if DEBUGIT == DEBUG_AGC
			*diag.r++ = (double)pgaGainReg;
		#endif

		if (SNRflag >= 0)	// If SNRflag < 0, then we should not look for packet.
		{
			//---- count to see if time to do SNR calc -----------
			sample = (recSignal - recSignalArray) - prevSnrSample;
			if( sample < 0 )
			{
				sample += (i16)RX_CIRC_BUFFER_LEN;
			}
			
			#if COMPILE_MODE == DSP_COMPILE		
				DebugDelay();				// Flush C54x if in debug mode.
				if(sample >= (SNR_FFT_INTERVAL*RX_SRC_INC*6))	
				{
					// Something's wrong here
					DebugDelay();				// Flush C54x if in debug mode.
					PostErrorCode(0xBADB, "sample_interrupt", "agc.c", "Long delay between SNR Flags");
					#if SAVETRACE == TRUE
						dmaPtr = ReadRxDMAPointer();	// Read current Rx DMA location.
						SaveTraceData(0xBADB);			// trace marker
						SaveTraceData((u16)recSignal); 	// our pointer 
						SaveTraceData((u16)dmaPtr);		// dma pointer

						SaveTraceData(prevSnrSample);	// prevSnrSample is global i16 var
						SaveTraceData(sample); 			// time from last packet det
						SaveTraceData(SNRflag);			// packet detect flag
					#endif
					
					DebugDelay();				// Flush C54x if in debug mode.
					DebugDelay();				// Flush C54x if in debug mode.
				}
			#endif
			
			if( sample >= (SNR_FFT_INTERVAL*RX_SRC_INC) )
			{
				prevSnrSample  = (recSignal - recSignalArray);
				SNRflag = 1;
			}
		}
	}	// done with AGC

	return;
}	


#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		lookForPacket()
//
// Description: 	Look for the start of a packet by comparing power in bins near the top
//					of the transmission band to bins just beyond the transmission band.
//
// Modifies:		SNRflag, agcState
//
// Revision History:
//==========================================================================================
i16* lookForPacket(void)
{
	u16				phError = 0;
	
	TurnLEDsOn(LED_RX_SEARCH);			// Turn on green LED

	backFFT( fftArray, recSignal );		// grab the last FFT_LEN samples and get freq resp

	phError = ddphase(PreambleArray, fftArray + CARRIER_LOW, CARRIER_LEN );

	if( phError  > DDPHASE_HI_LIM  )   	//	decide if this lookes like a packet
	{									//	if so, switch to preamble state
		preambleDetCount = 0;			//	start over
		agcState = AgcIdle;
	}
	else
	{
		if( phError <  DDPHASE_LO_LIM )
		{
			preambleDetCount++;		  	//	and count # of good preamble snrs seen
			agcState = AgcPreamble;
		}
	}

	if( preambleDetCount >= SNR_PRE_DET_COUNT )
	{
		agcState = AgcHold;				// We have seen enough good preamble, so switch modes.
		// The last frame must have been good, so back pointer up one frame and 
		// include it in the equalization data
		// recSignal = WrapRecPtr(recSignal, -FFT_LEN*RX_SRC_INC);
	}

	#if SAVETRACE == TRUE
		//if( ((AFEctrl2>>3) & 7) < 7)	// Save when AGC is not maxed out
		if( preambleDetCount > 0 )		// Save when we think we see a packet
		{								// store 0x8000 + pgaGain + agcState  + preambDetCount			
			SaveTraceData(0x8000 + (((AFEctrl2>>3) & 7)<<8)  + ((u16)agcState<<4) + preambleDetCount);	 
			SaveTraceData((u16)recSignal);
			SaveTraceData(phError);
		}
	#endif

	TurnLEDsOff(LED_RX_SEARCH);	// Turn off green LED

    return recSignal;
}

#endif


#if COMPILE_MODE == MEX_COMPILE
//==========================================================================================
// Function:		lookForPacket()
//
// Description: 	Look for the start of a packet by comparing power in bins near the top
//					of the transmission band to bins just beyond the transmission band.
//
// Modifies:		SNRflag, agcState
//
// Revision History:
//==========================================================================================
i16 *lookForPacket( dCplxPtr recAgc,  dCplxPtr rec )
{
	i16				sampleCount = 0;
	iCplx			*fftBuf;

	i16				phError = 0;
	i16				preambleDetCount = 0;

	i16				overflow = 0;

	
	//---- set up debug buffer ----------------------------------------
	#if DEBUGIT == DEBUG_AGC
		diagArray = mxCreateDoubleMatrix(3, rec.len, mxREAL);
		mxSetName(diagArray, "agcCtrl");
		diag.r = mxGetPr(diagArray);
	#endif
	#if ( (DEBUGIT == DEBUG_SNR) || (DEBUGIT == DEBUG_DISTANCE2) )
		diagArray = mxCreateDoubleMatrix(4, rec.len, mxREAL);
		mxSetName(diagArray, "snrDiag");
		diag.r = mxGetPr(diagArray);
	#endif

	memset(recSignalArray, 0x80, RX_CIRC_BUFFER_LEN);		// see where we are 

	//---- main AGC loop ----------------------------------------------
	fakeInteruptCount = 0;
	agcState = AgcIdle;
	pgaGainReg = 0;

	//---- get pointer to rx buffer ---------------------------
	fakeCodecAndDMA( &recAgc.r, &rec.r,  pgaGainReg );
	sampleCount++;
	recSignal = ReadRxDMAPointer();			// init global var recSignal

	//---- loop through the received data -----------------------
	while( agcState != AgcHold )
	{
		//---- model getting ADC sample ---------------------------
		fakeCodecAndDMA( &recAgc.r, &rec.r,  pgaGainReg );
		sampleCount++;

		if( fakeInteruptCount > AGC_INTERVAL )
		{
			fakeInteruptCount = 0;
			sample_interupt();				// update agc gain using recSignal
		}
		
		if( SNRflag == 1 )
		{
			SNRflag = 0;

			backFFT( fftArray, recSignal );		// grab the last FFT_LEN samples and get freq resp

			fftBuf  = fftArray + CARRIER_LOW;
			phError = ddphase(PreambleArray, fftBuf, CARRIER_LEN );

			#if DEBUGIT == DEBUG_SNR
			{
				static i16	cnt = 0;
				char	vnameStr[20];
				sprintf(vnameStr,"ddDet%d",cnt);
				diagData(fftArray, FFT_LEN, vnameStr, diagICPLX);
				mexPrintf("%d: pre phaseError %d\n", cnt++, phError );
			}
			#endif

			if( phError  > DDPHASE_HI_LIM  )	//	decide if this lookes like a packet
			{									//	if so, switch to preamble state
				preambleDetCount = 0;			//	start over
				agcState = AgcIdle;
			}
			else
			{
				if( phError <  DDPHASE_LO_LIM )
				{
					preambleDetCount++;		  	//	and count # of good preamble snrs seen
					agcState = AgcPreamble;
				}
			}

			if( preambleDetCount >= SNR_PRE_DET_COUNT )
				agcState = AgcHold;
			
		}	// end of fft processing

		#if ( (DEBUGIT == DEBUG_SNR) || (DEBUGIT == DEBUG_DISTANCE2) )
			*diag.r++ = (double)(*(rxDMApointer-1));
			*diag.r++ = (double)preambleDetCount;
			*diag.r++ = (double)pgaGainReg;
			*diag.r++ = (double)phError;
		#endif

		if( sampleCount > (rec.len-1) )
		{
			#if ( (DEBUGIT == DEBUG_SNR) || (DEBUGIT == DEBUG_DISTANCE2) )
				mexPutArray(diagArray, "caller");
			#endif
			postError("Failed to find preamble.");
		}

	}		// end of while loop
		
	//mexPrintf( "Packet detected at sample count %d\n", sampleCount );

	//---- finish out the waveform for Matlab --------------
	for( ; sampleCount < rec.len ; sampleCount++ )
	{
		overflow += fakeCodecAndDMA( &recAgc.r, &rec.r,  pgaGainReg );
		if( overflow > AGC_OVERFLOW_COUNT )
		{
			diagData(recSignalArray, RX_CIRC_BUFFER_LEN, "rxArray", diagI16);
			postError("Too many overflowed samples");
		}
	}

	#if DEBUGIT == DEBUG_AGC
		mexPutArray(diagArray, "caller");
	#endif
	#if ( (DEBUGIT == DEBUG_SNR) || (DEBUGIT == DEBUG_DISTANCE2) )
		mexPutArray(diagArray, "caller");
	#endif
	
   return recSignal;
}
#endif


//==========================================================================================
// Function:		ddphase()
//
// Description: 	Calculate the double difference phase from the carriers passed.
//
//		    dphase = carrier2-carrier1,  carrier3-carrier2, ...  
//		 
//		                    y2           y1             x1*y2 - y1*x2 
//		    dphase1 = atan(----) - atan(----)  =  atan(---------------)
//		                    x2           x1             x1*x2 + y1*y2
//		 
//		    ddphase = dphase2-dphase1,  dphase3-dphase2, ...
//		 
//		                     x2*y3 - y2*x3 	         x1*y2 - y1*x2 
//		    ddphase1 = atan(---------------) - atan(---------------)
//		                     x2*x3 + y2*y3		     x1*x2 + y1*y2
//		 
//		 					 ( x1*x2 +y1*y2)*( x2*y3 -y2*x3) - ( x1*y2 -y1*x2)*( x2*x3 +y2*y3)
//		    ddphase1 = atan(-------------------------------------------------------------------)
//		 					 ( x1*x2 +y1*y2)*( x2*x3 +y2*y3) + ( x1*y2 -y1*x2)*( x2*y3 -y2*x3)
//		 					 y  
//		             = atan(---)
//		 					 x  
//		    y = ( x1*x2 +y1*y2)*( x2*y3 -y2*x3) 
//		      - ( x1*y2 -y1*x2)*( x2*x3 +y2*y3) 
//		 
//		    x = ( x1*x2 +y1*y2)*( x2*x3 +y2*y3) 
//		      + ( x1*y2 -y1*x2)*( x2*y3 -y2*x3)
//		   
//		  then convenient groupings are:
//		    pc = [ (x1*x2 +y1*y2)  (x1*y2 -y1*x2) ]     // previous coefficients
//		    cc = [ (x2*x3 +y2*y3)  (x2*y3 -y2*x3) ]     // current coefficients
//		    y  = pc1*cc2 - pc2*cc1;
//		    x  = pc1*cc1 + pc2*cc2;
//		    b1 =  -y > x;
//		    b0 =  |y|>|x|;
//			
//		  c-style notation:
//		    pc = [ (x0*x1 +y0*y1)  (x0*y1 -y0*x1) ]     // previous coefficients
//		    cc = [ (x1*x2 +y1*y2)  (x1*y2 -y1*x2) ]     // current coefficients
//		    y  = pc0*cc1 - pc1*cc0;
//		    x  = pc0*cc0 + pc1*cc1;
//		    b1 =  -y > x;
//		    b0 =  |y|>|x|;
//
//
// Revision History:
//==========================================================================================
#define MSHIFT		8
u16 ddphase(const i16 *preamble, iCplx *carriers, i16 carrierLen )
{
	i16				n;
	i16				c0r, c0i, c1r, c1i;
	i16				pc0, pc1, cc0, cc1;
	i32				x,   y;
	i16				b1,  b0;
	
	i16				ddph;
	i16				phError = 0;
	
	static const u16 ddphaseDistance[16] = 
				{	0, 1, 2, 1, 
					1, 0, 1, 2,
					2, 1, 0, 1,
					1, 2, 1, 0 
				};

#define	DEBUG_MSHIFT	FALSE	// Set TRUE to enable diagnostics
	#if (DEBUG_MSHIFT == TRUE)
		i32				xMax=0;
		i32				yMax = 0;
		i16				cc0Max = 0;
		i16				cc1Max = 0;
	#endif

	#if DEBUGIT == DEBUG_DDPH
		diagArray = mxCreateDoubleMatrix(4, carrierLen, mxREAL);
		mxSetName(diagArray, "ddphDiag");
		diag.r = mxGetPr(diagArray);
	#endif

	preamble += 2;				// ignoring first two symbols
	
	c0r = carriers->re;			// carrier 0
	c0i = carriers->im;
	carriers++;
	c1r = carriers->re;			// carrier 1
	c1i = carriers->im;
	carriers++;

	cc0 = ( (i32)c0r * (i32)c1r + (i32)c0i * (i32)c1i ) >> MSHIFT;
	cc1 = ( (i32)c0r * (i32)c1i - (i32)c0i * (i32)c1r ) >> MSHIFT;

	//----calc double differenece phase --------------------------------
	for( n = 2;  n < carrierLen;  n++ )
	{
		c0r = c1r;
		c0i = c1i;
		c1r = carriers->re;		// carriers 2 to end
		c1i = carriers->im;
		carriers++;

		pc0 = cc0;				// do the trig math
		pc1 = cc1;
		cc0 = ( (i32)c0r * (i32)c1r + (i32)c0i * (i32)c1i ) >> MSHIFT;
		cc1 = ( (i32)c0r * (i32)c1i - (i32)c0i * (i32)c1r ) >> MSHIFT;

		y	= (i32)pc0*(i32)cc1 - (i32)pc1*(i32)cc0;
		x	= (i32)pc0*(i32)cc0 + (i32)pc1*(i32)cc1;
	    
	    b1 = -y > x;
	    b0 = labs(y) > labs(x);
	    //ddph = b1*2 + b0;		// here is the double-difference phase
		ddph = b1*8 + b0*4;		// here is the double diff phase * 4

		phError += ddphaseDistance[ddph + *preamble++];  // get sum of distances as phase error

		#if DEBUGIT == DEBUG_DDPH
			*diag.r++ = (double)cc0;
			*diag.r++ = (double)cc1;
			*diag.r++ = (double)y;
			*diag.r++ = (double)x;
		#endif

		#if (DEBUG_MSHIFT == TRUE)
			if (abs(cc0) > cc0Max)
			{
				cc0Max = abs(cc0);
			}
			if (abs(cc1) > cc1Max)
			{
				cc1Max = abs(cc1);
			}
			if (labs(x) > xMax)
			{
				xMax = labs(x);
			}
			if (labs(y) > yMax)
			{
				yMax = labs(y);
			}
		#endif

	}

	#if DEBUGIT == DEBUG_DDPH
		mexPutArray(diagArray, "caller");
	#endif

		
	#if (DEBUG_MSHIFT == TRUE)
		#if SAVETRACE == TRUE
			SaveTraceData(0x9999);
			SaveTraceData((u16)cc0Max);
			SaveTraceData((u16)cc1Max);
			SaveTraceData(0x0000);
			SaveTraceData((u16)xMax);
			SaveTraceData((u16)yMax);
		#endif
	#endif
	
	return phError;
}

