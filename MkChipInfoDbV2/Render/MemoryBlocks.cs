using Dapper;
using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class MemoryBlocks : IRender
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

			// Type of memory
			fh.WriteLine("// Type of memory");
			fh.WriteLine("enum EnumMemoryType : uint8_t");
			fh.WriteLine("{");
			int cnt = 0;
			string sql = "SELECT * FROM EnumMemoryTypes";
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
			fh.WriteLine("enum EnumMemAccessType : uint8_t");
			fh.WriteLine("{");
			cnt = 0;
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

			// Type of memory access
			fh.WriteLine("// Memory write protection (FRAM parts)");
			fh.WriteLine("// IMPORTANT: Zeroes shall be handled as 'None'");
			fh.WriteLine("enum EnumWriteProtection : uint8_t");
			fh.WriteLine("{");
			cnt = 0;
			sql = "SELECT * FROM EnumWriteProtection";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Wp;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// Addr={1,-3} Bits={2,-4} Msk={3,-3} Pwd={4}"
					, last
					, row.WpAddress.ToString()
					, row.WpBits.ToString()
					, row.WpMask.ToString()
					, row.WpPwd.ToString()
					));
			}
			fh.WriteLine("\tkWpLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Memory Write protection decode table
struct ALIGNED MemWrProt
{
	// Address
	uint16_t wp_addr_;
	// Bits
	uint16_t wp_bits_;
	// Mask
	uint16_t wp_mask_;
	// Password
	uint16_t wp_pwd_;
};

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Table to decode EnumWriteProtection enum");
			fh.WriteLine("static constexpr const MemWrProt mem_wr_prot[] =");
			fh.WriteLine("{");
			string sql = "SELECT * FROM EnumWriteProtection";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine("\t{{ 0x{0:X4}, 0x{1:X4}, 0x{2:X4}, 0x{3:X4}, }},\t// {4}"
					, row.WpAddress
					, row.WpBits
					, row.WpMask
					, row.WpPwd
					, row.Wp
					);
			}
			fh.WriteLine("};");
			fh.WriteLine();
			fh.WriteLine("static_assert(_countof(mem_wr_prot) == kWpLast_+1, \"EnumWriteProtection and mem_wr_prot[] sizes shall match!\");");
			fh.WriteLine();
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Access to the Memory write protection record
ALWAYS_INLINE static const MemWrProt &GetMemWrProt(EnumWriteProtection i)
{
	return mem_wr_prot[i];
}

");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}

	}
}

