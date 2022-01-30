using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class MemoryLayout : IRender
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

			// Segment Size of a memory block
			fh.WriteLine("// Enumeration that specifies the segment size of a memory block");
			fh.WriteLine("enum EnumSegmentSize : uint32_t");
			fh.WriteLine("{");
			int cnt = 0;
			string sql = "SELECT * FROM EnumSegSize";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Value;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Integral.ToString()));
			}
			fh.WriteLine("\tkSegLast_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();

			// Start of a memory block
			fh.WriteLine("// Enumeration that specifies the address start of a memory block");
			fh.WriteLine("enum EnumMemLayout : uint8_t");
			fh.WriteLine("{");
			cnt = 0;
			sql = "SELECT * FROM EnumAddressSize";
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = row.Value;
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// Addr={1,-7} Siz={2,-6} Seg={3,-4} Bnks={4}"
					, last
					, row.Start.ToString()
					, row.Size.ToString()
					, row.SegmentSize.ToString()
					, row.Banks.ToString()
					));
			}
			fh.WriteLine("\tkBlk_Last_ = {0}", last);
			fh.WriteLine("}};\t// {0} values", cnt);
			fh.WriteLine();
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// A compact form to store layout data for a memory block (Start address, size, segments and banks)
struct ALIGNED MemoryLayout
{
	// Start address: mantissa
	uint32_t start_ : 12;
	// Start address exponent: 2^0, 2^4, 2^8 or 2^12
	uint32_t start_shl_ : 2;
	// Block size: mantissa
	uint32_t size_ : 9;
	// Block size exponent: 2^0, 2^4, 2^8 or 2^12
	uint32_t size_shl_ : 2;
	// Segment size : see EnumSegmentSize enumeration
	EnumSegmentSize seg_size_ : 3;
	// Number of banks minus 1 (add 1 when retrieving)
	uint32_t banks_ : 2;
};

");
		}

		void MkSegSizes(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Table to decode EnumSegmentSize enum");
			fh.WriteLine("static constexpr const uint16_t seg_sizes[] =");
			fh.WriteLine("{");
			string sql = "SELECT * FROM EnumSegSize";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}"
					, row.Integral.ToString()
					, row.Value
					));
			}
			fh.WriteLine("};");
			fh.WriteLine();
			fh.WriteLine("static_assert(_countof(seg_sizes) == kSegLast_+1, \"EnumSegmentSize and seg_sizes[] sizes shall match!\");");
			fh.WriteLine();
		}
		void MkMemBlocks(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// All possible Address/Size/Segment/Banks records, matching EnumMemLayout enum");
			fh.WriteLine("static constexpr const MemoryLayout mem_layouts[] =");
			fh.WriteLine("{");
			string sql = @"
				SELECT 
					q1.Value, 
					q1.Start, 
					q1.Size, 
					printf('kSeg_0x%x', q1.SegmentSize) AS SegmentSize,
					q1.Banks - 1 AS Banks, 
					q2.a_seg0,
					q2.a_shift0,
					q2.a_seg4,
					q2.a_shift4,
					q2.a_seg8,
					q2.a_shift8,
					q2.a_seg12,
					q2.a_shift12,
					q3.s_seg0,
					q3.s_shift0,
					q3.s_seg4,
					q3.s_shift4,
					q3.s_seg8,
					q3.s_shift8,
					q3.s_seg12,
					q3.s_shift12
				FROM
					EnumAddressSize AS q1, 
					StartShift AS q2,
					SizeShift AS q3
				WHERE
					q1.Start == q2.Start 
					AND q1.Size == q3.Size
";
			int cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				UInt32 aval = 0;
				UInt32 als = 0;
				if (row.a_seg0 != 0)
				{
					aval = Convert.ToUInt32(row.a_shift0, 16);
					als = 0;
				}
				else if (row.a_seg8 != 0)
				{
					aval = Convert.ToUInt32(row.a_shift8, 16);
					als = 2;
				}
				else if (row.a_seg4 != 0)
				{
					aval = Convert.ToUInt32(row.a_shift4, 16);
					als = 1;
				}
				else if (row.a_seg12 != 0)
				{
					aval = Convert.ToUInt32(row.a_shift12, 16);
					als = 3;
				}
				else
					throw new InvalidDataException("Memory address bit-packing is not possible");
				UInt32 sval = 0;
				UInt32 sls = 0;
				if (row.s_seg0 != 0)
				{
					sval = Convert.ToUInt32(row.s_shift0, 16);
					sls = 0;
				}
				else if (row.s_seg8 != 0)
				{
					sval = Convert.ToUInt32(row.s_shift8, 16);
					sls = 2;
				}
				else if (row.s_seg4 != 0)
				{
					sval = Convert.ToUInt32(row.s_shift4, 16);
					sls = 1;
				}
				else if (row.s_seg12 != 0)
				{
					sval = Convert.ToUInt32(row.s_shift12, 16);
					sls = 3;
				}
				else
					throw new InvalidDataException("Memory size bit-packing is not possible");
				fh.WriteLine("\t{{ 0x{0:X3}, {1}, 0x{2:X3}, {3}, {4}, {5} }},	// {6}"
					, aval, als
					, sval, sls
					, row.SegmentSize
					, row.Banks
					, row.Value
					);
			}
			fh.WriteLine("}};	// {0} values", cnt);
			fh.WriteLine();
			fh.WriteLine("static_assert(_countof(mem_layouts) == kBlk_Last_+1, \"EnumMemLayout and mem_layouts[] sizes shall match!\");");
			fh.WriteLine();
		}
		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("static_assert(sizeof(MemoryLayout) == 4, \"Total used memory space has changed and may impact Flash capacity!\");");
			fh.WriteLine();

			MkSegSizes(fh, conn);
			MkMemBlocks(fh, conn);
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Decodes a memory block entry into scalar values.
ALWAYS_INLINE static void DecodeMemBlock(EnumMemLayout idx	// in: enum of the memory block
										, uint32_t &addr	// out: the start address of the block
										, uint32_t &size	// out: the size of the memory block
										, uint16_t &segsz	// out: size of a segment
										, uint8_t &banks)	// out: number of memory banks
{
	const MemoryLayout &blk = mem_layouts[idx];
	addr = (blk.start_ << (4 * blk.start_shl_));
	size = (blk.size_ << (4 * blk.size_shl_));
	segsz = seg_sizes[blk.seg_size_];
	banks = blk.banks_ + 1;
}

");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}


