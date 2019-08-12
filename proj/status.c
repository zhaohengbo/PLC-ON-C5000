//==========================================================================================
// Filename:			status.c
//
// Description:		Status indicators loop for ofdm power line modem.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"
#include "ofdm_datatypes.h"
#include "mcbsp54.h"
#include "Flash.h"

u16			LEDGPOReg;		// Fake LED register

#define	SER_CLK_DIV					10		// 150 MHz / 10 = 15 MHz clock for UART_MCBSP and LED_MCBSP
#if COMPILE_MODE == DSP_COMPILE
   asm("SER_CLK_DIV			.set	10");
#endif


#define	MAX_MCBSP_WAIT	(18*SER_CLK_DIV)	// Max wait = 18 serial clock ticks		


#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		ConfigureLEDMCBSP()
//
// Description: 	This function configures the MCBSP port to talk to the status LEDs.
//					It should be called during board initialization.
//
// Revision History:
//==========================================================================================
u16	ConfigureLEDMCBSP (void)
{
	// ACHTUNG!  Please don't move these #defines to a .h file.  I used them here instead
	// of 'const' statements because they save code space.

	// MCBSP = Multi-Channel Buffered Serial Port
	// MCBSP Port number 
//	#define	PORT_NO		LED_MCBSP		 

	// MCBSP Serial Port Control Register 1
	// Enable receive port with standard settings.
	#define SPCR1_CTRL	( (DLB_DISABLE<<DLB) | (RXJUST_RJZF<<RJUST) 					\
						| (CLK_STOP_DISABLED<<CLKSTP) | (DX_ENABLE_OFF<<DXENA)			\
						| (ABIS_DISABLE<<ABIS) | (INTM_RDY<<RINTM) | (RX_ENABLE<<RRST)	)

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
    	  			 	| (FSYNC_POL_LOW<<3)   | (FSYNC_POL_HIGH<<2)					\
		  			 	| (CLKX_POL_FALLING<<1)| (CLKR_POL_FALLING<<0)					)



//XXX 	// Configure pins as serial ports instead of GPIOs, 
//XXX 	// Generate transmit frame-sync on the XFS pin using internal sample rate generator.
//XXX 	// Receive frame-sync is input from RFS pin (which is wired to XFS on board).
//XXX 	#define	PCR_CTRL	( (IO_DISABLE<<13)     | (IO_DISABLE<<12) 						\
//XXX 		  				| (FSYNC_MODE_INT<<11) | (FSYNC_MODE_EXT<<10)					\
//XXX 		  			 	| (CLK_MODE_INT<<9)    | (CLK_MODE_EXT<<8) 						\
//XXX     	  			 	| (FSYNC_POL_HIGH<<3)  | (FSYNC_POL_HIGH<<2)					\
//XXX 		  			 	| (CLKX_POL_FALLING<<1)| (CLKR_POL_FALLING<<0)					)



	// MCBSP Receive Control Register 1
	// 1 words/frame, 16-bit word
	#define	RCR1_CTRL	( (1-1)<<RFRLEN1 | (WORD_LENGTH_16<<RWDLEN1)	)

	// MCBSP Receive Control Register 2
	// Standard
	#define	RCR2_CTRL	( (SINGLE_PHASE<<RPHASE) | (0<<RFRLEN2) | (0<<RWDLEN2) 			\
    					| (NO_COMPAND_MSB_1ST<<RCOMPAND) | (NO_FRAME_IGNORE<<RFIG) 		\
    					| (DATA_DELAY2<<RDATDLY)										)

	// 1 word/frame, 16-bit word	
	#define	XCR1_CTRL	((1-1)<<XFRLEN1 | (WORD_LENGTH_16<<XWDLEN1)		)

    // MCBSP Transmit Control Register 2
	// Standard	operation except for zero data delay at start of frame
	#define	XCR2_CTRL	( (SINGLE_PHASE<<XPHASE) | (0<<XFRLEN1) | (0<<XWDLEN1) 			\
    					| (NO_COMPAND_MSB_1ST<<XCOMPAND) | (NO_FRAME_IGNORE<<XFIG) 		\
    					| (DATA_DELAY0<<XDATDLY)										)
 
	// MCBSP Sample Rate Generator Register 1
	// Frame-sync pulse 16 CLKG periods wide, set Serial clock rate 
	#define	SRGR1_CTRL	((16-1)<<FWID) | ((SER_CLK_DIV-1)<<CLKGDV)				


	// MCBSP Sample Rate Generator Register 2
	// Generate periodic Frame-Sync pulses 17 CLKG periods apart
    // MCBSP Sample Rate Generator Register 2
	// Serial clock free-running, derived from CPU clock
    #define	SRGR2_CTRL	((GSYNC_OFF<<GSYNC) | (CLKS_POL_FALLING<<CLKSP) 			\
    					| (CLKSM_CPU<<CLKSM) |(FSX_FSG<<FSGM) |  ((17-1)<<FPER))
				

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

	u16		uPortNum = LED_MCBSP;		//
	u16		uStatus = SUCCESS;			// Return value.

 	#ifdef FLOW
		LOG_printf(&Trace, "Entered ConfigureLEDMCBSP()");
	#endif

	DebugDelay();		// Flush C54x if in debug mode.

	// Initialize the MCBSP port which is connected to the Status LEDs Shift register
	InitMCBSP(uPortNum,   SPCR1_CTRL, SPCR2_CTRL, PCR_CTRL,
			  RCR1_CTRL,  RCR2_CTRL,  XCR1_CTRL,  XCR2_CTRL,
			  SRGR1_CTRL, SRGR2_CTRL, MCR1_CTRL,  MCR2_CTRL,
			  RCERA_CTRL, RCERB_CTRL, XCERA_CTRL, XCERB_CTRL);


	// Shut off frame sync generation (FS)
	MCBSP_SUBREG_BITWRITE(uPortNum, SPCR2_SUBADDR, FRST, FRST_SZ, 0);

	uLEDShadow = 0;

//AFE 	// ---------- DMA Configuration ---------------
//AFE 	SET_REG(DMPREC, 0);			//!!!TEMP Disable all DMA channels before configuring them
//AFE 	DMA_reset();				// Reset all DMA registers.
//AFE 
//AFE //NORMAL 	// All channels are disabled for now.  DMA will keep running when emulator is paused.
//AFE //NORMAL 	// DMA Channels 4 and 2 have high priority.
//AFE //NORMAL 	// Interrupts will be directed to HWI_SINT 6 through 13.
//AFE //NORMAL 	#define DMPREC_INIT (	(RUN<<DMA_FREE) | (LO<<DPRC5) | (HI<<DPRC4) | (LO<<DPRC3) \
//AFE //NORMAL 						 					| (HI<<DPRC2) | (LO<<DPRC1) | (LO<<DPRC0) \
//AFE //NORMAL 					   | (INTOSEL2<<INTSEL) | (DIS<<DE5)  |  (DIS<<DE4) | (DIS<<DE3)  \
//AFE //NORMAL 											| (DIS<<DE2)  |  (DIS<<DE1) | (DIS<<DE0) )	  
//AFE 
//AFE 	// All channels are disabled for now.  DMA will keep pause when emulator is paused. <<<!!!DEBUG
//AFE 	// DMA Channels 4 and 2 have high priority.
//AFE 	// Interrupts will be directed to HWI_SINT 6 through 13.
//AFE 	#define DMPREC_INIT (	(STOP<<DMA_FREE) | (LO<<DPRC5) | (HI<<DPRC4) | (LO<<DPRC3) \
//AFE 						 					| (HI<<DPRC2) | (LO<<DPRC1) | (LO<<DPRC0) \
//AFE 					   | (INTOSEL2<<INTSEL) | (DIS<<DE5)  |  (DIS<<DE4) | (DIS<<DE3)  \
//AFE 											| (DIS<<DE2)  |  (DIS<<DE1) | (DIS<<DE0) )	  
//AFE 
//AFE 	SET_REG(DMPREC, DMPREC_INIT);

	return(uStatus);
}
#endif

#if COMPILE_MODE == DSP_COMPILE
//==========================================================================================
// Function:		WriteMCBSP16()
//
// Description: 	This function sends a 16-bit message through the MCBSP after waiting
//					to ensure that any previous messages have completed.
// Revision History:
//==========================================================================================
u16 WriteMCBSP16(u16 uPortNum, u16 uMCBSPmsg)
{
	u16		i= 0; 						// Loop counter to prevent long delays					 
	u16		uStatus = SUCCESS;			// Return value

	#define	PORT0	0
	#define	PORT1	1
	#define	PORT2	2

//	#ifdef FLOW
//		LOG_printf(&Trace, "Entered WriteMCBSP16()");
//	#endif

	switch (uPortNum)
	{
	case PORT0:	
		MCBSP0_SUBREG_BITWRITE(SPCR2_SUBADDR, FRST, FRST_SZ, 1);  // Turn on FS

		// Wait for any previous MCBSP transmissions to complete, with an upper limit of
		// MAX_MCBSP_WAIT to keep DSP from hanging if hardware is goofed up.
		while ( (!MCBSP_XRDY(PORT0)) && (i++<MAX_MCBSP_WAIT) )
		{
		}

		// Send message to DAC via MCBSP
		REG_WRITE(DXR1_ADDR(PORT0), uMCBSPmsg); 	
		break;

	case PORT1:
		MCBSP1_SUBREG_BITWRITE(SPCR2_SUBADDR, FRST, FRST_SZ, 1);  // Turn on FS

		// Wait for any previous MCBSP transmissions to complete, with an upper limit of
		// MAX_MCBSP_WAIT to keep DSP from hanging if hardware is goofed up.
		while ( (!MCBSP_XRDY(PORT1)) && (i++<MAX_MCBSP_WAIT) )
		{
		}

		// Send message to DAC via MCBSP
		REG_WRITE(DXR1_ADDR(PORT1), uMCBSPmsg); 	
		break;

	case PORT2:
		MCBSP2_SUBREG_BITWRITE(SPCR2_SUBADDR, FRST, FRST_SZ, 1);  // Turn on FS

		// Wait for any previous MCBSP transmissions to complete, with an upper limit of
		// MAX_MCBSP_WAIT to keep DSP from hanging if hardware is goofed up.
		while ( (!MCBSP_XRDY(PORT2)) && (i++<MAX_MCBSP_WAIT) )
		{
		}

		// Send message to DAC via MCBSP
		REG_WRITE(DXR1_ADDR(PORT2), uMCBSPmsg); 	
		break;
	}

	return (uStatus);
}





//==========================================================================================
// Function:		TurnLEDsOn()
//
// Description: 	This function turns on one or more status LEDs.
//
// Revision History:
//==========================================================================================
void TurnLEDsOn(u16 uMask)
{

	if( FlashLEDMask == 0 ) // if flash not displaying any status
		SetBits(uLEDShadow, uMask);	
    else
		AssignBits(uLEDShadow, ~0u, FlashLEDMask);	
        
	WriteMCBSP16(LED_MCBSP, uLEDShadow);
}


//==========================================================================================
// Function:		TurnLEDsOff()
//
// Description: 	This function turns off one or more status LEDs.
//
// Revision History:
//==========================================================================================
void TurnLEDsOff(u16 uMask)
{

	if( FlashLEDMask == 0 ) // if flash not displaying any status
		ClearBits(uLEDShadow, uMask);	
    else
		AssignBits(uLEDShadow, ~0u, FlashLEDMask);	

	WriteMCBSP16(LED_MCBSP, uLEDShadow);
}


//==========================================================================================
// Function:		AssignLEDs()
//
// Description: 	This function sets and clears each led according to the mask passed
//
// Revision History:
//==========================================================================================
void AssignLEDs(u16 uMask)
{

	if( FlashLEDMask == 0 ) // if flash not displaying any status
		AssignBits(uLEDShadow, ~0u, uMask);	
    else
		AssignBits(uLEDShadow, ~0u, FlashLEDMask);	

	WriteMCBSP16(LED_MCBSP, uLEDShadow);
}

#endif

