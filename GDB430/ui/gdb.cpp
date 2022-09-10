#include "stdproj.h"
#include "drivers/TapMcu.h"

#include "gdb.h"
#include "reader.h"
#include "util/util.h"
#include "util/parser.h"
#include "util/expr.h"
#include "util/gdb_proto.h"
#include "util/crc32.h"


#define BMP_FEATURES	0

/************************************************************************
** GDB server
*/

Gdb::Gdb()
	: wide_regs_(true)
{
}


int Gdb::StartNoAckMode(Parser &parser)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);
	char ch = parser.GetNextChar();
	if (ch == '+')
		GdbData::send_ack_ = 1;
	else if (ch == '-')
		GdbData::send_ack_ = -1;
	else
		return GdbData::InvalidArg(__FUNCTION__, "Invalid character found");
	return GdbData::OK();
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
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	uint32_t reg = 0;

	parser.SkipSpaces();
	if(!parser.HasMore())
		return GdbData::MissingArg(__FUNCTION__);
	// Parse register number
	reg = parser.GetUint32(10);
	// Validate
	if (parser.HasMore())
		return GdbData::InvalidArg(__FUNCTION__, parser.GetArg());
	Trace() << "Reading register" << reg << '\r';
	reg = g_TapMcu.GetReg(reg);
	if (reg == UINT32_MAX)
		return GdbData::ErrorJtag(__FUNCTION__);
	else
	{
		GdbData response;
		AppendRegisterContents(response, reg);
		return response.FlushAck();
	}
}


int Gdb::ReadRegisters(Parser &parser)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	address_t regs[DEVICE_NUM_REGS];

	Trace() << "Reading registers\n";
	if (!g_TapMcu.GetRegs(regs))
		return GdbData::ErrorJtag(__FUNCTION__);

	GdbData response;

	for (int i = 0; i < DEVICE_NUM_REGS; i++)
		AppendRegisterContents(response, regs[i]);

	return response.FlushAck();
}


int Gdb::MonitorCommand(Parser &parser)
{
	MonitorBuf::Init();
	if(parser.GetCurChar() != ',')
		return GdbData::MissingArg(__FUNCTION__);
	parser.SkipChar();
	// Convert hex buffer into byte/ascii equivalent
	parser.UnhexifyBufferAndReset();

	Trace() << "Monitor command received: " << parser.GetRawData() << '\n';

	process_command(parser.GetRawData());

	if (!MonitorBuf::len)
		return GdbData::OK();

	GdbData response;
	response.AppendData(MonitorBuf::buf, MonitorBuf::len);

	return response.FlushAck();
}


int Gdb::WriteRegister(Parser &parser)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	parser.SkipSpaces();

	uint8_t shift = 0;
	// Parser register value
	uint32_t reg = parser.GetUint32(16);
	// Validate syntax and register range
	char ch = parser.GetNextChar();
	if (ch != '=')
		return GdbData::InvalidArg(__FUNCTION__, "Invalid character found");
	if (reg >= 16)
		return GdbData::InvalidArg(__FUNCTION__, "Invalid register number");
	// Parser register value
	uint32_t val = parser.GetHexLsb();	// Hex value MSB order
	if(parser.HasMore())
		return GdbData::InvalidArg(__FUNCTION__, "Too many parameters");
	Debug() << "Setting value " << f::X<8>(val) << " for register " << reg << '\n';
	if (!g_TapMcu.SetReg(reg, val))
		return GdbData::ErrorJtag(__FUNCTION__, "Failed to set register value");
	return GdbData::OK();
}


int Gdb::WriteRegisters(Parser &parser)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	address_t regs[DEVICE_NUM_REGS];
	uint8_t bytes = 2;

	parser.SkipSpaces();

	size_t len = strlen(parser.GetRawData());

	if (len >= DEVICE_NUM_REGS * 8)
		bytes = 4;

	if (len < DEVICE_NUM_REGS * 2 * bytes)
		return GdbData::InvalidArg(__FUNCTION__, "Short argument");

	Trace() << "Writing registers (" << bytes * 8 << " bits each)\n";
	for (int i = 0; i < DEVICE_NUM_REGS; i++)
		regs[i] = parser.GetHexLsb(bytes);

	if (g_TapMcu.SetRegs(regs) < 0)
		return GdbData::ErrorJtag(__FUNCTION__);
	return GdbData::OK();
}


int Gdb::ReadMemory(Parser &parser)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	parser.SkipSpaces();

	address_t addr = parser.GetUint32(16);
	char ch = parser.GetNextChar();
	if(ch != ',')
		return GdbData::InvalidArg(__FUNCTION__, "malformed memory read request");
	address_t length = parser.GetUint32(16);

	if (length == 0)
		return GdbData::InvalidArg(__FUNCTION__, "length mismatch");

	if (length > GDB_MAX_XFER)
		length = GDB_MAX_XFER;

	Trace() << "Reading " << f::N<4>(length) << " bytes from 0x" << f::X<4>(addr) << '\n';
	
	uint8_t buf[length];

	if (!g_TapMcu.ReadMem(addr, buf, length))
		return GdbData::ErrorJtag(__FUNCTION__);

	GdbData response;
	response.AppendData(buf, length);
	return response.FlushAck();
}


int Gdb::WriteMemory(Parser &parser, bool bin_mode)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	parser.SkipSpaces();
	address_t addr = parser.GetUint32(16);
	char ch = parser.GetNextChar();
	if (ch != ',')
		return GdbData::InvalidArg(__FUNCTION__, "malformed memory write request");
	address_t length = parser.GetUint32(16);
	ch = parser.GetNextChar();
	if (ch != ':')
		return GdbData::InvalidArg(__FUNCTION__, "malformed memory write request");

	// reuse buffer
	uint32_t buflen = bin_mode 
		? parser.UnescapeBinBufferAndReset()
		: parser.UnhexifyBufferAndReset()
		;

	if (buflen != length)
		return GdbData::InvalidArg(__FUNCTION__, "length mismatch");

	Trace() << "Writing " << f::N<4>(length) << " bytes to 0x" << f::X<4>(addr) << '\n';

	if (!g_TapMcu.WriteMem(addr, parser.GetRawBuffer(), buflen))
		return GdbData::ErrorJtag(__FUNCTION__);
	return GdbData::OK();
}


bool Gdb::SetPc(Parser &parser)
{
	parser.SkipSpaces();
	if (!parser.HasMore())
		return true;	// OK, argument is optional

	uint32_t v = parser.GetUint32(16);
	return g_TapMcu.SetReg(0, v);
}


int Gdb::RunFinalStatus()
{
	address_t regs[DEVICE_NUM_REGS];
	int i;

	if(! g_TapMcu.IsAttached())
		return GdbData::NotAttached(__FUNCTION__);

	if (!g_TapMcu.GetRegs(regs))
		return GdbData::ErrorJtag(__FUNCTION__, "Error reading registers!");

	GdbData response;
	response << "T05";
	for (i = 0; i < OPT_SHORT_QUERY_REPLY; i++)
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

	if (!g_TapMcu.IsAttached())
		return GdbData::ProcExited(__FUNCTION__);

	if (! SetPc(parser)
		|| g_TapMcu.SingleStep() < 0)
		return GdbData::ErrorJtag(__FUNCTION__);

	return RunFinalStatus();
}


int Gdb::Run(Parser &parser)
{
	Trace() << "Running\n";

	if (!g_TapMcu.IsAttached())
		return GdbData::ProcExited(__FUNCTION__);

	if (! SetPc(parser)
		|| g_TapMcu.Run() < 0)
		return GdbData::ErrorJtag(__FUNCTION__);

	for (;;)
	{
		device_status_t status = g_TapMcu.Poll();

		if (status == DEVICE_STATUS_ERROR)
			return GdbData::ErrorJtag(__FUNCTION__);

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
	if (!g_TapMcu.Halt())
		return GdbData::ErrorJtag(__FUNCTION__);

	return RunFinalStatus();
}


int Gdb::SetBreakpoint(Parser &parser)
{
	if (!g_TapMcu.IsAttached())
		return GdbData::ErrorJtag(__FUNCTION__);

	// Restart scanner as z/Z has a mixed syntax
	parser.RestartScanner();
	bool bSet = parser.GetCurChar() == 'Z';
	parser.SkipChar();		// command
	parser.SkipSpaces();	// space is optional
	const char *tok = parser.GetNextArg(",");
	/* Make sure there's a type argument */
	if (tok == NULL)
		return GdbData::MissingArg(__FUNCTION__);
	DeviceBpType type;
	switch (atoi(tok))
	{
	case 0:
	case 1:
		type = DeviceBpType::kBpTypeBreak;
		break;

	case 2:
		type = DeviceBpType::kBpTypeWrite;
		break;

	case 3:
		type = DeviceBpType::kBpTypeRead;
		break;

	case 4:
		type = DeviceBpType::kBpTypeWatch;
		break;

	default:
		return GdbData::InvalidArg(__FUNCTION__, "unsupported breakpoint type");
	}
	// Skip ','
	parser.SkipChar();

	// Parse the address token
	tok = parser.GetNextArg(",");
	// There needs to be an address specified
	if (tok == NULL)
		return GdbData::MissingArg(__FUNCTION__);
	// Parse the breakpoint address
	address_t addr = strtoul(tok, NULL, 16);

	// TODO: handle 3rd parameter

	if (bSet)
	{
		if (g_TapMcu.Set(addr, type, true) == BkptId::kInvalidBkpt)
			return GdbData::ErrorJtag(__FUNCTION__);

		Trace() << "Breakpoint set at 0x" << f::X<4>(addr) << '\n';
	}
	else
	{
		g_TapMcu.Set(addr, type, false);
		Trace() << "Breakpoint cleared at 0x" << f::X<4>(addr) << '\n';
	}
	return GdbData::OK();
}


int Gdb::RestartProgram()
{
	if (g_TapMcu.SoftReset() < 0)
		return GdbData::ErrorJtag(__FUNCTION__);
	return GdbData::OK();
}


#if OPT_MULTIPROCESS
int Gdb::SendThreadList(Parser &parser)
{
	return GdbData::Send("m0");
}

int Gdb::SendThreadListClose(Parser &parser)
{
	return GdbData::Send("l");
}
#endif


int Gdb::SendC(Parser &parser)
{
	GdbData response;
	response << "QC0";
	return response.FlushAck();
}


int Gdb::SendCRC(Parser &parser)
{
	// Skip ':'
	parser.SkipChar();
	// Get address
	address_t addr = parser.GetUint32(16);
	if(parser.GetCurChar() != ',')
		return GdbData::MissingArg(__FUNCTION__);
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
		if (!g_TapMcu.ReadMem(addr, buf, todo))
			return GdbData::ErrorJtag(__FUNCTION__);
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
		return GdbData::InvalidArg(__FUNCTION__, parser.GetCurChar());
	parser.SkipChar();
	wide_regs_ = false;
	// Iterate host features
	char *arg;
	while ((arg = parser.GetNextListArg()) != NULL)
	{
		/* This is a hack to distinguish msp430-elf-gdb
		** from msp430-gdb. The former expects 32-bit
		** register fields.
		*/
		if (strcmp(arg, "swbreak+") == 0)
			wide_regs_ = true;
		parser.SkipChar();
	}

	GdbData response;
	response <<
		"PacketSize=" << f::X<1>(GDB_MAX_XFER * 2) // best if this is the first
		<< ";QStartNoAckMode+"
#if OPT_MEMORY_MAP
		";qXfer:memory-map:read+"
#endif
		;
	return response.FlushAck();
}

void Gdb::OneMemMap(GdbData &response, const MemInfo &mem)
{
	const char *typ = NULL;
	if (mem.type_ == ChipInfoDB::kMtypFlash)
		typ = "flash";
	else if (mem.type_ == ChipInfoDB::kMtypRam)
		typ = "ram";
	else if (mem.type_ == ChipInfoDB::kMtypRom)
		typ = "rom";
	// Found?
	if (typ != NULL)
	{
		// Basic segment information
		response 
			<< " <memory type=\"" << typ << "\" "
			"start=\"0x" << f::X<5>(mem.start_)
			<< "\" length=\"0x" << f::X<5>(mem.size_) << '"'
			;
		// Is segment organized in blocks?
		if (mem.segsize_ > 1)
		{
			response << ">\n"
				"    <property name=\"blocksize\">0x" << f::X<3>(mem.segsize_) << "</property>\n"
				<< "  </memory>\n";
			;
		}
		else
			response << "/>\n";
	}
}


#if OPT_MEMORY_MAP
int Gdb::HandleXfer(Parser &parser)
{
	// Order we want to report segments
	static const ChipInfoDB::EnumMemoryKey memclass[] =
	{ 
		ChipInfoDB::kMkeyMain,
		ChipInfoDB::kMkeyInfo,
		ChipInfoDB::kMkeyRam,
		ChipInfoDB::kMkeyRam2,
	};

	uint32_t offset, length;
	// Skip ':'
	parser.SkipChar();
	// Get 'object'
	const char *cmd = parser.GetNextArg(":");
	// Test for unknown object type
	if(strcmp(cmd, "memory-map") != 0)
		return GdbData::Unsupported();
	// Skip ':'
	parser.SkipChar();
	// Next token should be "read"
	cmd = parser.GetNextArg(":");
	// Malformed command?
	if (strcmp(cmd, "read") != 0)
		goto invalid_syntax;
	// Skip ':'
	parser.SkipChar();
	// Next token should be empty
	cmd = parser.GetNextArg(":");
	if(*cmd != 0)
	{
invalid_syntax:
		return GdbData::InvalidArg(__FUNCTION__, parser.GetArg());
	}
	// Skip ':'
	parser.SkipChar();
	offset = parser.GetUint32();	// ignored
	// Skip ','
	parser.SkipChar();
	length = parser.GetUint32();	// ignored
	GdbData response;
	/*
	** Issue that may happen: We are not honoring the 'length' input argument.
	** This is an issue. Hopefully length will always be greater than out response...
	** Just in case, we respond only to "offset 0".
	*/
	if (offset == 0)
	{
		response <<
			"l"	// data
			"<?xml version=\"1.0\"?>\n"
			"<!DOCTYPE memory-map PUBLIC \" +//IDN gnu.org//DTD GDB Memory Map V1.0//EN\" \"http://sourceware.org/gdb/gdb-memory-map.dtd\">\n"
			"<memory-map>\n"
			;
	}
	else
	{
		// Just in case: No more packet to come
		response << 'l';	// EOF
	}
	const ChipProfile &prof = g_TapMcu.GetChipProfile();
	for (int j = 0; j < _countof(memclass); ++j)
	{
		ChipInfoDB::EnumMemoryKey what = memclass[j];
		for (int i = 0; i < _countof(prof.mem_); ++i)
		{
			const MemInfo &mem = prof.mem_[i];
			if (mem.valid_ == false)
				break;
			if (mem.class_ == what)
				OneMemMap(response, mem);
		}
	}
	response <<
		"</memory-map>\n"
		;
	return response.FlushAck();
}
#endif	// OPT_MEMORY_MAP


class CToggleLed
{
public:
	CToggleLed() NO_INLINE
	{
		if (g_TapMcu.IsAttached())
			GreenLedOff();
		else
			RedLedOn();
	}
	~CToggleLed() NO_INLINE
	{
		GreenLedOn();
		if (g_TapMcu.IsAttached())
			RedLedOn();
		else
			RedLedOff();
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
	const char *cmd = parser.GetNextArg(":,;0123456789");
	for (size_t i = 0; i < ntab; ++i)
	{
		const GdbOneCmd &entry = table[i];
		if (strcmp(cmd, entry.text) == 0)
		{
			if (entry.fn)
				return (this->*entry.fn)(parser);
			else
				return GdbData::Unsupported();
		}
	}
	/* For unknown/unsupported packets, return an empty reply */
	return GdbData::Unsupported(__FUNCTION__, parser.GetRawBuffer());
}


int Gdb::ProcessCommand(char *buf_p)
{
	static GdbOneCmd qCmds[] =
	{
		{"Attached", NULL},
		{"C", &Gdb::SendC},
		{"CRC", &Gdb::SendCRC},
		{"L", NULL},
		{"Offsets", NULL},
		{"Rcmd", &Gdb::MonitorCommand},
		{"Supported", &Gdb::SendSupported},
		{"Symbol", NULL},
		{"TStatus", NULL},
		{"ThreadExtraInfo", NULL},
#if OPT_MEMORY_MAP
		{"Xfer", &Gdb::HandleXfer},
#endif
#if OPT_MULTIPROCESS
		{"fThreadInfo", &Gdb::SendThreadList},
		{"sThreadInfo", &Gdb::SendThreadListClose},
#else
		{"fThreadInfo", NULL},
		{"sThreadInfo", NULL},
#endif
	};
	static GdbOneCmd qqCmds[] =
	{ 
		{"StartNoAckMode", &Gdb::StartNoAckMode},
	};
	static GdbOneCmd vCmds[] =
	{
		{"Kill", NULL},
		{"MustReplyEmpty", NULL},
	};

	CToggleLed led_ctrl;

	Parser parser(buf_p);

	Debug() << "ProcessCommand: " << f::M<68>(buf_p) << '\n';
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
		return GdbData::OK();

	case 'c': // Continue
		return Run(parser);

#if BMP_FEATURES
	case 0x04:
#endif

	case 'D':	/* GDB 'detach' command. */
		GdbData::OK();
		return -1;

	case 'g': // Read registers
		return ReadRegisters(parser);

	case 'G': // Write registers
		return WriteRegisters(parser);

	case 'H': // Set thread for subsequent operations
		return GdbData::OK();

	case 'k': // kill target
		GdbData::Unsupported();
		return -1;

	case 'm': // Read memory
		return ReadMemory(parser);

	case 'M': // Write memory
		return WriteMemory(parser, false);

	case 'p': // Read single register
		return ReadRegister(parser);

	case 'P': // Write single register
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
		
	case 'Q':
		return ProcessCmdTable(parser, qqCmds, _countof(qqCmds));

	case 'r': // Restart
	case 'R':
		return RestartProgram();

	case 's': /* Single step */
		return SingleStep(parser);

	case 'v':	// General query packet
		return ProcessCmdTable(parser, vCmds, _countof(qCmds));

	case 'X': // 'X addr,len:XX': Write binary data to addr
		return WriteMemory(parser, true);

	case 'z':
	case 'Z':
		return SetBreakpoint(parser);
	}

	/* For unknown/unsupported packets, return an empty reply */
	return GdbData::Unsupported(__FUNCTION__, buf_p);
}


void Gdb::ReaderLoop()
{
	while (true)
	{
		int len = 0;

		// Read packet sharing buffer with output
		len = gdb_read_packet(GdbOutBuffer::GetDataBuffer());
		if (len < 0)
			return;
		if (len && ProcessCommand(GdbOutBuffer::GetDataBuffer()) < 0)
			return;
		Trace() << '\0';	// heart beat to flush BMP 64-byte data buffers
	}
}


int Gdb::Serve()
{
	for (int i = 5; !g_TapMcu.Open(); --i)
	{
		if (i == 0)
			return 0;
		StopWatch().Delay<500>();
	}
	wide_regs_ = true;

	Debug() << "starting GDB reader loop...\n";
	ReaderLoop();
	Debug() << "... reader loop returned\n";
	g_TapMcu.Close();
	return /*data.error_ ? -1 :*/ 0;
}

