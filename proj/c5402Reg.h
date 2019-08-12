//==========================================================================================
// Filename:		c5402Reg.h
//
// Copyright (C) 2000 - 2001 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================


/***********************************************************************/
/*Function:     None        	                                       */
/*file name:	c5402Reg.h											   */
/*Description:  File defines ONLY those DSP, McBSP1, and DMA Registers */
/*				objects used in conjunction with this Application Note.*/
/*Inputs:   	None												   */
/*Outputs:  	None	                                               */
/*Returns: 		None												   */
/* AUTHOR        : AAP Application Group, L. Philipose, Dallas         */
/*                 CREATED 2000(C) BY TEXAS INSTRUMENTS INCORPORATED.  */
/***********************************************************************/


/******************************************************************/
/* Define Interrupt Flag and Interrupt Mask Registers             */
/******************************************************************/
#define IMR_BASE        0x00
#define IMR_ADDR        ((volatile IMR_REG *)   ((char *) IMR_BASE))

#define IFR_BASE        0x01
#define IFR_ADDR        ((volatile IFR_REG *)   ((char *) IFR_BASE))


typedef union {
        struct {
                unsigned int res        :2;
                unsigned int dmac5      :1;
                unsigned int dmac4      :1;
                unsigned int bxint1     :1;
                unsigned int brint1     :1;
                unsigned int hint       :1;
                unsigned int int3       :1;
                unsigned int tint1      :1;
                unsigned int dmac0      :1;
                unsigned int bxint0     :1;
                unsigned int brint0     :1;
                unsigned int tint0      :1;
                unsigned int int2       :1;
                unsigned int int1       :1;
                unsigned int int0       :1;
                } bitval;
        unsigned int value;
} IFR_REG;

typedef union {
        struct {
                unsigned int res        :2;
                unsigned int dmac5      :1;
                unsigned int dmac4      :1;
                unsigned int bxint1     :1;
                unsigned int brint1     :1;
                unsigned int hint       :1;
                unsigned int int3       :1;
                unsigned int tint1      :1;
                unsigned int dmac0      :1;
                unsigned int bxint0     :1;
                unsigned int brint0     :1;
                unsigned int tint0      :1;
                unsigned int int2       :1;
                unsigned int int1       :1;
                unsigned int int0       :1;
                } bitval;
        unsigned int value;
} IMR_REG;



/******************************************************************/
/*Status Registers  */
/******************************************************************/
#define ST0_BASE        0x06
#define ST1_BASE        0x07
#define ST0_ADDR        ((volatile ST0_REG *)   ((char *) ST0_BASE))
#define ST1_ADDR        ((volatile ST1_REG *)   ((char *) ST1_BASE))


typedef union {
        struct {
                unsigned int arp        :3;
                unsigned int tc         :1;
                unsigned int c          :1;
                unsigned int ova        :1;
                unsigned int ovb        :1;
                unsigned int dp         :9;
                } bitval;
        unsigned int value;
} ST0_REG;

typedef union {
        struct {
                unsigned int braf       :1;
                unsigned int cpl        :1;
                unsigned int xf         :1;
                unsigned int hm         :1;
                unsigned int intm       :1;
                unsigned int zero       :1;
                unsigned int ovm        :1;
                unsigned int sxm        :1;
                unsigned int c16        :1;
                unsigned int frct       :1;
                unsigned int cmpt       :1;
                unsigned int asmm       :5;
                } bitval;
        unsigned int value;
} ST1_REG;
       
/******************************************************************/
/*PMST */
/******************************************************************/
#define PMST_BASE       0x1d
#define PMST_ADDR       ((volatile PMST_REG *)  ((char *) PMST_BASE))


typedef union {
        struct {
                unsigned int iptr       :9;
                unsigned int mpmc       :1;
                unsigned int ovly       :1;
                unsigned int avis       :1;
                unsigned int drom       :1;
                unsigned int clkoff     :1;
                unsigned int smul       :1;
                unsigned int sst        :1;
                } bitval;
        unsigned int value;
} PMST_REG;



/*********************************************************************/
/* Structure for McBSP */
/*********************************************************************/
/*-------------------------------------------------------------------*/
/* McBSP 1 */
/*-------------------------------------------------------------------*/
#define DRR21_BASE      0x40
#define DRR11_BASE      0x41
#define DXR21_BASE      0x42
#define DXR11_BASE      0x43
#define SPSA1_BASE      0x48
#define SPCR11_BASE     0x49
#define SPCR21_BASE     0x49
#define RCR11_BASE      0x49
#define RCR21_BASE      0x49
#define XCR11_BASE      0x49
#define XCR21_BASE      0x49
#define SRGR11_BASE     0x49
#define SRGR21_BASE     0x49
#define MCR11_BASE      0x49
#define MCR21_BASE      0x49
#define RCERA1_BASE     0x49
#define RCERB1_BASE     0x49
#define XCERA1_BASE     0x49
#define XCERB1_BASE     0x49
#define PCR1_BASE       0x49

#define SPCR11_SUB      0x00
#define SPCR21_SUB      0x01
#define RCR11_SUB       0x02
#define RCR21_SUB       0x03
#define XCR11_SUB       0x04
#define XCR21_SUB       0x05
#define SRGR11_SUB      0x06
#define SRGR21_SUB      0x07
#define MCR11_SUB       0x08
#define MCR21_SUB       0x09
#define RCERA1_SUB      0x0A
#define RCERB1_SUB      0x0B
#define XCERA1_SUB      0x0C
#define XCERB1_SUB      0x0D
#define PCR1_SUB        0x0E

#define DRR21_ADDR      (*(volatile unsigned int *)DRR21_BASE)
#define DRR11_ADDR      (*(volatile unsigned int *)DRR11_BASE)
#define DXR21_ADDR      (*(volatile unsigned int *)DXR21_BASE)
#define DXR11_ADDR      (*(volatile unsigned int *)DXR11_BASE)
#define SPSA1_ADDR      (*(volatile unsigned int *)SPSA1_BASE)
#define SPCR11_ADDR     ((volatile SPCR1_REG *) ((char *) SPCR11_BASE))
#define SPCR21_ADDR     ((volatile SPCR2_REG *) ((char *) SPCR21_BASE))
#define RCR11_ADDR      ((volatile RCR1_REG *)  ((char *) RCR11_BASE))
#define RCR21_ADDR      ((volatile RCR2_REG *)  ((char *) RCR21_BASE))
#define XCR11_ADDR      ((volatile XCR1_REG *)  ((char *) XCR11_BASE))
#define XCR21_ADDR      ((volatile XCR2_REG *)  ((char *) XCR21_BASE))
#define SRGR11_ADDR     ((volatile SRGR1_REG *) ((char *) SRGR11_BASE))
#define SRGR21_ADDR     ((volatile SRGR2_REG *) ((char *) SRGR21_BASE))
#define MCR11_ADDR      ((volatile MCR1_REG *)  ((char *) MCR11_BASE))
#define MCR21_ADDR      ((volatile MCR2_REG *)  ((char *) MCR21_BASE))
#define RCERA1_ADDR     ((volatile RCERA_REG *) ((char *) RCERA1_BASE))
#define RCERB1_ADDR     ((volatile RCERB_REG *) ((char *) RCERB1_BASE))
#define XCERA1_ADDR     ((volatile XCERA_REG *) ((char *) XCERA1_BASE))
#define XCERB1_ADDR     ((volatile XCERB_REG *) ((char *) XCERB1_BASE))
#define PCR1_ADDR       ((volatile PCR_REG *)   ((char *) PCR1_BASE))


/*-------------------------------------------------------------------*/
/* SPCR1 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int dlb:1;
                unsigned int rjust:2;
                unsigned int clkstp:2;
                unsigned int rsrvd:3;
                unsigned int dxena:1;
                unsigned int abis:1;
                unsigned int rintm:2;
                unsigned int rsyncerr:1;
                unsigned int rfull:1;
                unsigned int rrdy:1;
                unsigned int rrst:1;
                } bitval;
        unsigned int value;
} SPCR1_REG;

/*-------------------------------------------------------------------*/
/* SPCR2 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsrvd:6;
                unsigned int free:1;
                unsigned int soft:1;
                unsigned int frst:1;
                unsigned int grst:1;
                unsigned int xintm:2;
                unsigned int xsyncerr:1;
                unsigned int xempty:1;
                unsigned int xrdy:1;
                unsigned int xrst:1;
                } bitval;
        unsigned int value;
} SPCR2_REG;

/*-------------------------------------------------------------------*/
/* PCR */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsrvd1:2;
                unsigned int xioen:1;
                unsigned int rioen:1;
                unsigned int fsxm:1;
                unsigned int fsrm:1;
                unsigned int clkxm:1;
                unsigned int clkrm:1;
                unsigned int rsrvd2:1;
                unsigned int clks_stat:1;
                unsigned int dx_stat:1;
                unsigned int dr_stat:1;
                unsigned int fsxp:1;
                unsigned int fsrp:1;
                unsigned int clkxp:1;
                unsigned int clkrp:1;
                } bitval;
        unsigned int value;
} PCR_REG;

/*-------------------------------------------------------------------*/
/* RCR1 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsrvd1:1;
                unsigned int rfrlen1:7;
                unsigned int rwdlen1:3;
                unsigned int rsrvd2:5;
                } bitval;
        unsigned int value;
} RCR1_REG;

/*-------------------------------------------------------------------*/
/* RCR2 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rphase:1;
                unsigned int rfrlen2:7;
                unsigned int rwdlen2:3;
                unsigned int rcompand:2;
                unsigned int rfig:1;
                unsigned int rdatdly:2;
                } bitval;
        unsigned int value;
} RCR2_REG;

/*-------------------------------------------------------------------*/
/* XCR1 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsrvd1:1;
                unsigned int xfrlen1:7;
                unsigned int xwdlen1:3;
                unsigned int rsrvd2:5;
                } bitval;
        unsigned int value;
} XCR1_REG;


/*-------------------------------------------------------------------*/
/* XCR2 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int xphase:1;
                unsigned int xfrlen2:7;
                unsigned int xwdlen2:3;
                unsigned int xcompand:2;
                unsigned int xfig:1;
                unsigned int xdatdly:2;
                } bitval;
        unsigned int value;
} XCR2_REG;

/*-------------------------------------------------------------------*/
/* SRGR1 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int fwid:8;
                unsigned int clkdiv:8;
                } bitval;
        unsigned int value;
} SRGR1_REG;

/*-------------------------------------------------------------------*/
/* SRGR2 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int gsync:1;
                unsigned int clksp:1;
                unsigned int clksm:1;
                unsigned int fsgm:1;
                unsigned int fper:12;
                } bitval;
        unsigned int value;
} SRGR2_REG;

/*-------------------------------------------------------------------*/
/* MCR1 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsrvd1:7;
                unsigned int rpbblk:2;
                unsigned int rpablk:2;
                unsigned int rcblk:3;
                unsigned int rsrvd2:1;
                unsigned int rmcm:1;
                } bitval;
        unsigned int value;
} MCR1_REG;

/*-------------------------------------------------------------------*/
/* MCR2 */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsrvd1:7;
                unsigned int xpbblk:2;
                unsigned int xpablk:2;
                unsigned int xcblk:3;
                unsigned int xmcm:2;
                } bitval;
        unsigned int value;
} MCR2_REG;

/*-------------------------------------------------------------------*/
/* RCERA */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int RCEA15:1;
                unsigned int RCEA14:1;
                unsigned int RCEA13:1;
                unsigned int RCEA12:1;
                unsigned int RCEA11:1;
                unsigned int RCEA10:1;
                unsigned int RCEA9:1;
                unsigned int RCEA8:1;
                unsigned int RCEA7:1;
                unsigned int RCEA6:1;
                unsigned int RCEA5:1;
                unsigned int RCEA4:1;
                unsigned int RCEA3:1;
                unsigned int RCEA2:1;
                unsigned int RCEA1:1;
                unsigned int RCEA0:1;
                } bitval;
        unsigned int value;
} RCERA_REG;

/*-------------------------------------------------------------------*/
/* RCERB */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int RCEB15:1;
                unsigned int RCEB14:1;
                unsigned int RCEB13:1;
                unsigned int RCEB12:1;
                unsigned int RCEB11:1;
                unsigned int RCEB10:1;
                unsigned int RCEB9:1;
                unsigned int RCEB8:1;
                unsigned int RCEB7:1;
                unsigned int RCEB6:1;
                unsigned int RCEB5:1;
                unsigned int RCEB4:1;
                unsigned int RCEB3:1;
                unsigned int RCEB2:1;
                unsigned int RCEB1:1;
                unsigned int RCEB0:1;
                } bitval;
        unsigned int value;
} RCERB_REG;

/*-------------------------------------------------------------------*/
/* XCERA */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int XCEA15:1;
                unsigned int XCEA14:1;
                unsigned int XCEA13:1;
                unsigned int XCEA12:1;
                unsigned int XCEA11:1;
                unsigned int XCEA10:1;
                unsigned int XCEA9:1;
                unsigned int XCEA8:1;
                unsigned int XCEA7:1;
                unsigned int XCEA6:1;
                unsigned int XCEA5:1;
                unsigned int XCEA4:1;
                unsigned int XCEA3:1;
                unsigned int XCEA2:1;
                unsigned int XCEA1:1;
                unsigned int XCEA0:1;
                } bitval;
        unsigned int value;
} XCERA_REG;

/*-------------------------------------------------------------------*/
/* XCERB */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int XCEB15:1;
                unsigned int XCEB14:1;
                unsigned int XCEB13:1;
                unsigned int XCEB12:1;
                unsigned int XCEB11:1;
                unsigned int XCEB10:1;
                unsigned int XCEB9:1;
                unsigned int XCEB8:1;
                unsigned int XCEB7:1;
                unsigned int XCEB6:1;
                unsigned int XCEB5:1;
                unsigned int XCEB4:1;
                unsigned int XCEB3:1;
                unsigned int XCEB2:1;
                unsigned int XCEB1:1;
                unsigned int XCEB0:1;
                } bitval;
        unsigned int value;
} XCERB_REG;


typedef union {
        struct {
                unsigned int SPCR1_REG :16;	
                unsigned int SPCR2_REG :16;	                        
                unsigned int PCR_REG   :16;	
                unsigned int RCR1_REG  :16;	
                unsigned int RCR2_REG  :16;	
                unsigned int XCR1_REG  :16;	
                unsigned int XCR2_REG  :16;	
                unsigned int SRGR1_REG :16;	
                unsigned int SRGR2_REG :16;	
                unsigned int MCR1_REG  :16;	
                unsigned int MCR2_REG  :16;	
                unsigned int RCERA_REG :16;	
                unsigned int RCERB_REG :16;	
                unsigned int XCERA_REG :16;	
                unsigned int XCERB_REG :16;
                } RegVal;
        unsigned int value;
} MCBSP;


/*********************************************************************/
/* Structure for DMA */
/*********************************************************************/
#define DMAPREC_BASE    0x54
#define DMAPREC_ADDR    ((volatile DMPREC_REG *)        ((char *) DMAPREC_BASE))

#define DMSBA_BASE      0x55
#define DMSBA_ADDR      (*(volatile unsigned int *) DMSBA_BASE)

#define DMSBAI_BASE     0x56    /* Auto-incrementing Sub-Address Register */
#define DMSBAI_ADDR     (*(volatile unsigned int *)     DMSBAI_BASE)

#define DMSBANOI_BASE   0x57    /* Sub-Address Register without Auto-increment */
#define DMSBANOI_ADDR   (*(volatile unsigned int *)     DMSBANOI_BASE)



/* Sub addressing offsets */
#define DMSRC0_SUB      0x00
#define DMDST0_SUB      0x01
#define DMCTR0_SUB      0x02
#define DMSFC0_SUB      0x03
#define DMMCR0_SUB      0x04
#define DMSRC1_SUB      0x05
#define DMDST1_SUB      0x06
#define DMCTR1_SUB      0x07
#define DMSFC1_SUB      0x08
#define DMMCR1_SUB      0x09
#define DMSRC2_SUB      0x0A
#define DMDST2_SUB      0x0B
#define DMCTR2_SUB      0x0C
#define DMSFC2_SUB      0x0D
#define DMMCR2_SUB      0x0E
#define DMSRC3_SUB      0x0F
#define DMDST3_SUB      0x10
#define DMCTR3_SUB      0x11
#define DMSFC3_SUB      0x12
#define DMMCR3_SUB      0x13
#define DMSRC4_SUB      0x14
#define DMDST4_SUB      0x15
#define DMCTR4_SUB      0x16
#define DMSFC4_SUB      0x17
#define DMMCR4_SUB      0x18
#define DMSRC5_SUB      0x19
#define DMDST5_SUB      0x1A
#define DMCTR5_SUB      0x1B
#define DMSFC5_SUB      0x1C
#define DMMCR5_SUB      0x1D
#define DMSRCP_SUB      0x1E
#define DMDSTP_SUB      0x1F
#define DMIDX0_SUB      0x20
#define DMIDX1_SUB      0x21
#define DMFRI0_SUB      0x22
#define DMFRI1_SUB      0x23
#define DMGSA_SUB       0x24
#define DMGDA_SUB       0x25
#define DMGCR_SUB       0x26
#define DMGFR_SUB       0x27


/* Define the base addresses for auto-incrementing */
/* Autoincrementing addresses will be denotated with a A ending */
#define DMSRC0_BASEA    0x56
#define DMSRC1_BASEA    0x56
#define DMSRC2_BASEA    0x56
#define DMSRC3_BASEA    0x56
#define DMSRC4_BASEA    0x56
#define DMSRC5_BASEA    0x56

#define DMDST0_BASEA    0x56
#define DMDST1_BASEA    0x56
#define DMDST2_BASEA    0x56
#define DMDST3_BASEA    0x56
#define DMDST4_BASEA    0x56
#define DMDST5_BASEA    0x56

#define DMCTR0_BASEA    0x56
#define DMCTR1_BASEA    0x56
#define DMCTR2_BASEA    0x56
#define DMCTR3_BASEA    0x56
#define DMCTR4_BASEA    0x56
#define DMCTR5_BASEA    0x56

#define DMSFC0_BASEA    0x56
#define DMSFC1_BASEA    0x56
#define DMSFC2_BASEA    0x56
#define DMSFC3_BASEA    0x56
#define DMSFC4_BASEA    0x56
#define DMSFC5_BASEA    0x56

#define DMMCR0_BASEA    0x56
#define DMMCR1_BASEA    0x56
#define DMMCR2_BASEA    0x56
#define DMMCR3_BASEA    0x56
#define DMMCR4_BASEA    0x56
#define DMMCR5_BASEA    0x56

#define DMSRCP_BASEA    0x56
#define DMDSTP_BASEA    0x56

#define DMIDX0_BASEA    0x56
#define DMIDX1_BASEA    0x56

#define DMFRI0_BASEA    0x56
#define DMFRI1_BASEA    0x56

#define DMSGA_BASEA             0x56
#define DMGDA_BASEA             0x56
#define DMGCR_BASEA             0x56
#define DMGFR_BASEA             0x56

#define DMSRC0_ADDRA    (*(volatile unsigned int *)     DMSRC0_BASEA)
#define DMSRC1_ADDRA    (*(volatile unsigned int *)     DMSRC1_BASEA)
#define DMSRC2_ADDRA    (*(volatile unsigned int *)     DMSRC2_BASEA)
#define DMSRC3_ADDRA    (*(volatile unsigned int *)     DMSRC3_BASEA)
#define DMSRC4_ADDRA    (*(volatile unsigned int *)     DMSRC4_BASEA)
#define DMSRC5_ADDRA    (*(volatile unsigned int *)     DMSRC5_BASEA)

#define DMDST0_ADDRA    (*(volatile unsigned int *)     DMDST0_BASEA)
#define DMDST1_ADDRA    (*(volatile unsigned int *)     DMDST1_BASEA)
#define DMDST2_ADDRA    (*(volatile unsigned int *)     DMDST2_BASEA)
#define DMDST3_ADDRA    (*(volatile unsigned int *)     DMDST3_BASEA)
#define DMDST4_ADDRA    (*(volatile unsigned int *)     DMDST4_BASEA)
#define DMDST5_ADDRA    (*(volatile unsigned int *)     DMDST5_BASEA)

#define DMCTR0_ADDRA    (*(volatile unsigned int *)     DMCTR0_BASEA)
#define DMCTR1_ADDRA    (*(volatile unsigned int *)     DMCTR0_BASEA)
#define DMCTR2_ADDRA    (*(volatile unsigned int *)     DMCTR0_BASEA)
#define DMCTR3_ADDRA    (*(volatile unsigned int *)     DMCTR0_BASEA)
#define DMCTR4_ADDRA    (*(volatile unsigned int *)     DMCTR0_BASEA)
#define DMCTR5_ADDRA    (*(volatile unsigned int *)     DMCTR0_BASEA)

#define DMSFC0_ADDRA    ((volatile DMSFCn_REG *)        ((char *) DMSFC0_BASEA))
#define DMSFC1_ADDRA    ((volatile DMSFCn_REG *)        ((char *) DMSFC1_BASEA))
#define DMSFC2_ADDRA    ((volatile DMSFCn_REG *)        ((char *) DMSFC2_BASEA))
#define DMSFC3_ADDRA    ((volatile DMSFCn_REG *)        ((char *) DMSFC3_BASEA))
#define DMSFC4_ADDRA    ((volatile DMSFCn_REG *)        ((char *) DMSFC4_BASEA))
#define DMSFC5_ADDRA    ((volatile DMSFCn_REG *)        ((char *) DMSFC5_BASEA))

#define DMMCR0_ADDRA    ((volatile DMMCRn_REG *)        ((char *) DMMCR0_BASEA))
#define DMMCR1_ADDRA    ((volatile DMMCRn_REG *)        ((char *) DMMCR1_BASEA))
#define DMMCR2_ADDRA    ((volatile DMMCRn_REG *)        ((char *) DMMCR2_BASEA))
#define DMMCR3_ADDRA    ((volatile DMMCRn_REG *)        ((char *) DMMCR3_BASEA))
#define DMMCR4_ADDRA    ((volatile DMMCRn_REG *)        ((char *) DMMCR4_BASEA))
#define DMMCR5_ADDRA    ((volatile DMMCRn_REG *)        ((char *) DMMCR5_BASEA))

#define DMSRCP_ADDRA    ((volatile DMSRCP_REG *)        ((char *) DMSRCP_BASEA))
#define DMDSTP_ADDRA    ((volatile DMDSTP_REG *)        ((char *) DMDSTP_BASEA))

#define DMIDX0_ADDRA    (*(volatile unsigned int *)     DMIDX0_BASEA)
#define DMIDX1_ADDRA    (*(volatile unsigned int *)     DMIDX1_BASEA)
                                                                
#define DMFRI0_ADDRA    (*(volatile unsigned int *)     DMFRI0_BASEA)
#define DMFRI1_ADDRA    (*(volatile unsigned int *)     DMFRI1_BASEA)

#define DMSGA_ADDRA             (*(volatile unsigned int *)     DMSGA_BASEA)
#define DMGDA_ADDRA             (*(volatile unsigned int *)     DMGDA_BASEA)
#define DMGCR_ADDRA             (*(volatile unsigned int *)     DMGCR_BASEA)
#define DMGFR_ADDRA             (*(volatile unsigned int *)     DMGFR_BASEA)


/* Define the base addresses without auto-incrementing  */
#define DMSRC0_BASE     0x57
#define DMSRC1_BASE     0x57
#define DMSRC2_BASE     0x57
#define DMSRC3_BASE     0x57
#define DMSRC4_BASE     0x57
#define DMSRC5_BASE     0x57

#define DMDST0_BASE     0x57
#define DMDST1_BASE     0x57
#define DMDST2_BASE     0x57
#define DMDST3_BASE     0x57
#define DMDST4_BASE     0x57
#define DMDST5_BASE     0x57

#define DMCTR0_BASE     0x57
#define DMCTR1_BASE     0x57
#define DMCTR2_BASE     0x57
#define DMCTR3_BASE     0x57
#define DMCTR4_BASE     0x57
#define DMCTR5_BASE     0x57

#define DMSFC0_BASE     0x57
#define DMSFC1_BASE     0x57
#define DMSFC2_BASE     0x57
#define DMSFC3_BASE     0x57
#define DMSFC4_BASE     0x57
#define DMSFC5_BASE     0x57

#define DMMCR0_BASE     0x57
#define DMMCR1_BASE     0x57
#define DMMCR2_BASE     0x57
#define DMMCR3_BASE     0x57
#define DMMCR4_BASE     0x57
#define DMMCR5_BASE     0x57

#define DMSRCP_BASE     0x57
#define DMDSTP_BASE     0x57

#define DMIDX0_BASE     0x57
#define DMIDX1_BASE     0x57

#define DMFRI0_BASE     0x57
#define DMFRI1_BASE     0x57

#define DMSGA_BASE      0x57
#define DMGDA_BASE      0x57
#define DMGCR_BASE      0x57
#define DMGFR_BASE      0x57

#define DMSRC0_ADDR     (*(volatile unsigned int *)     DMSRC0_BASE)
#define DMSRC1_ADDR     (*(volatile unsigned int *)     DMSRC1_BASE)
#define DMSRC2_ADDR     (*(volatile unsigned int *)     DMSRC2_BASE)
#define DMSRC3_ADDR     (*(volatile unsigned int *)     DMSRC3_BASE)
#define DMSRC4_ADDR     (*(volatile unsigned int *)     DMSRC4_BASE)
#define DMSRC5_ADDR     (*(volatile unsigned int *)     DMSRC5_BASE)
                                                           
#define DMDST0_ADDR     (*(volatile unsigned int *)     DMDST0_BASE)
#define DMDST1_ADDR     (*(volatile unsigned int *)     DMDST1_BASE)
#define DMDST2_ADDR     (*(volatile unsigned int *)     DMDST2_BASE)
#define DMDST3_ADDR     (*(volatile unsigned int *)     DMDST3_BASE)
#define DMDST4_ADDR     (*(volatile unsigned int *)     DMDST4_BASE)
#define DMDST5_ADDR     (*(volatile unsigned int *)     DMDST5_BASE)
                                                           
#define DMCTR0_ADDR     (*(volatile unsigned int *)     DMCTR0_BASE)
#define DMCTR1_ADDR     (*(volatile unsigned int *)     DMCTR1_BASE)
#define DMCTR2_ADDR     (*(volatile unsigned int *)     DMCTR2_BASE)
#define DMCTR3_ADDR     (*(volatile unsigned int *)     DMCTR3_BASE)
#define DMCTR4_ADDR     (*(volatile unsigned int *)     DMCTR4_BASE)
#define DMCTR5_ADDR     (*(volatile unsigned int *)     DMCTR5_BASE)

#define DMSFC0_ADDR     ((volatile DMSFCn_REG *)        ((char *) DMSFC0_BASE))
#define DMSFC1_ADDR     ((volatile DMSFCn_REG *)        ((char *) DMSFC1_BASE))
#define DMSFC2_ADDR     ((volatile DMSFCn_REG *)        ((char *) DMSFC2_BASE))
#define DMSFC3_ADDR     ((volatile DMSFCn_REG *)        ((char *) DMSFC3_BASE))
#define DMSFC4_ADDR     ((volatile DMSFCn_REG *)        ((char *) DMSFC4_BASE))
#define DMSFC5_ADDR     ((volatile DMSFCn_REG *)        ((char *) DMSFC5_BASE))

#define DMMCR0_ADDR     ((volatile DMMCRn_REG *)        ((char *) DMMCR0_BASE))
#define DMMCR1_ADDR     ((volatile DMMCRn_REG *)        ((char *) DMMCR1_BASE))
#define DMMCR2_ADDR     ((volatile DMMCRn_REG *)        ((char *) DMMCR2_BASE))
#define DMMCR3_ADDR     ((volatile DMMCRn_REG *)        ((char *) DMMCR3_BASE))
#define DMMCR4_ADDR     ((volatile DMMCRn_REG *)        ((char *) DMMCR4_BASE))
#define DMMCR5_ADDR     ((volatile DMMCRn_REG *)        ((char *) DMMCR5_BASE))

#define DMSRCP_ADDR     ((volatile DMSRCP_REG *)        ((char *) DMSRCP_BASE))
#define DMDSTP_ADDR     ((volatile DMDSTP_REG *)        ((char *) DMDSTP_BASE))

#define DMIDX0_ADDR     (*(volatile unsigned int *)     DMIDX0_BASE)
#define DMIDX1_ADDR     (*(volatile unsigned int *)     DMIDX1_BASE)
                                                           
#define DMFRI0_ADDR     (*(volatile unsigned int *)     DMFRI0_BASE)
#define DMFRI1_ADDR     (*(volatile unsigned int *)     DMFRI1_BASE)

#define DMSGA_ADDR      (*(volatile unsigned int *)     DMSGA_BASE)
#define DMGDA_ADDR      (*(volatile unsigned int *)     DMGDA_BASE)
#define DMGCR_ADDR      (*(volatile unsigned int *)     DMGCR_BASE)
#define DMGFR_ADDR      (*(volatile unsigned int *)     DMGFR_BASE)



/*-------------------------------------------------------------------*/
/* DMPREC */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int free:1;
                unsigned int rsvd:1;
                unsigned int dprc:6;
                unsigned int intosel:2;
                unsigned int de:6;
                } bitval;
        unsigned int value;
} DMPREC_REG;

/*-------------------------------------------------------------------*/
/* DMFCn */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int dsyn:4;
                unsigned int dblw:1;
                unsigned int rsrvd:3;
                unsigned int framecount:8;
                } bitval;
        unsigned int value;
} DMSFCn_REG;

/*-------------------------------------------------------------------*/
/* DMMCRn */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int autoinit:1;
                unsigned int dinm:1;
                unsigned int imod:1;
                unsigned int ctmod:1;
                unsigned int rsrvd1:1;
                unsigned int sind:2;
                unsigned int dms:2;
                unsigned int rsrvd2:1;
                unsigned int dind:3;
                unsigned int dmd:2;
                } bitval;
        unsigned int value;
} DMMCRn_REG;


/*-------------------------------------------------------------------*/
/* DMSRCP */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsvd       :9;
                unsigned int source     :7;
                } bitval;
        unsigned int value;
} DMSRCP_REG;

/*-------------------------------------------------------------------*/
/* DMDSTP */
/*-------------------------------------------------------------------*/
typedef union {
        struct {
                unsigned int rsvd       :9;
                unsigned int dest       :7;
                } bitval;
        unsigned int value;
} DMDSTP_REG;
