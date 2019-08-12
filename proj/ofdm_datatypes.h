/*========================================================================
 Filename:			ofdm_datatypes.c

 Description:		This file defines the data types and macros used in the 
 					OFDM powerline modem.

 Copyright (C) 2002 - 2003 Texas Instruments Incorporated
 Texas Instruments Proprietary Information
 Use subject to terms and conditions of TI Software License Agreement

 Revision History:
==========================================================================*/
#ifndef OFDM_DATATYPES_H			// Header file guard
#define OFDM_DATATYPES_H


//---- define variable sizes ---------------------------------
#if COMPILE_MODE == MEX_COMPILE
	typedef char			 i8;
	typedef	short			 i16;
	typedef short 			 s16;
	typedef	short			 q16;
	typedef int				 i32;
	typedef int				 s32;
	typedef int				 q32;
	typedef unsigned char	 u8;
	typedef	unsigned short	 u16;
	typedef unsigned int	 u32;

	typedef	unsigned long	 viterbi_int32;
#endif

#if COMPILE_MODE == DSP_COMPILE
	typedef char			 i8;
	typedef	short			 i16;
	typedef	short			 q16;
	typedef	short			 s16;
	typedef long			 i32;
	typedef long			 s32;
	typedef long			 q32;
	typedef unsigned char	 u8;
	typedef	unsigned short	 u16;
	typedef unsigned long	 u32;

	typedef	unsigned long	 viterbi_int32;
#endif


//---- complex array definition ------------------------------
typedef struct
{
	i16				re;
	i16				im;
}	iCplx;				// basic complex data element

typedef struct 
{
	iCplx			*c;
	i16				len;
}	iCplxPtr;			// container for complex data

typedef struct 
{
	i16				*r;
	i16				len;
}	iRealPtr;			// container for real data

typedef struct 
{
	i16				*r;
	i16				*start;
	i16				*end;
	i16				len;
	i16				offset;
}	iCircPtr;			// container for real data

typedef struct 
{
	u16				*r;
	u16				len;
}	uRealPtr;

typedef struct 
{
	u8				*r;
	int				len;
}	bytePtr;

#if COMPILE_MODE == MEX_COMPILE
	typedef i16	DATA;

	typedef struct 
	{
		double			*r;
		double			*i;
		int				len;
	}	dCplxPtr;
#endif

//#if COMPILE_MODE == MEX_COMPILE
//	typedef	i16	TXDATA;
//#else
//	typedef	u16	TXDATA;
//#endif
#define	TXDATA	i16


typedef	struct
{
	//int			prevState[2];		// Previous states to this state
	//int			encoderEst[2];		// 2 bit encoder ouput for each path
	i32				pathMetric;			// running total metric for this state
	viterbi_int32	pathMemory;			// estimate of the original data
										// assuming this is the right path
} 	pathInfoType;

	
#define		METRIC_SIZE		16		// Choose one of these two
//#define	METRIC_SIZE		32
//#define METRIC_OFFSET 0 		// Use signed values with no offset		
#define METRIC_OFFSET -24576 		// Use unsigned values with large negative offset		
//#define METRIC_OFFSET 2048		// Use unsigned values with offset much bigger than any possible distance measurement		
#if (METRIC_SIZE == 32) 
	#if (METRIC_OFFSET <= 0)
		typedef		i32		MetricType;
	#else
		typedef		u32		MetricType;
	#endif
#else			// 16 bit words
	#if (METRIC_OFFSET <= 0)
		typedef		i16		MetricType;
	#else
		typedef		u16		MetricType;
	#endif
#endif



//---- Modem State bit definitions --------------------------------------------------------------
typedef struct
{
	unsigned int		RS232_Msg_Received		: 1;		
	unsigned int		RX_Symbols_Ready		: 1;
} 	taskFlagType;	



typedef	enum
{					IdleState, 
					TxMsgReceived, 
					PreambleDetected,
					FrameAligned,
					RxMsgReceived,
					RxMsgDecoded, 
					PreambleMaybe
}   modemStateType;

typedef	enum
{					AgcIdle, 
					AgcPreamble, 
					AgcHold 
}   agcStateType;

typedef	enum
{					diagU16, 
					diagI16,
					diagICPLX
}	diagType;


// PlcStates
typedef enum
{
	PLC_IGNORE,
	PLC_RECEIVE_COMMAND,			// Only possible on SLAVE
	PLC_RECEIVE_RESPONSE,			// Only possilbe on MASTER
	PLC_RECEIVE_POLLING,			// Only possible on MASTER - response to periodic poll.
	PLC_FIND_CLOSEST_SLAVE,			// Only possible on MASTER
	PLC_FIND_ALL_SLAVES				// Only possible on MASTER
} PlcStateType;

// UartStates
typedef enum
{
	UART_IDLE,						// Only possible on SLAVE
	UART_RECEIVE_COMMAND,			// Only possible on MASTER
	UART_RECEIVE_DATA,				// Only possible on MASTER
	UART_RECEIVE_RESPONSE			// Possible on either MASTER or SLAVE
} UartStateType;


//==========================================================================================
// Macros
//==========================================================================================
//#define ceilingMacro(ratnum)	( (int)( (double)(ratnum) - (double)((int)((double)(ratnum) + 1.0)) ) + ((int)((double)(ratnum) + 1.0)) )

#define iSquare(num, shift)		( ( (i32)(num) * (i32)(num) ) >> shift )



#define HighWord(x)	( (s16)((x)>>16) )
#define LowWord(x)	( (s16) (x) )   

#if (COMPILE_MODE==DSP_COMPILE)
	#define 	SATMACROS	(~(TRUE))	//~TRUE ==> Use assembly functions for speed
	//#define 	SATMACROS	(TRUE)		// TRUE ==> Use C macros for maximum portability
#else
	#define 	SATMACROS	(TRUE)		// TRUE ==> Use C macros for maximum portability
#endif

#if (SATMACROS == TRUE)
	#define Max(A,B) ((A) > (B) ? (A):(B)) 
	#define Min(A,B) ((A) < (B) ? (A):(B))
	#define Saturate(x,LoLim,HiLim) Max((LoLim), Min((HiLim), (x) ) )
	#define LSaturate(x,LoLim,HiLim) Max((LoLim), Min((HiLim), (x) ) )
	#define USaturate(x,LoLim,HiLim) Max((LoLim), Min((HiLim), (x) ) )
	#define Sat16(x) Saturate((x), -32767, 32767)
#else
	extern q16	Max(q16 A, q16 B);		// Max function defined in subs.asm
	extern q16	Min(q16 A, q16 B);		// Min function defined in subs.asm

//	#define  Saturate(x,LoLim,HiLim) ((x) > (HiLim) ? (HiLim) : ((x) > (LoLim) ? (x) : (LoLim) ))
	extern q16	Saturate(q32 qlVal, q16 qLoLim, q16 qHiLim);

	#define	USaturate(x,LoLim,HiLim) ((x) > (HiLim) ? (HiLim) : ((x) > (LoLim) ? (x) : (LoLim) ))
//	extern u16	USaturate(u16 uVal, u16 uLoLim, u16 uHiLim);

	#define LSaturate(x,LoLim,HiLim) ((x) > (HiLim) ? (HiLim) : ((x) > (LoLim) ? (x) : (LoLim) ))

	extern q16	Sat16(q32 qlX);			// Saturate value to +/-32767
//	#define Sat16(x) Saturate((x), -32767, 32767)
#endif


#define IsEven(x) ((((x)>>1)<<1) == (x) ? 1:0)
#define IsOdd(x)  ((((x)>>1)<<1) != (x) ? 1:0)

// Bit Macros:
#define SetBits(var, mask) 				((var) |= (mask))
#define ClearBits(var, mask) 			((var) &= (~(mask)))
#define AssignBits(var, mask, val) 		((var) = (((var)&(~(mask)))|((val)&(mask))))
#define GetBits(var, mask)				((var) & (mask))
#define TestBits(var, mask, val)		((((var) & (mask)) == (val)) ? 1 : 0)
#define TestBit(var, val)				((var)&(val) ? 1:0)


#if COMPILE_MODE == DSP_COMPILE

///////////////////////////////////////////////////////////////////////////////////////

// the following section is used as a debugging tool to manage the XF pin on the DSP.
//
// The operation is based on a mask comparison to determine if this invocation of the
// Set_XF() and Clear_XF() functions should be executed or ignored.
//
// Initially, the user sets the XF_MASK value to include those bits which represent
// which sections of code bracketed by the Set_XF() and Clear_XF() functions are to
// operate on the XF pin of the DSP.  Any sections of code which should not be included
// in operation of the XF pin should have their corrosponding bit(s) cleared in the
// XF_MASK value.
//
// Example:
// foo( )
//   {
//   Set_XF( XF_FOO );
//   // do foo operations
//   Clear_XF( XF_FOO );
//   }
//
// bar( )
//   {
//   Set_XF( XF_BAR );
//   // do bar operations
//   Clear_XF( XF_BAR );
//   }
//
// If the XF_MASK has the XF_FOO bit set and the XF_BAR bit clear, then the XF pin will be
// set high during the time the DSP is executing the foo() function.  If the XF_BAR bit is
// set high also, the XF pin will be high during execution of both the foo() and bar()
// functions.
//
// The first bit of the XF_MASK is reserved for general purpose XF pin operation and two
// special macros are created to both simplify operation and be backward compatible with
// existing code.
//
// Alternately, if XF_DYNAMIC is defined, the XF_MASK is implemented as a global variable
// which allows the XF pin operation to be managed by poking values into the XF_MASK
// variable dynamically.  This can speed up debugging at the cost of a couple of extra
// instructions for the Set_XF() and Clear_XF() functions.
//
// For the dynamic mode, the Set_XF() and Clear_XF() functions each require approximately
// five machine instructions to implement depending on the context of the function call.
//
// In the static mode, with XF_DYNAMIC not defined, the Set_XF() and Clear_XF() functions
// will optimize away to either one or zero machine instructions.  If the test succeeds,
// the SSBX or RSBX instruction will be inserted into code, otherwise no instructions are
// inserted into the code
//
// see details of implementation below.

// definitions for the XF bit macros to use, replace unused definitions with more descriptive names
#define XF_GENERAL  ( 0x0001u ) // used for general purpose activation, default for macro invocation
#define XF_FLASH_SM ( 0x0002u ) // indicates the flash management state machine is active
#define XF_UNUSED2  ( 0x0004u )
#define XF_UNUSED3  ( 0x0008u )
#define XF_UNUSED4  ( 0x0010u )
#define XF_UNUSED5  ( 0x0010u )
#define XF_UNUSED6  ( 0x0010u )
#define XF_UNUSED7  ( 0x0010u )
#define XF_UNUSED8  ( 0x0010u )
#define XF_UNUSED9  ( 0x0010u )
#define XF_UNUSED10 ( 0x0010u )
#define XF_UNUSED11 ( 0x0010u )
#define XF_UNUSED12 ( 0x0010u )
#define XF_UNUSED13 ( 0x0010u )
#define XF_UNUSED14 ( 0x0010u )
#define XF_UNUSED15 ( 0x0010u )

// definitions for general use
#define SetXF()     Set_XF( XF_GENERAL )
#define ClearXF() Clear_XF( XF_GENERAL )

#ifdef XF_DYNAMIC
// Provide access to a dynamic global variable to be used.
// Currently defined in global.c
extern volatile u16 XF_MASK;

#else
// define which forms are accepted for the XF bit macro, the general form is the default
#define XF_MASK XF_GENERAL

#endif // XF_DYNAMIC

// create the functions to operate on the XF bit.
asm("XFBit	.set	13");                       // create an assembly constant for the XF bit

inline void Set_XF( u16 mask )
	{
	if( ( mask & XF_MASK ) != 0 )				// if active for this compilation
		asm("	SSBX 1, XFBit");				// set the XF bit
	return;
	}

inline void Clear_XF( u16 mask )
	{
	if( ( mask & XF_MASK ) != 0 )				// if active for this compilation
		asm("	RSBX 1, XFBit");				// clear the XF bit
	return;
	}

// end of Set_XF() and Clear_XF() implementation
///////////////////////////////////////////////////////////////////////////////////////

	// DSP clock frequency in MHz
	#define MIPS	(150)
	asm("MIPS	.set	150"); 

	// Macros for generating controlled short delays      
	// The value of n must be between 2 and 257.
	#define RPTNOP(n)	asm("	RPT #((" #n ")-2)");\
						asm("	NOP")

	// Do a single nop instruction 
//	#define NOP()
//	#define NOP()	asm("	NOP")
	inline void NOP(void)
	{	
		asm("	NOP");
	}

	// Delay1us will delay for 1 us.  Unfortunately, it will lock out interrupts during the
	// delay, so be very careful when using this in time-critical routines.
	#define	Delay1us()	RPTNOP(MIPS)

	#define	DEBUG_DELAY    TRUE		// Choose TRUE for debug, FALSE for release
	#if 	DEBUG_DELAY == TRUE
		#define	DebugDelay()	RPTNOP(4)	// Add NOPs to flush the C54x pipe.  Convenient for breakpoints.
	#else
		#define	DebugDelay()				// Dummy macro that takes no time or space
	#endif
#else
	#define NOP()  		_asm NOP
	#define RPTNOP(n)	//_asm NOP
	#define SetXF() 	
	#define ClearXF()	
	#define Set_XF( mask )
	#define Clear_XF( mask )
	#define	DebugDelay()				// Dummy macro that takes no time or space
#endif

#endif 	// OFDM_DATATYPES_H       Header file guard
