using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Eem : IRender
	{
		static Dictionary<string, string> Db2Cpp = new Dictionary<string, string>()
		{
			{ "EMEX_LOW", "kEmexLow" },
			{ "EMEX_MEDIUM", "kEmexMedium" },
			{ "EMEX_HIGH", "kEmexHigh" },
			{ "EMEX_EXTRA_SMALL_5XX", "kEmexExtraSmall5xx" },
			{ "EMEX_SMALL_5XX", "kEmexSmall5xx" },
			{ "EMEX_LARGE_5XX", "kEmexLarge5xx" },
		};

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			string sql = @"
				SELECT DISTINCT
					EemType
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (!Db2Cpp.ContainsKey(row.EemType))
					throw new InvalidDataException(String.Format("Table EemType contains a value '{0}' that is not expected!", row.EemType));
			}
			foreach (var row in conn.Query(@"
SELECT
	MAX(""Index"") AS Idx
FROM
	EemTimersPacked
				"))
			{
				if(row.Idx > 19)
					throw new InvalidDataException(String.Format("Table EemTimers contains a value '{0}' that will break the final enumeration!"));
			}
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used to indicate all_eem_timers elements are grouped");
			fh.WriteLine("static constexpr uint8_t kET_Group = 0;");
			fh.WriteLine("// Indicates that element in all_eem_timers is the first of a group");
			fh.WriteLine("static constexpr uint8_t kET_First = 1;");
		}

		void DeclareEnumEemType(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Embedded Emulation Module (EEM) type
enum EnumEemType : uint8_t
{
	kEmexLow					// 2 bkpt (SLAA393F)
	, kEmexMedium				// 3 bkpt (SLAA393F)
	, kEmexHigh					// 8 bkpt (SLAA393F)
	, kEmexExtraSmall5XX		// 2 bkpt (SLAU414F)
	, kEmexSmall5XX				// 3 bkpt (SLAU414F)
	, kEmexMedium5XX			// 5 bkpt (SLAU414F) -- not used
	, kEmexLarge5XX				// 8 bkpt (SLAU414F)
	, kEemUpper_ = kEmexLarge5XX
};	// 7 values

");
		}
		void DeclareEnumEemTimers(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Enumeration with valid indexes for EemTimers");
			fh.WriteLine("enum EnumEemTimers : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					TimerPK
				FROM
					EemTimersPacked
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = String.Format("kEmmTimer_{0}", row.TimerPK);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.TimerPK));
			}
			fh.WriteLine("\tkEmmTimer_Last_ = {0}", last);
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}
		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			DeclareEnumEemType(fh, conn);
			DeclareEnumEemTimers(fh, conn);
		}
		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			//fh.WriteLine("typedef uint8_t EtwCodes[32];");
			fh.Write(@"
// A single EEM Timer register setup
struct EmmTimer
{
	// Index of time register
	uint8_t index_ : 6;
	// DefaultStop flag
	uint8_t default_stop_ : 1;
	// Marks the start of a register group
	uint8_t group_start_ : 1;
	// Register value
	uint8_t value_;
};

// A structure with the current timer settings
struct ALIGNED EtwCodes
{
	// Control mask for ETCLKSEL values
	uint32_t clk_ctrl_;
	// Individual ETKEYSEL register values
	uint8_t etw_codes_[32];
};

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			string sql = @"
				SELECT DISTINCT
					TimerPK,
					IIF(""Index"" > 15, 47-""Index"", 15-""Index"") AS Idx,
					Value,
					DefaultStop,
					""Index"",
					Name
				FROM
					EemTimersPacked
				ORDER BY 1, 2
			";
			uint cnt = 0;
			long old_pk = -1;
			fh.WriteLine("// All possible EemTimer records, ordered and delimited");
			fh.WriteLine("static constexpr const EmmTimer all_eem_timers[] =");
			fh.WriteLine("{");
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (old_pk != row.TimerPK)
				{
					fh.WriteLine("\t// kEmmTimer_{0}", row.TimerPK);
					fh.WriteLine("\t{{ {0,2}, {1}, kET_First, 0x{2:X2} }},\t// {3,2} - {4}"
						, row.Idx
						, row.DefaultStop
						, row.Value
						, row.@Index
						, row.Name
						);
				}
				else
				{
					fh.WriteLine("\t{{ {0,2}, {1}, kET_Group, 0x{2:X2} }},\t// {3,2} - {4}"
						, row.Idx
						, row.DefaultStop
						, row.Value
						, row.@Index
						, row.Name
						);
				}
				old_pk = row.TimerPK;
			}
			fh.WriteLine("\t// Guard for DecodeEemTimer() operate on last record");
			fh.WriteLine("\t{ 0, 0, kET_First, 0 }");
			fh.WriteLine("};");
			fh.WriteLine();
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
ALWAYS_INLINE static void DecodeEemTimer(EtwCodes &ret, EnumEemTimers cfg)
{
	// Initialize result structure
	memset(&ret, 0, sizeof(EtwCodes));
	// This algorithm is sensitive and will crash if this is not valid
	if (cfg > kEmmTimer_Last_)
		return;
	const EmmTimer *p = all_eem_timers;
	// Scan up to the start of the desired group
	for(uint8_t cur = 0; cur < cfg; ++p)
	{
		if (p->group_start_)
			++cur;
	}
	// Now apply settings
	do
	{
		ret.etw_codes_[p->index_] = p->value_;
		// Set bit for default stop
		if (p->group_start_)
			ret.clk_ctrl_ |= 1 << p->index_;
		// Move to next record
		++p;
	}
	while (p->group_start_ == 0);	// stop at the start of the next record
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

