//==========================================================================================
// Filename:		mcbsp54.h
//
// Copyright (C) 2000 - 2001 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

/******************************************************************************/
/*  MCBSP54.H - MCBSP54 routines header file.                                 */
/*                                                                            */
/*     This module provides the devlib implementation for the MCBSP           */
/*     on the TMS320C54x DSP.                                                 */
/*                                                                            */
/*  MACRO FUNCTIONS:                                                          */
/* 	MCBSP_BYTES_PER_WORD 	- return # of bytes required to hold #            */
/*                        	  of bits indicated by wdlen                      */
/*  MCBSP_ENABLE()          - starts serial port receive and/or transmit      */
/* 	MCBSP_TX_RESET() 		- reset transmit side of serial port              */
/* 	MCBSP_RX_RESET() 		- reset receive side of serial port               */
/* 	MCBSP_DRR1_READ() 	- read data value from serial port              	  */
/*   		use instead: MCBSP_SUBREG_READ(... , DRR1_SUBADDR, ... )          */
/* 	MCBSP_DRR2_READ() 	- read data value from serial port              	  */
/*   		use instead: MCBSP_SUBREG_READ(... , DRR2_SUBADDR, ... )          */
/* 	MCBSP_DRR12_READ() 	- read data value from serial port              	  */
/*                      	  return value as unsigned long                   */
/* 	MCBSP_DXR12_WRITE() 	- write data value from serial port               */
/* 	MCBSP_IO_ENABLE() 	- place port in general purpose I/O mode        	  */
/* 	MCBSP_IO_DISABLE() 	- take port out of general purpose I/O mode     	  */
/* 	MCBSP_FRAME_SYNC_ENABLE - sets FRST bit in SPCR                           */
/* 	MCBSP_FRAME_SYNC_RESET 	- clrs FRST bit in SPCR                           */
/* 	MCBSP_SAMPLE_RATE_ENABLE- sets GRST bit in SPCR                           */
/* 	MCBSP_SAMPLE_RATE_RESET - clrs GRST bit in SPCR                           */
/* 	MCBSP_RRDY 			- returns selected ports RRDY                         */
/* 	MCBSP_XRDY 			- returns selected ports XRDY                   	  */
/* 	MCBSP_LOOPBACK_ENABLE 	- places selected port in loopback                */
/* 	MCBSP_LOOPBACK_DISABLE 	- takes port out of DLB                           */
/*                                                                            */
/*  FUNCTIONS:                                                                */
//		None
/*                                                                            */
/*                                                                            */
/*  AUTHOR:                                                                   */
/*     Stefan Haas                                                            */
/*                                                                            */
/*  REVISION HISTORY:                                                         */
/*                                                                            */
/*    DATE       AUTHOR                       DESCRIPTION                     */
/*   -------   -------------      ------------------------------------------  */
/*   13OCT98   St Haas            Original.                                   */
/*                                                                            */
//	02/21/2001	HEM		Added constants for clock source.
//  11/21/2001  HEM		Commented out mcbsp_init.  
//						Not used any more, and was causing compiler remarks.
//  05/05/2003	HEM		Renamed clock start constant to eliminate'/' in name.
/******************************************************************************/


   

                                                                 
/******************************************************************/
/* This header file  defines the data structures and macros to    */
/* necessary to address the Multi-Channel Serial Port             */
/******************************************************************/
#ifndef _MCBSP_H_
#define _MCBSP_H_

#include "regs54xx.h"


/* Bits, Bitfields, ... */

#define MCBSP_RX     1
#define MCBSP_TX     2
#define MCBSP_BOTH   3 

/* CONFIGURATION REGISTER BIT and BITFIELD values */
/* Serial Port Control Register SPCR1 */

#define DLB_ENABLE          0x01     /* Enable Digital Loopback Mode          */
#define DLB_DISABLE         0x00     /* Disable Digital Loopback Mode         */

#define RXJUST_RJZF         0x00     /* Receive Right Justify Zero Fill       */      
#define RXJUST_RJSE         0x01     /* Receive Right Justify Sign Extend     */
#define RXJUST_LJZF         0x02     /* Receive Left Justify Zero Fill        */
                                                                              
#define CLK_STOP_DISABLED	0x00   /* Normal clocking for non-SPI mode      */ 
#define CLK_START_WO_DELAY  0x10   /* Clock starts without delay            */
#define CLK_START_W_DELAY	0x11   /* Clock starts with delay               */

#define DX_ENABLE_OFF		0x00   /* no extra delay for turn-on time       */
#define DX_ENABLE_ON		0x01   /* enable extra delay for turn-on time   */

#define ABIS_DISABLE		0x00   /* A-bis mode is disabled                */
#define ABIS_ENABLE			0x01   /* A-bis mode is enabled                 */


/* Serial Port Control Registers SPCR1 and SPCR2 */
                                                                             
#define INTM_RDY            0x00     /* R/X INT driven by R/X RDY             */
#define INTM_BLOCK          0x01     /* R/X INT driven by new multichannel blk*/
#define INTM_FRAME          0x02     /* R/X INT driven by new frame sync      */
#define INTM_SYNCERR        0x03     /* R/X INT generated by R/X SYNCERR      */

#define RX_RESET			0x00	 /* R or X in reset */
#define RX_ENABLE			0x01	 /* R or X enabled */


/* Serial Port Control Register SPCR2 */

#define SP_FREE_OFF		0x00     /* Free running mode is diabled          */
#define SP_FREE_ON		0x01     /* Free running mode is enabled          */

#define SOFT_DISABLE		0x00     /* SOFT mode is disabled                 */
#define SOFT_ENABLE		0x01     /* SOFT mode is enabled                  */

#define FRAME_GEN_RESET		0x00     /* Frame Synchronization logic is reset  */
#define FRAME_GEN_ENABLE	0x01     /* Frame sync signal FSG is generated    */

#define SRG_RESET			0x00     /* Sample Rate Generator is reset        */
#define SRG_ENABLE		0x01     /* Sample Rate Generator is enabled      */


/* Pin Control Register PCR */

#define IO_DISABLE		0x00     /* No General Purpose I/O Mode           */
#define IO_ENABLE			0x01     /* General Purpose I/0 Mode enabled      */

#define CLKR_POL_RISING     0x01     /* R Data Sampled on Rising Edge of CLKR */
#define CLKR_POL_FALLING    0x00     /* R Data Sampled on Falling Edge of CLKR*/
#define CLKX_POL_RISING     0x00     /* X Data Sent on Rising Edge of CLKX    */
#define CLKX_POL_FALLING    0x01     /* X Data Sent on Falling Edge of CLKX   */
#define FSYNC_POL_HIGH      0x00     /* Frame Sync Pulse Active High          */
#define FSYNC_POL_LOW       0x01     /* Frame Sync Pulse Active Low           */

#define CLK_MODE_EXT        0x00     /* Clock derived from external source    */
#define CLK_MODE_INT        0x01     /* Clock derived from internal source    */

#define FSYNC_MODE_EXT      0x00     /* Frame Sync derived from external src  */
#define FSYNC_MODE_INT      0x01     /* Frame Sync dervived from internal src */

/* Transmit Receive Control Register XCR/RCR */

#define SINGLE_PHASE        0x00     /* Selects single phase frames           */
#define DUAL_PHASE          0x01     /* Selects dual phase frames             */

#define MAX_FRAME_LENGTH    0x7f     /* maximum number of words per frame     */

#define WORD_LENGTH_8       0x00     /* 8 bit word length (requires filling)  */
#define WORD_LENGTH_12      0x01     /* 12 bit word length       ""           */
#define WORD_LENGTH_16      0x02     /* 16 bit word length       ""           */
#define WORD_LENGTH_20      0x03     /* 20 bit word length       ""           */
#define WORD_LENGTH_24      0x04     /* 24 bit word length       ""           */
#define WORD_LENGTH_32      0x05     /* 32 bit word length (matches DRR DXR sz*/

#define MAX_WORD_LENGTH     0x20     /* maximum number of bits per word       */

#define NO_COMPAND_MSB_1ST  0x00     /* No Companding, Data XFER starts w/MSb */
#define NO_COMPAND_LSB_1ST  0x01     /* No Companding, Data XFER starts w/LSb */
#define COMPAND_ULAW        0x02     /* Compand ULAW, 8 bit word length only  */
#define COMPAND_ALAW        0x03     /* Compand ALAW, 8 bit word length only  */

#define FRAME_IGNORE        0x01     /* Ignore frame sync pulses after 1st    */
#define NO_FRAME_IGNORE     0x00     /* Utilize frame sync pulses             */

#define DATA_DELAY0         0x00     /* 1st bit in same clk period as fsync   */
#define DATA_DELAY1         0x01     /* 1st bit 1 clk period after fsync      */
#define DATA_DELAY2         0x02     /* 1st bit 2 clk periods after fsync     */
  
/* Sample Rate Generator Register SRGR */

/* Clock mode (ext. / int.) see PCR */

#define MAX_SRG_CLK_DIV     0xff     /* max value to divide Sample Rate Gen Cl*/
#define MAX_FRAME_WIDTH     0xff     /* maximum FSG width in CLKG periods     */
#define MAX_FRAME_PERIOD    0x0fff   /* FSG period in CLKG periods            */

#define FSX_DXR_TO_XSR      0x00     /* Transmit FSX due to DXR to XSR copy   */
#define FSX_FSG             0x01     /* Transmit FSX due to FSG               */

#define CLKS_POL_FALLING    0x00     /* falling edge generates CLKG and FSG   */
#define CLKS_POL_RISING     0x01     /* rising edge generates CLKG and FSG    */

#define GSYNC_OFF           0x00     /* CLKG always running                   */
#define GSYNC_ON            0x01     /* CLKG and FSG synch'ed to FSR          */ 

#define	CLKSM_CLKS	0 				/* Sample rate generator clock derived from CLKS pin */
#define	CLKSM_CPU	1   			/* Sample rate generator clock derived from CPU clock */
                                    
/* Multi-channel Control Register 1 and 2 MCR1/2 */

#define RMCM_CHANNEL_ENABLE	0x00 	 /* all 128 channels enabled              */
#define RMCM_CHANNEL_DISABLE	0x01 	 /* all channels disabled, selected by    */
						 /* enabling RP(A/B)BLK, RCER(A/B)        */

#define XMCM_CHANNEL_DX_DRIVEN 0x00  /* transmit data over DX pin for as many */
						 /* number of words as required           */
#define XMCM_XCER_CHAN_TO_DXR	0x01 	 /* selected channels written to DXR      */
#define XMCM_ALL_WORDS_TO_DXR	0x02 	 /* all words copied to DXR(1/2),         */
						 /* DX only driven for selected words     */
#define XMCM_CHANNEL_SYM_RX		0x03 	 /* symmetric transmit and receive        */
						 /* operation                             */
     


#ifdef _INLINE
#define __INLINE static inline
#else
#define __INLINE
#endif

/********* Function Definitions ***********************************/
// 
// 
// __INLINE void mcbsp_init(unsigned short port_no, 
// 				 unsigned int spcr1_ctrl, unsigned int spcr2_ctrl,
//                          unsigned int rcr1_ctrl,  unsigned int rcr2_ctrl,
//                          unsigned int xcr1_ctrl,  unsigned int xcr2_ctrl,
//                          unsigned int srgr1_ctrl, unsigned int srgr2_ctrl,
//                          unsigned int mcr1_ctrl,  unsigned int mcr2_ctrl,
//                          unsigned int rcera_ctrl, unsigned int rcerb_ctrl,
//                          unsigned int xcera_ctrl, unsigned int xcerb_ctrl,
//                          unsigned int pcr_ctrl);
// 
// 
// #ifdef _INLINE
// /******************************************************************/
// /* mcbsp_init - initialize and start serial port operation        */
// /*                                                                */
// /******************************************************************/
// static inline void mcbsp_init(unsigned short port_no, 
// 							 unsigned int spcr1_ctrl, unsigned int spcr2_ctrl,
//                              unsigned int rcr1_ctrl,  unsigned int rcr2_ctrl,
//                              unsigned int xcr1_ctrl,  unsigned int xcr2_ctrl,
//                              unsigned int srgr1_ctrl, unsigned int srgr2_ctrl,
//                              unsigned int mcr1_ctrl,  unsigned int mcr2_ctrl,
//                              unsigned int rcera_ctrl, unsigned int rcerb_ctrl,
//                              unsigned int xcera_ctrl, unsigned int xcerb_ctrl,
//                              unsigned int pcr_ctrl)
// {
//    /****************************************************************/
//    /* Place port in reset - setting XRST & RRST to 0               */
//    /****************************************************************/
//    MCBSP_SUBREG_BITWRITE(port_no, SPCR1_SUBADDR, RRST, RRST_SZ, 0);
//    MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, XRST, XRST_SZ, 0);
//     
//    /****************************************************************/
//    /* Set values of all control registers                          */
//    /****************************************************************/
//    MCBSP_SUBREG_WRITE(port_no, RCR1_SUBADDR, rcr1_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, RCR2_SUBADDR, rcr2_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, XCR1_SUBADDR, xcr1_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, XCR2_SUBADDR, xcr2_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, SRGR1_SUBADDR, srgr1_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, SRGR2_SUBADDR, srgr2_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, MCR1_SUBADDR, mcr1_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, MCR2_SUBADDR, mcr2_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, RCERA_SUBADDR, rcera_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, RCERB_SUBADDR, rcerb_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, XCERA_SUBADDR, xcera_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, XCERB_SUBADDR, xcerb_ctrl);
//    MCBSP_SUBREG_WRITE(port_no, PCR_SUBADDR, pcr_ctrl); 
//    
//    MCBSP_SUBREG_BITWRITE(port_no, SPCR1_SUBADDR, RRST, RRST_SZ, 1);
//    MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, XRST, XRST_SZ, 1);
// }
//     
// #endif
// 
// 

/********* Macro Definitions **************************************/


/******************************************************************/
/* MCBSP_BYTES_PER_WORD - return # of bytes required to hold #    */
/*                        of bits indicated by wdlen              */
/******************************************************************/
#define MCBSP_BYTES_PER_WORD(wdlen) \
        ((int)((wdlen) + 1) / 2)

/******************************************************************/
/* MCBSP_ENABLE(unsigned short port_no, unsigned short type) -    */
/*            starts serial port receive and/or transmit          */
/*            type= 1 rx, type= 2 tx, type= 3 both                */
/******************************************************************/
#define MCBSP_ENABLE(port_no,mode) \
         REG_WRITE(SPCR1_ADDR(port_no), \
          (MCBSP_SUBREG_READ(port_no, SPCR1_SUBADDR) | (mode & 1))); \
         REG_WRITE(SPCR2_ADDR(port_no), \
          (MCBSP_SUBREG_READ(port_no, SPCR2_SUBADDR) | ((mode >> 1) & 1)))

/******************************************************************/
/* MCBSP_TX_RESET() - reset transmit side of serial port          */
/*                                                                */
/******************************************************************/
#define MCBSP_TX_RESET(port_no)\
        MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, XRST, XRST_SZ, 0);
  

/******************************************************************/
/* MCBSP_RX_RESET() - reset receive side of serial port           */
/*                                                                */
/******************************************************************/
#define MCBSP_RX_RESET(port_no)\
	  MCBSP_SUBREG_BITWRITE(port_no, SPCR1_SUBADDR, RRST, RRST_SZ, 0);
  

/******************************************************************/
/* MCBSP_DRR1_READ() - read data value from serial port           */
/******************************************************************/
/* use instead: MCBSP_SUBREG_READ(... , DRR1_SUBADDR, ... )*/
  

/******************************************************************/
/* MCBSP_DRR2_READ() - read data value from serial port           */
/******************************************************************/
/* use instead: MCBSP_SUBREG_READ(... , DRR2_SUBADDR, ... )*/

                       

/******************************************************************/
/* MCBSP_DRR12_READ() - read data value from serial port          */
/*                      return value as unsigned long             */
/******************************************************************/
#define MCBSP_DRR12_READ(port_no)\
     (((*(volatile unsigned long* DRR2_ADDR(port_no)))<<16) &\
      (REG_READ(DRR1_ADDR(port_no)))
                            

/******************************************************************/
/* MCBSP_DXR12_WRITE() - write data value to serial port          */
/******************************************************************/
#define MCBSP_DXR12_WRITE(port_no, value)\
     (REG_WRITE(DXR2_ADDR(port_no), (unsigned int) (value >> 16)),\
      (REG_WRITE(DXR1_ADDR(port_no), (unsigned int) value)) )
     

/******************************************************************/
/* MCBSP_IO_ENABLE() - place port in general purpose I/O mode     */
/******************************************************************/
#define MCBSP_IO_ENABLE(port_no) \
        { MCBSP_TX_RESET(port_no); MCBSP_RX_RESET(port_no); \
          MCBSP_SUBREG_BITWRITE(port_no, PCR_SUBADDR, RIOEN, 2, 0x0003) } 


/******************************************************************/
/* MCBSP_IO_DISABLE() - take port out of general purpose I/O mode */
/******************************************************************/
#define MCBSP_IO_DISABLE(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, PCR_SUBADDR, RIOEN, 2, 0x0000)


/******************************************************************/
/* MCBSP_FRAME_SYNC_ENABLE - sets FRST bit in SPCR                */
/******************************************************************/
#define MCBSP_FRAME_SYNC_ENABLE(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, FRST, FRST_SZ, 0x0001)


/******************************************************************/
/* MCBSP_FRAME_SYNC_RESET - clrs FRST bit in SPCR                 */
/******************************************************************/
#define MCBSP_FRAME_SYNC_RESET(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, FRST, FRST_SZ, 0x0000)


/******************************************************************/
/* MCBSP_SAMPLE_RATE_ENABLE - sets GRST bit in SPCR               */
/******************************************************************/
#define MCBSP_SAMPLE_RATE_ENABLE(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, GRST, GRST_SZ, 0x0001)


/******************************************************************/
/* MCBSP_SAMPLE_RATE_RESET - clrs GRST bit in SPCR                */
/******************************************************************/
#define MCBSP_SAMPLE_RATE_RESET(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, SPCR2_SUBADDR, GRST, GRST_SZ, 0x0000)


/******************************************************************/
/* MCBSP_RRDY - returns selected ports RRDY                       */
/******************************************************************/
#define MCBSP_RRDY(port_no) \
        MCBSP_SUBREG_BITREAD(port_no, SPCR1_SUBADDR, RRDY, RRDY_SZ)


/******************************************************************/
/* MCBSP_XRDY - returns selected ports XRDY                       */
/******************************************************************/
#define MCBSP_XRDY(port_no) \
        MCBSP_SUBREG_BITREAD(port_no, SPCR2_SUBADDR, XRDY, XRDY_SZ)


/******************************************************************/
/* MCBSP_LOOPBACK_ENABLE - places selected port in loopback       */
/******************************************************************/
#define MCBSP_LOOPBACK_ENABLE(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, SPCR1_SUBADDR, DLB, DLB_SZ, 0x0001)


/******************************************************************/
/* MCBSP_LOOPBACK_DISABLE - takes port out of DLB                 */
/******************************************************************/
#define MCBSP_LOOPBACK_DISABLE(port_no) \
        MCBSP_SUBREG_BITWRITE(port_no, SPCR1_SUBADDR, DLB, DLB_SZ, 0x0000)


#ifdef __INLINE
#undef __INLINE
#endif

#endif /*_MCBSP_H_*/
