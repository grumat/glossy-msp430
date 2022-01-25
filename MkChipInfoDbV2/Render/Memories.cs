using Dapper;
using Microsoft.Data.Sqlite;
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
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			string last = "";
			fh.WriteLine("// Memory class");
			fh.WriteLine("enum MemoryKey : uint8_t");
			fh.WriteLine("{");
			int cnt = 0;
			string sql = "SELECT * FROM EnumMemoryNames";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Name;
				fh.WriteLine("\t{0},", last);
			}
			fh.WriteLine("\tkMkeyLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Type of memory
			fh.WriteLine("// Type of memory");
			fh.WriteLine("enum MemoryType : uint8_t");
			fh.WriteLine("{");
			cnt = 1;
			fh.WriteLine("\tkMtypNone,", last);
			sql = "SELECT * FROM EnumMemoryTypes";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.@Type;
				fh.WriteLine("\t{0},", last);
			}
			fh.WriteLine("\tkMtypLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Type of memory access
			fh.WriteLine("// Type of memory access");
			fh.WriteLine("enum MemAccessType : uint8_t");
			fh.WriteLine("{");
			cnt = 1;
			fh.WriteLine("\tkAccNone,", last);
			sql = "SELECT * FROM EnumMemAccessType";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.@Type;
				fh.WriteLine("\t{0},", last);
			}
			fh.WriteLine("\tkAccLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Start of a memory block
			fh.WriteLine("// Enumeration that specifies the address start of a memory block");
			fh.WriteLine("enum AddressStart : uint16_t");
			fh.WriteLine("{");
			cnt = 1;
			fh.WriteLine("\tkStart_None,", last);
			sql = "SELECT * FROM EnumAddressStart";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Value;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Integral.ToString()));
			}
			fh.WriteLine("\tkStartLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Size of a memory block
			fh.WriteLine("// Enumeration that specifies the size of a memory block");
			fh.WriteLine("enum BlockSize : uint16_t");
			fh.WriteLine("{");
			cnt = 1;
			fh.WriteLine("\tkSize_None,", last);
			sql = "SELECT * FROM EnumBlockSize";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Value;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Integral.ToString()));
			}
			fh.WriteLine("\tkSizeLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Segment Size of a memory block
			fh.WriteLine("// Enumeration that specifies the segment size of a memory block");
			fh.WriteLine("enum SegmentSize : uint16_t");
			fh.WriteLine("{");
			cnt = 1;
			fh.WriteLine("\tkSeg_None,", last);		// TODO: can be discarded?
			sql = "SELECT * FROM EnumSegSize";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Value;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Integral.ToString()));
			}
			fh.WriteLine("\tkSegLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();
		}
		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
		}
		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

