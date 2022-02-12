using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class ActivationKey : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeActivationKey to indicate unpopulated field");
			fh.WriteLine("static constexpr uint32_t kNoActivationKey = 0xFFFFFFFF;");
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// ActivationKey values (probably useless type definition)");
			fh.WriteLine("enum EnumActivationKey : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					ActivationKey
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.ActivationKey == null)
					last = "kAct_None";
				else
					last = String.Format("kAct_{0:X8}", row.ActivationKey);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.ActivationKey));
			}
			fh.WriteLine("\tkAct_Last_ = {0}", last);
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
// Decodes the 'ActivationKey' field
ALWAYS_INLINE static uint32_t DecodeActivationKey(EnumActivationKey v)
{
	// Table map to solve the 'revision' field
	static constexpr uint32_t from_enum_to_val[] =
	{
");
			string sql = @"
				SELECT DISTINCT
					ActivationKey
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (row.ActivationKey == null)
					fh.WriteLine("\t\tkNoActivationKey");
				else
					fh.WriteLine("\t\t, 0x{0:X8}", row.ActivationKey);
			}
			fh.Write(@"	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_val) == EnumActivationKey::kAct_Last_ + 1, ""enum range does not match table"");

	// Resolves using the table
	return from_enum_to_val[v];
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

