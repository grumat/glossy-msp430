#pragma once

#include "util/output_util.h"

//! Monitor ('qRcmd') command database: the table of commands plus lookup and
//! enumeration. Originally MSPDebug's C `cmddb`, modernized to a thin class.
class CmdDb
{
public:
	//! Command handler. Receives the remaining (unparsed) argument string.
	using Func = int (*)(char **arg);

	//! Command availability by target connection state (bitmask). The monitor
	//! dispatcher (MonitorCmd::Dispatch) refuses a command whose 'states' field does
	//! not include the current state, so e.g. device operations are rejected
	//! with a hint until a target is connected (jtag_scan/sbw_scan), and
	//! pre-connect helpers are rejected once attached.
	enum State : uint8_t
	{
		kPre  = 0x01,			//!< valid before a target is connected
		kPost = 0x02,			//!< valid after a target is connected
		kAny  = kPre | kPost,	//!< valid in any state
	};

	//! A single command-table entry.
	struct Record
	{
		const char	*name;
		Func		func;
		const char	*help;
		const char	*brief = nullptr;	//!< one-line summary for the 'help' listing
		uint8_t		states = kAny;		//!< default: available in any state
	};

	//! Enumeration callback. Returns 0 to continue, < 0 to abort.
	using EnumFunc = int (*)(void *user_data, const Record *r);

	//! Fetch a command record by exact, then unambiguous-prefix, match.
	//! Returns 0 on success (filling \p out), -1 if not found or ambiguous.
	static int Get(const char *name, Record *out);

	//! Enumerate every command record. Returns 0, or -1 if \p func aborts.
	static int Enum(EnumFunc func, void *user_data);

private:
	static const Record kCommands_[];
};
