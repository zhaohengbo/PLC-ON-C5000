//==========================================================================================
// Filename:		DspTesterConst.c
//
// Description:		Initialize the variable address arrays used by the tester.
//					This file along with DspTesterConst.h MUST match the corresponding
//					file used by the tester: DspTesterConst.m.
//
// Copyright (C) 2001 - 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

#include "ofdm_modem.h"

void InitTesterParms(void)
{
	u16		i;			// Loop index
	
	
	// Initialize the default arrays of command parm addresses
	// "Real" addresses are filled in below based on Master/Slave.
	for(i=0; i<30; i++)		// Point unused entries to the scratch reg addresses
	{
		uppParms16[i] = (u16*) &uScratch16;
	}
	for(i=0; i<10; i++)		// Point unused entries to the scratch reg addresses
	{
		uppParms32[i] = (u32*) &ulScratch32;
	}


	//=============================================================================================
	// Initialize the arrays of command parm addresses (replace defaults assigned above)
	// These definitions must match those defined in DspTesterConst.m
	//=============================================================================================
	if (uBoard == BOARD_MASTER)
	{
		uppParms16[0] = (u16*) &uAutoPoll;
		uppParms16[1] = (u16*) &uErrCntAddr;
		uppParms16[2] = (u16*) &uFlashWriteStatus;
		uppParms16[3] = (u16*) &uCodeVersion;
		uppParms16[4] = (u16*) &uHoldOffPLC;
		#if (SAVETRACE == TRUE) || (SAVESYMBOLS == TRUE)
			uppParms16[27] = (u16*) &uTraceDataAddr;
			uppParms16[28] = (u16*) &uTraceDataLen;	
		#endif

		uppParms32[0] = (u32*) &ulMyAddr;
		uppParms32[1] = (u32*) &ulClosestSlaveAddr;
		uppParms32[2] = (u32*) &ulBerCounter;
		uppParms32[3] = (u32*) &ulBerDelay;
	}

	else	// uBoard == BOARD_SLAVE
	{
		uppParms16[0] = (u16*) &uBerMissedPackets;
		uppParms16[1] = (u16*) &uBerErrorCounter;
		uppParms16[2] = (u16*) &uErrCntAddr;
		#if (SAVETRACE == TRUE) || (SAVESYMBOLS == TRUE)
			uppParms16[27] = (u16*) &uTraceDataAddr;
			uppParms16[28] = (u16*) &uTraceDataLen;	
		#endif

		uppParms32[0] = (u32*) &ulMyAddr;
		uppParms32[1] = (u32*) &ulBerCounter;
	}

	return;
}


