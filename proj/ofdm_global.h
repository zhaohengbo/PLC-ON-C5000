/*=================================================================================
 Filename:			ofdm_global.h

 Description:		global declarations for power line modem.

 Copyright (C) 2000 - 2002 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
========================================================================*/

/*==============================================================
	Global debug vars
===============================================================*/
#if (COMPILE_MODE == MEX_COMPILE) && (DEBUGIT >= DEBUG_IFFT)
	extern mxArray		*diagArrayIn;
	extern dCplxPtr		diagIn;
	extern mxArray		*diagArrayOut;
	extern dCplxPtr		diagOut;
	extern i16			diagCount;
#elif (COMPILE_MODE == MEX_COMPILE) &&  (DEBUGIT > DEBUG_NONE)
	extern mxArray		*diagArray;
	extern dCplxPtr		diag;
	extern i16			diagCount;
#endif


/*==============================================================
	Global vars
===============================================================*/
extern i16			recSignalArray[RX_CIRC_BUFFER_LEN];	// receive circular buffer
extern TXDATA		txSignalArray[TX_BUFFER_LEN];		// waveform we are building
 
extern u16			txUserDataArray[DATA_BUFFER_LEN]; 	// byte-wide buffer for user data
extern u16			rxUserDataArray[DATA_BUFFER_LEN]; 	// byte-wide buffer for user data
extern u16			rxUserDataCopy[NUM_USER_BYTES/2]; 	// shadow copy used to prevent overwriting
extern u16			CRCtableArray[256];					// CRC table buffer

extern iCplx  		fftArray[FFT_LEN];  				// working FFT buffer for calcSNR

extern iCplx		symbolArray[CARRIER_LEN];			// 2-bit symbols
extern i16			distanceArray[CARRIER_LEN*2];  		// distance metric buffer, also encode buffer
extern iCplx		phaseEqArray[CARRIER_LEN];			// freq equalization buffer

#if (SAVETRACE == TRUE) || (SAVESYMBOLS == TRUE)
	#if (SAVESYMBOLS == TRUE)
		#define	TRACE_BUFF_SIZE		(CARRIER_LEN*2*(DATA_FRAMES_PER_BLOCK*NUM_DATA_BLOCKS+1))
	#else
//		#define	TRACE_BUFF_SIZE		4095
		#define	TRACE_BUFF_SIZE		510
	#endif
	//extern u16	uTraceAGCFound;
	extern u16	uTraceIndex;
	extern u16	uTraceEnable;
	extern u16	uTraceData[TRACE_BUFF_SIZE];
	extern u16 	uTraceDataAddr;
	extern u16	uTraceDataLen;
#endif

extern const u16	scramblerLookup[127];				// scramble pattern
#if CARRIER_LEN <= 60
	extern const i16 	PreambleArray[CARRIER_LEN];		// known preamble phase, double-diff det
#else
	extern const i16 	PreambleArray[80];				// known preamble phase, double-diff det
#endif

extern const i16	ViterbiEncodeLookup[128];			// Hamming code
extern const i16	decodeTable[128];					// short Hamming code for decode

extern i16			*recSignal;

#if (AFE_TX_FORMAT == 24)	//  Transmit two 24-bit values
extern u32		IdleBuffArray[IDLE_BUFFER_LEN];
#else	// 	Transmit three 16-bit values
extern u16		IdleBuffArray[IDLE_BUFFER_LEN*AFE_SIZE];
#endif

extern u16*	uppParms16[30];
extern u32*	uppParms32[10];
extern u16	uScratch16;
extern u32	ulScratch32;

//---- global flag and control variables ----------------------------------------
extern modemStateType	modemState;		// an enum var that holds the state of the modem

extern agcStateType	agcState;
extern i16		preambleDetCount;
extern u16		frameAligned;  			// Frame alignment flag
extern volatile i16		SNRflag;
extern i16		AGCenable;
//extern i16		SNRcounter;
extern i16		prevSnrSample;			// used to time SNR calc

extern u16		AFEctrl1;
extern u16 		AFEctrl2;

extern u16		uLEDShadow;

extern u16		rxDMAoffset;	  	// simulates DMA pointer  //??? Not sure if this is correct for DSP_COMPILE mode

extern u16		uErrCnt[32];
extern u16		uErrCntAddr;

#define COMMAND_SIZE	(8)		// 8 16-bit words in a command from the host to the MASTER.
								// Don't make this bigger than the 16-byte FIFO without enabling
								// interrupts to get data out quickly.
extern u16		uUartCommand[COMMAND_SIZE];
extern u16		uBoard;
	#define BOARD_MASTER	(0)
	#define BOARD_SLAVE		(1)
extern PlcStateType		uPlcState;
extern UartStateType	uUartState;
extern u16 uUARTTarget;
extern u16 uUartDataIndex;
extern u16 uUartDataLength;
extern u16 uUartDataArray[25];
#define UART_BUFFER_SIZE 	(60)
extern u16 upUartBufferFifo[UART_BUFFER_SIZE];			// FIFO for uart characters.
extern volatile u16 uUartBufferPointerIn;
extern u16 uUartBufferPointerOut;
extern u16 uFlashLogStatus;
extern u16 uFlashWriteStatus;
	#define FLASH_STATUS_COMPLETE	  (0)
	#define FLASH_STATUS_PENDING	  (1)
	#define FLASH_STATUS_WRITING	  (2)
    #define FLASH_STATUS_ERASING      (3)
	#define FLASH_STATUS_TIMEOUT	  (4)
	//#define FLASh_STATUS_xxx		  (x)  // xxx > TIMEOUT are treated as errors.
    #define FLASH_STATUS_CHECKSUM     (5)
    #define FLASH_STATUS_ADDR_RANGE   (6)
    #define FLASH_STATUS_INVALID_REC  (7)
    #define FLASH_STATUS_WRITE_FAIL   (8)
    #define FLASH_STATUS_VERIFY_FAIL  (9)
    #define FLASH_STATUS_ERASE_FAIL   (10)
    #define FLASH_STATUS_NOT_ERASED   (11)
    #define FLASH_STATUS_NO_MODIFY    (12)
    #define FLASH_STATUS_MANUFACTURER (13)
    #define FLASH_STATUS_PART_NO      (14)
    #define FLASH_STATUS_IN_USE       (15)
extern u16 uFlashTarget;
	#define FLASH_TARGET_PROG	      (0)
	#define FLASH_TARGET_MSP	      (1)
	#define FLASH_TARGET_CODE_LIMIT   (1)  // maximum value for code space targets
	#define FLASH_TARGET_LOG          (2)
	#define FLASH_TARGET_BOOT         (3)
	#define FLASH_TARGET_COPY         (4)
    #define FLASH_TARGET_COUNT        (5)  // holds total number of targets available
// flash logging error codes
#define FLASH_LOG_NO_MEM              (1)
#define FLASH_LOG_TOO_BIG             (2)
#define FLASH_LOG_BUSY                (3)
#define FLASH_LOG_CORRUPT			  (4)
#define FLASH_LOG_SPACE_IN_USE        (5)
// flash access limit
#define FLASH_LOG_RECORD_MAX_SIZE     (16) // in 16 bit words

extern u16 uCodeVersion;
extern u16 uUartCommandIndex;
extern u16 uHostResponseStart;
extern u16 uHostResponseLength;
extern u16 uAckAfter485;
extern u16 upUartResponseBuffer[61];		// PLC packet = 128 bytes - addressing and checksum = 122 bytes = 61 words.
extern u16 uUartResponseIndex;
extern u16 uTransmitPacketReady;
extern u16 uFindSlaves;
extern u32 ulFindSlaveAddr;
extern u32 ulFindSlaveAddrMax;
extern u16 uSlaveFound;
extern u32 ulClosestSlaveAddr;
extern u32 ulMasterAddr;
extern u32 ulMyAddr;
extern u16 uAutoPoll;
extern u16 uAutoPollPause;
extern volatile u32 ulAutoPollCounter;
extern volatile u32 ulBerDelayCounter;
extern u32 ulBerDelay;
extern u16 uHoldOffPLC;
extern u16 uLastByteFlag;
extern u16 uCommandBlock;

#define T0_INTS_PER_SEC		(15625)					// 64 usec/int.	Correct for MIPS=150, SER_CLK_DIV=25 (250 kHz sample rate)
													// 		MIPS*1E6/SER_CLK_DIV/24/16
#define AUTO_POLL_COUNT		(1L*T0_INTS_PER_SEC)	// Auto poll at 1 Hz rate.

extern volatile u32 ulUartCounter;
extern volatile u32 ulFlashWriteCounter;
extern volatile u16 uFlashShow;
//extern u32 ulLastTxCounter;
extern volatile u32 ulLastRxCounter;
extern volatile u32 ulPlcResponseTimeoutCounter;
extern volatile u32 ulAutoPollResponseTimeoutCounter;
extern u32 ulBerCounter;
extern u32 ulNumBerPackets;
extern u16 uBerTestInProgress;
extern u16 uBerMissedPackets;
extern u16 uBerErrorCounter;
extern u32 ulBERSlaveAddr;

typedef struct BufferListStruct
{
	u16		target;			// destination for data UART_TARGET_HOST or UART_TARGET_EMETER
	u16		count;			// Number of words to send at this location.
	u16*	dataPtr;		// Pointer to start of data.
} BufferListStruct;

extern BufferListStruct UARTDataOut[SIZE_UART_OUT_ARRAY];


#if COMPILE_MODE == MEX_COMPILE
	extern i16	   	*rxDMApointer;	  	// simulates DMA pointer 
	extern const double AFE_RXPGA[11];	// models the gains of the TLFD240 or AFE1230 RX PGA	
#endif


