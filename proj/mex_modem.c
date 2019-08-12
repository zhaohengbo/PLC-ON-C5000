/*=================================================================================
 Filename:			mex_modem.c

 Description:		Mex function call for OFDM powerline modem 

 Copyright (C) 2000 - 2003 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
========================================================================*/
#include "ofdm_modem.h"


//---- Input Arguments ---------
#define	CMD_STR			prhs[0]
#define	PARM1			prhs[1]
#define	PARM2			prhs[2]

//---- Output Arguments --------
#define	OUT1			plhs[0]
#define	OUT2			plhs[1]
#define	OUT3			plhs[2]


#define MAKE_FIELD(a) #a, a
#define MAKE_FUNC_PTR(a) #a, ((void *)a)

/*===============================================================
   local function prototypes
===============================================================*/
void		getUserDataFromMatlab(u16 *userData, const mxArray *matInArray);
void		copyWaveform2Matlab( const mxArray **matOutArray, i16 *pTxSignal);
i16			*getSignalFromMatlab(dCplxPtr dRecSignal, int startingSample);
double		copyReadData2Matlab(dCplxPtr dDataBuffer, u16 *userData, i16 parityGood );
//void 		parms2Matlab( const mxArray **matOutArray );
void 		parms2Matlab( mxArray **matOutArray );
u16 		readDataFramesFromMatlab( u16 *pUserData, dCplxPtr drec );

static void cleanup(void);


/*==============================================================
I/O function
	calling syntax:  
	[errorCount, userData] = ofdm_dataDet(recSignal, dataStart)
===============================================================*/
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{ 
	char		   	*CmdStr;
	char			prtStr[80];
	int			   	n;

	//---- transmit vars -----------------------
	i16			   	*pTxSignal;
	i16				winSignal[WINDOW_LEN];

	//---- agc vars -----------------------
	dCplxPtr		drec;
	dCplxPtr		drecAgc;
	double			*dBufferPos;
	i16				*recPtr;

	//---- receive vars -----------------------
	i16				errCount = 0;

	double			*errorCountPtr = NULL;
	dCplxPtr		dDataBuffer;
	
	//i16			*recSignal;			// use global version instead

	#if DEBUGIT > DEBUG_NONE
		diag.r = NULL;
		diag.i = NULL;
	#endif

	//---- get command string ----------------------------
	if( mxIsChar(CMD_STR) != 1 )
		mexErrMsgTxt("First argument must be a command string");

	n = 1 + ( mxGetM(CMD_STR)*mxGetN(CMD_STR) );		// Get input command string
	CmdStr = mxCalloc(n, sizeof(char));
	n = mxGetString(CMD_STR, CmdStr, n);
	if( n != 0 )
		mexErrMsgTxt("Error allocating command string."); 

	/*====================================================================
	Decode command and call associated functions
	Decode "XMIT"
	=====================================================================*/
 	if( strcmp(CmdStr, "xmit")==0 ) 
	{
	    //---- Check for proper number of arguments ---------------
		if( nlhs != 1 )
			mexErrMsgTxt("Unexpected number of output arguments."); 

		if( nrhs != 2 )
			mexErrMsgTxt("Syntax:  [pTxSignal] = mex_modem('xmit', userData)");

		getUserDataFromMatlab(txUserDataArray, PARM1);

		/*======================================================================
		generate transmit signal
		======================================================================*/
		pTxSignal = makePreamble(txSignalArray, winSignal);			// start at current ptr and build preamble
		pTxSignal = makeDataFrames( pTxSignal, winSignal, txUserDataArray);	// start a current ptr and build data frames

		/*======================================================================
		copy the waveform back to Matlab
		======================================================================*/
		copyWaveform2Matlab(&OUT1, txSignalArray);
	}
	
	/*====================================================================
	Decode "REC"
	=====================================================================*/
	else if( strcmp(CmdStr, "rec")==0 ) 
	{
	    //---- Check for proper number of arguments ---------------
		if( nrhs != 2 )
			mexErrMsgTxt("Wrong number of input arguments.\nSyntax:  [errorCount, userData, rxAgc] = mex_modem('rec', recSignal)");

	   	//---- input vars ---------------------------
		drec.i   = NULL;
		drec.r   = mxGetPr(PARM1);
		drec.len = mxGetM(PARM1)*mxGetN(PARM1);
		
	   	//---- Allocate output vars ---------------------------
		dDataBuffer.len = DATA_BUFFER_LEN*2;
		dDataBuffer.r = NULL;
		dDataBuffer.i = NULL;
		drecAgc.r = NULL;

		switch( nlhs )
		{
		case 3:
			OUT3 = mxCreateDoubleMatrix(mxGetM(PARM1), mxGetN(PARM1), mxREAL);
			drecAgc.r = mxGetPr(OUT3);
			drecAgc.i = NULL;
			drecAgc.len = drec.len;
		case 2:
			OUT2 = mxCreateDoubleMatrix(dDataBuffer.len, 1, mxREAL);
			dDataBuffer.r = mxGetPr(OUT2);
		case 1:
			OUT1 = mxCreateDoubleMatrix(1, 1, mxREAL);
			errorCountPtr = mxGetPr(OUT1);
			break;
		default:
			mexErrMsgTxt("Unexpected number of output arguments."); 
		}

		/*====================================================================
		do agc : get signal from Matlab, AGC, return AGC-ed signal if requested
				 detect packet and return pointer to signal
		=====================================================================*/
		recSignal = lookForPacket( drecAgc, drec );

		/*====================================================================
		do frame align & equalization
		=====================================================================*/
		recSignal = frameAlign(phaseEqArray, recSignal );

		if (frameAligned == 0 )
			postError("stopped");

		/*====================================================================
		read data and do detection
		=====================================================================*/
		errCount = readDataFrames(rxUserDataArray, recSignal, phaseEqArray );
	 
		//---- return the results to Matlab---------------------------------
		*errorCountPtr = copyReadData2Matlab(dDataBuffer, rxUserDataArray, errCount );
	}
	
	/*====================================================================
	Report modem parameters back to Matlab
	=====================================================================*/
	else if( strcmp(CmdStr, "parm")==0 ) 
	{
	    //---- Check for proper number of arguments ---------------
		if( nlhs != 1 )
			mexErrMsgTxt("Need variable assignment to accept structure of parameters.\nSyntax:  parmStruct = mex_modem('parm')");

		parms2Matlab(&OUT1);
	}

	/*====================================================================
	Decode "Align"
	=====================================================================*/
	else if( strcmp(CmdStr, "align")==0 )
	{
	    int			n;
		iCplx		*feqPtr;

		//---- Check for proper number of arguments ---------------
		if( nrhs != 3 )
			mexErrMsgTxt("Wrong number of input arguments.\nSyntax:  [freqEq] = mex_modem('align', recSignal, packetStart)");

	   	//---- input vars ---------------------------
		drec.i   = NULL;
		drec.r   = mxGetPr(PARM1);
		drec.len = mxGetM(PARM1)*mxGetN(PARM1);
		if( drec.len > RX_CIRC_BUFFER_LEN )
			mexErrMsgTxt("recSignal data passed is too big.");
		
		n = (int)(*mxGetPr(PARM2));				// recSignal starts at this offset
		recSignal = recSignalArray + n;	
		

	   	//---- Allocate output vars ---------------------------
		if( nlhs != 1)
			mexErrMsgTxt("Wrong number of output arguments");
		
		OUT1 = mxCreateDoubleMatrix(CARRIER_LEN, 1, mxCOMPLEX);
		drecAgc.r = mxGetPr(OUT1);
		drecAgc.i = mxGetPi(OUT1);
		drecAgc.len = CARRIER_LEN;

		//---- get receive data -------------------------
		recPtr = recSignalArray;
		for( n = 0; n < drec.len; n++ )
		{
			*recPtr = (i16)(*drec.r++);
			recPtr++;
		}

		// ---- do frame align & equalization ----------------------
		recSignal = frameAlign(phaseEqArray, recSignal );

		feqPtr = phaseEqArray;
		for( n = 0; n < CARRIER_LEN; n++ )
		{
			*drecAgc.r++ = feqPtr->re;
			*drecAgc.i++ = feqPtr->im;
			feqPtr++;
		}
	}	
	
	/*====================================================================
	Decode "SYMB"
	=====================================================================*/
	else if( strcmp(CmdStr, "symb")==0 )
	{
		u16			n;

	    //---- Check for proper number of arguments ---------------
		if( nrhs != 2 )
			mexErrMsgTxt("Wrong number of input arguments.\nSyntax:  [errorCount, <userData>, <freqEq>] = mex_modem('symb', symbolArray)");

	   	//---- Allocate output vars ---------------------------
		dDataBuffer.len = DATA_BUFFER_LEN*2;
		dDataBuffer.r = NULL;
		dDataBuffer.i = NULL;
		drecAgc.r = NULL;
		drecAgc.i = NULL;
		
		switch( nlhs )
		{
		case 3:
			OUT3 = mxCreateDoubleMatrix(CARRIER_LEN, 1, mxCOMPLEX);
			drecAgc.r = mxGetPr(OUT3);
			drecAgc.i = mxGetPi(OUT3);
			drecAgc.len = CARRIER_LEN;
		case 2:
			OUT2 = mxCreateDoubleMatrix(dDataBuffer.len, 1, mxREAL);
			dDataBuffer.r = mxGetPr(OUT2);
		case 1:
			OUT1 = mxCreateDoubleMatrix(1, 1, mxREAL);
			errorCountPtr = mxGetPr(OUT1);
			break;
		default:
			mexErrMsgTxt("Unexpected number of output arguments."); 
		}

		/*====================================================================
		get equalization data 
		=====================================================================*/
		drec.len = mxGetM(PARM1)*mxGetN(PARM1);
		drec.r   = mxGetPr(PARM1);
		drec.i   = NULL;
		
		if( drecAgc.r != NULL )
		{
			if( drec.len < (CARRIER_LEN*2*(DATA_FRAMES_PER_BLOCK*NUM_DATA_BLOCKS+1)) )
				mexErrMsgTxt("Trace buffer is to short to get freq Eq data!");

			drec.r += CARRIER_LEN*2*DATA_FRAMES_PER_BLOCK*NUM_DATA_BLOCKS;
			for( n = 0; n < CARRIER_LEN; n++ )
			{
				*drecAgc.r++ = *drec.r++;
				*drecAgc.i++ = *drec.r++;
			}
			drec.r   = mxGetPr(PARM1);
		}
	
		if( drec.len < (CARRIER_LEN*2*DATA_FRAMES_PER_BLOCK*NUM_DATA_BLOCKS) )
			mexErrMsgTxt("Trace buffer is to short get all symbols!");

		/*====================================================================
		read data and do detection
		=====================================================================*/
		errCount = readDataFramesFromMatlab(rxUserDataArray, drec );
	 
		//---- return the results to Matlab---------------------------------
		*errorCountPtr = copyReadData2Matlab(dDataBuffer, rxUserDataArray, errCount );
	}
	
	/*====================================================================
	Decode "SYMB"
	=====================================================================*/
	else if( strcmp(CmdStr, "symOld")==0 )
	{
		u16			n;

	    //---- Check for proper number of arguments ---------------
		if( nrhs != 2 )
			mexErrMsgTxt("Wrong number of input arguments.\nSyntax:  [errorCount, <userData>, <freqEq>] = mex_modem('symb', symbolArray)");

	   	//---- input vars ---------------------------
		drec.i   = NULL;
		drec.r   = mxGetPr(PARM1);
		drec.len = mxGetM(PARM1)*mxGetN(PARM1);
		
	   	//---- Allocate output vars ---------------------------
		dDataBuffer.len = DATA_BUFFER_LEN*2;
		dDataBuffer.r = NULL;
		dDataBuffer.i = NULL;
		drecAgc.r = NULL;
		drecAgc.i = NULL;
		
		switch( nlhs )
		{
		case 3:
			OUT3 = mxCreateDoubleMatrix(CARRIER_LEN, 1, mxCOMPLEX);
			drecAgc.r = mxGetPr(OUT3);
			drecAgc.i = mxGetPi(OUT3);
			drecAgc.len = CARRIER_LEN;
		case 2:
			OUT2 = mxCreateDoubleMatrix(dDataBuffer.len, 1, mxREAL);
			dDataBuffer.r = mxGetPr(OUT2);
		case 1:
			OUT1 = mxCreateDoubleMatrix(1, 1, mxREAL);
			errorCountPtr = mxGetPr(OUT1);
			break;
		default:
			mexErrMsgTxt("Unexpected number of output arguments."); 
		}

		/*====================================================================
		get equalization data 
		=====================================================================*/
		if( drecAgc.r != NULL )
		{
			for( n = 0; n < CARRIER_LEN; n++ )
			{
				*drecAgc.r++ = *drec.r++;
				*drecAgc.i++ = *drec.r++;
			}
		}
		else
		{
			drec.r += CARRIER_LEN*2;
		}

		drec.r +=2;		// increment past uTraceAGCFound and uTraceIndex 

		/*====================================================================
		read data and do detection
		=====================================================================*/
		errCount = readDataFramesFromMatlab(rxUserDataArray, drec );
	 
		//---- return the results to Matlab---------------------------------
		*errorCountPtr = copyReadData2Matlab(dDataBuffer, rxUserDataArray, errCount );
	}
	
	/*====================================================================
	Unrecognized decode 
	=====================================================================*/
	else
	{
		strcpy( prtStr, "Unrecognized command string.  Valid syntax:\n" );
		strcat( prtStr, "		[txSignal]						   = mex_modem('xmit', userData)\n" );
		strcat( prtStr, "		[errorCount, <userData>]		   = mex_modem('rec', recSignal, PreambleStart)" ); 
		strcat( prtStr, "		[errorCount, <userData>, <freqEq>] = mex_modem('symb', symbolArray)" );
		strcat( prtStr, "		[freqEq]						   = mex_modem('align', recSignal, packetStart)" );
		mexErrMsgTxt(prtStr);
	}

}



/*====================================================================
functions
=====================================================================*/


/*====================================================================
	Allocate byte buffer and fill with float data from Matlab
=====================================================================*/
void getUserDataFromMatlab(u16 *userData, const mxArray *matInArray)
{
	char			prtStr[80];
	i16				n;					// loop counter
	i16				wordCount = 0;
	u16				hiByte = 0;
	dCplxPtr		dUserData;

	//---- Get user data as bytes ---------------------------
	dUserData.i   = NULL;
	dUserData.r   = mxGetPr(matInArray);
	dUserData.len = mxGetM(matInArray)*mxGetN(matInArray);
	if( dUserData.len > NUM_USER_BYTES )
	{
		sprintf(prtStr, "Packet set up for %d bytes max, you passed %d bytes.", 
			NUM_USER_BYTES, dUserData.len ); 
		mexErrMsgTxt(prtStr); 
	}
	
	//---- put 2 "bytes" from Matlab in each word of the userData buffer -------
	for(n = 0;  n < dUserData.len; n++ )
	{
		hiByte = (hiByte + 1) & 0x01;	
		if( hiByte == 1 )
		{
			*userData = ((u16)(*dUserData.r++)) << BYTE_LEN;
		}
		else
		{
			*userData++ += (u16)(*dUserData.r++);
			wordCount++;
		}
	}

	if( hiByte == 1 )	// finish out last data word if necessary
	{
		userData++;
		wordCount++;
	}

	for( ; wordCount < DATA_BUFFER_LEN; wordCount++ )	// make the rest of buffer zeros
		*userData++ = 0;

}

	
/*====================================================================
  Copy the integer transmit waveform into the double Matlab array.
  The Matlab array is intialized to all zeros, so to add a zero pad
  to the front of the Matlab array, add an offset to the Matlab array 
  pointer.
=====================================================================*/
void copyWaveform2Matlab( const mxArray **matOutArray, i16 *pTxSignal)
{
	int				n;
	int				txSignalLen;
	dCplxPtr		dTxSignal;		// pointer to user data in Matlab array
	
	txSignalLen = NUM_PRE_FRAMES * FFT_LEN
				+ (int)SYNC_FIELD_LEN
				+ NUM_DATA_BLOCKS * DATA_FRAMES_PER_BLOCK * (CYCLIC_PREFIX_LEN + FFT_LEN);
	dTxSignal.len = WAVE_ZERO_PAD_FRONT + txSignalLen + WAVE_ZERO_PAD_BACK;

	*matOutArray = mxCreateDoubleMatrix(dTxSignal.len, 1, mxREAL);

	dTxSignal.r = mxGetPr(*matOutArray) + WAVE_ZERO_PAD_FRONT;	// offset pointer to make zero pad

	for( n = 0; n < txSignalLen;  n++ )		// do the copy
		*dTxSignal.r++ = (double)(*pTxSignal++);

	#if DEBUGIT == DEBUG_PUT_MAT
	{	i16		*ptr = pTxSignal;
		diagArray = mxCreateDoubleMatrix(dTxSignal.len, 1, mxREAL);
		mxSetName(diagArray, "ofdm_diag");
		diag.r = mxGetPr(diagArray);
		for( n = 0 ; n < TX_BUFFER_LEN ; n++ )
			*diag.r++ = (double)(*ptr++);
		mexPutArray(diagArray, "caller");
	}
	#endif

}


/*====================================================================
	Get and integer copy of the received signal from Matlab
	
=====================================================================*/
i16 *getSignalFromMatlab(dCplxPtr dRecSignal, int startingSample)
{
	i16				n;
	i16				*recSignal;
	
	//---- copy it -----------------------------------------
	recSignal = recSignalArray;
	for( n = 0;  n < dRecSignal.len;  n++ )
	{
		*recSignal++ = (i16)(*dRecSignal.r++);
		if( n >= RX_CIRC_BUFFER_LEN )
			mexErrMsgTxt("Overran circular buffer.");
	}
	return recSignalArray + startingSample -1;	// the -1 is a Matlab thing!
}


/*====================================================================
	if asked for, copy the received data into a double Matlab array 
	and return parityGood, else assume that the data was all zeros,
	count the non-zero bits and return the number of non-zero bits as
	the error count.
=====================================================================*/
double copyReadData2Matlab(dCplxPtr dDataBuffer, u16 *userData, i16 parityGood )
{
	i16				n;

	if( dDataBuffer.r != NULL )
	{
		for( n = 0; n < DATA_BUFFER_LEN;  n++ )
		{	
			//*dDataBuffer.r++ = (double)(*userData++);
			*dDataBuffer.r++ = (double)(*userData >> BYTE_LEN);
			*dDataBuffer.r++ = (double)(0x00FF & *userData++);
		}
		return parityGood;			
	}
	else
	{
		return countErrors(userData);
	}
}


/*====================================================================
	Send data to the matlab workspace

=====================================================================*/
void	diagData( void *array, u16 len, char *matName, diagType flag)
{
	mxArray		*diagArray;
	dCplxPtr	diag;

	u16			n;
	u16			*unum;
	i16			*inum;
	iCplx		*jnum;

	switch( flag )
	{
	case diagU16:
		diagArray = mxCreateDoubleMatrix(len, 1, mxREAL);
		diag.r = mxGetPr(diagArray);
		unum = (u16 *)array;
		for( n = 0 ; n < len; n++ )
		{
			*diag.r++ = (double)(*unum++);
		}
		break;
	case diagI16:
		diagArray = mxCreateDoubleMatrix(len, 1, mxREAL);
		diag.r = mxGetPr(diagArray);
		inum = (i16 *)array;
		for( n = 0 ; n < len; n++ )
		{
			*diag.r++ = (double)(*inum++);
		}
		break;
	case diagICPLX:
		diagArray = mxCreateDoubleMatrix(len, 1, mxCOMPLEX);
		diag.r = mxGetPr(diagArray);
		diag.i = mxGetPi(diagArray);
		jnum = (iCplx *)array;
		for( n = 0 ; n < len; n++ )
		{
			*diag.r++ = (double)(jnum->re);
			*diag.i++ = (double)(jnum->im);
			jnum++;
		}
		break;
	default:
		postError("Unrecognized diag type.");
	}

	mxSetName(diagArray, matName);
	mexPutArray(diagArray, "caller");

}


//====================================================================
//	return to Matlab some important constant parameters
//  11/17/03  Hagen		added more constants to returned parms
//=====================================================================
//void parms2Matlab( const mxArray **matOutArray )
void parms2Matlab( mxArray **matOutArray )
{
	i16			n;
	mxArray 	*fieldValue;
	//i16			fieldNamesLen;
	i16			numFields = 0;

	char		**fieldNames;

	struct		parmDataSt
	{
		char		*name;
		double		value;
	};
	struct parmDataSt	parmData[]  =  
		{	MAKE_FIELD( FFT_LEN               ),
  			MAKE_FIELD( CYCLIC_PREFIX_LEN     ),
  			MAKE_FIELD( WINDOW_LEN            ),
  			MAKE_FIELD( SYNC_FIELD_LEN        ),
  			MAKE_FIELD( NUM_PRE_FRAMES        ),
  			MAKE_FIELD( NUM_USER_BYTES        ),
  			MAKE_FIELD( CARRIER_LOW           ),
  			MAKE_FIELD( CARRIER_HIGH          ),
  			MAKE_FIELD( CARRIER_LEN           ),
  			MAKE_FIELD( NUM_DATA_BLOCKS       ),
  			MAKE_FIELD( NUM_SYMBOLS           ),
  			MAKE_FIELD( DATA_FRAMES_PER_BLOCK ),
  			MAKE_FIELD( SYMBOL_OFFSET         ),
  			MAKE_FIELD( DATA_BUFFER_LEN       ),
  			MAKE_FIELD( TX_BUFFER_LEN         ),
  			MAKE_FIELD( RX_CIRC_BUFFER_LEN    ),
  			MAKE_FIELD( VITERBI_ENCODE_SHIFT  ),			 
  			MAKE_FIELD( VITERBI_DISTANCE_SHIFT),			 
  			MAKE_FIELD( PRE_IFFT_SCALE        ),			 
  			MAKE_FIELD( DATA_IFFT_SCALE       ),			 
  			MAKE_FIELD( IMP_IFFT_SCALE        ),			 
  			MAKE_FIELD( DDPHASE_MSHIFT        ),			 
  			MAKE_FIELD( DDPHASE_HI_LIM        ),			 
  			MAKE_FIELD( DDPHASE_LO_LIM        ),			 
  			MAKE_FIELD( PWR_SCALE_SHIFT       ),			 
  			MAKE_FIELD( CARRIER_SCALE         ),			 
  			MAKE_FIELD( PWR_SHIFT             ),			 
  			MAKE_FIELD( FEQ_SHIFT             ),
  			MAKE_FIELD( DEBUGIT               ),			 
			"parmEnd",			0
		};
  			
	while( strcmp( parmData[numFields].name, "parmEnd" ) != 0 )
		numFields++;

	fieldNames	   = malloc( numFields*sizeof(char*) );
	for( n = 0; n < numFields; n++ )
		fieldNames[n] = parmData[n].name;

	*matOutArray = mxCreateStructMatrix(1, 1, numFields, fieldNames);

	for( n = 0; n < numFields; n++ )
	{
		fieldValue = mxCreateDoubleMatrix(1,1,mxREAL);
		*mxGetPr(fieldValue) = parmData[n].value;
		mxSetFieldByNumber( *matOutArray, 0, n, fieldValue );
	}
	return;
}

/*
#if 0	
//====================================================================
//	function pointer dereference
//
//=====================================================================
void displayFunctionAddr( void )
{
	static int CallCount = 0;
	struct 
	{
		char *name;
		void *func;
	}  funcs[] =	
		{		
		    MAKE_FUNC_PTR(mexFunction),
		    MAKE_FUNC_PTR(lookForPacket),
//SNR_CALC  MAKE_FUNC_PTR(getPowerFromFFT),
		    MAKE_FUNC_PTR(frameAlign),
		    MAKE_FUNC_PTR(readDataFrames),
		    MAKE_FUNC_PTR(decodeData),
		    MAKE_FUNC_PTR(aveFFT),
		    MAKE_FUNC_PTR(AlignPhaseCompare),
		    MAKE_FUNC_PTR(getFrameStartFromImpulse),
		    MAKE_FUNC_PTR(getFreqEq),
		    MAKE_FUNC_PTR(dataFFT),
		    MAKE_FUNC_PTR(dataPhaseCompare),
		    MAKE_FUNC_PTR(deinterleaveSymbols),
		  //MAKE_FUNC_PTR(deinterleave),
		    MAKE_FUNC_PTR(viterbiDecodeSoft),
		    MAKE_FUNC_PTR(viterbiDecodeHard),
		    MAKE_FUNC_PTR(compareParityCheckBytes),
		    MAKE_FUNC_PTR(countErrors),
		    MAKE_FUNC_PTR(fillCarriers),
		    MAKE_FUNC_PTR(mexfft),
		    MAKE_FUNC_PTR(mexrfft),
		    MAKE_FUNC_PTR(circFFT),
		    MAKE_FUNC_PTR(circFFTshort),
		    MAKE_FUNC_PTR(backFFT),
		    MAKE_FUNC_PTR(diagData),
		    MAKE_FUNC_PTR(getUserDataFromMatlab),
		    MAKE_FUNC_PTR(copyWaveform2Matlab),
		    MAKE_FUNC_PTR(getSignalFromMatlab),
		    MAKE_FUNC_PTR(copyReadData2Matlab),
		    MAKE_FUNC_PTR(parms2Matlab),
		    "End of funcs", 0
		};

	for(n = 0; strcmp((funcs + n)->name,"End of funcs") != 0; n++)
		mexPrintf("%-30s = %p\n", (funcs + n)->name, (funcs + n)->func);

	mexPrintf("Call Count = %10d\n", ++CallCount);
	//if(CallCount >= 33)
	//	CallCount += 7;

	mexPrintf("rx buffer   %p\n", recSignalArray );
	//mexPrintf("dma offset  %p =  %d\n", &rxDMAoffset, rxDMAoffset );
	mexPrintf("rx pointer  %p =  %d\n", recSignal, (i16)(recSignal-recSignalArray) );

	return;
}
#endif
*/

