//==========================================================================================
// Filename:		dma.h
//
// Description:		Generic template which includes headers in proper format.
//
// Copyright (C) 2000 - 2001 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#ifndef dma_h						// Header file guard
#define dma_h


//==========================================================================================
// System #include files <filename.h>
// Application #includes "filename.h"
//==========================================================================================


//==========================================================================================
// #Defined Compile-Time Flags
//==========================================================================================

//==========================================================================================
// #Defined Constant Definitions
//==========================================================================================


//==========================================================================================
// Macro Definitions
//==========================================================================================


//==========================================================================================
// External Functions
//==========================================================================================


//==========================================================================================
// External Variables
//==========================================================================================


//==========================================================================================
// Constants
//==========================================================================================
// --------- DMA Channel Numbers ----------
	#define	DMA_CHANNEL0	0
	#define	DMA_CHANNEL1	1
	#define	DMA_CHANNEL2	2
	#define	DMA_CHANNEL3	3
	#define	DMA_CHANNEL4	4
	#define	DMA_CHANNEL5	5

#if (PROCESSOR == 5410) 
// -------- DMA Synchronization Modes ----------
	#define	DMASYNC_NOSYNC	0x0		// No Sync Event (nonsynchronized operation)
	#define DMASYNC_REVT0	0x1		// MCBSP 0 Receive  Event (REVT0)
	#define DMASYNC_XEVT0	0x2		// MCBSP 0 Transmit Event (XEVT0)
	#define DMASYNC_REVT2	0x3		// MCBSP 2 Receive  Event (REVT2)
	#define DMASYNC_XEVT2	0x4		// MCBSP 2 Transmit Event (XEVT2)
	#define DMASYNC_REVT1	0x5		// MCBSP 1 Receive  Event (REVT1)
	#define DMASYNC_XEVT1	0x6		// MCBSP 1 Transmit Event (XEVT1)
	#define DMASYNC_REVTA0	0x7		// MCBSP 0 Receive Event  - ABIS Mode (REVTA0)
	#define DMASYNC_XEVTA0	0x8		// MCBSP 0 Transmit Event - ABIS Mode (XEVTA0)
	#define DMASYNC_REVTA2	0x9		// MCBSP 2 Receive Event  - ABIS Mode (REVTA2)
	#define DMASYNC_XEVTA2	0xA		// MCBSP 2 Transmit Event - ABIS Mode (XEVTA2)
	#define DMASYNC_REVTA1	0xB		// MCBSP 1 Receive Event  - ABIS Mode (REVTA1)
	#define DMASYNC_XEVTA1	0xC		// MCBSP 1 Transmit Event - ABIS Mode (XEVTA1)
	#define DMASYNC_TINT	0xD		// Timer Interrupt Event
	#define DMASYNC_XINT3	0xE		// External Interrupt 3 (INT3) Event
	#define DMASYNC_RSVD	0xF		// Reserved

#elif ((PROCESSOR == 5402) || (PROCESSOR == 5472))
// -------- DMA Synchronization Modes ----------
	#define	DMASYNC_NOSYNC	0x0		// No Sync Event (nonsynchronized operation)
	#define DMASYNC_REVT0	0x1		// MCBSP 0 Receive  Event (REVT0)
	#define DMASYNC_XEVT0	0x2		// MCBSP 0 Transmit Event (XEVT0)
	#define DMASYNC_REVT1	0x5		// MCBSP 1 Receive  Event (REVT1)
	#define DMASYNC_XEVT1	0x6		// MCBSP 1 Transmit Event (XEVT1)
	#define DMASYNC_TINT0	0xD		// Timer 0 Interrupt Event
	#define DMASYNC_XINT3	0xE		// External Interrupt 3 (INT3) Event
	#define DMASYNC_TINT1	0xF		// Timer 1 Interrupt Event
#endif

	#define RUN 	1
	#define STOP 	0
	#define HI 		1
	#define LO		0
	#define EN 		1
	#define DIS		0
// ---------- DMA Interrupt Multiplexer Control Bits ----------
// These bits select how the DMA interrupts will be assigned in the interrupt vector table
// and IMR/IMF registers.
// Note that SINT7/TINT1/DMACH1 is available on the 5402 but not 5472(Orion).
	#define INTOSEL0	0	// SINT7= Timer 1 Int,  SINT10= MCBSP 1 RINT, SINT11= MCBSP 1 XINT
	#define INTOSEL1	1	// SINT7= Timer 1 Int,  SINT10= DMA Ch 2 Int, SINT11= DMA Ch 3 Int
	#define INTOSEL2	2	// SINT7= DMA Ch 1 Int, SINT10= DMA Ch 2 Int, SINT11= DMA Ch 3 Int
  //#define INTOSEL3	3   // -- Reserved: Do not use --	


//==========================================================================================
// Structures
//==========================================================================================

//==========================================================================================
// Forward Declarations
//==========================================================================================


//==========================================================================================
// Function Prototypes
//==========================================================================================
void DMA_reset(void);

void DMA_init(  unsigned int channel, 
                unsigned int source,
                unsigned int destination,
                unsigned int count, 
                unsigned int frame_sync, 
                unsigned int control_mode);



//==========================================================================================
#endif									// End of header guard: #ifndef dma_h


