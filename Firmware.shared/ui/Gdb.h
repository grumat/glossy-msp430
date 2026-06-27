#pragma once


//! Support to multiprocess protocol
#define OPT_MULTIPROCESS	1
//! Number of registers to reply on '?' command
#define OPT_SHORT_QUERY_REPLY	2

class Parser;
class GdbData;
struct MemInfo;


class Gdb
{
public:
	Gdb();

	int Serve();

protected:
	//! Pointer to a packet-handler method.
	using CmdHandler = int (Gdb::*)(Parser &);
	//! One row of a command-dispatch table; a nullptr handler replies "unsupported".
	struct OneCmd
	{
		const char	*text;
		CmdHandler	fn;
	};

protected:
	int ProcessCommand(char *buf, int len);
	void ReaderLoop();

protected:
	int ReadRegister(Parser &parser);
	int ReadRegisters(Parser &parser);
	int MonitorCommand(Parser &parser);
	int WriteRegister(Parser &parser);
	int WriteRegisters(Parser &parser);
	int ReadMemory(Parser &parser);
	int WriteMemory(Parser &parser, bool bin_mode);
	bool SetPc(Parser &parser);
	int SingleStep(Parser &parser);
	int Run(Parser &parser);
	int RunFinalStatus();
	int SendSupported(Parser &parser);
#if OPT_MEMORY_MAP
	int HandleXfer(Parser &parser) DEBUGGABLE;
#endif
#if OPT_MULTIPROCESS
	int SendThreadList(Parser &parser);
	int SendThreadListClose(Parser &parser);
#endif
	int RestartProgram();
	int SetBreakpoint(Parser &parser);
	int SendC(Parser &parser);
	int SendCRC(Parser &parser);
	int StartNoAckMode(Parser &parser);

protected:
	void AppendRegisterContents(GdbData &response, uint32_t r);
	template <size_t N>
	int ProcessCmdTable(Parser &parser, const OneCmd (&table)[N]);
	void OneMemMap(GdbData &response, const MemInfo &mem);

protected:
	bool wide_regs_;
};

