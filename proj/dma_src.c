//==========================================================================================
// Filename:		dma_src.c
//
// Copyright (C) 2000 - 2001 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================


/***********************************************************************/
/*																	   */
/*Function:     DMA_reset()                                            */
/*file name:	dma_src.c											   */
/*Description:  This function performs a reset of all DMA  registers.  */
/*Inputs:   	None												   */
/*Outputs:  	None	                                               */
/*Returns: 		None												   */
/* AUTHOR: 		AAP Application Group, L. Philipose, Dallas            */
/*              CREATED 2000(C) BY TEXAS INSTRUMENTS INCORPORATED.     */
/***********************************************************************/

#include "ofdm_modem.h"
#include "ofdm_datatypes.h"
#include "c5402Reg.h"

void DMA_reset(void)
{ 	/* DMA Initialization */
    /* Zero all Frame and Sync Registers */
    
    /* Clear Channel 0 */
	DMSBA_ADDR = DMSFC0_SUB;        /* Sub-address Frame-Sync 		*/
    DMSFC0_ADDR->value = 0x0;

    DMSBA_ADDR = DMSRC0_SUB;        /* Sub-address Source 			*/
    DMSRC0_ADDR = 0x0;

    /* Clear Channel 1 */
    DMSBA_ADDR = DMSFC1_SUB;        /* Sub-address Frame/Sync 		*/
    DMSFC1_ADDR->value = 0x0;

    DMSBA_ADDR = DMSRC1_SUB;        /* Sub-address Source 			*/
    DMSRC1_ADDR = 0x0;
        
    /* Clear Channel 2 */
    DMSBA_ADDR = DMSFC2_SUB;        /* Sub-address Frame/Sync 		*/
    DMSFC2_ADDR->value = 0x0;

    DMSBA_ADDR = DMSRC2_SUB;        /* Sub-address Source 			*/
    DMSRC2_ADDR = 0x0;

    /* Clear Channel 3 */
    DMSBA_ADDR = DMSFC3_SUB;        /* Sub-address Frame/Sync 		*/
    DMSFC3_ADDR->value = 0x0;

    DMSBA_ADDR = DMSRC3_SUB;        /* Sub-address Source 			*/
    DMSRC3_ADDR = 0x0;
        
 	/* Clear Channel 4 */
    DMSBA_ADDR = DMSFC4_SUB;        /* Sub-address Frame/Sync 		*/
    DMSFC4_ADDR->value = 0x0;

    DMSBA_ADDR = DMSRC4_SUB;        /* Sub-address Source 			*/
    DMSRC4_ADDR = 0x0;
        
    /* Clear Channel 5 */
    DMSBA_ADDR = DMSFC5_SUB;        /* Sub-address Frame/Sync 		*/
    DMSFC5_ADDR->value = 0x0;

    DMSBA_ADDR = DMSRC5_SUB;        /* Sub-address Source 			*/
    DMSRC5_ADDR = 0x0;

    /* General Stop/Reset of DMA */
    DMAPREC_ADDR->value = 0x0;      /* STOP all DMA channels 								*/
        
    DMSBA_ADDR = DMSRCP_SUB;        /* Sub-address Source Program Page Address 				*/
    DMSRCP_ADDRA->value = 0x0;      /* Use auto-increment address 							*/
    DMDSTP_ADDRA->value = 0x0;      /* Destination page (Auto) 								*/
    DMIDX0_ADDRA = 0x0;     		/* Index Register 0 (Auto) 								*/
    DMIDX1_ADDRA = 0x0; 			/* Index Register 1 (Auto) 								*/
    DMFRI0_ADDRA = 0x0;     		/* Frame Index Register 0 (Auto) 						*/
    DMFRI1_ADDRA = 0x0;     		/* Frame index Register 1 (Auto) 						*/
    DMSGA_ADDRA = 0x0;      		/* Global Source Address Reload Register (Auto) 		*/
    DMGDA_ADDRA = 0x0;      		/* Global Destination Address Reload Register (Auto) 	*/
    DMGCR_ADDRA = 0x0;      		/* Global Count Reload Register (Auto) 					*/
    DMGFR_ADDRA = 0x0;      		/* Global Frame Count Reload Register (Auto) 			*/
        
}

/***********************************************************************/
/*																	   */
/*DMA_init                                                             */
/*file name:	dma_src.c											   */
/*Description:  This function performs the initialization of all DMA   */
/*              channels. It is capable of initializing the channels   */
/*				one at a time.										   */
/*																	   */
/*Inputs:   	channel - selects the channel to be initialized 0,1    */
/*				source - source of data (input/read)				   */
/*				destination - destination of data (output/write)	   */
/*				count - number of DMA transfers toi be performed  	   */
/*				frame_sync - element that initiates each transfer      */
/*				control_mode - transfer mode control                   */
/*																	   */
/*Outputs:  	None	                                               */
/*Returns: 		None												   */
/*Note: 		Although DMA_init sets up the channel, it does NOT     */
/*  start the DMA channel running. This must be done separately by     */
/* 	modifying the DMPREC register.  								   */
/*																	   */
/* AUTHOR        : AAP Application Group, L. Philipose, Dallas         */
/*                 CREATED 2000(C) BY TEXAS INSTRUMENTS INCORPORATED.  */
/***********************************************************************/

void DMA_init(  u16 channel, 
                u16 source,
                u16 destination,
                u16 count, 
                u16 frame_sync, 
                u16 control_mode)
{

	switch(channel)
	{
        case 0: /*Intialize DMA channel 0 Registers*/

                DMSBA_ADDR = DMSRC0_SUB;        /* Sub Address Register */
                DMSRC0_ADDRA = source;
                DMDST0_ADDRA = destination;
                DMCTR0_ADDRA = count;
                DMSFC0_ADDRA->value = frame_sync;
                DMMCR0_ADDRA->value = control_mode;
                break;
                
        case 1:  /*Intialize DMA channel 1 Registers*/
                DMSBA_ADDR = DMSRC1_SUB;        /* Sub Address Register */
                DMSRC1_ADDRA = source;
                DMDST1_ADDRA = destination;
                DMCTR1_ADDRA = count;
                DMSFC1_ADDRA->value = frame_sync;
                DMMCR1_ADDRA->value = control_mode;
                break;
                
                
        case 2:  /*Intialize DMA channel 2 Registers*/
                DMSBA_ADDR = DMSRC2_SUB;        /* Sub Address Register */
                DMSRC2_ADDRA = source;
                DMDST2_ADDRA = destination;
                DMCTR2_ADDRA = count;
                DMSFC2_ADDRA->value = frame_sync;
                DMMCR2_ADDRA->value = control_mode;
                break;
                
                
        case 3:  /*Intialize DMA channel 3 Registers*/
                DMSBA_ADDR = DMSRC3_SUB;        /* Sub Address Register */
                DMSRC3_ADDRA = source;
                DMDST3_ADDRA = destination;
                DMCTR3_ADDRA = count;
                DMSFC3_ADDRA->value = frame_sync;
                DMMCR3_ADDRA->value = control_mode;
                break;
                
                
        case 4:  /*Intialize DMA channel 4 Registers*/
                DMSBA_ADDR = DMSRC4_SUB;        /* Sub Address Register */
                DMSRC4_ADDRA = source;
                DMDST4_ADDRA = destination;
                DMCTR4_ADDRA = count;
                DMSFC4_ADDRA->value = frame_sync;
                DMMCR4_ADDRA->value = control_mode;
                break;
                
                
        case 5:  /*Intialize DMA channel 5 Registers*/
                DMSBA_ADDR = DMSRC5_SUB;        /* Sub Address Register */
                DMSRC5_ADDRA = source;
                DMDST5_ADDRA = destination;
                DMCTR5_ADDRA = count;
                DMSFC5_ADDRA->value = frame_sync;
                DMMCR5_ADDRA->value = control_mode;
                break;
	}
}



