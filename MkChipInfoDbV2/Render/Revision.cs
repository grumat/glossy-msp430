using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Revision : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeRevision to indicate unpopulated field");
			fh.WriteLine("static constexpr uint8_t kNoRev = 0xFF;");
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Revision values");
			fh.WriteLine("enum EnumRevision : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Revision
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.Revision == null)
					last = "kRev_None";
				else
					last = String.Format("kRev_{0:x2}", row.Revision);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Revision));
			}
			fh.WriteLine("\tkRev_Last_ = {0}", last);
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
// Decodes the 'revision' field
ALWAYS_INLINE static uint8_t DecodeRevision(EnumRevision v)
{
	// Table map to solve the 'revision' field
	static constexpr uint8_t from_enum_to_revision_val[] =
	{
");
			string sql = @"
				SELECT DISTINCT
					Revision
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (row.Revision == null)
					fh.WriteLine("\t\tkNoRev");
				else
					fh.WriteLine("\t\t, 0x{0:x2}", row.Revision);
			}
			fh.Write(@"	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_revision_val) == EnumRevision::kRev_Last_ + 1, ""enum range does not match table"");

	// Resolves using the table
	return from_enum_to_revision_val[v];
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

