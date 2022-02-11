using Dapper;
using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Cpu : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// CPU Architecture");
			fh.WriteLine("enum EnumCpuType : uint8_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Architecture
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			int cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Architecture;
				fh.WriteLine("\tk{0},", last);
			}
			fh.WriteLine("\tkCpuLast_ = k{0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
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

