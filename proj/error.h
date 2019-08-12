//==========================================================================================
// Filename:		error.h
//
// Description:		Error code constant definitions.
// 					The first nibble (4-bits) identify the board reporting an error:
//							0 for master
//							1 for slave
//							2 for meter
// 					The second nibble represents the command number the error is
//					associated with.
//
//
// Copyright (C) 2003 Texas Instruments Incorporated
// Texas Instruments Proprietary Information
// Use subject to terms and conditions of TI Software License Agreement
//
// Revision History:
//==========================================================================================

// Error codes for master commands.
	// Error codes not tied to a specific command
		#define ERR_M_UNKNOWN_COMMAND		(0x0010)
		#define ERR_SLAVE_RESPONSE_TIMEOUT	(0x0020)
		#define ERR_RECV_DATA_TIMEOUT		(0x0030)
		#define ERR_MASTER_EMETER_TIMEOUT	(0x0040)
		#define ERR_SLAVE_EMETER_TIMEOUT	(0x0050)


	// Error codes for command "Master write flash"
		#define FLASH_ERROR					(0x600)		// This is used as offset with different status values.

// Error codes for slave commands.
	// Error codes not tied to a specific command
		#define ERR_S_UNKNOWN_COMMAND		(0x1010)
