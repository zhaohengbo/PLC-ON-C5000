//==========================================================================================
// Filename:		AFE.c
//
// Description:		Functions related to the AFE-1230 Analog Front-End interface.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

// TI specific defines
#include "ofdm_modem.h"
#include "ofdm_datatypes.h"
#include "dma.h"
#include "mcbsp54.h"


#define	INTERPOLATE	TRUE		// Interpolate halfway between values when transmitting in SEND_TWICE mode.
#if (INTERPOLATE == TRUE)		// Make this semi-global, for use by FillAFETxBuff... functions.
	i16	prevTemp = 0x8000;
#endif

#define CONTENTS_OF32(addr) \
        (*((volatile unsigned long*)(addr)))

#define REG_READ32(addr) \
        (CONTENTS_OF32(addr))

#define REG_WRITE32(addr,val) \
        (CONTENTS_OF32(addr) = (val))

//------------------------------------------------------------------------------------------
// MCBSP Serial Clock Setup
//------------------------------------------------------------------------------------------
//	MCBSP clock rate:  		MCLK = MIPS / SER_CLK_DIV
//  AFE Sample Rate:   		Fs = MCLK / 24
//  MCBSP Frame Sync Rate:	FSR = MCLK/48
//  For MIPS=150  and SER_CLK_DIV=20  ==> MCLK = 7.5 MHz,  Fs = 312.5 kHz, FSR = 156.25 kHz
//															0dB	-.2dB	-3dB	
//	SER_CLK_DIV	MCLK(MHz)	Fs(kHz)		FSR			.25*Fs	.28*Fs	.31*Fs	.38*Fs	.5*Fs
//	1			150.00		6250.0		3125.0		1562.5	1750.0	1937.5	2375.0	3125.0
//	2			75.00		3125.0		1562.5		781.3	875.0	968.8	1187.5	1562.5
//	3			50.00		2083.3		1041.7		520.8	583.3	645.8	791.7	1041.7
//	4			37.50		1562.5		781.3		390.6	437.5	484.4	593.8	781.3
//	5			30.00		1250.0		625.0		312.5	350.0	387.5	475.0	625.0
//	6			25.00		1041.7		520.8		260.4	291.7	322.9	395.8	520.8
//	7			21.43		892.9		446.4		223.2	250.0	276.8	339.3	446.4
//	8			18.75		781.3		390.6		195.3	218.8	242.2	296.9	390.6
//	9			16.67		694.4		347.2		173.6	194.4	215.3	263.9	347.2
//	10			15.00		625.0		312.5		156.3	175.0	193.8	237.5	312.5
//	11			13.64		568.2		284.1		142.0	159.1	176.1	215.9	284.1
//	12			12.50		520.8		260.4		130.2	145.8	161.5	197.9	260.4
//	13			11.54		480.8		240.4		120.2	134.6	149.0	182.7	240.4
//	14			10.71		446.4		223.2		111.6	125.0	138.4	169.6	223.2
//	15			10.00		416.7		208.3		104.2	116.7	129.2	158.3	208.3
//	16			9.38		390.6		195.3		97.7<<<	109.4	121.1	148.4	195.3
//	17			8.82		367.6		183.8		91.9	102.9	114.0	139.7	183.8
//	18			8.33		347.2		173.6		86.8	97.2<<<	107.6	131.9	173.6
//	19			7.89		328.9		164.5		82.2	92.1	102.0	125.0	164.5
//	20			7.50		312.5		156.3		78.1	87.5	96.9<<<	118.8	156.3
//	21			7.14		297.6		148.8		74.4	83.3	92.3	113.1	148.8
//	22			6.82		284.1		142.0		71.0	79.5	88.1	108.0	142.0
//	23			6.52		271.7		135.9		67.9	76.1	84.2	103.3	135.9
//	24			6.25		260.4		130.2		65.1	72.9	80.7	99.0	130.2
//	25			6.00		250.0		125.0		62.5	70.0	77.5	95.0<<<	125.0
//	26			5.77		240.4		120.2		60.1	67.3	74.5	91.3	120.2
//	27			5.56		231.5		115.7		57.9	64.8	71.8	88.0	115.7
//	28			5.36		223.2		111.6		55.8	62.5	69.2	84.8	111.6
//	29			5.17		215.5		107.8		53.9	60.3	66.8	81.9	107.8
//	30			5.00		208.3		104.2		52.1	58.3	64.6	79.2	104.2
//	31			4.84		201.6		100.8		50.4	56.5	62.5	76.6	100.8
//	32			4.69		195.3		97.7		48.8	54.7	60.5	74.2	97.7
//	33			4.55		189.4		94.7		47.3	53.0	58.7	72.0	94.7<<<
//	34			4.41		183.8		91.9		46.0	51.5	57.0	69.9	91.9
//	35			4.29		178.6		89.3		44.6	50.0	55.4	67.9	89.3
//	36			4.17		173.6		86.8		43.4	48.6	53.8	66.0	86.8
//	37			4.05		168.9		84.5		42.2	47.3	52.4	64.2	84.5
//	38			3.95		164.5		82.2		41.1	46.1	51.0	62.5	82.2
//	39			3.85		160.3		80.1		40.1	44.9	49.7	60.9	80.1
//	40			3.75		156.3		78.1		39.1	43.8	48.4	59.4	78.1

#define	SER_CLK_DIV					25		// 25 = 125 kHz
#if COMPILE_MODE == DSP_COMPILE
   asm("SER_CLK_DIV			.set	25");
#endif

#define	SER_CLK_DIV_SLOW			25
#if COMPILE_MODE == DSP_COMPILE
   asm("SER_CLK_DIV_SLOW	.set	25");
#endif
 
// Serial clock sample rate generator setup.
#define	SRGR1_CTRL_SLOW	((1-1)<<FWID) | ((SER_CLK_DIV_SLOW-1)<<CLKGDV)				
#define	SRGR1_CTRL_FAST	((1-1)<<FWID) | ((SER_CLK_DIV-1)<<CLKGDV)				

//-------------------------------------------------------------------------------------
// Generate periodic Frame-Sync pulses 48 CLKG periods apart
// MCBSP Sample Rate Generator Register 2
// Serial clock free-running, derived from CPU clock
//-------------------------------------------------------------------------------------
#define	SRGR2_CTRL_DMA	((GSYNC_OFF<<GSYNC) | (CLKS_POL_FALLING<<CLKSP) 			\
					  | (CLKSM_CPU<<CLKSM) |(FSX_FSG<<FSGM) |  ((48-1)<<FPER))


// MCBSP Sample Rate Generator Register 2
// Serial clock free-running, derived from CPU clock, 
// Frame-sync pulse sent due to DXR->XSR copy, with a width of 1 CLKG period
#define	SRGR2_CTRL_NORM	((GSYNC_OFF<<GSYNC) | (CLKS_POL_FALLING<<CLKSP) 				\
						| (CLKSM_CPU<<CLKSM) |(FSX_DXR_TO_XSR<<FSGM) |  ((1-1)<<FPER))

#define	MAX_MCBSP_WAIT	(18*SER_CLK_DIV)	// Max wait = 18 serial clock ticks		

 

//==========================================================================================
// Function:		DelayNus(N)
//
// Description: 	This function delays N microseconds.  It locks out interrupts for 1 us
//					at a time, so use cautiously in time-critical routines.
//
// Revision History:
//==========================================================================================
void DelayNus(u16 uN)
{
	u16		i;	// loop counter
	for (i=0; i<uN; i++)
	{
		RPTNOP(MIPS-10);	// Delay 1 us per loop (locks out interrupts)
	} 
}      

  
//==========================================================================================
// Function:		DelayNms(N)
//
// Description: 	This function delays N milliseconds.  It locks out interrupts for 1 us
//					at a time, so use cautiously in time-critical routines.
//
// Revision History:
//==========================================================================================
void DelayNms(u16 uN)
{
	u16		i;	// loop counter
	for (i=0; i<uN; i++)
	{
		DelayNus(1000);		// Delay 1 ms per loop 
	} 
}      


#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		InitMCBSP()
//
// Description: 	This function initializes a Multi-Channel Buffered Serial Port (MCBSP).
//					It replaces the mcbsp_init function included in mcbsp54.h which did not
//					properly initialize the SPCR1 and SPCR2 registers.
//
// Revision History:
//==========================================================================================
void InitMCBSP(u16 port_no,    u16 spcr1_ctrl, u16 spcr2_ctrl, u16 pcr_ctrl,
               u16 rcr1_ctrl,  u16 rcr2_ctrl,  u16 xcr1_ctrl,  u16 xcr2_ctrl,
               u16 srgr1_ctrl, u16 srgr2_ctrl, u16 mcr1_ctrl,  u16 mcr2_ctrl,
               u16 rcera_ctrl, u16 rcerb_ctrl, u16 xcera_ctrl, u16 xcerb_ctrl)
{
	#define	MCBSP_PORT0	0
	#define	MCBSP_PORT1	1
	#define	MCBSP_PORT2	2
	
	switch	(port_no)
	{
	case	MCBSP_PORT0:
		/****************************************************************/
		/* Place port in reset - setting XRST & RRST to 0               */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SPCR1_SUBADDR, 0x0000);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SPCR2_SUBADDR, 0x0000);

		/****************************************************************/
		/* Set values of all control registers                          */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SPCR1_SUBADDR, (spcr1_ctrl & 0xFFFE));
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SPCR2_SUBADDR, (spcr2_ctrl & 0xFFFE));
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, PCR_SUBADDR,   pcr_ctrl); 
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, RCR1_SUBADDR,  rcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, RCR2_SUBADDR,  rcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, XCR1_SUBADDR,  xcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, XCR2_SUBADDR,  xcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SRGR1_SUBADDR, srgr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SRGR2_SUBADDR, srgr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, MCR1_SUBADDR,  mcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, MCR2_SUBADDR,  mcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, RCERA_SUBADDR, rcera_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, RCERB_SUBADDR, rcerb_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, XCERA_SUBADDR, xcera_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, XCERB_SUBADDR, xcerb_ctrl);

		/****************************************************************/
		/* Release clock generator from reset and wait 2 clock cycles   */
		/****************************************************************/
		MCBSP_SUBREG_BITWRITE(MCBSP_PORT0, SPCR2_SUBADDR, GRST, GRST_SZ, 1);
		RPTNOP(2*100/20);	// Wait 2 cycles on 100 MIPS DSP & 20 MHz serial clock

		/****************************************************************/
		/* Release receiver and transmitter from reset                  */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SPCR1_SUBADDR, spcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT0, SPCR2_SUBADDR, spcr2_ctrl);
		break;

	case	MCBSP_PORT1:
		/****************************************************************/
		/* Place port in reset - setting XRST & RRST to 0               */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SPCR1_SUBADDR, 0x0000);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SPCR2_SUBADDR, 0x0000);

		/****************************************************************/
		/* Set values of all control registers                          */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SPCR1_SUBADDR, (spcr1_ctrl & 0xFFFE));
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SPCR2_SUBADDR, (spcr2_ctrl & 0xFFFE));
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, PCR_SUBADDR,   pcr_ctrl); 
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, RCR1_SUBADDR,  rcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, RCR2_SUBADDR,  rcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, XCR1_SUBADDR,  xcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, XCR2_SUBADDR,  xcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SRGR1_SUBADDR, srgr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SRGR2_SUBADDR, srgr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, MCR1_SUBADDR,  mcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, MCR2_SUBADDR,  mcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, RCERA_SUBADDR, rcera_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, RCERB_SUBADDR, rcerb_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, XCERA_SUBADDR, xcera_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, XCERB_SUBADDR, xcerb_ctrl);

		/****************************************************************/
		/* Release clock generator from reset and wait 2 clock cycles   */
		/****************************************************************/
		MCBSP_SUBREG_BITWRITE(MCBSP_PORT1, SPCR2_SUBADDR, GRST, GRST_SZ, 1);
		RPTNOP(2*100/20);	// Wait 2 cycles on 100 MIPS DSP & 20 MHz serial clock

		/****************************************************************/
		/* Release receiver and transmitter from reset                  */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SPCR1_SUBADDR, spcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT1, SPCR2_SUBADDR, spcr2_ctrl);
		break;

	case	MCBSP_PORT2:
		/****************************************************************/
		/* Place port in reset - setting XRST & RRST to 0               */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SPCR1_SUBADDR, 0x0000);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SPCR2_SUBADDR, 0x0000);

		/****************************************************************/
		/* Set values of all control registers                          */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SPCR1_SUBADDR, (spcr1_ctrl & 0xFFFE));
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SPCR2_SUBADDR, (spcr2_ctrl & 0xFFFE));
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, PCR_SUBADDR,   pcr_ctrl); 
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, RCR1_SUBADDR,  rcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, RCR2_SUBADDR,  rcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, XCR1_SUBADDR,  xcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, XCR2_SUBADDR,  xcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SRGR1_SUBADDR, srgr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SRGR2_SUBADDR, srgr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, MCR1_SUBADDR,  mcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, MCR2_SUBADDR,  mcr2_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, RCERA_SUBADDR, rcera_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, RCERB_SUBADDR, rcerb_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, XCERA_SUBADDR, xcera_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, XCERB_SUBADDR, xcerb_ctrl);

		/****************************************************************/
		/* Release clock generator from reset and wait 2 clock cycles   */
		/****************************************************************/
		MCBSP_SUBREG_BITWRITE(MCBSP_PORT2, SPCR2_SUBADDR, GRST, GRST_SZ, 1);
		RPTNOP(2*100/20);	// Wait 2 cycles on 100 MIPS DSP & 20 MHz serial clock

		/****************************************************************/
		/* Release receiver and transmitter from reset                  */
		/****************************************************************/
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SPCR1_SUBADDR, spcr1_ctrl);
		MCBSP_SUBREG_WRITE(MCBSP_PORT2, SPCR2_SUBADDR, spcr2_ctrl);
		break;
	}

	return;
}
#endif


#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		ConfigureAFE()
//
// Description: 	This function configures the ADE1230 Analog Front-End  as well as the
//					 MCBSP ports and the DMA channels which talk to them.  
//					It should be called during board initialization.
//					The DMA channel gets configured to read the correct number of channels
//                  (QUAD_CHANNELS +LOCAL_CHANNELS) the correct number of times 
//					(OVERSAMPLE_RATE).
//
//	Assumptions:	* There are two ADC MCBSP busses: one for local sensors, one for remote.
//					* All the A/D converters on the local bus are the same model.
//					* All the A/D converters on the remote bus are the same model.
//					* The remote and local ADCs are similar enough that they can use the 
//					  same MCBSP settings but slightly different ADC settings.
// Revision History:
//==========================================================================================
u16	ConfigureAFE (void)
{
	// ACHTUNG!  Please don't move these #defines to a .h file.  I used them here instead
	// of 'const' statements because they save code space.
	// MCBSP = Multi-Channel Buffered Serial Port

#if (AFE_RX_FORMAT == 24)	// Receive two 24-bit words
	// MCBSP Serial Port Control Register 1
	// Enable receive port with standard settings except left-justified.
	#define SPCR1_CTRL	( (DLB_DISABLE<<DLB) | (RXJUST_LJZF<<RJUST) 					\
						| (CLK_STOP_DISABLED<<CLKSTP) | (DX_ENABLE_OFF<<DXENA)			\
						| (ABIS_DISABLE<<ABIS) | (INTM_RDY<<RINTM) | (RX_ENABLE<<RRST)	)
#else
	// MCBSP Serial Port Control Register 1
	// Enable receive port with standard settings.
	#define SPCR1_CTRL	( (DLB_DISABLE<<DLB) | (RXJUST_RJZF<<RJUST) 					\
						| (CLK_STOP_DISABLED<<CLKSTP) | (DX_ENABLE_OFF<<DXENA)			\
						| (ABIS_DISABLE<<ABIS) | (INTM_RDY<<RINTM) | (RX_ENABLE<<RRST)	)
#endif

	// MCBSP Serial Port Control Register 2
	// Enable transmit port with standard settings.
	#define	SPCR2_CTRL	( (SP_FREE_ON<<FREE) | (SOFT_DISABLE<<SOFT)						\
						| (FRAME_GEN_ENABLE<<FRST) | (SRG_ENABLE<<GRST) 				\
						| (INTM_RDY<<XINTM) | (RX_ENABLE<<XRST)							)

	// MCBSP Pin Control Register
	// Configure pins as serial ports instead of GPIOs, 
	// Generate transmit frame-sync on the XFS pin using internal sample rate generator.
	// Receive frame-sync is input from RFS pin (which is wired to XFS on board).
	#define	PCR_CTRL	( (IO_DISABLE<<13)     | (IO_DISABLE<<12) 						\
		  				| (FSYNC_MODE_INT<<11) | (FSYNC_MODE_EXT<<10)					\
		  			 	| (CLK_MODE_INT<<9)    | (CLK_MODE_EXT<<8) 						\
    	  			 	| (FSYNC_POL_HIGH<<3)  | (FSYNC_POL_HIGH<<2)					\
		  			 	| (CLKX_POL_FALLING<<1)| (CLKR_POL_FALLING<<0)					)


#if (AFE_RX_FORMAT == 24)	// Receive two 24-bit words
	// MCBSP Receive Control Register 1
	// 2 words/frame, 24-bit word	
	#define	RCR1_CTRL	((2-1)<<RFRLEN1 | (WORD_LENGTH_24<<RWDLEN1)		)
#else					// Receive three 16-bit words
	#if (SEND_TWICE == TRUE)
		// MCBSP Receive Control Register 1
		// 1 words/frame, 16-bit word
		#define	RCR1_CTRL	( (1-1)<<RFRLEN1 | (WORD_LENGTH_16<<RWDLEN1)	)
	#else
		// MCBSP Receive Control Register 1
		// 3 words/frame, 16-bit words
		#define	RCR1_CTRL	( (3-1)<<RFRLEN1 | (WORD_LENGTH_16<<RWDLEN1)	)
	#endif
#endif

	// MCBSP Receive Control Register 2
	// Standard
	#define	RCR2_CTRL	( (SINGLE_PHASE<<RPHASE) | (0<<RFRLEN2) | (0<<RWDLEN2) 			\
    					| (NO_COMPAND_MSB_1ST<<RCOMPAND) | (NO_FRAME_IGNORE<<RFIG) 		\
    					| (DATA_DELAY2<<RDATDLY)										)


#if (AFE_TX_FORMAT == 24)	// Send two 24-bit words
	// MCBSP Transmit Control Register 1
	// 2 words/frame, 24-bit word	
	#define	XCR1_CTRL	((2-1)<<XFRLEN1 | (WORD_LENGTH_24<<XWDLEN1)		)
#else					// Send three 16-bit words
	// MCBSP Transmit Control Register 1
	// 3 words/frame, 16-bit words
	#define	XCR1_CTRL	( (3-1)<<XFRLEN1 | (WORD_LENGTH_16<<XWDLEN1)	)
#endif

    // MCBSP Transmit Control Register 2
	// Standard
	#define	XCR2_CTRL	( (SINGLE_PHASE<<XPHASE) | (0<<XFRLEN1) | (0<<XWDLEN1) 			\
    					| (NO_COMPAND_MSB_1ST<<XCOMPAND) | (NO_FRAME_IGNORE<<XFIG) 		\
    					| (DATA_DELAY1<<XDATDLY)										)
 
	// MCBSP Sample Rate Generator Register 1
	// Frame-sync pulse 1 CLKG period wide, set Serial clock rate 
	#define	SRGR1_CTRL	((2-1)<<FWID) | ((SER_CLK_DIV-1)<<CLKGDV)				

     						
	//#define SRGR2_CTRL_NORM = defined in sensor.h
	//#define SRGR2_CTRL_DMA= defined in sensor.h	
				

	// This next group of registers are all set to zero because we are not using the 
	// Multi-Channel features of the MCBSP.  
	// We will instead use the DMA sequencer to scan through the ADC input channels.

	// MCBSP Multichannel Register 1
	#define	MCR1_CTRL	0
    			 
	// MCBSP Multichannel Register 2
	#define	MCR2_CTRL	0
    	
    // MCBSP Receive Channel Enable Register Part A			 
	#define	RCERA_CTRL	0
    
    // MCBSP Receive Channel Enable Register Part B				
	#define	RCERB_CTRL	0
    
    // MCBSP Transmit Channel Enable Register Part A			
	#define	XCERA_CTRL	0

    // MCBSP Transmit Channel Enable Register Part B				
	#define	XCERB_CTRL	0

	//--------------------------------------------------------------------------------------

	u16		uPortNum = AFE_MCBSP;		//
	u16		uStatus = SUCCESS;			// Return value.

 	#ifdef FLOW
		LOG_printf(&Trace, "Entered ConfigureAFE()");
	#endif


	// Initialize the MCBSP port which is connected to the AFE
	InitMCBSP(uPortNum,   SPCR1_CTRL,      SPCR2_CTRL,   PCR_CTRL,
			  RCR1_CTRL,  RCR2_CTRL,        XCR1_CTRL,  XCR2_CTRL,
			  SRGR1_CTRL, SRGR2_CTRL_NORM,  MCR1_CTRL,  MCR2_CTRL,
			  RCERA_CTRL, RCERB_CTRL,      XCERA_CTRL, XCERB_CTRL);


	// Shut off frame sync generation (FS)
	MCBSP_SUBREG_BITWRITE(uPortNum, SPCR2_SUBADDR, FRST, FRST_SZ, 0);



	// ---------- DMA Configuration ---------------
	SET_REG(DMPREC, 0);			//!!!TEMP Disable all DMA channels before configuring them
	DMA_reset();				// Reset all DMA registers.

//NORMAL 	// All channels are disabled for now.  DMA will keep running when emulator is paused.
//NORMAL 	// DMA Channels 4 and 2 have high priority.
//NORMAL 	// Interrupts will be directed to HWI_SINT 6 through 13.
//NORMAL 	#define DMPREC_INIT (	(RUN<<DMA_FREE) | (LO<<DPRC5) | (HI<<DPRC4) | (LO<<DPRC3) \
//NORMAL 						 					| (HI<<DPRC2) | (LO<<DPRC1) | (LO<<DPRC0) \
//NORMAL 					   | (INTOSEL2<<INTSEL) | (DIS<<DE5)  |  (DIS<<DE4) | (DIS<<DE3)  \
//NORMAL 											| (DIS<<DE2)  |  (DIS<<DE1) | (DIS<<DE0) )	  

	// All channels are disabled for now.  DMA will keep pause when emulator is paused. <<<!!!DEBUG
	// DMA Channels 4 and 2 have high priority.
	// Interrupts will be directed to HWI_SINT 6 through 13.
	#define DMPREC_INIT (	(STOP<<DMA_FREE) | (LO<<DPRC5) | (HI<<DPRC4) | (LO<<DPRC3) \
						 					| (HI<<DPRC2) | (LO<<DPRC1) | (LO<<DPRC0) \
					   | (INTOSEL2<<INTSEL) | (DIS<<DE5)  |  (DIS<<DE4) | (DIS<<DE3)  \
											| (DIS<<DE2)  |  (DIS<<DE1) | (DIS<<DE0) )	  

	SET_REG(DMPREC, DMPREC_INIT);

	return(uStatus);
}
#endif



#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		ConfigureMCBSPDMA()
//
// Description: 	Configure the Multi-Channel Buffered Serial Port for DMA operations.
//					Generates a single frame-sync pulse per transmitted word.
//
// Revision History:
//==========================================================================================
void ConfigureMCBSPDMA(u16 uPortNum)
{
	RPTNOP(24*SER_CLK_DIV_SLOW);	// Wait 24 cycles of slow serial clock to ensure all 
									// previous transmissions are complete.  

	//-------------------------------------------------------------------------------------
	// Configure the MCBSP clocks and frame generators for DMA transmission mode.
	//-------------------------------------------------------------------------------------

	//Place transmit port in reset and turn off clock generator and frame sync generator.
	MCBSP_SUBREG_WRITE(uPortNum, SPCR2_SUBADDR, 0x0000);

	// Use the fast serial clock for these transfers
	MCBSP_SUBREG_WRITE(uPortNum, SRGR1_SUBADDR, SRGR1_CTRL_FAST);

	// Configure the MCBSP to transmit frame syncs due to DXR load
	MCBSP_SUBREG_WRITE(uPortNum, SRGR2_SUBADDR, SRGR2_CTRL_DMA);

	// Release clock generator from reset and wait 2 clock cycles -----
	MCBSP_SUBREG_BITWRITE(uPortNum, SPCR2_SUBADDR, GRST, GRST_SZ, 1);  //!!!TEMP!!!
	RPTNOP(2*SER_CLK_DIV);	// Wait 2 cycles of fast serial clock

	// Release transmitter from reset, enable frame sync.                 
	MCBSP_SUBREG_WRITE(uPortNum, SPCR2_SUBADDR, SPCR2_CTRL); 
	RPTNOP(2*SER_CLK_DIV);	// Wait 2 cycles of fast serial clock ??Is this Needed??

	return;
}
#endif  
  	     
  
#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		DMAWriteAFE()
//
// Description: 	Use the DMA engine and MCBSP to transmit a block of Tx data to the AFE.
//
// Revision History:
//==========================================================================================
#if (AFE_TX_FORMAT == 24)	// Send two 24-bit words
void DMAWriteAFE(u32 *upADCCmdListAddr, u16 uArraySize)
#else					// Send three 16-bit words
void DMAWriteAFE(u16 *upADCCmdListAddr, u16 uArraySize)
#endif
{
//!!! Move to Regs54xx.h or DMA.h
#define DBLW_SNGL	0
#define DBLW_DBL	1


#define	AFE_DMA_MASK 	(EN<<DE5)	// Enable DMA channel 5


	//----- Disable the DMA Engine while we configure it -----
	DMPREC &= ~(AFE_DMA_MASK); 

	// Configure MCBSP for automatic periodic Frame Sync generation

	/****************************************************************/
	/* Place ports in reset - setting XRST & RRST to 0              */
	/*  and turning off clock generator and frame sync generator    */
	/****************************************************************/

	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR1_SUBADDR, 0x0000);
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR2_SUBADDR, 0x0000);

 	// ----- Set the serial clock speed for these transfers -----
 	MCBSP_SUBREG_WRITE(AFE_MCBSP, SRGR1_SUBADDR, SRGR1_CTRL_FAST);

	// ----- Configure the MCBSP to transmit periodic frame syncs automatically
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SRGR2_SUBADDR, SRGR2_CTRL_DMA);

	// ----- Configure the DMA channel which sends commands to the ADC. -----
//	// Generate an interrupt to the DSP at the end of the block transfer.
//	// Post-increment source from data space, leave data space destination unmodified.
//	// Generate an interrupt to the DSP at the end of the block transfer.
//	#define DMA_CONTROL_WRITE_ADC2	( (0<<AUTOINIT) | (1<<DINM) | (0<<IMOD) | (0<<CTMOD) \
//									| (1<<SIND)  	| (1<<DMS)  | (0<<DIND) | (1<<DMD) )


	// Post-increment source from data space, leave data space destination unmodified.
	// Auto-reload at end of block.  Generate interrupts at end of frame and end of block.
	#define DMA_CONTROL_WRITE_ADC2	( (1<<AUTOINIT) | (1<<DINM) | (1<<IMOD) | (0<<CTMOD) \
									| (1<<SIND)  	| (1<<DMS)  | (0<<DIND) | (1<<DMD) )
									
	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGSA_SUBADDR, (u32)(upADCCmdListAddr));//  DMA Global Source Address
	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGCR_SUBADDR, uArraySize-1);			//  DMA Global Element Count
	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGFR_SUBADDR, (1-1));					//  DMA Global Frame Count	


#if AFE_MCBSP == 0
	#define DMASYNC_XEVT	DMASYNC_XEVT0
#elif AFE_MCBSP == 1
	#define DMASYNC_XEVT	DMASYNC_XEVT1
#else
	#define DMASYNC_XEVT	DMASYNC_XEVT2
#endif

#if (AFE_TX_FORMAT == 24)	// Send two 24-bit words
 	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGDA_SUBADDR, DXR2_ADDR(AFE_MCBSP));	//  DMA Global Destination Address
	// Set up DMA to synchronize transfer with MCBSP transmit complete. 
	// Send 1 frame in double-word mode.		
	#define DMA_SYNC_WRITE_ADC2 ((DMASYNC_XEVT<<DSYN)|(DBLW_DBL<<DBLW)|((1-1)<<FRAMECOUNT)) 

	// Set up DMA Tx Channel to write commands to the ADC from an ADC command list.
	DMA_init(  	DMA_CHANNEL5,				// u16 channel 
           		(u32)(upADCCmdListAddr+1),	// u16 source= 2nd entry in ADC Command List
           		DXR2_ADDR(AFE_MCBSP),		// u16 destination = ADC MCBSP Transmit reg
           		(uArraySize-1-1),			// u16 count (extra -1 because of manual tx of 1st element) 
          		DMA_SYNC_WRITE_ADC2,		// u16 frame_sync = transmit complete event
				DMA_CONTROL_WRITE_ADC2);	// u16 control_mode
	
		                                                                                                                                                                                   
#else					// Send three 16-bit words
	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGDA_SUBADDR, DXR1_ADDR(AFE_MCBSP));		//  DMA Global Destination Address

	// Set up DMA to synchronize transfer with MCBSP transmit complete. 
	// Send 1 frame in single-word mode.
	#define DMA_SYNC_WRITE_ADC2 ((DMASYNC_XEVT<<DSYN)|(DBLW_SNGL<<DBLW)|((1-1)<<FRAMECOUNT)) 

	// Set up DMA Tx Channel to write commands to the ADC from an ADC command list.
	DMA_init(  	DMA_CHANNEL5,				// u16 channel 
           		(u16)(upADCCmdListAddr+1),	// u16 source= 2nd entry in ADC Command List
           		DXR1_ADDR(AFE_MCBSP),		// u16 destination = ADC MCBSP Transmit reg
           		(uArraySize-1-1),			// u16 count (extra -1 because of manual tx of 1st element) 
          		DMA_SYNC_WRITE_ADC2,		// u16 frame_sync = transmit complete event
				DMA_CONTROL_WRITE_ADC2);	// u16 control_mode

#endif

                                                                                          
	//-------------------------------------------------------------------------------------
	// ----- Release clock generator from reset and wait 2 clock cycles -----
	//-------------------------------------------------------------------------------------
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, GRST, GRST_SZ, 1);  //!!!TEMP!!!
	RPTNOP(2*SER_CLK_DIV_SLOW);	// Wait 2 cycles of slow serial clock

	//-------------------------------------------------------------------------------------
	// Release receiver and transmitter from reset.                 
	//-------------------------------------------------------------------------------------
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR1_SUBADDR, SPCR1_CTRL);
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR2_SUBADDR, (SPCR2_CTRL & (~(1<<FRST)))); // Turn on SPCR2 except for FRST

	//-------------------------------------------------------------------------------------
	//----- Arm the DMA Engine -----
	//-------------------------------------------------------------------------------------
	DMPREC |= (AFE_DMA_MASK); 

	//-------------------------------------------------------------------------------------
    //----- Load data transmit register with first word to start transmission -----
	//-------------------------------------------------------------------------------------
	REG_WRITE32(DXR2_ADDR(AFE_MCBSP), upADCCmdListAddr[0]); // Send first entry in CmdList to ADC 

	//-------------------------------------------------------------------------------------
	// Turn on frame generation, which should start sending word we just loaded in DXR1.
	//-------------------------------------------------------------------------------------
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 1); 

	//-------------------------------------------------------------------------------------
	// The DMA transfer is now running.  We can now return to the main code and wait for
	// the transfer-complete interrupt from the DMA engine.
   	//-------------------------------------------------------------------------------------


//----------------- Start DMAXfer
	return;
}
#endif 
  

#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		DMAReadAFE()
//
// Description: 	Use the DMA engine and MCBSP to quickly read the AFE Rx signal.
//
// Revision History:
//==========================================================================================
void DMAReadAFE(u16 uRxBuffAddr, u16 uArraySize)
{
#define DBLW_SNGL	0
#define DBLW_DBL	1
#define	AFE_RX_DMA_MASK 	(EN<<DE4)	// Enable DMA channel 4(Rx Data)

	//----- Disable the DMA Engine while we configure it -----
	DMPREC &= ~(AFE_RX_DMA_MASK); 

	// Configure MCBSP for automatic periodic Frame Sync generation

	/****************************************************************/
	/* Place ports in reset - setting XRST & RRST to 0              */
	/*  and turning off clock generator and frame sync generator    */
	/****************************************************************/
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR1_SUBADDR, 0x0000);
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR2_SUBADDR, 0x0000);

 	// ----- Set the serial clock speed for these transfers -----
 	MCBSP_SUBREG_WRITE(AFE_MCBSP, SRGR1_SUBADDR, SRGR1_CTRL_FAST);

	// ----- Configure the MCBSP to transmit periodic frame syncs automatically
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SRGR2_SUBADDR, SRGR2_CTRL_DMA);

	// Generate an interrupt to the DSP at the end of the block transfer.
	// Same source address from data space, post-increment destination address in data space.
	// Use ABU (auto-buffering) mode.
	#define DMA_CONTROL_READ_ADC2	( (0<<AUTOINIT) | (1<<DINM) | (0<<IMOD) | (1<<CTMOD) \
									| (0<<SIND)     | (1<<DMS)  | (1<<DIND) | (1<<DMD) )

#if AFE_MCBSP == 0
	#define DMASYNC_REVT	DMASYNC_REVT0
#elif AFE_MCBSP == 1
	#define DMASYNC_REVT	DMASYNC_REVT1
#else
	#define DMASYNC_REVT	DMASYNC_REVT2
#endif

#if (AFE_RX_FORMAT == 24)	// Read two 24-bit words
	// Set up DMA to synchronize transfer with MCBSP receive complete. 
	// Read 1 frame in double-word mode.
	#define DMA_SYNC_READ_ADC2 ((DMASYNC_REVT<<DSYN)|(DBLW_DBL<<DBLW)|((1-1)<<FRAMECOUNT)) 
//	// Read 2 frames in single-word mode.
 //	#define DMA_SYNC_READ_ADC2 ((DMASYNC_REVT2<<DSYN)|(DBLW_SNGL<<DBLW)|((2-1)<<FRAMECOUNT)) 
//	// Read 1 frame in single-word mode.
//	#define DMA_SYNC_READ_ADC2 ((DMASYNC_REVT2<<DSYN)|(DBLW_SNGL<<DBLW)|((1-1)<<FRAMECOUNT)) 

	// Set up DMA Rx Channel to read sensor measuremement from the ADC into upRawLocal[];
	DMA_init(  	DMA_CHANNEL4,				// u16 channel, 
/*				DRR1_ADDR(AFE_MCBSP),		// u16 source= ADC MCBSP Data Receive register  */
				DRR2_ADDR(AFE_MCBSP),		// u16 source= ADC MCBSP Data Receive register 	
	        	uRxBuffAddr,				// u16 dest= Raw readings array 
	          	(uArraySize),				// u16 count
	          	DMA_SYNC_READ_ADC2,			// u16 frame_sync = receive complete event 
				DMA_CONTROL_READ_ADC2);		// u16 control_mode

#else					// Read three 16-bit words

	// Set up DMA to synchronize transfer with MCBSP receive complete. 
	// Read 1 frame in single-word mode.
	#define DMA_SYNC_READ_ADC2 ((DMASYNC_REVT<<DSYN)|(DBLW_SNGL<<DBLW)|((1-1)<<FRAMECOUNT)) 
	// Set up DMA Rx Channel to read sensor measuremement from the ADC into upRawLocal[];
	DMA_init(  	DMA_CHANNEL4,				// u16 channel, 
				DRR1_ADDR(AFE_MCBSP),		// u16 source= hi word of MCBSP Data Receive reg
           		uRxBuffAddr,				// u16 dest= Raw readings array 
           		(uArraySize-1),				// u16 count
           		DMA_SYNC_READ_ADC2,			// u16 frame_sync = receive complete event 
				DMA_CONTROL_READ_ADC2);		// u16 control_mode
#endif

	//-------------------------------------------------------------------------------------
	// ----- Release clock generator from reset and wait 2 clock cycles -----
	//-------------------------------------------------------------------------------------
	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, GRST, GRST_SZ, 1);  //!!!TEMP!!!
	RPTNOP(2*SER_CLK_DIV_SLOW);	// Wait 2 cycles of slow serial clock

	//-------------------------------------------------------------------------------------
	// Release receiver and transmitter from reset.                 
	//-------------------------------------------------------------------------------------
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR1_SUBADDR, SPCR1_CTRL);
	MCBSP_SUBREG_WRITE(AFE_MCBSP, SPCR2_SUBADDR, (SPCR2_CTRL & (~(1<<FRST)))); // Turn on SPCR2 except for FRST

	//-------------------------------------------------------------------------------------
	//----- Arm the DMA Engine -----
	//-------------------------------------------------------------------------------------
	DMPREC |= (AFE_RX_DMA_MASK); 

//	//-------------------------------------------------------------------------------------
//    //----- Load data transmit register with first word to start transmission -----
//	//-------------------------------------------------------------------------------------
//	REG_WRITE(DXR12_ADDR, upADCCmdListAddr[0]); // Send first entry in CmdList to ADC 
//
//	//-------------------------------------------------------------------------------------
//	// Turn on frame generation, which should start sending word we just loaded in DXR1.
//	//-------------------------------------------------------------------------------------
//	MCBSP_SUBREG_BITWRITE(AFE_MCBSP, SPCR2_SUBADDR, FRST, FRST_SZ, 1); 
//
	//-------------------------------------------------------------------------------------
	// The DMA transfer is now running.  We can now return to the main code and wait for
	// the transfer-complete interrupt from the DMA engine.
   	//-------------------------------------------------------------------------------------


//----------------- Start DMAXfer
	return;
}
#endif 


//==========================================================================================
// Function:		FillIdleBuffer()
//
// Description: 	This function copies data from an integer source buffer to the
//					idle buffer in a format compatible with the AFE (Analog Front End).
//
// Revision History:
//==========================================================================================
void FillIdleBuffer(void)
{
	u16		i;	// Loop counter
	u16		temp;
	u16* 	pDest = IdleBuffArray;
	static u16	AFEctrl1Old = 0x8000;	// Old copy of AFE control var 1
	static u16	AFEctrl2Old = 0x8000;	// Old copy of AFE control var 2

	static	i16	IdleArray[IDLE_BUFFER_LEN] = {0,0};

	// Fill the idle buffer if either control word has changed
	if ( (AFEctrl1 != AFEctrl1Old) || (AFEctrl2 != AFEctrl2Old) )
	{
		#if COMPILE_MODE == MEX_COMPILE
		  	for(i = 0; i<IDLE_BUFFER_LEN; i++ )
			{
				*pDest++ = IdleArray[i];
			}
		#else	// DSP_COMPILE
			#if	(SEND_TWICE == TRUE)
				// 	Transmit three 16-bit values
				// Fill Temp Transmit buffer with real data interleaved between control words
				// Send same data word twice, with different control words
			  	for(i = 0; i<IDLE_BUFFER_LEN; i++ )
				{
					temp = IdleArray[i];
					*pDest++ = temp;
					*pDest++ = (AFEctrl1 << 8) + ((temp>>8)&0x00ff);
					*pDest++ = ((temp&0x00ff) << 8) + AFEctrl2;
				}
			#else	// SEND_TWICE == FALSE
				// 	Transmit three 16-bit values
				// Fill Temp Transmit buffer with real data interleaved between control words
			  	for(i = 0; i<IDLE_BUFFER_LEN; i+=2 )
				{
					temp = IdleArray[i];		// Grab first Tx value and send it
					*pDest++ = temp;
					temp = IdleArray[i+1];		// Spread second Tx value over the next two words
					*pDest++ = (AFEctrl1 << 8) + ((temp>>8)&0x00ff);
					*pDest++ = ((temp&0x00ff) << 8) + AFEctrl2;
				}
			#endif
		#endif		
	}
	
	AFEctrl1Old = AFEctrl1;
	AFEctrl2Old = AFEctrl2;
	
	return;
}


//==========================================================================================
// Function:		FillAFETxBuffCplx()
//
// Description: 	This function copies data from a complex source buffer to a destination
//					buffer while modifying the destination data format to be compatible
//					with the AFE (Analog Front End).
//
// Revision History:
//==========================================================================================
TXDATA* FillAFETxBuffCplx(TXDATA* pDest, iCplx* pSrc, u16 uCnt)
#if COMPILE_MODE == MEX_COMPILE
{
	u16		i;	// Loop counter
	for( i = 0; i < uCnt; i++ )
	{
		*pDest++ = (pSrc++)->re;	// Copy the real part of the source into the dest
	}

	return(pDest);
}
#else
{
	u16		i;	// Loop counter
	i16		temp;

	WaitForTxBufferFree(pDest, uCnt);		// Wait until DMA is not using destination memory segment

	// 	Transmit three 16-bit values
	// Fill Temp Transmit buffer with real data interleaved between control words
  	for(i = 0; i<(uCnt/2); i++ )
	{
		temp = 	(i16)((pSrc++)->re);		// First transmit value
		*pDest++ = temp;
		temp = 	(i16)((pSrc++)->re);		// Second transmit value
		*pDest++ = (AFEctrl1 << 8) + ((temp>>8)&0x00ff);
		*pDest++ = ((temp&0x00ff) << 8) + AFEctrl2;
	}

	return(pDest);
}
#endif


//==========================================================================================
// Function:		FillAFETxBuffI16()
//
// Description: 	This function copies data from an integer source buffer to a destination
//					buffer while modifying the destination data format to be compatible
//					with the AFE (Analog Front End).
//
// Revision History:
//==========================================================================================
TXDATA* FillAFETxBuffI16 (TXDATA* pDest, i16* pSrc, u16 uCnt)
{
#if COMPILE_MODE == MEX_COMPILE
	memcpy( pDest, pSrc, uCnt*sizeof(i16) ); 
	pDest += uCnt;
	return(pDest);

#else
	u16		i;	// Loop counter
	i16		temp;

	WaitForTxBufferFree(pDest, uCnt);		// Wait until DMA is not using destination memory segment

	// 	Transmit three 16-bit values
	// Fill Temp Transmit buffer with real data interleaved between control words
  	for(i = 0; i<(uCnt/2); i++ )
	{
		temp = 	(i16)*pSrc++;		// First transmit value
		*pDest++ = temp;
		temp = 	(i16)*pSrc++;		// Second transmit value
		*pDest++ = (AFEctrl1 << 8) + ((temp>>8)&0x00ff);
		*pDest++ = ((temp&0x00ff) << 8) + AFEctrl2;
	}

	return(pDest);
#endif
}


//==========================================================================================
// Function:		WaitForTxBufferFree()
//
// Description: 	This function polls the DMA source address point until the required 
//					section in the Tx buffer is available.
//
// Revision History:
//==========================================================================================
void WaitForTxBufferFree(TXDATA* uStart, u16 uCnt) 
{
	TXDATA*		uSrc;
	TXDATA*		uFirstSrc;
	TXDATA*		uPrevSrc;
	u16		uStallCnt= 0;

	uSrc = ReadTxDMAPointer();								// Read DMA source pointer in TxBuffer
	DebugDelay();	// Flush C54x if in debug mode.
	DebugDelay();	// Flush C54x if in debug mode.
	uFirstSrc = uSrc;										// Take a snapshot of first value seen

	while (  (uSrc >= uStart) 								// Wait until we are out of the needed segment
		  && (uSrc < (uStart + (uCnt*AFE_SIZE))) )
	{	
		uPrevSrc = uSrc;									// Save the previous DMA pointer value
		uSrc = ReadTxDMAPointer();							// Read DMA source pointer in TxBuffer
		
		//  Look for stalled pointer 
		if (uSrc == uPrevSrc) 								
		{
			DelayNus(1);									
			uStallCnt++;
			if (uStallCnt>1000)
			{
				DebugDelay();	// Flush C54x if in debug mode.
				break;			// Bail out if DMA pointer is not moving for 1000 consecutive readings
			}
		}
		else
		{
			uStallCnt = 0;		// Reset consecutive stalled counter
		}
		
		//  Look for roll-over 
		if (uSrc < uFirstSrc) 								 
		{
			DebugDelay();		// Flush C54x if in debug mode.
			break;				// Bail out if rollover seen, else we might wait here forever.
		}

		FlashStateMachine( );   // execute current state of flash management state machine
		DebugDelay();			// Flush C54x if in debug mode.
	}

	DebugDelay();				// Flush C54x if in debug mode.
 	return;
 }


//==========================================================================================
// Function:		WaitForRange()
//
// Description: 	This function polls the DMA source address point is inside the 
//					selected range.
//
// Revision History:
//==========================================================================================
void WaitForRange(TXDATA* uStart, u16 uCnt) 
{
	TXDATA*		uSrc;
	TXDATA*		uFirstSrc;

	uSrc = ReadTxDMAPointer();								// Read DMA source pointer in TxBuffer
	uFirstSrc = uSrc;										// Take a snapshot of first value seen

	while (  (uSrc < uStart) 
	 	   ||(uSrc >= (uStart + (uCnt*AFE_SIZE))) )			// Wait until we are inside the needed segment
	{		
		DebugDelay();				// Flush C54x if in debug mode.
		//...
		//    We could do something useful here, like polling the host interface
		//...

		DebugDelay();				// Flush C54x if in debug mode.
	
		uSrc = ReadTxDMAPointer();	// Read DMA source pointer in TxBuffer
		if (uSrc < uFirstSrc) 		//  Look for roll-over 
		{
			DebugDelay();			// Flush C54x if in debug mode.
			//break;			   	// Bail out if rollover seen, else we might wait here forever.
		}

		FlashStateMachine( );  		// execute current state of flash management state machine
		DebugDelay();				// Flush C54x if in debug mode.
	}

	DebugDelay();				// Flush C54x if in debug mode.
	return;
 }


//==========================================================================================
// Function:		ReadRxDMAPointer()
//
// Description: 	This function read the DMA destination address of the receive buffer.
//
// Revision History:
//==========================================================================================
i16* ReadRxDMAPointer(void)
{	
#if COMPILE_MODE == MEX_COMPILE
	if( rxDMApointer >= recSignalArray + RX_CIRC_BUFFER_LEN )
		rxDMApointer = recSignalArray;
	return rxDMApointer;

#else 	//COMPILE_MODE == DSP_COMPILE
	i16*	pRxDest;
	u16		uRxCtr;

	uRxCtr = DMA_SUBREG_READ(DMA_CHANNEL4, DMCTR_SUBADDR);
	RPTNOP(4);
	
	pRxDest = (i16*)DMA_SUBREG_READ(DMA_CHANNEL4, DMDST_SUBADDR);

	RPTNOP(4);
	if (uRxCtr != RX_CIRC_BUFFER_LEN)
	{
		DebugDelay();				// Flush C54x if in debug mode.
		PostErrorCode(0xBAD8, "ReadRxDMAPointer", "AFE.c", "DMA Counter corrupted");
		#if SAVETRACE == TRUE
			SaveTraceData(0xBAD8);			//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData((u16)pRxDest); 	//!!!DEBUG  Put a marker in the trace buffer
			SaveTraceData(uRxCtr);			//!!!DEBUG  Put a marker in the trace buffer
		#endif
		DebugDelay();				// Flush C54x if in debug mode.
		DebugDelay();				// Flush C54x if in debug mode.
	}

	return (pRxDest);
#endif
}


//==========================================================================================
// Function:		ReadTxDMAPointer()
//
// Description: 	This function read the DMA source address of the transmit buffer.
//
// Revision History:
//==========================================================================================
TXDATA* ReadTxDMAPointer(void)
{
	TXDATA* temp;
	temp = (TXDATA*)DMA_SUBREG_READ(DMA_CHANNEL5, DMSRC_SUBADDR);

	return(temp);
}


//==========================================================================================
// Function:		WaitForRxBufferFree()
//
// Description: 	This function polls the DMA destination address point until the required 
//					section in the receive buffer is available.
//
// Revision History:
//==========================================================================================
void WaitForRxBufferFree(i16* uStart, u16 uCnt) 
{
	i16*	uDest;
	i16*	uFirstDest;

	SetXF();					// Turn on XF flag for debug
	uDest = ReadRxDMAPointer();	// Read DMA destination pointer in RxBuffer
	uFirstDest = uDest;			// Take a snapshot of first value seen

	while ((uDest >= uStart) && (uDest < (uStart + uCnt)) )	// Wait until we are out of the needed segment
	{	
		uDest = ReadRxDMAPointer();		// Read DMA destination pointer in RxBuffer
		if (uDest < uFirstDest) 		//  Look for roll-over 
		{
			DebugDelay();	// Flush C54x if in debug mode.
			break;			// Bail out if rollover seen, else we might wait here forever.
		}

		FlashStateMachine( );   // execute current state of flash management state machine
		DebugDelay();	   		// Flush C54x if in debug mode.
	}

	ClearXF();		// Turn off XF flag for debug

	if (uCnt > 0x300)		// Put this here as a trigger point for errors
	{
		DebugDelay();		// Flush C54x if in debug mode.
		DebugDelay();		// Flush C54x if in debug mode.
	}

	DebugDelay();				// Flush C54x if in debug mode.
 	return;
 }



//==========================================================================================
// Function:		JamDMAReloadRegs()
//
// Description: 	This function jams the DMA Global Reload registers so that when the 
//					present frame finishes it will jump to next desired location.
//
// Revision History:
//==========================================================================================
void JamDMAReloadRegs(TXDATA* uStart, u16 uCnt)
{
#if COMPILE_MODE == DSP_COMPILE
	// If we are almost at the end of the previous block, wait for it to roll over
	//  before we jam the global registers.  We don't want the DMA to expire while
	//  we're in the middle of changing them.  Mainly useful when switching from 
	//	idle to preamble.
	while( DMA_SUBREG_READ(DMA_CHANNEL5, DMCTR_SUBADDR) <= 1 )		
	{
		DebugDelay();				// Flush C54x if in debug mode.
	}

	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGSA_SUBADDR, (u16)uStart);	//  DMA Global Source Address
	DMA_SUBREG_WRITE(DMA_CHANNEL5, DMGCR_SUBADDR, (uCnt*AFE_SIZE)-1);	//  DMA Global Element Count

	// Wait until the last frame has started being transmitted
	WaitForRange(uStart, uCnt);
	DebugDelay();				// Flush C54x if in debug mode.
#endif

	return;
}
