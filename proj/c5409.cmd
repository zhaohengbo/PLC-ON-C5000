/****************************************************************************
* FILENAME: $Source: /db/sds/syseng/rtdx/tutorial/c54x/sect_4/less_3/c5402.cmd,v $
* VERSION : $Revision: 1.1 $
* DATE    : $Date: 2000/01/26 18:49:42 $
* Copyright (c) 1997 Texas Instruments Incorporated
*
* COMMAND FILE FOR LINKING TMS5409 Targets
*
*   Usage:  lnk500 <obj files...>    -o <out file> -m <map file> lcf.cmd
*           cl500  <src files...> -z -o <out file> -m <map file> lcf.cmd
*
*   Description: This file is a sample command file that can be used
*                for linking programs built with the TMS54x C Compiler.   
*                Use it as a guideline; you may want to change
*                the allocation scheme according to the size of your
*                program and the memory layout of your target system.
*
*   Notes: (1)   You must specify the directory in which rts.lib is
*                located.  Either add a "-i<directory>" line to this
*                file, or use the system environment variable C_DIR to
*                specify a search path for libraries.
*
*          (2)   If the run-time support library you are using is not
*                named rts.lib, be sure to use the correct name here.
*
* Revision History:
****************************************************************************/
-c
-stack 0x0280 
-heap  0x0010

/* This symbol defines the interrupt mask to be applied to IMR inside
   RTDX Critical Code sections.

   This example masks the timer interrupt.
*/
_RTDX_interrupt_mask = ~0x0008;		/* interrupts masked by RTDX		*/

MEMORY  
{
/* Note: Assume PMST = 0xffe0
        PMST    Bit     Name   Value
        		15-7	IPTR	0xff80
                6       MP/!MC  1
                5       OVLY    1
                3       DROM    0
*/
	PAGE 0:		/* Program Space */
		RSV1		(R)			: o=00000h l=00080h		/* Reserved 					*/
		USERVECT	(RWIX)		: o=00080h l=00080h		/* user vectors */
		MODEMID		(RWIX)		: o=00100h l=00010h		/* modem id and other per serial number constants */
  		DARAML		(RWIX)		: o=00110h l=045F0h		/* On-Chip Dual-Access RAM	*/
		VECS		(RWIX)		: o=0ff80h l=00080h		/* Interrupt Vector Table	*/

	PAGE 1:		/* Data Space */
		MMRS		(RW)		: o=00000h l=00060h		/* Memory-Mapped Registers	*/
		SPAD		(RW)		: o=00060h l=00020h		/* Scratch-Pad RAM			*/
  		DARAMM		(RWIX) 		: o=04700h l=01900h		/* On-Chip Dual-Access RAM	*/
  		DARAMMH		(RWIX) 		: o=06000h l=01E00h		/* On-Chip Dual-Access RAM	*/
  		DARAMMHH	(RWIX) 		: o=07E00h l=00200h		/* On-Chip Dual-Access RAM	*/
  		DARAMH		(RWIX) 		: o=0C000h l=03F80h		/* On-Chip Dual-Access RAM	*/
  		DARAMHH		(RWIX) 		: o=0B000h l=01000h		/* On-Chip Dual-Access RAM	*/
}

SECTIONS
{

	.vectors	:>	USERVECT
	

	GROUP : > DARAML	/* group sections in overlay for contiguous addresses	*/
	{	
/*		.vectors		/* interrupt vector table	*/
		.text    	/* User code						*/
		.rtdx_text	/* RTDX code						*/
		.cinit   	/* initialization tables		*/
		.pinit   	/* initialization functions	*/
		.switch  	/* for C-switch tables			*/
	}

/*	.vectors 	: > VECS  PAGE 0	/* interrupt vector table	*/
/*	.intvecs 	: > VECS  PAGE 0	/* interrupt vector table	*/ 

	.data			: > SPAD  PAGE 1	/* asm data area */	

	GROUP : > DARAMM	/* group sections in overlay for contiguous addresses	*/
	{	
		.cio									/* C-IO Buffer				*/
		.rtdx_data							/* RTDX data area			*/
		.const								/* C constant tables		*/
		.bss									/* global & static vars	*/
		.stack		: fill = 0xBEEF	/* system stack			*/
		.sysmem		: fill = 0xDEAD	/* dynamic heap			*/
	}
	


  	.sintab 	:> DARAMM	PAGE 1	/* attempt to make FFT/iFFT work  */
/*  .sintab :	 > DARAMHH	PAGE 1	/* attempt to make FFT/iFFT work  */
	
	fftsect		:> DARAMMHH	PAGE 1 	/* MUST start on multiple of 512 */
/*	fftsect		:> DARAMHH	PAGE 1 	*/							/*	bufsect		:> DARAMH	PAGE 1 */
/*	bufsect		:> DARAMM	PAGE 1 */
	bufsect		:> DARAMMH	PAGE 1 

}
