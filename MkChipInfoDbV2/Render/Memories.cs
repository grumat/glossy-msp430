using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Memories : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used for memory configurations that inherits a base configuration");
			fh.WriteLine("static constexpr uint8_t kHasBaseConfig = 0x80;");
			fh.WriteLine("// Used for memory configurations as End of Records mark");
			fh.WriteLine("static constexpr uint8_t kEndOfConfigs = 0x00;");
		}

		// Collect configuration names
		List<string> MemGroups = new List<string>();
		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Memory configurations");
			fh.WriteLine("enum EnumMemoryConfigs : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT MemGroup AS val 
				FROM Memories2
				WHERE MemGroup != '_AllParts_'
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.val;
				MemGroups.Add(last);
				fh.WriteLine("\tkMCfg_{0},", last);
			}
			fh.WriteLine("\tkMCfg_Last_ = kMCfg_{0}", last);
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Memory configuration header
struct MemConfigHdr
{
	// Size of the configuration record in bytes
	uint8_t count_ : 7;
	// Always zero for this data type
	uint8_t has_base_ : 1;
	// Data of variable size
	EnumMemoryBlock mem_blocks[];
};

// Memory configuration header having a base configuration
struct MemConfigHdrEx
{
	// Size of the configuration record in bytes
	uint8_t count_ : 7;
	// Always one for this data type
	uint8_t has_base_ : 1;
	// Base configuration
	EnumMemoryConfigs base_cfg_;
	// Data of variable size
	EnumMemoryBlock mem_blocks[];
};

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			int cnt = 0;
			fh.WriteLine("// Memory configurations records having variable size");
			fh.WriteLine("static constexpr const uint8_t mem_config_blob[] =");
			fh.WriteLine("{");
			foreach (string key in MemGroups)
			{
				string sql = @"
					SELECT DISTINCT
						BlockId,
						RefTo
					FROM Memories2
					WHERE MemGroup = :key
					ORDER BY 1, 2
					";
				string @ref = null;
				List<string> blks = new List<string>();
				foreach (var row in conn.Query(sql, new { key = key }))
				{
					blks.Add(row.BlockId);
					if (@ref == null)
						@ref = row.RefTo;
					else if (@ref != row.RefTo)
						throw new InvalidDataException(String.Format("Table Memories2 does not support multiple inheritance. Element={0}", key));
				}
				fh.WriteLine("\t// kMCfg_{0}", key);
				// Size of record (with optional inheritance)
				if (@ref == null)
				{
					cnt += blks.Count + 1;
					fh.WriteLine("\t{0},", blks.Count + 1);
				}
				else
				{
					cnt += blks.Count + 2;
					fh.WriteLine("\t{0} + kHasBaseConfig, kMCfg_{1},", blks.Count + 2, @ref);
				}
				fh.Write("\t\t");
				foreach (string blk in blks)
				{
					fh.Write("{0}, ", blk);
				}
				fh.WriteLine();
			}
			fh.WriteLine("\t// END OF RECORDS");
			++cnt;
			fh.WriteLine("\tkEndOfConfigs");
			fh.WriteLine("}};\t// Total byte count: {0}", cnt);
			fh.WriteLine();
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
//  Locate a memory configuration by giving it's index
ALWAYS_INLINE static const MemConfigHdr *GetMemConfig(EnumMemoryConfigs idx)
{
	uint32_t c = 0;
	// Iterate the list
	for (uint8_t i = 0; i < idx; ++i)
	{
		// EOF mark can never happen
		assert(mem_config_blob[c] != kEndOfConfigs);
		c += ((const MemConfigHdr &)mem_config_blob[c]).count_;	// Jump to next record
	}
	return (const MemConfigHdr *)&mem_config_blob[c];
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

