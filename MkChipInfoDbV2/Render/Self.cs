using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Self : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeSelf to indicate unpopulated field");
			fh.WriteLine("static constexpr uint16_t kNoSelf = 0xFFFF;");
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Self values");
			fh.WriteLine("enum EnumSelf : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Self
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.Self == null)
					last = "kSelf_None";
				else
					last = String.Format("kSelf_{0:x4}", row.Self);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Self));
			}
			fh.WriteLine("\tkSelf_Last_ = {0}", last);
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
// Decodes the 'self' field
ALWAYS_INLINE static uint8_t DecodeSelf(EnumSelf v)
{
	return v == kSelf_None ? kNoSelf : 0x0000;
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

