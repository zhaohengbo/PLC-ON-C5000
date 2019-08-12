//==========================================================================================
// Filename:		regs54xx.h
//
// Description:		(Extension for regs.h)
//
// Copyright (C) 2000 - 2001 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Author: 			partly Stefan Haas
//
// Revision History:
//==========================================================================================


/******************************************************************/
/* Check to see if mmregs.h has been previously included by       */
/* another header, if so, skip this and go on                     */
/******************************************************************/
#if !defined(__54XXREGS)  
#include <limits.h>

//??? #include "regs.h"

/*----------------------------------------------------------------------------*/
/* MACRO FUNCTIONS                                                            */
/*----------------------------------------------------------------------------*/
#define CONTENTS_OF(addr) \
        (*((volatile unsigned int*)(addr)))

#define LENGTH_TO_BITS(length) \
        (~(0xffffffff << (length)))

/* MACROS to SET, CLEAR and RETURN bits and bitfields in Memory Mapped        */
/* locations using the address of the specified register.                     */

#define REG_READ(addr) \
        (CONTENTS_OF(addr))

#define REG_WRITE(addr,val) \
        (CONTENTS_OF(addr) = (val))

#define MASK_BIT(bit) \
        (1 << (bit))

#define RESET_BIT(addr,bit) \
        (CONTENTS_OF(addr) &= (~MASK_BIT(bit)))

#define GET_BIT(addr,bit) \
        (CONTENTS_OF(addr) & (MASK_BIT(bit)) ? 1 : 0)

#define SET_BIT(addr,bit) \
        (CONTENTS_OF(addr) = (CONTENTS_OF(addr)) | (MASK_BIT(bit)))

#define ASSIGN_BIT_VAL(addr,bit,val) \
        ( (val) ? SET_BIT(addr,bit) : RESET_BIT(addr,bit) )

#define CREATE_FIELD(bit,length) \
        (LENGTH_TO_BITS(length) << (bit))

#define RESET_FIELD(addr,bit,length) \
        ( CONTENTS_OF(addr) &= (~CREATE_FIELD(bit,length)))

#define TRUNCATE(val,bit,length) \
        (((unsigned int)(val) << (bit)) & (CREATE_FIELD(bit, length)))

#define MASK_FIELD(bit,val,length)\
        TRUNCATE(val, bit, length)

#define GET_FIELD(addr,bit,length) \
       ((CONTENTS_OF(addr) & CREATE_FIELD(bit,length)) >> bit)

#define LOAD_FIELD(addr,val,bit,length) \
        (CONTENTS_OF(addr) &= (~CREATE_FIELD(bit,length))\
                               | TRUNCATE(val, bit, length))  


/******************************************************************************/
/* Memory-mapped Byte Manipulation Macros                                     */
/******************************************************************************/
#define CSET_BIT(reg,bit) \
((*((volatile unsigned char *)(reg))) |= (MASK_BIT(bit)))

#define CGET_BIT(reg,bit) \
((*((volatile unsigned char *)(reg))) & (MASK_BIT(bit)) ? 1 : 0)

#define CCLR_BIT(reg,bit) \
((*((volatile unsigned char *)(reg))) &= (~MASK_BIT(bit)))

#define CGET_FIELD(reg,bit,length) \
((*((volatile unsigned char *)(reg)) & (MASK_FIELD(bit,length))) >> bit)

#define CLOAD_FIELD(reg,bit,length,val) \
   ((*((volatile unsigned char *)(reg))) = \
((*((volatile unsigned char *)(reg)) & (~MASK_FIELD(bit,length)))) | (val<<bit))

#define CREG_READ(addr) \
(*((unsigned char *)(addr)))

#define CREG_WRITE(addr,val) \
(*((unsigned char *)(addr)) = (val))

/* MACROS to SET, CLEAR and RETURN bits and bitfields in Memory Mapped        */
/* and Non-Memory Mapped using register names.                                */

#define GET_REG(reg) \
        (reg)

#define SET_REG(reg,val) \
        ((reg)= (val))

#define GET_REG_BIT(reg,bit) \
        ((reg) & MASK_BIT(bit) ? 1 : 0)

#define SET_REG_BIT(reg,bit) \
        ((reg) |= MASK_BIT(bit))

#define RESET_REG_BIT(reg,bit) \
        ((reg) &= (~MASK_BIT(bit)))

#define GET_REG_FIELD(reg,bit,length) \
        (reg & CREATE_FIELD(bit,length)) >> bit)

#define LOAD_REG_FIELD(reg,val,bit,length) \
        (reg &= (~CREATE_FIELD(bit,length)) | (val<<bit))
           
/*****************MCBSP Registers, Bits, Bitfields*****************/
/*-------------------------------------------------------------------*/
/* Define bit fields for Serial Port Control Registers 1 and 2       */
/*-------------------------------------------------------------------*/
#define DLB			15
#define DLB_SZ		 1

#define RJUST		13
#define RJUST_SZ	       2

#define CLKSTP		11
#define CLKSTP_SZ	       2

#define DXENA		 7
#define DXENA_SZ	       1

#define ABIS		 6
#define ABIS_SZ		 1

#define RINTM		 4
#define RINTM_SZ	       2

#define RSYNCERR	       3
#define RSYNCERR_SZ	 1

#define RFULL		 2
#define RFULL_SZ	       1

#define RRDY 		 1
#define RRDY_SZ		 1

#define RRST		 0
#define RRST_SZ		 1

#define FREE		 9
#define FREE_SZ		 1

#define SOFT		 8
#define SOFT_SZ		 1

#define FRST		 7
#define FRST_SZ		 1

#define GRST		 6
#define GRST_SZ		 1

#define XINTM		 4
#define XINTM_SZ	       2

#define XSYNCERR	       3
#define XSYNCERR_SZ	 1

#define XEMPTY		 2
#define XEMPTY_SZ	       1

#define XRDY		 1
#define XRDY_SZ		 1

#define XRST		 0
#define XRST_SZ 	       1

/*-------------------------------------------------------------------*/
/* Define bit fields for Receive Control Registers 1 and 2           */
/*-------------------------------------------------------------------*/
#define RFRLEN1		 8
#define RFRLEN1_SZ	 7

#define RWDLEN1		 5
#define RWDLEN1_SZ	 3

#define RPHASE		15
#define RPHASE_SZ	       1

#define RFRLEN2		 8
#define RFRLEN2_SZ	 7

#define RWDLEN2		 5
#define RWDLEN2_SZ	 3

#define RCOMPAND	       3
#define RCOMPAND_SZ	 2

#define RFIG		 2
#define RFIG_SZ		 1

#define RDATDLY		 0
#define RDATDLY_SZ	 2

/*-------------------------------------------------------------------*/
/* Define bit fields for Transmit Control Registers 1 and 2          */
/*-------------------------------------------------------------------*/
#define XFRLEN1		 8
#define XFRLEN1_SZ	 7

#define XWDLEN1		 5
#define XWDLEN1_SZ	 2

#define XPHASE		15
#define XPHASE_SZ	       1

#define XFRLEN2		 8
#define XFRLEN2_SZ	 7

#define XWDLEN2		 5
#define XWDLEN2_SZ	 3

#define XCOMPAND	       3
#define XCOMPAND_SZ	 2

#define XFIG		 2
#define XFIG_SZ  	       1

#define XDATDLY		 0
#define XDATDLY_SZ       2

/*-------------------------------------------------------------------*/
/* Define bit fields for Sample Rate Generator Registers 1 and 2     */
/*-------------------------------------------------------------------*/
#define FWID		 8
#define FWID_SZ	       8

#define CLKGDV		 0
#define CLKGDV_SZ	       8

#define GSYNC		15
#define GSYNC_SZ	       1

#define CLKSP		14 
#define CLKSP_SZ	       1

#define CLKSM		13 
#define CLKSM_SZ	       1

#define FSGM		12 
#define FSGM_SZ		 1

#define FPER		 0
#define FPER_SZ		12

/*-------------------------------------------------------------------*/
/* Define bit fields for Multi-Channel Control Registers 1 and 2     */
/*-------------------------------------------------------------------*/
#define RPBBLK		 7
#define RPBBLK_SZ	       2

#define RPABLK		 5
#define RPABLK_SZ	       2

#define RCBLK		 2
#define RCBLK_SZ	       3

#define RMCM		 0
#define RMCM_SZ		 1

#define XPBBLK		 7
#define XPBBLK_SZ	       2

#define XPABLK		 5
#define XPABLK_SZ	       2

#define XCBLK		 2
#define XCBLK_SZ	       3

#define XMCM		 0
#define XMCM_SZ		 2

/*-------------------------------------------------------------------*/
/* Define bit fields for Receive Channel Enable Register Partition A */
/*-------------------------------------------------------------------*/
#define RCEA15		15
#define RCEA15_SZ	       1

#define RCEA14		14
#define RCEA14_SZ	       1

#define RCEA13		13 
#define RCEA13_SZ	       1

#define RCEA12		12 
#define RCEA12_SZ	       1
   
#define RCEA11		11 
#define RCEA11_SZ	       1

#define RCEA10		10
#define RCEA10_SZ	       1

#define RCEA9		 9
#define RCEA9_SZ	       1

#define RCEA8		 8
#define RCEA8_SZ	       1

#define RCEA7		 7
#define RCEA7_SZ	       1

#define RCEA6		 6
#define RCEA6_SZ	       1

#define RCEA5		 5
#define RCEA5_SZ	       1

#define RCEA4		 4
#define RCEA4_SZ	       1

#define RCEA3		 3
#define RCEA3_SZ	       1

#define RCEA2		 2
#define RCEA2_SZ	       1

#define RCEA1		 1
#define RCEA1_SZ	       1

#define RCEA0		 0
#define RCEA0_SZ	       1

/*-------------------------------------------------------------------*/
/* Define bit fields for Receive Channel Enable Register Partition B */
/*-------------------------------------------------------------------*/
#define RCEB15		15
#define RCEB15_SZ	       1

#define RCEB14		14
#define RCEB14_SZ	       1

#define RCEB13		13 
#define RCEB13_SZ	       1

#define RCEB12		12 
#define RCEB12_SZ	       1

#define RCEB11		11 
#define RCEB11_SZ	       1

#define RCEB10		10
#define RCEB10_SZ	       1

#define RCEB9		 9
#define RCEB9_SZ	       1

#define RCEB8		 8
#define RCEB8_SZ	       1

#define RCEB7		 7
#define RCEB7_SZ	       1

#define RCEB6		 6
#define RCEB6_SZ	       1

#define RCEB5		 5
#define RCEB5_SZ	       1

#define RCEB4		 4
#define RCEB4_SZ	       1

#define RCEB3		 3
#define RCEB3_SZ	       1

#define RCEB2		 2
#define RCEB2_SZ	       1

#define RCEB1		 1
#define RCEB1_SZ	       1

#define RCEB0		 0
#define RCEB0_SZ	       1

/*-------------------------------------------------------------------*/
/* Define bit fields for Transmit Channel Enable Register Partition A*/
/*-------------------------------------------------------------------*/
#define XCEA15		15
#define XCEA15_SZ	       1

#define XCEA14		14
#define XCEA14_SZ	       1

#define XCEA13		13 
#define XCEA13_SZ	       1

#define XCEA12		12 
#define XCEA12_SZ	       1

#define XCEA11		11 
#define XCEA11_SZ	       1

#define XCEA10		10
#define XCEA10_SZ	       1

#define XCEA9		 9
#define XCEA9_SZ	       1

#define XCEA8		 8
#define XCEA8_SZ	       1

#define XCEA7		 7
#define XCEA7_SZ	       1

#define XCEA6		 6
#define XCEA6_SZ	       1

#define XCEA5		 5
#define XCEA5_SZ	       1

#define XCEA4		 4
#define XCEA4_SZ	       1

#define XCEA3		 3
#define XCEA3_SZ	       1

#define XCEA2		 2
#define XCEA2_SZ	       1

#define XCEA1		 1
#define XCEA1_SZ	       1

#define XCEA0		 0
#define XCEA0_SZ	       1

/*-------------------------------------------------------------------*/
/* Define bit fields for Transmit Channel Enable Register Partition B*/
/*-------------------------------------------------------------------*/
#define XCEB15		15
#define XCEB15_SZ	       1

#define XCEB14		14
#define XCEB14_SZ	       1

#define XCEB13		13 
#define XCEB13_SZ	       1

#define XCEB12		12 
#define XCEB12_SZ	       1

#define XCEB11		11 
#define XCEB11_SZ	       1

#define XCEB10		10
#define XCEB10_SZ	       1

#define XCEB9		 9
#define XCEB9_SZ	       1

#define XCEB8		 8
#define XCEB8_SZ	       1

#define XCEB7		 7
#define XCEB7_SZ	       1

#define XCEB6		 6
#define XCEB6_SZ	       1

#define XCEB5		 5
#define XCEB5_SZ	       1

#define XCEB4		 4
#define XCEB4_SZ	       1

#define XCEB3		 3
#define XCEB3_SZ	       1

#define XCEB2		 2
#define XCEB2_SZ	       1

#define XCEB1		 1
#define XCEB1_SZ	       1

#define XCEB0		 0
#define XCEB0_SZ	       1

/*-------------------------------------------------------------------*/
/* Define bit fields for Pin Control Register		               */
/*-------------------------------------------------------------------*/
#define XIOEN		13
#define XIOEN_SZ	       1

#define RIOEN		12
#define RIOEN_SZ	       1

#define FSXM		11
#define FSXM_SZ 	       1

#define FSRM		10 
#define FSRM_SZ		 1

#define CLKXM		 9
#define CLKXM_SZ	       1

#define CLKRM		 8
#define CLKRM_SZ	       1

#define CLKS_STAT	 6
#define CLKS_STAT_SZ	 1

#define DX_STAT		 5
#define DX_STAT_SZ	 1

#define DR_STAT		 4
#define DR_STAT_SZ	 1

#define FSXP		 3
#define FSXP_SZ		 1

#define FSRP		 2
#define FSRP_SZ		 1

#define CLKXP		 1
#define CLKXP_SZ	       1

#define CLKRP		 0
#define CLKRP_SZ	       1


                      





  

/*****************DMA   Registers, Bits, Bitfields*****************/

/*----------------------------------------------------------------------------*/
/*  Define bit fields for DMPRE Register                                      */
/*----------------------------------------------------------------------------*/
#define DMA_FREE 	     15
#define DMA_FREE_SZ	1

#define DPRC		8
#define DPRC_SZ		6

#define DPRC5		13
#define DPRC5_SZ	       1

#define DPRC4		12
#define DPRC4_SZ	       1

#define DPRC3		11
#define DPRC3_SZ	       1

#define DPRC2		10
#define DPRC2_SZ	       1

#define DPRC1		9
#define DPRC1_SZ	      1

#define DPRC0		8
#define DPRC0_SZ	      1


#define INTSEL		6
#define INTSEL_SZ	      2

#define DE			0
#define DE_SZ		6

#define DE5			5
#define DE5_SZ		1

#define DE4			4
#define DE4_SZ		1

#define DE3			3
#define DE3_SZ		1

#define DE2			2
#define DE2_SZ		1

#define DE1			1
#define DE1_SZ		1

#define DE0			0
#define DE0_SZ		1
            
            
/*----------------------------------------------------------------------------*/
/*  Define bit fields for DMSEFCn Register                                    */
/*----------------------------------------------------------------------------*/

#define FRAMECOUNT	0
#define FRAMECOUNT_SZ	8

#define DSYN		12
#define DSYN_SZ		4

//TYPO #define DLBW		11
//TYPO #define DLBW_SZ		1

#define DBLW		11
#define DBLW_SZ		1


/*----------------------------------------------------------------------------*/
/*  Define bit fields for DMMRCn Register                                     */
/*----------------------------------------------------------------------------*/
#define AUTOINIT	      15
#define AUTOINIT_SZ	1

#define DINM		14
#define DINM_SZ		1

#define IMOD		13
#define IMOD_SZ		1

#define CTMOD		12
#define CTMOD_SZ	      1

#define SIND		8
#define SIND_SZ		3

#define DMS			6
#define DMS_SZ		2

#define DIND		2
#define DIND_SZ		3

#define DMD			0
#define DMD_SZ		2


/****************************/
/* Register Definition  DMA */
/****************************/  
#define DMPREC      *(volatile unsigned int*)0x54
#define DMPRE_ADDR  0x54
#define DMSBA_ADDR	0x55 
#define DMSAI_ADDR	0x56

#define DMA_ACCSUB_ADDR	0x57

#define DMSRC_SUBADDR	0x00
#define DMDST_SUBADDR	0x01
#define DMCTR_SUBADDR	0x02
#define DMSFC_SUBADDR	0x03
#define DMMCR_SUBADDR	0x04

#define DMSRCP_SUBADDR	0x1E
#define DMDSTP_SUBADDR	0x1F
#define DMIDX0_SUBADDR	0x20
#define DMIDX1_SUBADDR	0x21
#define DMFRI0_SUBADDR	0x22
#define DMFRI1_SUBADDR	0x23           
#define DMGSA_SUBADDR	0x24
#define DMGDA_SUBADDR	0x25
#define DMGCR_SUBADDR	0x26
#define DMGFR_SUBADDR	0x27


/******************************************************************/
/* Subregister Read / Write  		       				               */
/******************************************************************/
#define MCBSP_SUBREG_WRITE(port, subaddr, value) \
        ((REG_WRITE(SPSA_ADDR(port), subaddr)), (REG_WRITE(MCBSP_ACCSUB_ADDR(port), value)))

#define MCBSP_SUBREG_READ(port, subaddr) \
        ((REG_WRITE(SPSA_ADDR(port), subaddr)), (REG_READ(MCBSP_ACCSUB_ADDR(port))))


//original #define DMA_SUBREG_WRITE(chan, subaddr, value) \
//original         ((subaddr>=0x1E) ?\
//original         (REG_WRITE(DMSBA_ADDR, (subaddr)), REG_WRITE(DMSAI_ADDR, value))\
//original         :(REG_WRITE(DMSBA_ADDR, (chan*5+subaddr)), REG_WRITE(DMSAI_ADDR, value)) )	

#define DMA_SUBREG_WRITE(chan, subaddr, value) \
        ((subaddr>=0x1E) ?\
        (REG_WRITE(DMSBA_ADDR, (subaddr)), REG_WRITE(DMA_ACCSUB_ADDR, value))\
        :(REG_WRITE(DMSBA_ADDR, (chan*5+subaddr)), REG_WRITE(DMA_ACCSUB_ADDR, value)) )	


#define DMA_SUBREG_READ(chan, subaddr) \
		((subaddr>=0x1E) ?\
        (REG_WRITE(DMSBA_ADDR, (subaddr)), REG_READ(DMA_ACCSUB_ADDR))\
        :(REG_WRITE(DMSBA_ADDR, (chan*5+subaddr)), REG_READ(DMA_ACCSUB_ADDR)) )

//original #define DMA_SUBREG_READ(chan, subaddr) \
//original 		((subaddr>=0x1E) ?\
//original         (REG_WRITE(DMSBA_ADDR, (subaddr)), REG_READ(DMSAI_ADDR))\
//original         :(REG_WRITE(DMSBA_ADDR, (chan*5+subaddr)), REG_READ(DMSAI_ADDR)) )


/******************************************************************/
/* Subregister Bit Field Read / Write     				          */
/******************************************************************/
#define MCBSP_SUBREG_BITWRITE(port, subaddr, bit, size, value) \
       	REG_WRITE(MCBSP_ACCSUB_ADDR(port), (((REG_WRITE(SPSA_ADDR(port), subaddr), REG_READ(MCBSP_ACCSUB_ADDR(port))) & ~CREATE_FIELD(bit, size)) | ((value) << (bit)) ) )
                            
#define MCBSP_SUBREG_BITREAD(port, subaddr, bit, size) \
       	(unsigned int) (REG_WRITE(SPSA_ADDR(port), subaddr), (REG_READ(MCBSP_ACCSUB_ADDR(port)) & CREATE_FIELD(bit, size)) >>(bit) )


#define DMA_SUBREG_BITWRITE(chan, subaddr, bit, size, value) \
       	((subaddr>=0x1E) ?\
       	(REG_WRITE(DMSBA_ADDR, (((REG_WRITE(DMSBA_ADDR, subaddr), REG_READ(DMA_ACCSUB_ADDR)) & ~CREATE_FIELD(bit, size)) | ((value) << (bit)) ) ) )\
        :(REG_WRITE(DMSBA_ADDR, (((REG_WRITE(DMSBA_ADDR, (chan)), (REG_READ(DMA_ACCSUB_ADDR) & CREATE_FIELD(bit, size))>>(bit)))))))                                                                    
		
#define DMA_SUBREG_BITREAD(chan, subaddr, bit, size) \
       	((subaddr>=0x1E) ?\
       	((unsigned int) (REG_WRITE(DMSBA_ADDR, subaddr), (REG_READ(DMA_ACCSUB_ADDR) & CREATE_FIELD(bit, size)) >>(bit) ))\
       	:((unsigned int) (REG_WRITE(DMSBA_ADDR, (chan*5+subaddr)), (REG_READ(DMA_ACCSUB_ADDR) & CREATE_FIELD(bit, size)) >>(bit) )))
 

/*-------------------------------------------------------------------------*/
/*  |     The following part of 54XXregs.h was not needed for my purposes. */
/*  |     It has already been included in the regs.h I have received from  */
/*  \/    Karen Baldwin (?) some time ago.                                 */


/********************/
/* Interrupt Vectors*/
/********************/
#define BASE_VEC_ADR    0x80
#define RESET_VEC		   0x0
#define NMI_VEC			4
#define SINT17_VEC		8
#define SINT18_VEC		12
#define SINT19_VEC		16
#define SINT20_VEC		20
#define SINT21_VEC		24
#define SINT22_VEC		28
#define SINT23_VEC		32
#define SINT24_VEC		36
#define SINT25_VEC		40
#define SINT26_VEC		44
#define SINT27_VEC		48
#define SINT28_VEC		52
#define SINT29_VEC		56
#define SINT30_VEC		60
#define INT0_VEC		   64
#define INT1_VEC		   68
#define INT2_VEC		   72
#define TINT0_VEC		   76
#define RINT0_VEC		   80
#define XINT0_VEC		   84
#define DMAC0_VEC		   88
#define TINT1_VEC		   92
#define INT3_VEC		   96
#define HPI_VEC			100
#define RINT1_VEC		   104
#define XINT1_VEC		   108
#define DMAC2_VEC		   104
#define DMAC3_VEC		   108
#define DMAC4_VEC		   112
#define DMAC5_VEC		   116


/*********************************************************************/
/* Define data structures for all memory mapped registers            */
/*********************************************************************/
/*----------------------------------------------------------------*/
/* Data bitfields Period for Timer                                */
/*----------------------------------------------------------------*/
#define	TIMSOFT		11
#define	TIMSOFT_SZ	 1

#define	TIMFREE		10
#define	TIMFREE_SZ	 1
			
#define PSC		     6
#define PSC_SZ		 4

#define TRB			 5
#define TRB_SZ		 1

#define TSS			 4
#define TSS_SZ		 1

#define TDDR		 0
#define TDDR_SZ		 4

/*---------------------------------------------------------------*/
/* Data bitfields for Clock Mode Register                        */
/*---------------------------------------------------------------*/
#define PLLMUL		12
#define PLLMUL_SZ	 	4

#define PLLDIV		11
#define PLLDIV_SZ	 	1

#define PLLCOUNT	 	3
#define PLLCOUNT_SZ	8

#define PLLON_OFF	 	2
#define PLLON_OFF_SZ 	1

#define PLLNDIV		1
#define PLLNDIV_SZ	1

#define PLLSTATUS	 	0
#define PLLSTATUS_SZ 	1


/*----------------------------------------------------------------*/
/* Define bit fields for Software Wait State Register             */
/*----------------------------------------------------------------*/
#define IO		    	12
#define IO_SZ		 3

#define DATA_HI		 9
#define DATA_HI_SZ	 3

#define DATA_LO		 6
#define DATA_LO_SZ	 3

#define PROGRAM_HI	 3
#define PROGRAM_HI_SZ	 3

#define PROGRAM_LO	 0
#define PROGRAM_LO_SZ	 3

/*-------------------------------------------------------------------*/
/* Define bitfields for Bank Switch Control Register                 */
/*-------------------------------------------------------------------*/
#define BNKCMP		12
#define BNKCMP_SZ	 	4

#define PS_DS		11
#define PS_DS_SZ	 	1

#define HBH		     	 2
#define HBH_SZ		 1

#define BH		     	 1
#define BH_SZ		 1

#define EXIO		     	 0
#define EXIO_SZ		 1

/*-------------------------------------------------------------------*/
/* Define bitfields for Interruput Mask Register                     */
/*-------------------------------------------------------------------*/
#define INT0		 0
#define INT1		 1
#define INT2		 2
#define TINT0		 3

#define RINT0		 4
#define XINT0		 5

#define TINT1		 7

#define INT3		 8
#define HPI			 9
#define RINT1		10
#define XINT1		11

#define DMAC0		6
#define DMAC1		7
#define DMAC2		10
#define DMAC3		11
#define DMAC4		12
#define DMAC5		13


/*-------------------------------------------------------------*/
/* DEFINE DATA STRUCTURE FOR HOST PORT INTERFACE CONTROL REG   */
/*-------------------------------------------------------------*/
#define BOB		     0
#define SMOD		 1
#define DSPINT		 2
#define HINT		 3
#define XHPIA		 4

/******************************************************************/
/* Define Interrupt Flag and Interrupt Mask Registers             */
/******************************************************************/
#define IMR	*(volatile unsigned int*)0x00
#define IMR_ADDR		0x0 

#define IFR	*(volatile unsigned int*)0x01
#define IFR_ADDR		0x1

/******************************************************************/
/* NOTE:  YOU CAN ACCESS THESE REGISTERS IN THIS MANNER ONLY	  */
/* IF THE SUBADDRESS REGISTER HAS BEEN DEFINED ALREADY  		  */
/******************************************************************/

/******************************************************************/
/* MultiChannel Buffer Serial 0 defined for 54XX				  */
/******************************************************************/
	#define SPCR10	*(volatile unsigned int*)0x39
	#define SPCR10_ADDR	0x39

	#define SPCR20	*(volatile unsigned int*)0x39
	#define SPCR20_ADDR	0x39

	#define DRR20	*(volatile unsigned int*)0x20
	#define DRR20_ADDR	0x20

	#define DRR10	*(volatile unsigned int*)0x21
	#define DRR10_ADDR	0x21

	#define DXR20	*(volatile unsigned int*)0x22
	#define DXR20_ADDR	0x22

	#define DXR10	*(volatile unsigned int*)0x23
	#define DXR10_ADDR	0x23

	#define	SPSA0_ADDR			0x38
	#define	SPSD0_ADDR			0x39
	#define	MCBSP_ACCSUB0_ADDR	0x39

/******************************************************************/
/* MultiChannel Buffer Serial 1 defined for 54XX				  */
/******************************************************************/
	#define SPCR11	*(volatile unsigned int*)0x49
	#define SPCR11_ADDR	0x49

	#define SPCR21	*(volatile unsigned int*)0x49
	#define SPCR21_ADDR	0x49

	#define DRR21	*(volatile unsigned int*)0x40
	#define DRR21_ADDR	0x40

	#define DRR11	*(volatile unsigned int*)0x41
	#define DRR11_ADDR	0x41

	#define DXR21	*(volatile unsigned int*)0x42     

	#define DXR21_ADDR	0x42

	#define DXR11	*(volatile unsigned int*)0x43
	#define DXR11_ADDR	0x43

	#define	SPSA1_ADDR			0x48
	#define	SPSD1_ADDR			0x49
	#define	MCBSP_ACCSUB1_ADDR	0x49

/******************************************************************/
/* MultiChannel Buffer Serial 2 defined for 54XX				  */
/******************************************************************/
	#define SPCR12	*(volatile unsigned int*)0x35
	#define SPCR12_ADDR	0x35 

	#define SPCR22	*(volatile unsigned int*)0x35
	#define SPCR22_ADDR	0x35 

	#define DRR22	*(volatile unsigned int*)0x30
	#define DRR22_ADDR	0x30

	#define DRR12	*(volatile unsigned int*)0x31
	#define DRR12_ADDR	0x31

	#define DXR22	*(volatile unsigned int*)0x32
	#define DXR22_ADDR	0x32

	#define DXR12	*(volatile unsigned int*)0x33
	#define DXR12_ADDR	0x33

	#define	SPSA2_ADDR			0x34
	#define	SPSD2_ADDR			0x35
	#define	MCBSP_ACCSUB2_ADDR	0x35



/*******************************/
/* Register Definition  MCBSP  */
/*******************************/  

/*-----------------PORT----------------|------2--------------|-----1------|------0------|*/
//Added by T. kyiamah
#define SPCR1_ADDR(port)	((port==2) ? SPCR12_ADDR: (port ? SPCR11_ADDR : SPCR10_ADDR))
#define SPCR2_ADDR(port)	((port==2) ? SPCR22_ADDR: (port ? SPCR21_ADDR : SPCR20_ADDR))

#define SPSA_ADDR(port)		((port==2) ? SPSA2_ADDR: (port ? SPSA1_ADDR : SPSA0_ADDR))
#define SPSD_ADDR(port)		((port==2) ? SPSD2_ADDR: (port ? SPSD1_ADDR : SPSD0_ADDR))

#define DRR2_ADDR(port)		((port==2) ? DRR22_ADDR: (port ? DRR21_ADDR : DRR20_ADDR))
#define DRR1_ADDR(port)		((port==2) ? DRR12_ADDR: (port ? DRR11_ADDR : DRR10_ADDR))
#define DXR2_ADDR(port)		((port==2) ? DXR22_ADDR: (port ? DXR21_ADDR : DXR20_ADDR))
#define DXR1_ADDR(port)		((port==2) ? DXR12_ADDR: (port ? DXR11_ADDR : DXR10_ADDR))

#define MCBSP_ACCSUB_ADDR(port)	((port==2) ? MCBSP_ACCSUB2_ADDR: (port ? MCBSP_ACCSUB1_ADDR : MCBSP_ACCSUB0_ADDR))



#define MCBSP0_SUBREG_WRITE(subaddr, value) \
        ((REG_WRITE(SPSA0_ADDR, subaddr)), (REG_WRITE(SPSD0_ADDR, value)))

#define MCBSP0_SUBREG_READ(subaddr) \
        ((REG_WRITE(SPSA0_ADDR, subaddr)), (REG_READ(SPSD0_ADDR)))

#define MCBSP0_SUBREG_BITWRITE(subaddr, bit, size, value) \
       	REG_WRITE(SPSD0_ADDR, (((REG_WRITE(SPSA0_ADDR, subaddr), REG_READ(SPSD0_ADDR)) & ~CREATE_FIELD(bit, size)) | ((value) << (bit)) ) )
                            
#define MCBSP0_SUBREG_BITREAD(subaddr, bit, size) \
       	(unsigned int) (REG_WRITE(SPSA0_ADDR, subaddr), (REG_READ(SPSD0_ADDR) & CREATE_FIELD(bit, size)) >>(bit) )



#define MCBSP1_SUBREG_WRITE(subaddr, value) \
        ((REG_WRITE(SPSA1_ADDR, subaddr)), (REG_WRITE(SPSD1_ADDR, value)))

#define MCBSP1_SUBREG_READ(subaddr) \
        ((REG_WRITE(SPSA1_ADDR, subaddr)), (REG_READ(SPSD1_ADDR)))

#define MCBSP1_SUBREG_BITWRITE(subaddr, bit, size, value) \
       	REG_WRITE(SPSD1_ADDR, (((REG_WRITE(SPSA1_ADDR, subaddr), REG_READ(SPSD1_ADDR)) & ~CREATE_FIELD(bit, size)) | ((value) << (bit)) ) )
                            
#define MCBSP1_SUBREG_BITREAD(subaddr, bit, size) \
       	(unsigned int) (REG_WRITE(SPSA1_ADDR, subaddr), (REG_READ(SPSD1_ADDR) & CREATE_FIELD(bit, size)) >>(bit) )




#define MCBSP2_SUBREG_WRITE(subaddr, value) \
        ((REG_WRITE(SPSA2_ADDR, subaddr)), (REG_WRITE(SPSD2_ADDR, value)))

#define MCBSP2_SUBREG_READ(subaddr) \
        ((REG_WRITE(SPSA2_ADDR, subaddr)), (REG_READ(SPSD2_ADDR)))


#define MCBSP2_SUBREG_BITWRITE(subaddr, bit, size, value) \
       	REG_WRITE(SPSD2_ADDR, (((REG_WRITE(SPSA2_ADDR, subaddr), REG_READ(SPSD2_ADDR)) & ~CREATE_FIELD(bit, size)) | ((value) << (bit)) ) )
                            
#define MCBSP2_SUBREG_BITREAD(subaddr, bit, size) \
       	(unsigned int) (REG_WRITE(SPSA2_ADDR, subaddr), (REG_READ(SPSD2_ADDR) & CREATE_FIELD(bit, size)) >>(bit) )






#define SPCR1_SUBADDR	0x00
#define SPCR2_SUBADDR	0x01
#define RCR1_SUBADDR	0x02
#define RCR2_SUBADDR	0x03
#define XCR1_SUBADDR 	0x04
#define XCR2_SUBADDR	0x05
#define SRGR1_SUBADDR	0x06
#define SRGR2_SUBADDR	0x07
#define MCR1_SUBADDR	0x08
#define MCR2_SUBADDR	0x09
#define RCERA_SUBADDR	0x0A
#define RCERB_SUBADDR	0x0B
#define XCERA_SUBADDR	0x0C
#define XCERB_SUBADDR	0x0D
#define PCR_SUBADDR	    0x0E


/******************************************************************/
/* Direct Memory Access defined for 54XX				          */
/******************************************************************/
#define DMPRE	  (0x54)
#define DMSBA	  (0x55)
#define DMSAI	  (0x56)
#define DMSRCP    (0x57)
#define DMDSTP    (0x57)
#define DMGSA     (0x57)
#define DMGDA     (0x57)
#define DMGCR     (0x57)
#define DMGFR     (0x57)
#define DMFRI(reg) ((reg) ? 0x57:0x57)
#define DMIDX(reg) ((reg) ? 0x57:0x57)
#define DMSRC(channel) ((channel) ? 0x57:0x57)
#define DMDST(channel) ((channel) ? 0x57:0x57)
#define DMCTR(channel) ((channel) ? 0x57:0x57)
#define DMSEFC(channel) ((channel) ? 0x57:0x57)
#define DMMCR(channel) ((channel) ? 0x57:0x57

#define DMA_REG_READ(dma_subaddress, channel) (DMSAI(channel)=dma_subaddress), *(volatile unsigned int*) DMFRI(channel))




/*----------------------------------------------------------------*/
/* Data bitfields Period for DMPRE                                */
/*----------------------------------------------------------------*/
#define DPRC5		13
#define DPRC5_SZ	 1

#define DPRC4		12
#define DPRC4_SZ	 1

#define DPRC3		11
#define DPRC3_SZ	 1

#define DPRC2		10
#define DPRC2_SZ	 1

#define DPRC1		 9
#define DPRC1_SZ	 1

#define DPRC0		 8
#define DPRC0_SZ	 1

#define INTSEL		 6
#define INTSEL_SZ    2

#define DE5			 5
#define DE5_SZ		 1

#define DE4			 4
#define DE4_SZ		 1

#define DE3			 3
#define DE3_SZ		 1

#define DE2			 2
#define DE2_SZ		 1

#define DE1			 1
#define DE1_SZ		 1

#define DE0			 0
#define DE0_SZ		 1

/*----------------------------------------------------------------*/
/* Data bitfields Period for DMSEFCn                              */
/*----------------------------------------------------------------*/
#define DSYN		12
#define DSYN_SZ		 4

#define FRAME_CNT    0
#define FRAME_CNT_SZ 8

/*----------------------------------------------------------------*/
/* Data bitfields Period for Mode Control Register                */
/*----------------------------------------------------------------*/
#define AUTOINIT	15
#define AUTOINIT_SZ	 1

#define DINM	    14
#define DINM_SZ      1

#define IMOD		13
#define IMOD_SZ      1

#define CTMOD		12
#define CTMOD_SZ     1

#define SIND		 8
#define SIND_SZ		 3

#define DMS			 6
#define DMS_SZ       2

#define DIND		 2
#define DIND_SZ      3

#define DMD			 0
#define DMD_SZ       2

/*******************************************************************/
/* TIMER REGISTER ADDRESSES   (TIM0 = Timer 0, TIM1 = Timer 1      */
/* Defined for all devices                                         */
/*******************************************************************/
#define TIM_ADDR(port) (port ? 0x30 : 0x24)
#define TIM(port)	*(volatile unsigned int*)TIM_ADDR(port)

#define PRD_ADDR(port)		(port ? 0x31 : 0x25)
#define PRD(port)	*(volatile unsigned int*)PRD_ADDR(port)

#define TCR_ADDR(port)		(port ? 0x32 : 0x26)
#define TCR(port)	*(volatile unsigned int*)TCR_ADDR(port)

/*********************************************************************/
/* EXTERNAL BUS CONTROL REGISTERS                                    */
/*********************************************************************/
#define BSCR	*(volatile unsigned int*)0x29
#define BSCR_ADDR	0x29

#define SWCR	*(volatile unsigned int*)0x2B
#define SWCR_ADDR	0x2B

#define SWWSR	*(volatile unsigned int*)0x28
#define SWWSR_ADDR	0x28

/*********************************************************************/
/* HOST PORT INTERFACE REGISTER ADDRESS                              */
/* Defined for C54XX					                             */
/*********************************************************************/
#define HPIC	*(volatile unsigned int*)0x2C
#define HPIC_ADDR	0x2C
#define HPI_ADDR	0x1000 

/*********************************************************************/
/* Defined flags for use in setting control for HPI host interface   */
/* control pins                                                      */
/* The value of these constants is their relative bit position in    */
/* the control structure for the host side of the HPI interface      */
/*********************************************************************/
#define HAS_PIN		0    
#define HBIL_PIN	   1    
#define HCNTL0_PIN	2    
#define HCNTL1_PIN	3
#define HCS_PIN		4
#define HD0_PIN		5
#define HDS1_PIN	   6
#define HDS2_PIN  	7
#define HINT_PIN  	8
#define HRDY_PIN	   9
#define HRW_PIN		10

/*********************************************************************/
/* CLOCK MODE REGISTER ADDRESS                                       */
/* Defined for C54XX				                                       */
/*********************************************************************/
#define CLKMD 	*(volatile unsigned int*)0x58
#define CLKMD_ADDR	0x58

/*********************************************************************/
/* Extended Program Counter -XPC register                            */
/*********************************************************************/
extern volatile unsigned int XPC;
#define XPC	*(volatile unsigned int*)0x1e
#define XPC_ADDR		0x1e

/*********************************************************************/
/* Program Control and Status Registers (PMST, ST0, ST1)             */
/*********************************************************************/
#define PMST	*(volatile unsigned int*)0x1d
#define PMST_ADDR	0x1d

#define ST0	*(volatile unsigned int*)0x06
#define ST0_ADDR	0x06

#define ST1	*(volatile unsigned int*)0x07
#define ST1_ADDR	0x07

/*********************************************************************/
/* General-purpose I/O pins control registers (GPIOCR, GPIOSR)       */
/*********************************************************************/
#define GPIOCR	*(volatile unsigned int*)0x3C
#define GPIOCR_ADDR	0x3C

#define GPIOSR	*(volatile unsigned int*)0x3D
#define GPIOSR_ADDR	0x3D

#define __54XXREGS
#endif
