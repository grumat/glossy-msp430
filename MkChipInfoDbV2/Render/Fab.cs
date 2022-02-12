using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Fab : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeFab to indicate unpopulated field");
			fh.WriteLine("static constexpr uint8_t kNoFab = 0xFF;");
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Fab values");
			fh.WriteLine("enum EnumFab : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Fab
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.Fab == null)
					last = "kFab_None";
				else
					last = String.Format("kFab_{0:x2}", row.Fab);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Fab));
			}
			fh.WriteLine("\tkFab_Last_ = {0}", last);
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
// Decodes the 'Fab' field
ALWAYS_INLINE static uint8_t DecodeFab(EnumFab v)
{
	return v == kFab_None ? kNoFab : 0x40;
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

