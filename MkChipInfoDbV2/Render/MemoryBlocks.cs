using Dapper;
using Microsoft.Data.Sqlite;
using System;
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
			fh.WriteLine("// Memory class");
			fh.WriteLine("enum EnumMemoryKey : uint8_t");
			fh.WriteLine("{");
			int cnt = 0;
			string sql = "SELECT * FROM EnumMemoryKeys";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Name;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}"
					, last
					, row.MemoryName
					));
			}
			fh.WriteLine("\tkMkeyLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Type of memory
			fh.WriteLine("// Type of memory");
			fh.WriteLine("enum EnumMemoryType : uint8_t");
			fh.WriteLine("{");
			cnt = 0;
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
			fh.WriteLine("enum EnumMemAccessType : uint8_t");
			fh.WriteLine("{");
			cnt = 1;
			fh.WriteLine("\tkAccNone,");
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

			// All Memory Blocks
			fh.WriteLine("// Index for Table containing all memory blocks");
			fh.WriteLine("enum EnumMemoryBlock : uint8_t");
			fh.WriteLine("{");
			cnt = 0;
			sql = @"
				SELECT * 
				FROM MemoryBlocksTables
				ORDER BY BlockId
				";
			string last_title = "";
			foreach (var row in conn.Query(sql))
			{
				string title = String.Format("{0}.{1}.{2}"
					, row.MemoryKey.Substring(5)
					, row.MemoryType.Substring(5)
					, row.AccessType.Substring(4)
					);
				if (title != last_title)
				{
					last_title = title;
					fh.WriteLine("\t// Name={0}", row.MemoryKey.Substring(5));
					fh.WriteLine("\t// Type={0} ({1})", row.MemoryType.Substring(5), row.AccessType.Substring(4));
				}
				last = row.BlockId;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}: {2} {3}"
					, row.BlockId
					, cnt.ToString()
					, row.MemLayout
					, row.MemWrProt
					));
				++cnt;
			}
			fh.WriteLine("\tkBlkLast_ = {0}", last);
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

// Memory Block structure
struct MemoryBlock
{
	// Key for the memory (zero or one per device part)
	EnumMemoryKey memory_key_ : 5;
	// Write protection control (FRAM devices)
	EnumWriteProtection wr_prot_ : 3;
	// Special cases for accessing the memory
	EnumMemAccessType access_type_ : 4;
	// Type of memory: Flash, RAM, ROM...
	EnumMemoryType memory_type_ : 2;
	// Not sure if firmware can use this...
	uint8_t protectable_ : 1;
	// Memory layout (start address, size, segment size and banks)
	EnumMemLayout mem_layout_;
};

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("static_assert(sizeof(MemWrProt) == 8, \"Total used memory space has changed and may impact Flash capacity!\");");
			fh.WriteLine("static_assert(sizeof(MemoryBlock) == 3, \"Total used memory space has changed and may impact Flash capacity!\");");
			fh.WriteLine();

			fh.WriteLine("// Table to decode EnumWriteProtection enum");
			fh.WriteLine("static constexpr const MemWrProt mem_wr_prot[] =");
			fh.WriteLine("{");
			string sql = "SELECT * FROM EnumWriteProtection";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine("\t{{ 0x{0:X4}, 0x{1:X4}, 0x{2:X4}, 0x{3:X4} }},\t// {4}"
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

			int cnt = 0;
			fh.WriteLine("// Table containing the diversity of possible memory blocks");
			fh.WriteLine("static constexpr const MemoryBlock all_mem_blocks[] =");
			fh.WriteLine("{");
			sql = @"
				SELECT * 
				FROM MemoryBlocksTables
				ORDER BY BlockId
				";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine("\t// {0}: {1}", cnt++, row.BlockId);
				fh.WriteLine("\t{{ {0}, {1}, {2}, {3}, {4}, {5}, }},"
					, row.MemoryKey
					, row.MemWrProt
					, row.AccessType
					, row.MemoryType
					, row.Protectable
					, row.MemLayout
					);
			}
			fh.WriteLine("};");
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

// Access to the Memory write protection record
ALWAYS_INLINE static const MemoryBlock &GetMemoryBlock(EnumMemoryBlock i)
{
	return all_mem_blocks[i];
}

");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}

	}
}

