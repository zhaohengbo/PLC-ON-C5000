*************************************************************************
*               (C)  Copyright Texas Instruments, Inc. 2000				*
*************************************************************************
*                                                                     	*
*  MODULE NAME:  C5402VEC.ASM                                         	*
*                                                                     	*
*       AUTHOR:  C5000 Applications										*
*                                                                     	*
*  DESCRIPTION:  TMS320VC5402 Interrupt Vector initialization table   	*
*																		*
*  NOTE:	     If DSP/BIOS used you will have to initialize interrupt	*
*		     vector table using the BIOS configuration tool				*
*		     															*
*                                                                     	*
*  DATE:  02/00				                      				    	*
*         11/07/03	EGO		Added INT1 vector for UART interrupt.      	*
*************************************************************************

		.title  "54xDSKPLUS Vector Table Initialization"
;		.ref    start, BRINT1_ISR
;		.ref    _c_int00, BRINT1_ISR
		.ref    _c_int00
		.ref	_UartISR		; INT1 (16C550)
		.ref	_TINT0_ISR		; Timer0 Interrupt Handler
		.ref 	_INVALID_ISR

        .sect   ".vectors"
	.global __vectors


;RESET:	BD	start		; RESET vector
__vectors:
RESET:	BD	_c_int00	; RESET vector
		NOP
		NOP
NMI:    BD	_INVALID_ISR			; ~NMI
		NOP
		NOP						

**********************************************************************
* 		S/W Interrupts
**********************************************************************
SINT17:	BD	_INVALID_ISR
		NOP
		NOP
SINT18:	BD	_INVALID_ISR
		NOP
		NOP
SINT19:	BD	_INVALID_ISR
		NOP
		NOP
SINT20:	BD	_INVALID_ISR
		NOP
		NOP
SINT21:	BD	_INVALID_ISR
		NOP
		NOP
SINT22:	BD	_INVALID_ISR
		NOP
		NOP
SINT23:	BD	_INVALID_ISR
		NOP
		NOP
SINT24:	BD	_INVALID_ISR
		NOP
		NOP
SINT25:	BD	_INVALID_ISR
		NOP
		NOP
SINT26:	BD	_INVALID_ISR
		NOP
		NOP
SINT27:	BD	_INVALID_ISR
		NOP
		NOP
SINT28:	BD	_INVALID_ISR
		NOP
		NOP
SINT29:	BD	_INVALID_ISR
		NOP
		NOP
SINT30:	BD	_INVALID_ISR
		NOP
		NOP

**************************************************************************
* 		Rest of the Interrupts
**************************************************************************

INT0:	BD	_INVALID_ISR
		NOP
		NOP

INT1:	BD	_UartISR
		NOP
		NOP

INT2:	BD	_INVALID_ISR
		NOP
		NOP

;TINT0:	BD	TINT0
TINT0:	BD	_TINT0_ISR
		NOP
		NOP					

RINT0:	BD	_INVALID_ISR
		NOP
		NOP

XINT0:	BD	_INVALID_ISR
		NOP
		NOP

DMAC0:	BD	_INVALID_ISR
		NOP
		NOP

TINT1:	BD	_INVALID_ISR
		NOP
		NOP

INT3:	BD	_INVALID_ISR
		NOP
		NOP

HPI:	BD	_INVALID_ISR
		NOP
		NOP

RINT1:	BD	_INVALID_ISR
		NOP
		NOP

XINT1:	BD	_INVALID_ISR
		NOP
		NOP

DMAC2:	BD	_INVALID_ISR
		NOP
		NOP

DMAC3:	BD	_INVALID_ISR
		NOP
		NOP
DMAC4:	BD	_INVALID_ISR
		NOP
		NOP

DMAC5:	BD	_INVALID_ISR
		NOP
		NOP







;HANG  
;HANG  **********************************************************************
;HANG  * 		S/W Interrupts
;HANG  **********************************************************************
;HANG  SINT17:	BD	SINT17
;HANG  		NOP
;HANG  		NOP
;HANG  SINT18:	BD	SINT18
;HANG  		NOP
;HANG  		NOP
;HANG  SINT19:	BD	SINT19
;HANG  		NOP
;HANG  		NOP
;HANG  SINT20:	BD	SINT20
;HANG  		NOP
;HANG  		NOP
;HANG  SINT21:	BD	SINT21
;HANG  		NOP
;HANG  		NOP
;HANG  SINT22:	BD	SINT22
;HANG  		NOP
;HANG  		NOP
;HANG  SINT23:	BD	SINT23
;HANG  		NOP
;HANG  		NOP
;HANG  SINT24:	BD	SINT24
;HANG  		NOP
;HANG  		NOP
;HANG  SINT25:	BD	SINT25
;HANG  		NOP
;HANG  		NOP
;HANG  SINT26:	BD	SINT26
;HANG  		NOP
;HANG  		NOP
;HANG  SINT27:	BD	SINT27
;HANG  		NOP
;HANG  		NOP
;HANG  SINT28:	BD	SINT28
;HANG  		NOP
;HANG  		NOP
;HANG  SINT29:	BD	SINT29
;HANG  		NOP
;HANG  		NOP
;HANG  SINT30:	BD	SINT30
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  **************************************************************************
;HANG  * 		Rest of the Interrupts
;HANG  **************************************************************************
;HANG  
;HANG  INT0:	BD	INT0
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  INT1:	BD	INT1
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  INT2:	BD	INT2
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  ;TINT0:	BD	TINT0
;HANG  TINT0:	BD	_TINT0_ISR
;HANG  		NOP
;HANG  		NOP					
;HANG  
;HANG  RINT0:	BD	RINT0
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  XINT0:	BD	XINT0
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  DMAC0:	BD	DMAC0
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  TINT1:	BD	TINT1
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  INT3:	BD	INT3
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  HPI:	BD	HPI
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  RINT1:	BD	RINT1
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  XINT1:	BD	XINT1
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  DMAC2:	BD	DMAC2
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  DMAC3:	BD	DMAC3
;HANG  		NOP
;HANG  		NOP
;HANG  DMAC4:	BD	DMAC4
;HANG  		NOP
;HANG  		NOP
;HANG  
;HANG  DMAC5:	BD	DMAC5
;HANG  		NOP
;HANG  		NOP
;HANG  