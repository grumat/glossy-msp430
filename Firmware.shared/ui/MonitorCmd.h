#pragma once

#include "util/output_util.h"

//! Handlers for the GDB 'monitor' (qRcmd) console commands registered in the
//! CmdDb table. Each method matches CmdDb::Func — int(char **arg) — and returns
//! 0 on success, < 0 on error. The argument cursor is advanced with get_arg().
class MonitorCmd
{
public:
	static int Help(char **arg);		//!< list commands / show help for one (defined in stdcmd.cpp)
	static int Regs(char **arg);		//!< read and display CPU registers
	static int Reset(char **arg);		//!< reset (and halt) the CPU
	static int Erase(char **arg);		//!< erase flash (all / segment / range / infoa)
	static int Run(char **arg);			//!< run until breakpoint or interrupt
	static int Set(char **arg);			//!< set a CPU register value

	// Monitor menu (#46)
	static int Version(char **arg);		//!< firmware version + compiled/active transports
	static int JtagScan(char **arg);	//!< select 4-wire JTAG and acquire the target
	static int SbwScan(char **arg);		//!< select 2-wire Spy-Bi-Wire and acquire the target
	static int ChipInfo(char **arg);	//!< show device and memory map
	static int Power(char **arg);		//!< read or set target supply voltage
};
