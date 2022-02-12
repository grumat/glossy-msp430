using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Subversion : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeSubversion to indicate unpopulated field");
			fh.WriteLine("static constexpr uint16_t kNoSubver = 0xFFFF;");
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Sub-version values");
			fh.WriteLine("enum EnumSubversion : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Subversion
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.Subversion == null)
					last = "kSubver_None";
				else
					last = String.Format("kSubver_{0:x4}", row.Subversion);
				fh.WriteLine(Utils.BeatifyEnum("\t{0} = {1},\t// {2}", last, row.Subversion ?? 3, row.Subversion));
			}
			fh.WriteLine("\tkSubver_Last_ = {0}", last);
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
// Decodes the 'sub-version' field
ALWAYS_INLINE static uint16_t DecodeSubversion(EnumSubversion v)
{
	return v == kSubver_None ? kNoSubver : (uint16_t)v;
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

