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

static int register_bytes;


#define BMP_FEATURES	0

/************************************************************************
 * GDB server
 */

void Gdb::AppendRegisterContents(GdbData &response, uint32_t r)
{
	response << f::X<2>((uint8_t)r)
		<< f::X<2>((uint8_t)(r >> 8))
		;
	if (register_bytes > 2)
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

static int run_final_status()
{
	address_t regs[DEVICE_NUM_REGS];
	int i;

	if(! g_tap_mcu.IsAttached())
	{
		Debug() << "MCU is not attached!\n";
		return GdbData::Send("W00");
	}

	if (g_tap_mcu.GetRegs(regs) < 0)
	{
		Debug() << "Error reading registers!\n";
		return GdbData::Send("E00");
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
		if (register_bytes > 2)
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
		return GdbData::Send("X1D");

	if (SetPc(parser) < 0
		|| g_tap_mcu.DeviceCtl(DEVICE_CTL_STEP) < 0)
		GdbData::Send("E00");

	return run_final_status();
}

int Gdb::Run(Parser &parser)
{
	Trace() << "Running\n";

	if (!g_tap_mcu.IsAttached())
		return GdbData::Send("X1D");

	if (SetPc(parser) < 0 ||
		g_tap_mcu.DeviceCtl(DEVICE_CTL_RUN) < 0)
		return GdbData::Send("E00");

	for (;;)
	{
		device_status_t status = g_tap_mcu.Poll();

		if (status == DEVICE_STATUS_ERROR)
			return GdbData::Send("E00");

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
		return GdbData::Send("E00");

	return run_final_status();
}

static int set_breakpoint(int enable, char *buf)
{
	char *parts[2];
	address_t addr;
	device_bptype_t type;
	int i;

	/* Break up the arguments */
	for (i = 0; i < 2; i++)
		parts[i] = strsep(&buf, ",");

	/* Make sure there's a type argument */
	if (!parts[0])
	{
		Error() << "gdb: breakpoint requested with no type\n";
		return GdbData::Send("E00");
	}

	switch (atoi(parts[0]))
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
		Error() << "gdb: unsupported breakpoint type: " << parts[0] << '\n';
		return GdbData::Send("");
	}

	/* There needs to be an address specified */
	if (!parts[1])
	{
		Error() << "gdb: breakpoint address missing\n";
		return GdbData::Send("E00");
	}

	/* Parse the breakpoint address */
	addr = strtoul(parts[1], NULL, 16);

	if (enable)
	{
		if (g_tap_mcu.SetBrk(-1, 1, addr, type) < 0)
		{
			Error() << "gdb: can't add breakpoint at 0x" << f::X<4>(addr) << '\n';
			return GdbData::Send("E00");
		}

		Trace() << "Breakpoint set at 0x" << f::X<4>(addr) << '\n';
	}
	else
	{
		g_tap_mcu.SetBrk(-1, 0, addr, type);
		Trace() << "Breakpoint cleared at 0x" << f::X<4>(addr) << '\n';
	}

	return GdbData::Send("OK");
}

static int restart_program()
{
	if (g_tap_mcu.DeviceCtl(DEVICE_CTL_RESET) < 0)
		return GdbData::Send("E00");

	return GdbData::Send("OK");
}

static int gdb_send_empty_threadlist(bool fStart)
{
	if(fStart)
	{
		return GdbData::Send("l");
		//return GdbData::Send("m 1");
		//return GdbData::Send("<?xml version=\"1.0\"?><threads></threads>");
	}
	else
		return GdbData::Send("l");
}

static int gdb_send_supported()
{
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

int Gdb::ProcessCommand(char *buf)
{
	CToggleLed led_ctrl;

	Parser parser(buf);

	Debug() << "process_gdb_command: " << buf << '\n';
	switch (parser.GetCurChar())
	{
	case '?': /* Return target halt reason */
		return run_final_status();

	case '!': /* Persistent mode */
		/*
		** This doesn't do anything, we support the extended
		** protocol anyway, but GDB will never send us a 'R'
		** packet unless we answer 'OK' here.
		*/
		return GdbData::Send("OK");

	case 'z':
	case 'Z':
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return set_breakpoint(buf[0] == 'Z', buf + 1);

	case 'r': /* Restart */
	case 'R':
		return restart_program();

	case 'g': /* Read registers */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return ReadRegisters(parser.SkipChar());

	case 'p': /* Read single register */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return ReadRegister(parser.SkipChar());

	case 'G': /* Write registers */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return WriteRegisters(parser.SkipChar());

	case 'P': /* Write single register */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return WriteRegister(parser.SkipChar());

	case 'q': /* Query */
		if (!strncmp(buf, "qRcmd,", 6))
			return MonitorCommand(parser.SkipChars(6));
		if (strncmp(buf, "qSupported", 10) == 0)
		{
			/* This is a hack to distinguish msp430-elf-gdb
			 * from msp430-gdb. The former expects 32-bit
			 * register fields.
			 */
			if (strstr(buf, "multiprocess+"))
				register_bytes = 4;
			else
				register_bytes = 2;

			return gdb_send_supported();
		}
		else if (strncmp(buf, "qfThreadInfo", 12) == 0)
			return gdb_send_empty_threadlist(true);
		else if (strncmp(buf, "qsThreadInfo", 12) == 0)
			return gdb_send_empty_threadlist(false);
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
	case 'v':	/* General query packet */
		break;
#endif

	case 'm': /* Read memory */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return ReadMemory(parser.SkipChar());

	case 'M': /* Write memory */
		if (!g_tap_mcu.IsAttached())
		{
	target_detached:
			return GdbData::Send("EFF");
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

	Trace() << "process_gdb_command: unknown command " << buf << '\n';

	/* For unknown/unsupported packets, return an empty reply */
	return GdbData::Send("");
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
	register_bytes = 2;

	/* Put the hardware breakpoint setting into a known state. */
	Trace() << "Clearing all breakpoints...\n";
	g_tap_mcu.ClearBrk();

	Debug() << "starting GDB reader loop...\n";
	ReaderLoop();
	Debug() << "... reader loop returned\n";
	g_tap_mcu.Close();
	return /*data.error_ ? -1 :*/ 0;
}

