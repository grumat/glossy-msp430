using Dapper;
using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Bits : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Size of CPU Bus");
			fh.WriteLine("enum EnumBitSize : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Bits
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Bits.ToString();
				fh.WriteLine(Utils.BeatifyEnum("\tk{0},\t// {1}", last, row.Bits));
			}
			fh.WriteLine("\tkBitsLast_ = k{0}", last);
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
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

