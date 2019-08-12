/*==============================================================
 Filename:			ofdm_functions.c

 Description:		This file contains the function prototypes used in the 
 					OFDM powerline modem.

 Copyright (C) 2002 - 2003 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
 ===============================================================*/


/*==============================================================
	Model_Main function prototypes
===============================================================*/

extern void 	sample_interupt();
extern void 	SaveTraceData(u16 uDat);

extern void 	postError( char *errTxt ); 
extern void 	PostErrorCode(u16	uErrCode, char *errFunc, char* errFile, char* errText );

/*==============================================================
	transmit function prototypes
===============================================================*/
extern void		postError( char *errTxt );

extern void		initCRCtable(u16 *CRCtable);
extern void		appendParityCheckBytes(u16 *userData, i16 numBytes);
extern void		scramble(u16 *userData, u16 userDataLen);
extern void 	interleaveSymbols(iCplx *symbols);				// hard

extern void 	viterbiEncodeFrame(iCplx *symbols);
//extern u16 	*viterbiEncodeData(iCplx *symbols, u16 *userData, u16 *viterbiState); 
extern void		mapSymbolPhase(iCplx *symbols);					// hard
extern void		assignSymbolPhase(iCplx *symbols);
extern void 	getPrePhases( iCplx *phase, const i16 *preamble );	// for double-diff phase det
extern void 	getSyncPhases( iCplx *phase, const i16 *preamble );	// for double-diff phase det
															
extern void		encodeSymbols(iCplx* symbols, u16 *userData );
//extern TXDATA*	makePreamble(TXDATA* pTxSignal );
//extern TXDATA*	makeDataFrames(TXDATA* pTxSignal, u16 *userData );
extern TXDATA* makePreamble(TXDATA* pTxSignal, i16 *prefix );
extern TXDATA* makeDataFrames(TXDATA* pTxSignal, i16 *postfix, u16 *pUserData );
 
/*==============================================================
	AGC function prototypes
===============================================================*/
#if COMPILE_MODE == MEX_COMPILE
	extern i16*	lookForPacket( dCplxPtr recAgc,  dCplxPtr rec );
#else
	extern i16*	lookForPacket(void);
#endif

//extern u16 		ddphase( iCplx *carriers, i16 carrierLen );
//SNR_CALC extern i16		getPowerFromFFT( iCplx *pBuf, int carrierLow, int carrierHigh);


/*==============================================================
	Receive function prototypes
===============================================================*/
extern i16		*frameAlign(iCplx *freqEq, i16 *recSignal);
extern u16		readDataFrames(u16 *userData, i16 *recSignal, iCplx *freqEq );
extern i16		decodeData(u16 *userData, iCplx *symbols);

//--- frame align functions ----------------------------------
extern i16		aveFFT( iCplx *aveCarrier, i16 *recSignal );
extern i16		ddAveFFT( iCplx *aveCarrier, i16 *recSignal );
extern i16		AlignPhaseCompare(i16 carLen, iCplx *prev, iCplx *curr );
extern i16		getFrameStartFromImpulse( iCplx *aveCarrier);
extern i16		getFreqEq( iCplx *freqEq, iCplx *aveCarrier, i16 numFramesAve );
extern u16		ddphase( const i16 *preamble, iCplx *carriers, i16 carrierLen );

//--- data detect functions ----------------------------------
extern i16		*dataFFT( iCplx *symbols, i16 *recSignal, iCplx *freqEq );
											
extern void 	viterbiEncodeInit(u16 *pUserData);
extern void 	initPathMemory( u16 *pUserData );
extern u32 		*viterbiStateLoop( MetricType *dist, i16 symbolCount );
extern void 	viterbiDecodeFrame( MetricType *distance, u16 numCarriers );
extern void 	flushPathMemory( void );
extern void 	flushPathMemoryV( void );

extern u16		compareParityCheckBytes(u16 *userData, i16 userDataLen);
extern i16		countErrors(u16 *userData);
	

/*==============================================================
	FFT function prototypes
===============================================================*/
extern i16* WrapRecPtr(i16* recPtr, i16 offset) ;
extern void		fillCarriers( iCplx  *fftBuf, const iCplx *carriers, i16 scale );

//#if COMPILE_MODE == MEX_COMPILE
//	#define cifft(x, len, scale)	mexfft( x, scale, "ifft")
//	#define cfft( x, len, scale) 	mexfft( x, scale, "fft")
//	#define rifft(x, len, scale)	mexrfft(x, scale, "ifft")
//	#define rfft( x, len, scale) 	mexrfft(x, scale, "fft")
//
//	extern void 	mexfft( i16 *x, i16 scale, char *cmdStr );  
//	extern void 	mexrfft( i16 *x, i16 scale, char *cmdStr );  
//#endif
#if COMPILE_MODE == MEX_COMPILE
	#define cifft(x, len, scale)	mexfft( x, len, scale, 'I', 'C')
	#define cfft( x, len, scale) 	mexfft( x, len, scale, 'F', 'C')
	#define rifft(x, len, scale)	mexfft( x, len, scale, 'I', 'R')
	#define rfft( x, len, scale) 	mexfft( x, len, scale, 'F', 'R')

	extern void 	mexfft(  i16 *x, i16 len, i16 scale, char cmdCh, char typeCh );  
#endif
// cifft and cfft defined in TI lib

extern i16		*circFFT( iCplx *fftBuffer, i16 *recSignal );
//extern i16		*circFFTshort( iCplx *fftBuffer, i16 *recSignal );
extern void	    backFFT( iCplx *fftBuffer, i16 *recSignal );


/*==============================================================
	MCBSP and DMA function prototypes
===============================================================*/
extern void DelayNus(u16 uN);
extern void DelayNms(u16 uN);
 
extern void InitMCBSP(u16 port_no,    u16 spcr1_ctrl, u16 spcr2_ctrl, u16 pcr_ctrl,
               u16 rcr1_ctrl,  u16 rcr2_ctrl,  u16 xcr1_ctrl,  u16 xcr2_ctrl,
               u16 srgr1_ctrl, u16 srgr2_ctrl, u16 mcr1_ctrl,  u16 mcr2_ctrl,
               u16 rcera_ctrl, u16 rcerb_ctrl, u16 xcera_ctrl, u16 xcerb_ctrl);
extern u16	ConfigureAFE (void);
extern void ConfigureMCBSPNormal(u16 uPortNum);
extern void ConfigureMCBSPDMA(u16 uPortNum);
extern u16 WriteMCBSP16(u16 uPortNum, u16 uMCBSPmsg);
extern u16 WriteMCBSP24(u16 uPortNum, u32 ulMCBSPmsg);
extern void DMARead2(i16 *upADCCmdListAddr, u16 uRawPosAddr, u16 uArraySize);
extern void DMAReadAFE(u16 uRxBuffAddr, u16 uArraySize);
#if (AFE_TX_FORMAT == 24)	// Send two 24-bit words
extern void DMAWriteAFE(u32 *upADCCmdListAddr, u16 uArraySize);
#else					// Send three 16-bit words
extern void DMAWriteAFE(u16 *upADCCmdListAddr, u16 uArraySize);
#endif

TXDATA* FillAFETxBuffCplx (TXDATA* pDest, iCplx* pSrc, u16 uCnt);
TXDATA* FillAFETxBuffI16 (TXDATA* pDest, i16* pSrc, u16 uCnt);
void FillIdleBuffer(void);
void WaitForTxBufferFree(TXDATA* uStart, u16 uCnt);
void WaitForRxBufferFree(i16* uStart, u16 uCnt);
//u16 ReadRxDMAPointer(void);
//u16 ReadTxDMAPointer(void);
i16* 	ReadRxDMAPointer(void);
TXDATA* ReadTxDMAPointer(void);
void WaitForLastFrame(TXDATA* uStart, u16 uCnt);
void JamDMAReloadRegs(TXDATA* uStart, u16 uCnt);

/*==============================================================
	diagnostic function prototypes
===============================================================*/
#if COMPILE_MODE == MEX_COMPILE
	void	diagData( void *array, u16 len, char *matName, diagType flag);
#else
	#define	diagData(a,b,c,d)			// Don't generate any code for this in DSP mode
#endif

/*==============================================================
	Main loop function prototypes
===============================================================*/
void CheckPLC(void);
void ReceivePLC(void);
void ReceiveUART(void);
void SendPLC(void);
void SendUART(void);
void AutoPoll(void);
void BERTest(void);
void CheckForClosestSlave(void);
void FindSlaves(void);
void TransmitPacket(void);
void CheckTimeouts(void);
void CheckFlash(void);
void InitTesterParms(void);

/*==============================================================
	Command function prototypes
===============================================================*/
void ProcessMasterCommand(void);
void ProcessSlaveCommand(void);
void ResetEMeter(void);

/*==============================================================
	Serial port / UART function prototypes
===============================================================*/
void ConfigureUART(void);
void InitializeUARTArray(void);
s16 WriteUARTValue(u16 target, u16 value);
s16 WriteUART(u16 target, s16 count, u16* dataPtr);
s16 WriteUARTString(u16* msgString);
void SelectUARTTarget(u16 UARTTarget);
void SelectRS485Direction(u16 RS485Dir);

/*==============================================================
	Flash prototypes needed outside of flash.c
===============================================================*/
#include "Flash.h"

//===============================================================
//	Status LED controls
//===============================================================
u16	ConfigureLEDMCBSP (void);
u16 WriteMCBSP16(u16 uPortNum, u16 uMCBSPmsg);
void TurnLEDsOn(u16 uMask);
void TurnLEDsOff(u16 uMask);
void AssignLEDs(u16 uMask);

