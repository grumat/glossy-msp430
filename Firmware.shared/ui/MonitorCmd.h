#pragma once

#include "util/MonitorBuf.h"

class Parser;

//! Handlers for the GDB 'monitor' (qRcmd) console commands registered in the
//! CmdDb table. Each method matches CmdDb::Func — int(Parser &) — and returns
//! 0 on success, < 0 on error. Arguments are pulled with Parser::GetArg().
class MonitorCmd
{
public:
	//! Dispatch one monitor command line: trim, look up in CmdDb, gate on the
	//! current connection state, and invoke the matching handler below.
	//! Returns 0 on success, < 0 on error / unknown / wrong-state command.
	static int Dispatch(char *line);

public:
	static int Help(Parser &arg);		//!< list commands / show help for one
	static int Regs(Parser &arg);		//!< read and display CPU registers
	static int Reset(Parser &arg);		//!< reset (and halt) the CPU
	static int Erase(Parser &arg);		//!< erase flash (all / segment / range / infoa)
	static int Run(Parser &arg);		//!< run until breakpoint or interrupt
	static int Set(Parser &arg);		//!< set a CPU register value

	// Monitor menu (#46)
	static int Version(Parser &arg);	//!< firmware version + compiled/active transports
	static int JtagScan(Parser &arg);	//!< select 4-wire JTAG and acquire the target
	static int SbwScan(Parser &arg);	//!< select 2-wire Spy-Bi-Wire and acquire the target
	static int ChipInfo(Parser &arg);	//!< show device and memory map
	static int Power(Parser &arg);		//!< read or set target supply voltage

private:
	//! Colorized CPU register dump to the shared MonitorStream (used by Regs /
	//! Reset / Run). \p regs is the full register file.
	static void ShowRegs(const address_t *regs);
};
