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

#include "stdproj.h"
#include "drivers/TapMcu.h"

#include "gdb.h"
#include "reader.h"
#include "util/util.h"
#include "util/parser.h"
#include "util/expr.h"
#include "util/gdb_proto.h"


#define BMP_FEATURES	0

/************************************************************************
 * GDB server
 */

Gdb::Gdb()
	: wide_regs_(false)
{
}


void Gdb::AppendRegisterContents(GdbData &response, uint32_t r)
{
	response << f::X<2>((uint8_t)r)
		<< f::X<2>((uint8_t)(r >> 8))
		;
	if (wide_regs_)
	{
		response << f::X<2>((uint8_t)(r >> 16))
			<< f::X<2>((uint8_t)(r >> 24))
			;
	}
}


int Gdb::ReadRegister(Parser &parser)
{
	uint32_t reg = 0;

	if(!parser.HasMore())
	{
		Error() << "gdb::ReadRegister: Missing argument!\n";
		return GdbData::Send(GdbData::kMissingArg);
	}
	// Parse register number
	reg = parser.GetUint32(10);
	// Validate
	if (parser.HasMore())
	{
		Error() << "gdb::ReadRegister: Invalid argument '" << parser.GetArg() << "' found!\n";
		return GdbData::Send(GdbData::kInvalidArg);
	}
	Trace() << "Reading register" << reg << '\r';
	reg = g_tap_mcu.GetReg(reg);
	if (reg == UINT32_MAX)
	{
		Error() << "Failed to read register value!\n";
		return GdbData::Send(GdbData::kJtagError);
	}
	else
	{
		GdbData response;
		AppendRegisterContents(response, reg);
		return response.FlushAck();
	}
}


int Gdb::ReadRegisters(Parser &parser)
{
	address_t regs[DEVICE_NUM_REGS];

	Trace() << "Reading registers\n";
	if (g_tap_mcu.GetRegs(regs) < 0)
		return GdbData::Send(GdbData::kJtagError);

	GdbData response;

	for (int i = 0; i < DEVICE_NUM_REGS; i++)
		AppendRegisterContents(response, regs[i]);

	return response.FlushAck();
}


int Gdb::MonitorCommand(Parser &parser)
{
	MonitorBuf::Init();
	// Convert hex buffer into byte/ascii equivalent
	parser.UnhexifyBufferAndReset();

	Trace() << "Monitor command received: " << parser.GetRawData() << '\n';

	process_command(parser.GetRawData());

	if (!MonitorBuf::len)
		return GdbData::Send(GdbData::kOk);

	GdbData response;
	response.AppendData(MonitorBuf::buf, MonitorBuf::len);

	return response.FlushAck();
}


int Gdb::WriteRegister(Parser &parser)
{
	uint8_t shift = 0;
	// Parser register value
	uint32_t reg = parser.GetUint32(10);
	// Validate syntax and register range
	char ch = parser.GetNextChar();
	if (ch != '=')
	{
		Error() << "Invalid character found " << ch << '\n';
		return GdbData::Send(GdbData::kInvalidArg);
	}
	if (reg >= 16)
	{
		Error() << "Invalid register number " << reg << '\n';
		return GdbData::Send(GdbData::kInvalidArg);
	}
	// Parser register value
	uint32_t val = parser.GetHexLsb();	// Hex value MSB order
	if(parser.HasMore())
	{
		Error() << "Too many parameters " << parser.GetRawData() << '\n';
		return GdbData::Send(GdbData::kInvalidArg);
	}
	Debug() << "Setting value " << f::X<8>(val) << " for register " << reg << '\n';
	if (!g_tap_mcu.SetReg(reg, val))
	{
		Error() << "Failed to set register value ";
		return GdbData::Send(GdbData::kJtagError);
	}
	return GdbData::Send(GdbData::kOk);
}


int Gdb::WriteRegisters(Parser &parser)
{
	address_t regs[DEVICE_NUM_REGS];
	uint8_t bytes = 2;

	size_t len = strlen(parser.GetRawData());

	if (len >= DEVICE_NUM_REGS * 8)
		bytes = 4;

	if (len < DEVICE_NUM_REGS * 2 * bytes)
	{
		Error() << "write_registers: short argument\n";
		return GdbData::Send(GdbData::kInvalidArg);
	}

	Trace() << "Writing registers (" << bytes * 8 << " bits each)\n";
	for (int i = 0; i < DEVICE_NUM_REGS; i++)
		regs[i] = parser.GetHexLsb(bytes);

	if (g_tap_mcu.SetRegs(regs) < 0)
		return GdbData::Send(GdbData::kJtagError);
	return GdbData::Send(GdbData::kOk);
}


int Gdb::ReadMemory(Parser &parser)
{
	address_t addr = parser.GetUint32(16);
	char ch = parser.GetNextChar();
	if(ch != ',')
	{
		Error() << "gdb: malformed memory read request\n";
		return GdbData::Send(GdbData::kInvalidArg);
	}
	address_t length = parser.GetUint32(16);

	if (length > GDB_MAX_XFER)
		length = GDB_MAX_XFER;

	Trace() << "Reading " << f::N<4>(length) << " bytes from 0x" << f::X<4>(addr) << '\n';

	if (g_tap_mcu.ReadMem(addr, parser.GetRawBuffer(), length) < 0)
		return GdbData::Send(GdbData::kJtagError);

	GdbData response;
	response.AppendData(parser.GetRawBuffer(), length);
	return response.FlushAck();
}


int Gdb::WriteMemory(Parser &parser)
{
	address_t addr = parser.GetUint32(16);
	char ch = parser.GetNextChar();
	if (ch != ',')
	{
		Error() << "gdb: malformed memory write request\n";
		return GdbData::Send(GdbData::kInvalidArg);
	}
	address_t length = parser.GetUint32(16);
	ch = parser.GetNextChar();
	if (ch != ':')
	{
		Error() << "gdb: malformed memory write request\n";
		return GdbData::Send(GdbData::kInvalidArg);
	}

	// reuse buffer
	uint32_t buflen = parser.UnhexifyBufferAndReset();

	if (buflen != length)
	{
		Error() << "gdb: length mismatch\n";
		return GdbData::Send(GdbData::kInvalidArg);
	}

	Trace() << "Writing " << f::N<4>(length) << " bytes to 0x" << f::X<4>(addr) << '\n';

	if (g_tap_mcu.WriteMem(addr, parser.GetRawBuffer(), buflen) < 0)
		return GdbData::Send(GdbData::kJtagError);

	return GdbData::Send(GdbData::kOk);
}


int Gdb::SetPc(Parser &parser)
{
	parser.SkipSpaces();
	if (!parser.HasMore())
		return 0;

	uint32_t v = parser.GetUint32(16);
	return g_tap_mcu.SetReg(0, v);
}


int Gdb::RunFinalStatus()
{
	address_t regs[DEVICE_NUM_REGS];
	int i;

	if(! g_tap_mcu.IsAttached())
	{
		Debug() << "MCU is not attached!\n";
		return GdbData::Send(GdbData::kNotAttached);
	}

	if (g_tap_mcu.GetRegs(regs) < 0)
	{
		Debug() << "Error reading registers!\n";
		return GdbData::Send(GdbData::kJtagError);
	}

	GdbData response;
	response << "T05";
	for (i = 0; i < 16; i++)
	{
		uint32_t r = regs[i];
		response << f::X<2>(i) << ':'
			<< f::X<2>((uint8_t)r) 
			<< f::X<2>((uint8_t)(r >> 8))
			;
		if (wide_regs_)
		{
			response
				<< f::X<2>((uint8_t)(r >> 16))
				<< f::X<2>((uint8_t)(r >> 24))
				;
		}
		response << ';';
	}

	return response.FlushAck();
}


int Gdb::SingleStep(Parser &parser)
{
	Trace() << "Single stepping\n";

	if (!g_tap_mcu.IsAttached())
		return GdbData::Send(GdbData::kNotAttached2);

	if (SetPc(parser) < 0
		|| g_tap_mcu.DeviceCtl(DEVICE_CTL_STEP) < 0)
		GdbData::Send(GdbData::kJtagError);

	return RunFinalStatus();
}


int Gdb::Run(Parser &parser)
{
	Trace() << "Running\n";

	if (!g_tap_mcu.IsAttached())
		return GdbData::Send(GdbData::kNotAttached2);

	if (SetPc(parser) < 0 ||
		g_tap_mcu.DeviceCtl(DEVICE_CTL_RUN) < 0)
		return GdbData::Send(GdbData::kJtagError);

	for (;;)
	{
		device_status_t status = g_tap_mcu.Poll();

		if (status == DEVICE_STATUS_ERROR)
			return GdbData::Send(GdbData::kJtagError);

		if (status == DEVICE_STATUS_HALTED)
		{
			Trace() << "Target halted\n";
			goto out;
		}

		if (status == DEVICE_STATUS_INTR)
			goto out;

		while (true)
		{
			int c = gUartGdb.GetChar();
			if (c < 0)
				break;
			if (c == 3)
			{
				Trace() << "Interrupted by gdb\n";
				goto out;
			}
		}
	}

out:
	if (g_tap_mcu.DeviceCtl(DEVICE_CTL_HALT) < 0)
		return GdbData::Send(GdbData::kJtagError);

	return RunFinalStatus();
}


int Gdb::SetBreakpoint(Parser &parser)
{
	if (!g_tap_mcu.IsAttached())
		return GdbData::Send(GdbData::kJtagError);

	// Restart scanner as z/Z has a mixed syntax
	parser.RestartScanner();
	bool bSet = parser.GetCurChar() == 'Z';
	parser.SkipChar();		// command
	parser.SkipSpaces();	// space is optional
	char *tok = parser.GetNextArg(",");
	/* Make sure there's a type argument */
	if (tok == NULL)
	{
		Error() << "gdb: breakpoint requested with no type\n";
		return GdbData::Send(GdbData::kMissingArg);
	}
	device_bptype_t type;
	switch (atoi(tok))
	{
	case 0:
	case 1:
		type = DEVICE_BPTYPE_BREAK;
		break;

	case 2:
		type = DEVICE_BPTYPE_WRITE;
		break;

	case 3:
		type = DEVICE_BPTYPE_READ;
		break;

	case 4:
		type = DEVICE_BPTYPE_WATCH;
		break;

	default:
		Error() << "gdb: unsupported breakpoint type: " << tok << '\n';
		return GdbData::Send(GdbData::kUnsupported);
	}
	// Skip ','
	parser.SkipChar();

	// Parse the address token
	tok = parser.GetNextArg(",");
	// There needs to be an address specified
	if (tok == NULL)
	{
		Error() << "gdb: breakpoint address missing\n";
		return GdbData::Send(GdbData::kMissingArg);
	}
	// Parse the breakpoint address
	address_t addr = strtoul(tok, NULL, 16);

	// TODO: handle 3rd parameter

	if (bSet)
	{
		if (g_tap_mcu.SetBrk(-1, 1, addr, type) < 0)
		{
			Error() << "gdb: can't add breakpoint at 0x" << f::X<4>(addr) << '\n';
			return GdbData::Send(GdbData::kJtagError);
		}

		Trace() << "Breakpoint set at 0x" << f::X<4>(addr) << '\n';
	}
	else
	{
		g_tap_mcu.SetBrk(-1, 0, addr, type);
		Trace() << "Breakpoint cleared at 0x" << f::X<4>(addr) << '\n';
	}

	return GdbData::Send(GdbData::kOk);
}


int Gdb::RestartProgram()
{
	if (g_tap_mcu.DeviceCtl(DEVICE_CTL_RESET) < 0)
		return GdbData::Send(GdbData::kJtagError);

	return GdbData::Send(GdbData::kOk);
}


int Gdb::SendEmptyThreadList(Parser &parser)
{
	return GdbData::Send("l");
}


int Gdb::SendCRC(Parser &parser)
{
	// Skip ':'
	parser.SkipChar();
	// Get address
	address_t addr = parser.GetUint32(16);
	if(parser.GetCurChar() != ',')
		return GdbData::Send(GdbData::kMissingArg);
	parser.SkipChar();
	// Get size
	uint32_t size = parser.GetUint32(16);

	CRC32 crc;
	uint8_t buf[512];
	while (size)
	{
		uint32_t todo = size;
		if (todo > _countof(buf))
			todo = _countof(buf);
		if (g_tap_mcu.ReadMem(addr, buf, todo) < 0)
			return GdbData::Send(GdbData::kJtagError);
		crc.Append(buf, todo);
		addr += todo;
		size -= todo;
	}
	GdbData response;
	response << 'C' << f::X<8>(crc.Get());
	return response.FlushAck();
}


int Gdb::SendSupported(Parser &parser)
{
	if (parser.GetCurChar() != ':')
		return GdbData::Send(GdbData::kInvalidArg);

	wide_regs_ = false;
	// Iterate host features
	char *arg;
	while ((arg = parser.GetNextListArg()) != NULL)
	{
		/* This is a hack to distinguish msp430-elf-gdb
		** from msp430-gdb. The former expects 32-bit
		** register fields.
		*/
		if (strcmp(arg, "multiprocess+") == 0)
			wide_regs_ = true;
		// TODO: more features?
	}

	GdbData response;
	response << "PacketSize=" << f::X<1>(GDB_MAX_XFER * 2)
#if BMP_FEATURES
		<< ";qXfer:memory-map:read+;qXfer:features:read+"
#endif
		;
	return response.FlushAck();
}


class CToggleLed
{
public:
	CToggleLed() NO_INLINE
	{
		if (g_tap_mcu.IsAttached())
			GreenLedOn();
		else
			RedLedOn();
	}
	~CToggleLed() NO_INLINE
	{
		if (g_tap_mcu.IsAttached())
		{
			GreenLedOff();
			RedLedOn();
		}
		else
		{
			RedLedOff();
			GreenLedOn();
		}
	}
};


typedef int (Gdb::*GdbCmdHandler)(Parser &);
struct GdbOneCmd
{
	const char *text;
	GdbCmdHandler fn;
};


int Gdb::ProcessCmdTable(Parser &parser, const GdbOneCmd *table, size_t ntab)
{
	const char *cmd = parser.GetNextArg(":,;");
	for (size_t i = 0; i < ntab; ++i)
	{
		const GdbOneCmd &entry = table[i];
		if (strcmp(cmd, entry.text) == 0)
		{
			if (entry.fn)
				return (this->*entry.fn)(parser);
			else
				goto unknown_cmd;
		}
	}
	Trace() << "process_gdb_command: unknown command " << parser.GetRawBuffer() << '\n';
unknown_cmd:
	/* For unknown/unsupported packets, return an empty reply */
	return GdbData::Send(GdbData::kUnsupported);
}


int Gdb::ProcessCommand(char *buf_p)
{
	static GdbOneCmd qCmds[] =
	{
		{"C", NULL},
		{"CRC", &Gdb::SendCRC},
		{"Rcmd", &Gdb::MonitorCommand},
		{"Supported", &Gdb::SendSupported},
		{"ThreadExtraInfo", NULL},
		{"fThreadInfo", &Gdb::SendEmptyThreadList},
		{"sThreadInfo", &Gdb::SendEmptyThreadList},
	};

	CToggleLed led_ctrl;

	Parser parser(buf_p);

	Debug() << "process_gdb_command: " << buf_p << '\n';
	char cmd = parser.GetCurChar();
	parser.SkipChar();
	// First char is command class
	switch (cmd)
	{
	case '?': // Return target halt reason
		return RunFinalStatus();

	case '!': // Persistent mode
		/*
		** This doesn't do anything, we support the extended
		** protocol anyway, but GDB will never send us a 'R'
		** packet unless we answer 'OK' here.
		*/
		return GdbData::Send(GdbData::kOk);

	case 'z':
	case 'Z':
		return SetBreakpoint(parser);

	case 'r': // Restart
	case 'R':
		return RestartProgram();

	case 'g': // Read registers
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return ReadRegisters(parser);

	case 'p': // Read single register
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return ReadRegister(parser);

	case 'G': // Write registers
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return WriteRegisters(parser);

	case 'P': // Write single register
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return WriteRegister(parser);

	case 'q': // Query
		return ProcessCmdTable(parser, qCmds, _countof(qCmds));
#if BMP_FEATURES
		else if (strncmp(packet, "qXfer:memory-map:read::", 23) == 0)
		{
		}
		else if (strncmp(packet, "qXfer:features:read:target.xml:", 31) == 0)
		{
		}
		else if (!strncmp(buf, "qCRC:", 5))
		{
		}
#endif
		break;

#if BMP_FEATURES
	case 'v':	// General query packet
		break;
#endif

	case 'm': // Read memory
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return ReadMemory(parser.SkipChar());

	case 'M': // Write memory
		if (!g_tap_mcu.IsAttached())
		{
target_detached:
			return GdbData::Send(GdbData::kJtagError);
		}
		return WriteMemory(parser.SkipChar());

#if BMP_FEATURES
	case 'X': /* 'X addr,len:XX': Write binary data to addr */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		break;
#endif

	case 'c': /* Continue */
		return Run(parser.SkipChar());

	case 's': /* Single step */
		return SingleStep(parser.SkipChar());

	case 'k': /* kill */
		return -1;

#if BMP_FEATURES
	case 0x04:
	case 'D':	/* GDB 'detach' command. */
		return -1;
#endif
	}

	Trace() << "process_gdb_command: unknown command " << buf_p << '\n';

unknown_cmd:
	/* For unknown/unsupported packets, return an empty reply */
	return GdbData::Send(GdbData::kUnsupported);
}


void Gdb::ReaderLoop()
{
	while (true)
	{
		char buf[GDB_BUF_SIZE];
		int len = 0;

		len = gdb_read_packet(buf);
		if (len < 0)
			return;
		if (len && ProcessCommand(buf) < 0)
			return;
	}
}


int Gdb::Serve()
{
	for (int i = 5; !g_tap_mcu.Open(); --i)
	{
		if (i == 0)
			return 0;
		StopWatch().Delay(500);
	}
	wide_regs_ = false;

	/* Put the hardware breakpoint setting into a known state. */
	Trace() << "Clearing all breakpoints...\n";
	g_tap_mcu.ClearBrk();

	Debug() << "starting GDB reader loop...\n";
	ReaderLoop();
	Debug() << "... reader loop returned\n";
	g_tap_mcu.Close();
	return /*data.error_ ? -1 :*/ 0;
}

