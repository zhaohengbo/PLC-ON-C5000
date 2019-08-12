/*==============================================================
 Filename:			ofdm_global.c

 Description:		This file allocates and initializes global 
 					variables used in the OFDM powerline modem.

 Copyright (C) 2002 - 2003 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
===============================================================*/

#define	I_AM_D
#include "ofdm_modem.h"
#include "ofdm_global.h"	  

/*==============================================================
	Global debug variable declarations
===============================================================*/
#if DEBUGIT >= DEBUG_IFFT && COMPILE_MODE == MEX_COMPILE
	mxArray			*diagArrayIn;
	dCplxPtr		diagIn;
	mxArray			*diagArrayOut;
	dCplxPtr		diagOut;
	i16				diagCount;
#elif DEBUGIT > DEBUG_NONE && COMPILE_MODE == MEX_COMPILE
	mxArray			*diagArray;
	dCplxPtr		diag = {NULL, NULL};
	i16				diagCount = 0;
#endif


/*==============================================================
	Global variable declarations, biggest buffers first
===============================================================*/
#if COMPILE_MODE == DSP_COMPILE
	// ACHTUNG!  recSignalArray >>MUST<< be first entry in this section to ensure it
	// lines up on proper power-of-two boundary
	#pragma	DATA_SECTION(recSignalArray, "bufsect");
#endif
i16				recSignalArray[RX_CIRC_BUFFER_LEN];// receive circular buffer

TXDATA			txSignalArray[TX_BUFFER_LEN];		// waveform we are building

u16				txUserDataArray[DATA_BUFFER_LEN]; 	// byte-wide buffer for user data
u16				rxUserDataArray[DATA_BUFFER_LEN]; 	// byte-wide buffer for user data
u16				rxUserDataCopy[NUM_USER_BYTES/2]; 	// shadow copy used to prevent overwriting
u16				CRCtableArray[256];					// CRC table buffer

iCplx			symbolArray[CARRIER_LEN];			// 2-bit symbols
i16				distanceArray[CARRIER_LEN*2]; 		// distance metric buffer, also encode buffer
iCplx			phaseEqArray[CARRIER_LEN];			// freq equalization buffer

#if (SAVETRACE == TRUE) || (SAVESYMBOLS == TRUE)
	//u16	uTraceAGCFound=0;
	u16	uTraceIndex = 0;
	u16 uTraceEnable = TRUE;
	u16	uTraceData[TRACE_BUFF_SIZE];
	u16 uTraceDataAddr = (u16) &uTraceData[0];		// Needed for Matlab tester
	u16 uTraceDataLen = TRACE_BUFF_SIZE;			// Needed for Matlab tester
#endif
	
#if COMPILE_MODE == DSP_COMPILE
	#pragma	DATA_SECTION(fftArray, "fftsect");
#endif
iCplx			fftArray[FFT_LEN];  				// working FFT buffer for calcSNR

u16*	uppParms16[30];		// List of addresses for command interface 16-bit parms
u32*	uppParms32[10];		// List of addresses for command interface 32-bit parms
u16		uScratch16;
u32		ulScratch32;

/*==============================================================
	Global flag and control variables
===============================================================*/
i16				*recSignal;

modemStateType 	modemState = IdleState;	// an enum var that holds the state of the modem

agcStateType	agcState;
i16		preambleDetCount = 0;
u16		frameAligned = 0;			// Frame alignment flag
volatile i16		SNRflag;
i16		AGCenable = 1;				// AGC enable
//i16		SNRcounter = 0;				// decide when to do SNRcalc 
i16		prevSnrSample = 0;			// used to time SNR calc
u16		AFEctrl1;
u16 	AFEctrl2;

#if (AFE_TX_FORMAT == 24)	//  Transmit two 24-bit values
	u32		IdleBuffArray[IDLE_BUFFER_LEN];
#else	// 	Transmit three 16-bit values
	u16		IdleBuffArray[IDLE_BUFFER_LEN*AFE_SIZE];
#endif


#if COMPILE_MODE == MEX_COMPILE
	i16	   		*rxDMApointer;						// simulates DMA pointer
#endif

u16		uLEDShadow;

u16	uErrCnt[32];
u16	uErrCntAddr = (u16) &uErrCnt;	// Variable tester can read to locate uErrCnt array.

u16	uUartCommand[COMMAND_SIZE];		// On the MASTER this is received from the host,
									//   on the SLAVE this is sent to the emeter.
u16 uBoard;
PlcStateType	uPlcState;
UartStateType	uUartState;
u16 uUARTTarget;					// Keeps track of which UART devide we're talking to.

// These are used to receive UART data, not a UART command, from the host (currently just used for Flash data).
u16 uUartDataIndex;
u16 uUartDataLength;
u16 uUartDataArray[25];				// 25 words is long enough for any expected flash data.  If this
u16 upUartBufferFifo[UART_BUFFER_SIZE];			// FIFO for uart characters.
volatile u16 uUartBufferPointerIn = 0;
u16 uUartBufferPointerOut = 0;
u16 uFlashWriteStatus=FLASH_STATUS_COMPLETE;	// Initial value to let things get started.
u16 uFlashLogStatus=FLASH_STATUS_COMPLETE;	// Initial value to let things get started.
u16 uFlashTarget;

u16 uCodeVersion;
u16 uUartCommandIndex;
u16 uHostResponseStart;				// } These two variables determine what portion of the slave
u16 uHostResponseLength;			// } response will be sent back to the host.
u16 uAckAfter485;					// Provides a method to let host know we're ready to receive chars.
u16 upUartResponseBuffer[61];		// PLC packet = 128 bytes - addressing and checksum = 122 bytes = 61 words.
u16 uUartResponseIndex;
u16 uTransmitPacketReady;
u16 uFindSlaves = 0;				// Not currently executing command to find all slaves.
u32 ulFindSlaveAddr;
u32 ulFindSlaveAddrMax;
u16 uSlaveFound;
u32 ulClosestSlaveAddr = 0L;	// Invalid address.
u32 ulMasterAddr;
u32 ulMyAddr;
u16 uAutoPoll = 1;				// Flag to permit auto-polling. Enable by default.
u16 uAutoPollPause = 0;			// Used to pause AutoPoll during UART commands.
volatile u32 ulAutoPollCounter = 0L;		// Reset counter.
volatile u32 ulBerDelayCounter = 0L;
u32 ulBerDelay = 516L;			// 62 usec/int * 516 = 32 msec
volatile u32 ulUartCounter;
volatile u32 ulFlashWriteCounter;
volatile u16 uFlashShow;                 // the LED frame rate counter for the flash erase cycles
//u32 ulLastTxCounter;
volatile u32 ulLastRxCounter = 0L;
volatile u32 ulPlcResponseTimeoutCounter = 0L;
volatile u32 ulAutoPollResponseTimeoutCounter = 0L;
u32 ulBerCounter;
u32 ulNumBerPackets;
u16 uBerTestInProgress;
u16 uBerMissedPackets;
u16 uBerErrorCounter;
u32 ulBERSlaveAddr;
u16 uHoldOffPLC = 0;
u16 uLastByteFlag = 0;
u16 uCommandBlock = 0;

BufferListStruct UARTDataOut[SIZE_UART_OUT_ARRAY];

/*==============================================================
	Global constant tables
===============================================================*/

//----scrambler pattern ----------------------------------------------
const u16	scramblerLookup[127] =
	{
	     3826,   51458,    9774,   46604,   54503,   46122,   64081,   47358,
	     7653,   37380,   19549,   27673,   43471,   26709,   62627,   29180,
	    15307,    9224,   39098,   55347,   21406,   53419,   59718,   58360,
	    30614,   18449,   12661,   45158,   42813,   41303,   53901,   51184,
	    61228,   36898,   25323,   24781,   20091,   17071,   42267,   36833,
	    56921,    8260,   50646,   49562,   40182,   34143,   18999,    8131,
	    48306,   16521,   35757,   33589,   14829,    2750,   37998,   16263,
	    31076,   33043,    5979,    1642,   29658,    5501,   10460,   32526,
	    62153,     550,   11958,    3284,   59316,   11002,   20920,   65053,
	    58770,    1100,   23916,    6569,   53096,   22004,   41841,   64571,
	    52004,    2200,   47832,   13139,   40656,   44009,   18147,   63607,
	    38472,    4401,   30128,   26279,   15777,   22482,   36295,   61679,
	    11408,    8802,   60256,   52558,   31554,   44965,    7055,   57822,
	    22816,   17605,   54977,   39580,   63109,   24394,   14111,   50108,
	    45632,   35211,   44419,   13625,   60682,   48788,   28223,   34681,
	    25729,    4887,   23302,   27251,   55829,   32040,   56447   
	};
#if 0				// old DECODING_HARD
const i16	scramblerLookup[127] = 
	{	 14,  242,  201,    2,   38,   46,  182,   12,  
		212,  231,  180,   42,  250,   81,  184,  254,
		 29,  229,  146,    4,   76,   93,  108,   25,  
		169,  207,  104,   85,  244,  163,  113,  252,   
		 59,  203,   36,    8,  152,  186,  216,   51,  
		 83,  158,  208,  171,  233,   70,  227,  248, 
		119,  150,   72,   17,   49,  117,  176,  102,  
		167,   61,  161,   87,  210,  141,  199,  240, 
		239,   44,  144,   34,   98,  235,   96,  205,  
		 78,  123,   66,  175,  165,   27,  143,  225,  
		222,   89,   32,   68,  197,  214,  193,  154,  
		156,  246,  133,   95,   74,   55,   31,  195, 
		188,  178,   64,  137,  139,  173,  131,   53,   
		 57,  237,   10,  190,  148,  110,   63,  135,  
		121,  100,  129,   19,   23,   91,    6,  106,  
		115,  218,   21,  125,   40,  220,  127  
	};
#endif

//---- Preamble --------------------------------------------------------------
// These are the 2-bit phase symbols that we look for in the preamble.
// They are summed twice to generate the phases that are transmitted.
#if CARRIER_LEN == 85
const i16 PreambleArray[CARRIER_LEN] =
	{	 0,     0,     2,     1,     3,
	     0,     3,     3,     0,     2,
	     3,     1,     1,     0,     0,
	     1,     3,     0,     2,     1,
	     2,     3,     2,     3,     2,
	     2,     2,     3,     3,     3,
	     1,     1,     1,     3,     3,
	     2,     3,     1,     3,     3,
	     0,     1,     1,     2,     1,
	     0,     1,     1,     1,     0,
	     1,     2,     0,     1,     0,
	     0,     2,     1,     1,     3,
	     2,     3,     3,     1,     2,
	     2,     2,     3,     2,     3,
	     2,     1,     2,     3,     2,
	     0,     3,     2,     1,     0,
	     3,     2,     0,     3,     3
	};
#endif

#if CARRIER_LEN == 41
const i16 PreambleArray[CARRIER_LEN] =				
	{	
		0,	0,	2,	0,	3,	2,	1,	0,	3,	3,		//BestPhase4: -369.02     162.23       2.42        2.44 
		2,	3,	3,	0,	3,	3,	3,	3,	0,	1,	
		0,	3,	2,	3,	0,	3,	0,	3,	0,	0,	
		0,	0,	2,	2,	0,	3,	3,	3,	1,	1
	};
#endif
#if CARRIER_LEN == 40
const i16 PreambleArray[CARRIER_LEN] =
	{
     0,     0,     2,     2,     0,     2,     1,     2,     2,     2,
     2,     0,     3,     3,     2,     0,     2,     0,     1,     1,
     2,     3,     1,     2,     3,     0,     1,     1,     0,     2,
     0,     1,     0,     2,     3,     2,     3,     0,     1,     2
	};
#endif
#if CARRIER_LEN == 50
const i16 PreambleArray[CARRIER_LEN] =
	{
     0,     0,     1,     3,     2,     3,     2,     0,     1,     2,
     0,     2,     2,     0,     3,     0,     3,     1,     2,     3,
     3,     2,     3,     3,     2,     0,     0,     1,     2,     1,
     1,     2,     0,     3,     2,     1,     2,     0,     2,     3,
     1,     2,     3,     2,     2,     1,     1,     3,     1,     3
	};
#endif
#if CARRIER_LEN == 60
const i16 PreambleArray[CARRIER_LEN] =
	{
     0,     0,     2,     2,     2,     3,     2,     0,     1,     0,
     1,     0,     3,     1,     2,     2,     0,     0,     0,     0,
     2,     3,     3,     0,     2,     3,     0,     1,     0,     2,
     3,     1,     0,     2,     2,     2,     2,     0,     2,     1,
     2,     3,     3,     0,     2,     0,     3,     2,     0,     2,
     3,     1,     1,     3,     0,     0,     2,     3,     0,     3
	};
#endif
//#if CARRIER_LEN == 70
//const i16 PreambleArray[CARRIER_LEN] =
//	{
//     0,     0,     3,     3,     0,     1,     3,     2,     2,     2,
//     2,     1,     1,     1,     2,     2,     0,     3,     3,     1,
//     3,     2,     3,     2,     1,     2,     2,     2,     1,     2,
//     0,     1,     0,     0,     1,     2,     2,     0,     3,     0,
//     3,     2,     1,     1,     2,     0,     3,     3,     3,     1,
//     0,     1,     0,     0,     2,     2,     1,     0,     2,     1,
//     3,     2,     3,     2,     3,     1,     1,     0,     1,     0
//	};
//#endif
//#if CARRIER_LEN == 75
//const i16 PreambleArray[CARRIER_LEN] =
//	{
//     0,     0,     1,     2,     0,     3,     3,     1,     1,     0,
//     2,     2,     0,     1,     3,     3,     0,     1,     1,     0,
//     2,     0,     2,     2,     1,     3,     0,     0,     2,     2,
//     3,     1,     2,     1,     2,     0,     2,     0,     3,     1,
//     0,     1,     1,     1,     2,     1,     1,     2,     0,     0,
//     1,     2,     2,     2,     0,     3,     2,     3,     2,     1,
//     1,     1,     3,     1,     3,     1,     1,     2,     1,     3,
//     1,     2,     2,     2,     2
//	};
//#endif
#if CARRIER_LEN > 60
//const i16 PreambleArray[CARRIER_LEN] =
const i16 PreambleArray[] =			// length = 80
	{
     0,     0,     1,     2,     0,     3,     3,     1,     1,     0,
     2,     2,     0,     1,     3,     3,     0,     1,     1,     0,
     2,     0,     2,     2,     1,     3,     0,     0,     2,     2,
     3,     1,     2,     1,     2,     0,     2,     0,     3,     1,
     0,     1,     1,     1,     2,     1,     1,     2,     0,     0,
     1,     2,     2,     2,     0,     3,     2,     3,     2,     1,
     1,     1,     3,     1,     3,     1,     1,     2,     1,     3,
     1,     2,     2,     2,     2,     3,     1,     1,     1,     0
	};
#endif



//---- Convolutional encoder lookup table --------------------------------------------------
const i16	ViterbiEncodeLookup[128] = 
	{
		  0,  3,  1,  2,  0,  3,  1,  2,	//  0 thru 7
		  3,  0,  2,  1,  3,  0,  2,  1,	//  8 - 15
		  3,  0,  2,  1,  3,  0,  2,  1,	// 16 - 23
		  0,  3,  1,  2,  0,  3,  1,  2,	// 24 - 31
		  2,  1,  3,  0,  2,  1,  3,  0,	// 32 - 39
		  1,  2,  0,  3,  1,  2,  0,  3,	// 40 - 47
		  1,  2,  0,  3,  1,  2,  0,  3,	// 48 - 55
		  2,  1,  3,  0,  2,  1,  3,  0,	// 56 - 63
		  3,  0,  2,  1,  3,  0,  2,  1,	// 64 - 71
		  0,  3,  1,  2,  0,  3,  1,  2,	// 72 - 79
		  0,  3,  1,  2,  0,  3,  1,  2,	// 80 - 87
		  3,  0,  2,  1,  3,  0,  2,  1,	// 88 - 95
		  1,  2,  0,  3,  1,  2,  0,  3,	// 96 - 103
		  2,  1,  3,  0,  2,  1,  3,  0,	//104 - 111
		  2,  1,  3,  0,  2,  1,  3,  0,	//112 - 119
		  1,  2,  0,  3,  1,  2,  0,  3		//120 - 127 
	};

//---- viterbi decoder values for each trellis state---------
const i16 decodeTable[] =
	{	  0,   3,   1,   2,		//  0 -   7
		  0,   3,   1,   2,		//  8 -  15
		  3,   0,   2,   1,		// 16 -  23
		  3,   0,   2,   1,		// 24 -  31
		  3,   0,   2,   1,		// 32 -  39
		  3,   0,   2,   1,		// 40 -  47
		  0,   3,   1,   2,		// 48 -  55
		  0,   3,   1,   2,		// 56 -  63
		  2,   1,   3,   0,		// 64 -  71
		  2,   1,   3,   0,		// 72 -  79
		  1,   2,   0,   3,		// 80 -  87
		  1,   2,   0,   3,		// 88 -  95
		  1,   2,   0,   3,		// 96 - 103
		  1,   2,   0,   3,		//104 - 111
		  2,   1,   3,   0,		//112 - 119
		  2,   1,   3,   0		//120 - 127
	};
	
	
#if COMPILE_MODE == MEX_COMPILE
//---- models the gains of the TLFD240 or AFE1230 RX PGA ------------------------
const double AFE_RXPGA[] =
	{	 1.0000,		//  0 dB
		 1.4125,		//  3 dB
		 1.9953,		//  6 dB
		 2.8184,		//  9 dB
		 3.9811,		// 12 dB
		 5.6234,		// 15 dB
		 7.9433,		// 18 dB
		11.2202,		// 21 dB
		15.8489,		// 24 dB
		22.3872,		// 27 dB
		31.6228, 		// 30 dB
	};

#endif

// see ofdm_datatypes.h to see implementation specifics for SetXF() and ClearXF()
#if COMPILE_MODE == DSP_COMPILE
#ifdef XF_DYNAMIC
volatile u16 XF_MASK = XF_GENERAL;
#endif
#endif

