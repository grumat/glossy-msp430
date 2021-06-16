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
#include "util/expr.h"
#include "util/gdb_proto.h"

static int register_bytes;


#define BMP_FEATURES	0

/************************************************************************
 * GDB server
 */

static int read_registers()
{
	address_t regs[DEVICE_NUM_REGS];

	Trace() << "Reading registers\n";
	if (g_tap_mcu.GetRegs(regs) < 0)
		return GdbData::Send("E00");

	GdbData response;

	for (int i = 0; i < DEVICE_NUM_REGS; i++)
		response << f::Xw(regs[i], register_bytes*2);

	return response.FlushAck();
}


static int monitor_command(char *buf)
{
	char cmd[128];
	int len = 0;
	int i;

	while (len + 1 < sizeof(cmd) && *buf && buf[1])
	{
		if (len + 1 >= sizeof(cmd))
			break;

		cmd[len++] = (hexval(buf[0]) << 4) | hexval(buf[1]);
		buf += 2;
	}
	cmd[len] = 0;

	Trace() << "Monitor command received: " << cmd << '\n';

	//capture_start(monitor_capture, &mbuf);
	process_command(cmd);
	//capture_end();

	if (!MonitorBuf::len)
		return GdbData::Send("OK");

	GdbData response;
	response.AppendData(MonitorBuf::buf, MonitorBuf::len);

	return response.FlushAck();
}

static int write_registers(char *buf)
{
	address_t regs[DEVICE_NUM_REGS];
	int nibbles = 4;

	size_t len = strlen(buf);

	if (len >= DEVICE_NUM_REGS * 8)
		nibbles = 8;

	if (len < DEVICE_NUM_REGS * nibbles)
	{
		Error() << "write_registers: short argument\n";
		return GdbData::Send("E00");
	}

	Trace() << "Writing registers (" << nibbles * 4 << " bits each)\n";
	for (int i = 0; i < DEVICE_NUM_REGS; i++)
	{
		uint32_t r = 0;

		for (int j = 0; j < nibbles; j++)
			r = (r << 4) |
			hexval(buf[i * nibbles +
				   nibbles - 1 - (j ^ 1)]);
		regs[i] = r;
	}

	if (g_tap_mcu.SetRegs(regs) < 0)
		return GdbData::Send("E00");

	return GdbData::Send("OK");
}

static int read_memory(char *text)
{
	char *length_text = strchr(text, ',');
	address_t length, addr;
	uint8_t buf[GDB_MAX_XFER];

	if (!length_text)
	{
		Error() << "gdb: malformed memory read request\n";
		return GdbData::Send("E00");
	}

	*(length_text++) = 0;

	length = strtoul(length_text, NULL, 16);
	addr = strtoul(text, NULL, 16);

	if (length > sizeof(buf))
		length = sizeof(buf);

	Trace() << "Reading " << f::N<4>(length) << " bytes from 0x" << f::X<4>(addr) << '\n';

	if (g_tap_mcu.ReadMem(addr, buf, length) < 0)
		return GdbData::Send("E00");

	GdbData response;
	response.AppendData(buf, length);
	return response.FlushAck();
}

static int write_memory(char *text)
{
	address_t length, addr;
	uint8_t buf[GDB_MAX_XFER];

	int buflen = 0;
	char *data_text = strchr(text, ':');
	char *length_text = strchr(text, ',');

	if (!(data_text && length_text))
	{
		Error() << "gdb: malformed memory write request\n";
		return GdbData::Send("E00");
	}

	*(data_text++) = 0;
	*(length_text++) = 0;

	length = strtoul(length_text, NULL, 16);
	addr = strtoul(text, NULL, 16);

	while (buflen < sizeof(buf) && *data_text && data_text[1])
	{
		buf[buflen++] = (hexval(data_text[0]) << 4) |
			hexval(data_text[1]);
		data_text += 2;
	}

	if (buflen != length)
	{
		Error() << "gdb: length mismatch\n";
		return GdbData::Send("E00");
	}

	Trace() << "Writing " << f::N<4>(length) << " bytes to 0x" << f::X<4>(addr) << '\n';

	if (g_tap_mcu.WriteMem(addr, buf, buflen) < 0)
		return GdbData::Send("E00");

	return GdbData::Send("OK");
}

static int run_set_pc(char *buf)
{
	address_t regs[DEVICE_NUM_REGS];

	if (!*buf)
		return 0;

	if (g_tap_mcu.GetRegs(regs) < 0)
		return -1;

	regs[0] = strtoul(buf, NULL, 16);
	return g_tap_mcu.SetRegs(regs);
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
		response << f::X<2>(i) << ':'
			<< f::Xw(regs[i], register_bytes*2) << ';';
	}

	return response.FlushAck();
}

static int single_step(char *buf)
{
	Trace() << "Single stepping\n";

	if (!g_tap_mcu.IsAttached())
		return GdbData::Send("X1D");

	if (run_set_pc(buf) < 0 
		|| g_tap_mcu.DeviceCtl(DEVICE_CTL_STEP) < 0)
		GdbData::Send("E00");

	return run_final_status();
}

static int run(char *buf)
{
	Trace() << "Running\n";

	if (!g_tap_mcu.IsAttached())
		return GdbData::Send("X1D");

	if (run_set_pc(buf) < 0 ||
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

static int gdb_send_empty_threadlist()
{
	return GdbData::Send("<?xml version=\"1.0\"?><threads></threads>");
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

static int process_gdb_command(char *buf)
{
	Debug() << "process_gdb_command: " << buf << '\n';
	switch (buf[0])
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
		return read_registers();

#if BMP_FEATURES
	case 'p': /* Read single register */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		break;
#endif

	case 'G': /* Write registers */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		return write_registers(buf + 1);

#if BMP_FEATURES
	case 'P': /* Write single register */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		break;
#endif

	case 'q': /* Query */
		if (!strncmp(buf, "qRcmd,", 6))
			return monitor_command(buf + 6);
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
			return gdb_send_empty_threadlist();
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
		return read_memory(buf + 1);

	case 'M': /* Write memory */
		if (!g_tap_mcu.IsAttached())
		{
	target_detached:
			return GdbData::Send("EFF");
		}
		return write_memory(buf + 1);

#if BMP_FEATURES
	case 'X': /* 'X addr,len:XX': Write binary data to addr */
		if (!g_tap_mcu.IsAttached())
			goto target_detached;
		break;
#endif

	case 'c': /* Continue */
		return run(buf + 1);

	case 's': /* Single step */
		return single_step(buf + 1);

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

static void gdb_reader_loop()
{
	while (true)
	{
		char buf[GDB_BUF_SIZE];
		int len = 0;

		len = gdb_read_packet(buf);
		if (len < 0)
			return;
		if (len && process_gdb_command(buf) < 0)
			return;
	}
}

static int gdb_server()
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
	gdb_reader_loop();
	Debug() << "... reader loop returned\n";
	g_tap_mcu.Close();
	return /*data.error_ ? -1 :*/ 0;
}

int cmd_gdb()
{
	if (gdb_server() < 0)
		return -1;
	return 0;
}

