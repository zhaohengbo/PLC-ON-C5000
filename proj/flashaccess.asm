;==========================================================================================
; Filename:			FlashAccess.asm
;
; Description:		Contains low level code for access to external flash
;
;					the C prototypes are in flash.h
;
; Copyright (C) 2003 Texas Instruments Incorporated
; Texas Instruments Proprietary Information
; Use subject to terms and conditions of TI Software License Agreement
;
; Revision History:
; 01/13/04 JEN		Initial implementation
;==========================================================================================

	.file 	"FlashAccess.asm"

;==========================================================================================
; Function:			FarWrite()		
;
; Description:		Writes data to far program space without the need to compile
;					with a far memory model.
;					
;					The address to write to is passed in the A and the data to write is
;					passed in on the stack.
;
; Revision History:
; 01/13/04 JEN		Initial implementation
;==========================================================================================
	.sect	".text"
	.global	_FarWrite
	.sym 	_FarWrite, _FarWrite, 32, 2, 0
_FarWrite:
		WRITA 	*SP(1)		; write stack data to address passed in A
        RET

;==========================================================================================
; Function:			FarRead()		
;
; Description:		Reads data from far program space without the need to compile
;					with a far memory model.
;					
;					The address to read from is passed in the A and the data read is
;					returned in A.
;
; Revision History:
; 01/13/04 JEN		Initial implementation
;==========================================================================================
	.sect	".text"
	.global	_FarRead
	.sym 	_FarRead, _FarRead, 32, 2, 0
_FarRead:
        FRAME   #-1			; make room for the return value
 		READA 	*SP(0)		; read address passed in A to top of stack
        LD      *SP(0),A 	; move return value into A for return
        FRAME   #1			; clean up
        RET
