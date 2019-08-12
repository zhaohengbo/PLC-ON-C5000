/*================================================================================
Filename:			viterbi.c

 Description:		Mex function call for data transmittion part of model of  
					power line modem.

 Copyright (C) 2002 - 2003 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
========================================================================*/
#include "ofdm_modem.h"


/*==============================================================
	global vars for this module
===============================================================*/
struct 
{
	u16				*pUserData;
	u16				encodeState;
	u16				inWord;
	i16				bitCount;
}	viterbiEnc;

struct 
{
	u32				*pState0Memory;
	u16				*pUserData;
	u16				outWord;
	i16				outWordBitCount;
	i16				symbolCount;
	iCplx			prevCarrier1;
}	pathMem;

MetricType		pathMetricA[VITERBI_NUM_STATES];
MetricType		pathMetricB[VITERBI_NUM_STATES];
u32				pathMemoryA[VITERBI_NUM_STATES];
u32				pathMemoryB[VITERBI_NUM_STATES];



/*==============================================================
	Init the global structure of state variables for the Viterbi Encoder
===============================================================*/
void viterbiEncodeInit(u16 *pUserData)
{
	viterbiEnc.encodeState = 0;
	viterbiEnc.pUserData = pUserData;
	viterbiEnc.inWord = *viterbiEnc.pUserData++;
	viterbiEnc.bitCount = WORD_LEN; 
	return;
}


/*==============================================================
	Viterbi Encoder for PLC modem
	Uses a K=7 convolutional code with two outputs for each bit input.  
	The two bit outputs returned in the real part of array symbols.
	
            +---+---+---+-----------+---y1
            |   |   |   |           |
	inbits -->D-->D-->D-->D-->D-->D-- 
           0|  1   2|  3|  4   5|  6|
            |       |   |       |   |
            +-------+---+-------+---+---y0

	The function returns pointer to the next user data bits.
===============================================================*/
void viterbiEncodeFrame(iCplx *symbols)
{
	i16				symCount = 0;
	u16				bit;

	for( symCount = 0; symCount < CARRIER_LEN; symCount++ )
	{
		bit = (viterbiEnc.inWord & WORD_MSB) >> WORD_LEN-VITERBI_K;		// shift >> 9
		viterbiEnc.inWord <<= 1;
		viterbiEnc.encodeState = bit + (viterbiEnc.encodeState >> 1 );
		(symbols++)->re = ViterbiEncodeLookup[viterbiEnc.encodeState];

		viterbiEnc.bitCount--;
		if( viterbiEnc.bitCount <= 0 )
		{
			viterbiEnc.inWord = *viterbiEnc.pUserData++;
			viterbiEnc.bitCount = WORD_LEN;
		}
	}
	return;
}


/*====================================================================
	init path memory and path metric structure for Soft Viterbi trellis
=====================================================================*/
void initPathMemory( u16 *pUserData )
{
	i16				state;						// trellis state
	MetricType		*currPathMetric;
	MetricType		*prevPathMetric;
	u32				*currPathMemory;
	u32				*prevPathMemory;

	//---- reset path metric and memory -----------------------
	currPathMetric = pathMetricA;
	prevPathMetric = pathMetricB;
	currPathMemory = pathMemoryA;
	prevPathMemory = pathMemoryB;
	for( state = 0; state < VITERBI_NUM_STATES;  state++ )
	{
		*currPathMetric++ = METRIC_OFFSET;
		*prevPathMetric++ = METRIC_OFFSET;
		*currPathMemory++ = 0;
		*prevPathMemory++ = 0;
	}
	
	//---- initialize Viterbi decode state variables -----------------------
	pathMem.pUserData = pUserData;
	pathMem.pState0Memory = NULL;
	pathMem.outWord = 0;
	pathMem.outWordBitCount = -(VITERBI_MEM_LEN+VITERBI_K-2);
	pathMem.symbolCount = 0;
	//pathMem.prevCarrier1.re = VITERBI_CAR1_RE;	// init for first carrier
	//pathMem.prevCarrier1.im = VITERBI_CAR1_IM;

	return;
}


/*====================================================================
	State Loop
=====================================================================*/
u32 *viterbiStateLoop( MetricType *dist, i16 symbolCount )
{

	i16					state;						// trellis state
	MetricType			metric0, metric1;			// working path metric
	MetricType			d0, d1;						//
	MetricType			*d0Ptr, *d1Ptr;
	static MetricType	distance[4];				//
	const i16			*decodePtr;
	
	MetricType			*currPathMetric;
	MetricType			*prevPathMetric;

	u32					*currPathMemory;
	u32					*prevPathMemory;
	u32 				*state0Memory;

			
	if( symbolCount & 0x01 )	// alternate, odd/even,  between the
	{							// two path memory buffers
		state0Memory   = pathMemoryB;
		currPathMemory = pathMemoryB;
		prevPathMemory = pathMemoryA;

		currPathMetric = pathMetricB;
		prevPathMetric = pathMetricA;
	}
	else
	{
		state0Memory   = pathMemoryA;
		currPathMemory = pathMemoryA;
		prevPathMemory = pathMemoryB;

		currPathMetric = pathMetricA;
		prevPathMetric = pathMetricB;
	}

	//---- retrieve the distance metrics------------------------------------
	distance[0] = *dist;
	distance[2] = -(*dist++);
	distance[1] = *dist;
	distance[3] = -(*dist);

	#if DEBUGIT == DEBUG_DISTANCE
		*diag.r++ = (double)distance[0];
		*diag.r++ = (double)distance[1];
	#endif

	decodePtr = decodeTable;	// reset the table that chooses the correct
								// distance metric for each branch

	/*--------------------------------------------------------------------
	Now cycle through each state in the trellis.  For each state select the 
	best branch from a previous state based on the distance metric. 
	Two previous states and two current states share branches and so are done
	at the same time.  Assign the cumulative metric and memory from the winning
	previous state to the current state.
	---------------------------------------------------------------------*/
	for( state = 0; state < VITERBI_NUM_STATES/2;  state++ )
	{
		//---- Select the proper 2 of 4 distance measurements to use for this state ------
		// Goofy layout here opimtizes processor pipeline
		d0Ptr = distance + *decodePtr++;
		d1Ptr = distance + *decodePtr++;
		d0 = *d0Ptr;
		d1 = *d1Ptr;

		//---- get the path metrics for each branch into the state ---------
		metric0 = (*prevPathMetric++) + d0;		// PM0 + d0
		metric1 = (*prevPathMetric--) + d1;		// PM1 + d1

		if( metric0 > metric1 )
		{
			(*currPathMetric) = metric0;	// save the winning path metric for this node
			(*currPathMemory) = (*prevPathMemory) << 1;	
		}
		else
		{
			(*currPathMetric) = metric1;	
			(*currPathMemory) = ((*(prevPathMemory+1)) << 1) + 1;
		}
		currPathMetric += VITERBI_NUM_STATES/2;
		currPathMemory += VITERBI_NUM_STATES/2;


		metric0 = (*prevPathMetric++) + d1;		// PM0 + d1
		metric1 = (*prevPathMetric++) + d0;		// PM1 + d0

		if( metric0 > metric1 )
		{
			(*currPathMetric) = metric0;	// save the winning path metric for this node
			(*currPathMemory) = (*prevPathMemory) << 1;	
		}
		else
		{
			(*currPathMetric) = metric1;	
			(*currPathMemory) = ((*(prevPathMemory+1)) << 1) + 1;	
		}
		currPathMetric -= (VITERBI_NUM_STATES/2 - 1);
		currPathMemory -= (VITERBI_NUM_STATES/2 - 1);

		prevPathMemory += 2;

	}
			
 	return(state0Memory);
}
			

/*====================================================================
	Soft Viterbi decoding algorithm
=====================================================================*/
void viterbiDecodeFrame( MetricType *distance, u16 numCarriers )
{
	i16				ncar;
	i16				state;						// trellis state
  	MetricType		pathMetric0;
 	#if METRIC_OFFSET >= 0
		MetricType		acc;
	#endif

	//---- loop thourgh the subcarriers ---------------------------------
	for( ncar = 0; ncar < numCarriers; ncar++ )
	{		
		pathMem.pState0Memory = viterbiStateLoop(distance, pathMem.symbolCount);
		distance += 2;				// get next pair of distance metrics 

		/*--------------------------------------------------------------------
		As the 32 bit path memmory (currPathMemory) fills up, we need to catch the
		data before it falls off the MSB end.  Since we are loading the path memory
		from the bits that fall off the end of the (virtual) state shift register, 
		we can ignore the first K bits that are loaded into the path memory.  So,
		the user data starts at VITERBI_MEM_LEN+VITERBI_K symbols from when we
		started the decoder.  This is accomplished by presetting the variable 
		outWordBitCount = VITERBI_MEM_LEN+VITERBI_K-2.  
		---------------------------------------------------------------------*/
		pathMem.outWord <<= 1;					// shift in a 1, else shift in a zero
		if( *pathMem.pState0Memory & VITERBI_MSB )
			pathMem.outWord |= 1;	

		pathMem.outWordBitCount++;
		if( pathMem.outWordBitCount > WORD_LEN-1 )	// if we've collected a whole byte
		{									// put it in the buffer
			*pathMem.pUserData++ = pathMem.outWord;			// but a word in the user data Buffer
			pathMem.outWordBitCount = 0;
		}

		pathMem.symbolCount++;

		#if DEBUGIT == DEBUG_VITERBI_STATES
		{
			MetricType		*currPathMetric;
			u32				*currPathMemory;

			if( pathMem.symbolCount & 0x01 )	// alternate, odd/even,  between the
			{
				currPathMetric = pathMetricB;
				currPathMemory = pathMemoryB;
			}
			else
			{
				currPathMetric = pathMetricA;
				currPathMemory = pathMemoryA;
			}
			for(state = 0; state < VITERBI_NUM_STATES; state++ )
			{
				*diag.r++ = (double)currPathMetric[state];
				*diag.r++ = (double)currPathMemory[state];
			}
		}
		#endif
	}	// ncar
	
	// All metrics are roughly equal, but keep growing from frame to frame.
	// Subtract a common amount from all metrics to keep them from rolling over
	// the 16-bit variable.
	#if (METRIC_SIZE == 16) 
	{ 
		#if METRIC_OFFSET < 0
 			pathMetric0 = pathMetricA[0];
		#else
			acc = METRIC_OFFSET - pathMetricA[0];
		#endif

		for( state = 0; state < VITERBI_NUM_STATES;  state++ )
		{
			
			#if METRIC_OFFSET < 0
				pathMetricA[state] -= pathMetric0;
				pathMetricA[state] += METRIC_OFFSET;
				pathMetricB[state] -= pathMetric0;
				pathMetricB[state] += METRIC_OFFSET;
			#else
				pathMetricA[state] += acc;
				pathMetricB[state] += acc;
			#endif
		}

	}
	#endif

	return;
}


/*====================================================================
	Flush the path memory by processing a set of metric values
	representing zeros through the algorithm 
=====================================================================*/
void flushPathMemoryV( void )
{
#define	ZERODIST0		100
#define	ZERODIST1		0

	i16				ncar;
	MetricType 		*distance;

	#if CARRIER_LEN < VITERBI_MEM_LEN
		#error 	  flushPathMemoryV does not support CARRIER_LEN less than VITERBI_MEM_LEN.
	#endif

	distance = distanceArray;
	for( ncar = 0; ncar < CARRIER_LEN; ncar++ )
	{
		*distance++ = ZERODIST0;
		*distance++ = ZERODIST1;
	}

	//---- loop thourgh the subcarriers ---------------------------------
	viterbiDecodeFrame( distanceArray, VITERBI_MEM_LEN );
	
	return;
}


/*====================================================================
	Shift out remaining bits in the path memory.
	Note that this routine assumes that at least VITERBI_MEM_LEN+VITERBI_K 
	bits have been processed.
=====================================================================*/
void flushPathMemory( void )
{
	i16				nbit;
	
	//u16				outWord = 0;
	//i16				outWordBitCount = -(VITERBI_MEM_LEN+VITERBI_K-2);


	//mexPrintf("flush path memory data pointer: %d\n", pathMem.pUserData - rxUserDataArray );

	/*------------------------------------------------------------------------
	shift off the rest of the path memory except the last byte this was tacked
	on by the encoder to force the states to zero at the end of the record.
	------------------------------------------------------------------------*/
	for( nbit = 0;  nbit < VITERBI_MEM_LEN-2;  nbit++ )
	{
		pathMem.outWord <<= 1;					// shift in a 1, else shift in a zero
		if( *pathMem.pState0Memory & VITERBI_MSB )
		pathMem.outWord |= 1;					

		pathMem.outWordBitCount++;
		if( pathMem.outWordBitCount > WORD_LEN-1 )	// if we've collected a whole word
		{									// put it in the buffer
			*pathMem.pUserData++ = pathMem.outWord;
			pathMem.outWordBitCount = 0;
		}

		*pathMem.pState0Memory <<= 1;
	}

	//mexPrintf("flush path memory data pointer: %d\n", pathMem.pUserData - rxUserDataArray );

	//---- finish up the last word ----------------------------
	if( pathMem.outWordBitCount < WORD_LEN )
	{
		pathMem.outWord <<= (WORD_LEN - pathMem.outWordBitCount);
		*pathMem.pUserData++ = pathMem.outWord;
	}

	//mexPrintf("flush path memory data pointer: %d\n", pathMem.pUserData - rxUserDataArray );
	return;
}




