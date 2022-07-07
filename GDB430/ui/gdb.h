/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009, 2010 Daniel Beer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once


//! Support to multiprocess protocol
#define OPT_MULTIPROCESS	0
//! Number of registers to reply on '?' command
#define OPT_SHORT_QUERY_REPLY	2

class Parser;
class GdbData;
struct GdbOneCmd;


class Gdb
{
public:
	Gdb();

	int Serve();

protected:
	int ProcessCommand(char *buf);
	void ReaderLoop();

protected:
	int ReadRegister(Parser &parser);
	int ReadRegisters(Parser &parser);
	int MonitorCommand(Parser &parser);
	int WriteRegister(Parser &parser);
	int WriteRegisters(Parser &parser);
	int ReadMemory(Parser &parser);
	int WriteMemory(Parser &parser);
	bool SetPc(Parser &parser);
	int SingleStep(Parser &parser);
	int Run(Parser &parser);
	int RunFinalStatus();
	int SendSupported(Parser &parser);
	int HandleXfer(Parser &parser) DEBUGGABLE;
#if OPT_MULTIPROCESS
	int SendEmptyThreadList(Parser &parser);
#endif
	int RestartProgram();
	int SetBreakpoint(Parser &parser);
	int SendC(Parser &parser);
	int SendCRC(Parser &parser);

protected:
	void AppendRegisterContents(GdbData &response, uint32_t r);
	int ProcessCmdTable(Parser &parser, const GdbOneCmd *table, size_t ntab);

protected:
	bool wide_regs_;
};

