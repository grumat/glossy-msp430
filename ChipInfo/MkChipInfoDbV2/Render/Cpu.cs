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
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Architecture;
				fh.WriteLine(Utils.BeatifyEnum("\tk{0},\t// {0}", last));
			}
			fh.WriteLine("\tkCpuLast_ = k{0}", last);
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// List of 'version' values for McuX family");
			fh.WriteLine("static constexpr uint16_t McuXs[] = {");
			string sql = @"
				SELECT DISTINCT
					Version
				FROM
					Chips
				WHERE 
					Architecture == 'CpuX'
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine("\t0x{0:x4},", row.Version);
			}
			fh.WriteLine("};");
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

