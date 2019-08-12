/******************************************************************/
/* intr.h V0.00                                                   */
/* Copyright (c) Texas Instruments , Incorporated  2000           */
// 05/22/03	HEM		Changed global interrupt enable from macros to 
//					inline functions to make compiler happy.
/******************************************************************/
#include "regs54xx.h"

#ifndef intr_h						// Header file guard
#define intr_h

typedef void (*ISRFUNC)(void);

void software_trap(int trap);     /* Initiates trap to given interrupt */

/******************************************************************/
/* Define all macros needed to enable/disable interrupts, set     */
/* interrupt vectors, allocate space for interrupt vectors and    */
/* set interrupt vector pointer.                                  */
/******************************************************************/
extern unsigned int _vectors;	/* Start label of vector table */
extern ISRFUNC isr_jump_table[]; /* Array of ISR pointers       */

/******************************************************************/
/* Define interrupt trap numbers                                  */
/******************************************************************/

#define RS_TRAP		0
#define NMI_TRAP	   1
#define INT0_TRAP	   16
#define INT1_TRAP	   17
#define INT2_TRAP	   18
#define TINT_TRAP	   19
#define RINT0_TRAP	20
#define XINT0_TRAP	21
#define DMAC0_TRAP	22
#define DMAC1_TRAP	23
#define INT3_TRAP	   24
#define HPINT_TRAP	25
#define RINT1_TRAP	26
#define DMAC2_TRAP	26
#define XINT1_TRAP	27
#define DMAC3_TRAP	27
#define DMAC4_TRAP	28
#define DMAC5_TRAP	29



#define SINTR		0
#define SINT16		1
#define SINT17		2
#define SINT18		3
#define SINT19		4
#define SINT20		5
#define SINT21		6
#define SINT22		7
#define SINT23		8
#define SINT24		9
#define SINT25		10
#define SINT26		11
#define SINT27		12
#define SINT28		13
#define SINT29		14
#define SINT30		15
#define SINT0		16
#define SINT1		17
#define SINT2		18
#define SINT3		19
#define SINT4		20
#define SINT5		21
#define SINT6		22
#define SINT7		23
#define SINT8		24
#define SINT9		25
#define SINT10		26
#define SINT11		27
#define SINT12      28
#define SINT13      29


/******************************************************************/
/* INTR_ENABLE - enables all masked interrupts by resetting INTM  */
/*               bit in Status Register 1                         */
/******************************************************************/
//#define INTR_GLOBAL_ENABLE()\
//        asm("\tRSBX	INTM")
inline void INTR_GLOBAL_ENABLE(void)	{ asm("\tRSBX	INTM"); }


/******************************************************************/
/* INTR_DISABLE - disables all masked interrupts by setting INTM  */
/*                bit in Status Register 1                        */
/******************************************************************/
//#define INTR_GLOBAL_DISABLE()\
//        asm("\tSSBX	INTM")
inline void INTR_GLOBAL_DISABLE(void)	{ asm("\tSSBX	INTM"); }

/******************************************************************/
/* INTR_CHECK_FLAG(flag) - check the corresponding flag in the IFR*/
/*                         register                               */
/******************************************************************/
#define INTR_CHECK_FLAG(flag)\
      (IFR & (0x1u << flag))

/******************************************************************/
/* INTR_CLR_FLAG(flag) - clears the corresponding flag in the IFR */
/*                       register                                 */
/******************************************************************/
#define INTR_CLR_FLAG(flag)\
      {IFR &= (0x1u << flag);}

/******************************************************************/
/* IDLE(mode) - sets CPU in idle mode based on level selected     */
/******************************************************************/
#define IDLE(mode)\
	{ asm("\tidle " #mode); }

/******************************************************************/
/* INTR_ENABLE (flag) - set interrupt vector flags to             */
/*     enable/reset specific device interrupts                    */
/*  flag - bit to set in interrupt mask register                  */
/******************************************************************/
#define INTR_ENABLE(flag)\
     IMR |= MASK_BIT(flag)

/******************************************************************/
/* INTR_DISABLE (flag) - set interrupt vector flags to reset      */
/*                       specific device interrupts               */
/*  flag - bit to set in interrupt mask register                  */
/******************************************************************/
#define INTR_DISABLE(flag)\
     IMR &= ~MASK_BIT(flag)

/******************************************************************/
/* INTR_INIT - sets interrupt vector pointer                      */
/******************************************************************/
#define INTR_INIT\
    {PMST &= 0x7f; PMST |= ( ( (unsigned int)&_vectors ) & 0xff80u); }

/******************************************************************/
/* INTR_HOOK(isr, isrfunc) - sets interrupt service routine vec   */
/*    isr - interrupt trap number (see TRAP instruction)          */
/*    isrfunc - address of interrupt service routine              */
/******************************************************************/
#define INTR_HOOK(trap_no, isrfunc) \
   { isr_jump_table[trap_no] = (ISRFUNC)isrfunc; }


#endif
