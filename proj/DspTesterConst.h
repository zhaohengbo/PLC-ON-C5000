// DSPTESTERCONST  Define constants which must match in the DSP and tester code sets.
//                 This file MUST MATCH the matlab script "DspTesterConst.m"

//==========================================================================================
// Revision History
//==========================================================================================


//==========================================================================================
// Command number constants
// These definitions must match those defined in DspTesterConst.m
//==========================================================================================
//	The MASTER supports the following commands:
//	(for the Master DSP)
	#define MC_GET_PARM			(0x01)	// Get parm		Get the value of a single status variable.
	#define MC_SET_PARM			(0x02)	// Set parm		Set the value of a single status variable.
	#define MC_GET_STATUS		(0x03)	// Get status 	Get the values of all status variables.
	#define MC_START_BER		(0x04)	// Start BER 	Reset counters and start BER testing.
	#define MC_ABORT_BER		(0x05)	// Abort BER 	Abort BER testing.
	#define MC_WRITE_FLASH		(0x06)	// Write flash	Set up to receive data for flash write.
	#define MC_READ_BLOCK		(0x07)	// Read block	Read a block of DSP Memory.
	#define MC_ERASE_FLASH		(0x08)	// Erase flash	Erase either DSP or MSP flash.
	#define MC_FIND_SLAVES		(0x09)	// Find Slaves  Start searching for all slaves.
	#define MC_ABORT_FIND		(0x0A)	// Abort Find	Abort find slaves command.
	#define MC_READ_FLASH		(0x0B)	// Read Flash	Used to read a block of flash data.
	#define MC_RESET_MSP		(0x0C)	// Reset MSP	Hard reset of the MSP430
	#define MC_EMETER_CMD		(0x10)	// Emeter cmd	Send a command to the emeter. (no response expected)
	#define MC_EMETER_REQ		(0x11)	// Emeter req	Send a request to the emeter. (response expected)
//	(for the Slave DSP)
	#define MC_GET_SLAVE_PARM	(0x101)	// Get parm		Get the value of a single status variable.
	#define MC_SET_SLAVE_PARM	(0x102)	// Set parm		Set the value of a single status variable.
	#define MC_GET_SLAVE_STATUS	(0x103)	// Get status	Get the values of all status variables.
	#define MC_READ_SLAVE_BLOCK	(0x107) // Read Block	Read a block of DSP Memory.
	#define MC_ERASE_SLAVE_FLASH (0x108) // Erase flash	Erase either DSP or MSP flash.
	#define MC_SLAVE_READ_FLASH	(0x10B)	// Read Flash	Used to read a block of flash data.
	#define MC_SLAVE_RESET_MSP	(0x10C)	// Reset MSP	Hard reset of the MSP430
	#define MC_SLAVE_EMETER_CMD	(0x110)	// Emeter cmd	Send a command to the emeter. (no response expected)
	#define MC_SLAVE_EMETER_REQ	(0x111)	// Emeter req	Send a request to the emeter. (response expected)

//	The SLAVE supports the following commands:
	#define SC_GET_PARM			(0x01)	// Get parm		Get the value of a single status variable.
	#define SC_SET_PARM			(0x02)	// Set parm		Set the value of a single status variable.
	#define SC_GET_STATUS		(0x03)	// Get status 	Get the values of all status variables.
	#define SC_BER				(0x04)	// Start BER 	Reset counters and start BER testing.
	#define SC_ABORT_BER		(0x05)	// Abort BER 	Abort BER testing.
	#define SC_AUTOPOLL			(0x06)	// Autopoll		Request an autopoll packet.
	#define SC_READ_BLOCK		(0x07)	// Read block	Read a block of DSP Memory.
	#define SC_ERASE_FLASH		(0x08)	// Erase flash	Erase either DSP or MSP flash.
	#define SC_READ_FLASH		(0x0B)	// Read Flash	Used to read a block of flash data.
	#define SC_RESET_MSP		(0x0C)	// Reset MSP	Hard reset of the MSP430
	#define SC_EMETER_CMD		(0x10)	// Emeter cmd	Send a command to the emeter. (no response expected)
	#define SC_EMETER_REQ		(0x11)	// Emeter req	Send a request to the emeter. (response expected)


