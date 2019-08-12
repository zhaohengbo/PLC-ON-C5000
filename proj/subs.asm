;+==========================================================================================
; Filename:		subs.asm
;
; Description:	Assembly function subroutines used to perform low level routines that
;				aren't possible from C.
;
; Copyright (C) 2000 - 2001 Texas Instruments Incorporated
; Texas Instruments Proprietary Information
; Use subject to terms and conditions of TI Software License Agreement
;
; Revision History:
; 08/27/01	EGO		Started file.  From owlink1 project.
; 10/03/01  HEM		Changed temp var from 03FFF to 08FFF. This must be fixed better.
;===========================================================================================

 .mmregs

;===========================================================================================
; Function:		ReadIO
;
; Description: 	This function is used to read a single word from IO space.
;				The single argument both in and out is passed in the A accumulator.
;
;				Note that this code MUST be in RAM not flash as it modifies the
;				opcode in order to return the desired value.
;
; Revision History:
;===========================================================================================
 .def _ReadIO

_ReadIO:
	PSHM	AR2						;Save context even if class notes say AR2 isn't needed.
	STM		08FFFH, AR2				;AR2 points to temp variable (3FFF)
	STL		A, *AR2					;Store the address to read in AR2
	MVDP	*AR2, #PORTR_OPCODE+1	;Tricky here.
 	MVDP	*AR2, #PORTR_OPCODE+1	;TI bug - write twice...
									;Place the address into the second word of the opcode
									;for the PORTR instruction.

PORTR_OPCODE:						;DO NOT SEPARATE LABEL FROM INSTRUCTION
	PORTR	#0, *AR2				;Read addess into AR2 (0 will be replaced at run time).

	LD		*AR2, A					;Place the result back in the A ACC for return to C.

	POPM	AR2						;Context restore.
	RET								;That's it.
	
	
;===========================================================================================
; Function:		WriteIO
;
; Description: 	This function is used to write a single word to IO space.
;				On calling the address (first argument) is passed in the A accumulator,
;				the value to write (second argument) is passed on the stack.
;				There is no return value.
;
;
; Revision History:
;===========================================================================================
 .def _WriteIO

_WriteIO:
 	PSHM	AR2						;Save context even if class notes say AR2 isn't needed.
	STM		08FFFH, AR2				;AR2 points to temp variable (3FFF)
 	STL		A, *AR2					;Store the address to be written to in AR2
 	MVDP	*AR2, #PORTW_OPCODE+1	;Tricky here.
 	MVDP	*AR2, #PORTW_OPCODE+1	;TI bug - write twice...
 									;Place the address into the second word of the opcode
 									;for the PORTW instruction.
; SP    --> AR2
; SP(1)	--> PC
; SP(2) --> arg(2)

PORTW_OPCODE:						;DO NOT SEPARATE FROM INSTRUCTION
 	PORTW	*SP(2), #0				;Write the passed value into desired address
 									;(0 will be replaced at run time).

 	POPM	AR2						;Context restore.
 	RET								;That's it.


;===========================================================================================
; Function:		ReadProg
;
; Description: 	This function is used to read a single word from program space.
;				The single argument both in and out is passed in the A accumulator.
;
;				Note that this code MUST be in RAM not flash as it modifies the
;				opcode in order to return the desired value.
;
; Revision History:
;===========================================================================================
 .def _ReadProg

_ReadProg:
	PSHM	AR2						;Save context even if class notes say AR2 isn't needed.
	STM		08FFFH, AR2				;AR2 points to temp variable (3FFF)
	STL		A, *AR2					;Store the address to read in AR2
	MVDP	*AR2, #MVPD_OPCODE+1	;Tricky here.
 	MVDP	*AR2, #MVDP_OPCODE+1	;TI bug - write twice...
									;Place the address into the second word of the opcode
									;for the MVPD instruction.

MVPD_OPCODE:						;DO NOT SEPARATE LABEL FROM INSTRUCTION
	MVPD	#0, *AR2				;Read addess into AR2 (0 will be replaced at run time).
	LD		*AR2, A					;Place the result back in the A ACC for return to C.

	POPM	AR2						;Context restore.
	RET								;That's it.


;===========================================================================================
; Function:		WriteProg
;
; Description: 	This function is used to write a single word to program space.
;				On calling the address (first argument) is passed in the A accumulator,
;				the value to write (second argument) is passed on the stack.
;				There is no return value.
;
;
; Revision History:
;===========================================================================================
 .def _WriteProg

_WriteProg:
 	PSHM	AR2						;Save context even if class notes say AR2 isn't needed.

	STM		08FFFH, AR2				;AR2 points to temp variable (3FFF)
 	STL		A, *AR2					;Store the address to be written to in AR2
 	MVDP	*AR2, #MVDP_OPCODE+1	;Tricky here.
 	MVDP	*AR2, #MVDP_OPCODE+1	;TI bug - write twice...
 									;Place the address into the second word of the opcode
 									;for the MVDP instruction.
; SP    --> AR2
; SP(1)	--> PC
; SP(2) --> arg(2)

MVDP_OPCODE:						;DO NOT SEPARATE FROM INSTRUCTION
 	MVDP	*SP(2), #0				;Write the passed value into desired address
 									;(0 will be replaced at run time).

 	POPM	AR2						;Context restore.
 	RET								;That's it.


;===========================================================================================
; Function:		Min
;
; Description: 	This function returns the smaller of two passed arguments.  
;
; Input:		AccA = Arg1
;				SP(0) = Arg2
;
; Output:		AccA = smaller of (Arg1, Arg2)
;
; Revision History:
; 02/05/02  HEM		New function.
; 02/07/02	HEM		Removed unneeded context save/restore to save 9 ticks.
;===========================================================================================
 .def _Min

_Min:								;First argument already in AccA
	LD		*SP(1), B				;Load second argument into AccB
	MIN		A						;Set AccA = MIN(AccA, AccB)
	RET


;===========================================================================================
; Function:		Max
;
; Description: 	This function returns the larger of two passed arguments.  
;
; Input:		AccA = Arg1
;				SP(1) = Arg2
;
; Output:		AccA = larger of (Arg1, Arg2)
;
; Revision History:
; 02/05/02  HEM		New function.
; 02/07/02	HEM		Removed unneeded context save/restore to save 9 ticks.
;===========================================================================================
 .def _Max

_Max:								;First argument already in AccA
	LD		*SP(1), B				;Load second argument into AccB
	MAX		A						;Set AccA = MAX(AccA, AccB)
	RET


;===========================================================================================
; Function:		Sat16
;
; Description: 	This function saturates a value to +/- 32767.  
;
; Input:		AccA = Arg1	= Value
;
; Output:		AccA 
;
; Revision History:
; 02/05/02  HEM		New function.
; 02/07/02	HEM		Removed unneeded context save/restore to save 8 ticks.
;					Return with RETD to save 2 ticks.
;===========================================================================================
 .def _Sat16

_Sat16:								;First argument already in AccA
	LD		#0x7FFF, B				;Set AccB = Upper limit = +32767
	MIN		A						;Set AccA = MIN(AccA, AccB)
	RETD							; |Delayed return 2 instructions from now|
	NEG		B						;Set AccB = Lower limit = -32767
	MAX		A						;Set AccA = MAX(AxxA, AccB)
	;RET							; |Delayed return occurs now|


;===========================================================================================
; Function:		Saturate
;
; Description: 	This function saturates a value to a specified range
;
; Input:		AccA  = Arg1 = Value
;				SP(1) = Arg2 = Lower Limit
;				SP(2) = Arg3 = Upper Limit
;
; Output:		AccA 
;
; Revision History:
; 02/05/02  HEM		New function.
; 02/07/02	HEM		Removed unneeded context save/restore to save 9 ticks.
;===========================================================================================
 .def _Saturate

_Saturate:							;First argument already in AccA
	LD		*SP(1), B				;Load second argument (lower limit) into AccB
	MAX		A						;Set AccA = MAX(AccA, AccB)
	LD		*SP(2), B				;Load third argument (upper limit) into AccB
	MIN		A						;Set AccA = MIN(AxxA, AccB)
	RET


;===========================================================================================
; Function:		USaturate
;
; Description: 	This function saturates an unsigned value to a specified range
;
; Input:		AccA  = Arg1 = Value
;				SP(1) = Arg2 = Lower Limit
;				SP(2) = Arg3 = Upper Limit
;
; Output:		AccA 
;
; Revision History:
; 02/05/02  HEM		New function.
; 02/07/02	HEM		Removed unneeded context save/restore to save 9 ticks.
;===========================================================================================
 .def _USaturate

_USaturate:							;First argument already in AccA
	LDU		*SP(1), B				;Load second argument (lower limit) into AccB
	MAX		A						;Set AccA = MAX(AccA, AccB)
	LDU		*SP(2), B				;Load third argument (upper limit) into AccB
	MIN		A						;Set AccA = MIN(AxxA, AccB)
	RET

