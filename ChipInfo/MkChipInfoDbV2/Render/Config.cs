using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Config : IRender
	{
		public static string Map(long? k)
		{
			return k == null ? "kCfg_None" : String.Format("kCfg_{0:x2}", k);
		}

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeConfig to indicate unpopulated field");
			fh.WriteLine("static constexpr uint8_t kNoConfig = 0xFF;");
			fh.WriteLine("// Used in DecodeConfigMask to filter a populated field");
			fh.WriteLine("static constexpr uint8_t kConfigMask = 0x7F;");
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Config values");
			fh.WriteLine("enum EnumConfig : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Config
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.Config == null)
					last = "kCfg_None";
				else
					last = String.Format("kCfg_{0:x2}", row.Config);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Config));
			}
			fh.WriteLine("\tkCfg_Last_ = {0}", last);
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Decodes the 'config' field
ALWAYS_INLINE static uint8_t DecodeConfig(EnumConfig v)
{
	// Table map to solve the 'config' field
	static constexpr uint8_t from_enum_to_config_val[] =
	{
");
			string sql = @"
				SELECT DISTINCT
					Config
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (row.Config == null)
					fh.WriteLine("\t\tkNoConfig");
				else
					fh.WriteLine("\t\t, 0x{0:x2}", row.Config);
			}
			fh.Write(@"	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_config_val) == EnumConfig::kCfg_Last_ + 1, ""enum range does not match table"");

	// Resolves using the table
	return from_enum_to_config_val[v];
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

