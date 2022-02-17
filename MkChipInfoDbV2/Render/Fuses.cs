using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Fuses : IRender
	{
		public static string Map(long? k)
		{
			return k == null ? "kFuse_None" : String.Format("kFuse_{0:x2}", k);
		}
		public static string Mask(long? k)
		{
			return k == null ? "kFMask_1f" : String.Format("kFMask_{0:x2}", k);
		}

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			// Validation of control logic: When Fuses is NULL, FusesMask is also NULL!!!
			string sql = @"
				SELECT DISTINCT
					Fuses, FusesMask
				FROM
					Chips
				WHERE 
					Fuses IS NULL 
					OR FusesMask IS NULL
				ORDER BY 1
			";
			int cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				// If this fails indicates that Fuses and FusesMask did not match NULL state
				// *** Firmware development relies on this assumption ***
				if (row.Fuses != null || row.FusesMask != null)
					throw new InvalidDataException("Control logic of Fuses presumes that Nullability also matches the mask attribute.");
			}
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Used in DecodeFuse to indicate unpopulated field");
			fh.WriteLine("static constexpr uint8_t kNoFuse = 0xFF;");
		}

		void DeclareFuseEnum(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine("// Fuses values");
			fh.WriteLine("enum EnumFuses : uint32_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Fuses
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			uint num = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				if (row.Fuses == null)
					last = "kFuse_None = 0x1f";
				else
				{
					// Validate sequence, as DecodeFuse() relies on this
					if (num != row.Fuses)
						throw new InvalidDataException("Fuses value does not represents a sequence as originally expected");
					last = String.Format("kFuse_{0:x2}", row.Fuses);
					if (num == 0)
						last += " = 0";
					++num;
				}
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.Fuses));
			}
			fh.WriteLine("\tkFuse_Last_ = {0}", last);
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}
		void DeclareFuseMaskEnum(TextWriter fh, SqliteConnection conn)
		{
			// No need to track the FusesMask NULL values, as this directly matches Fuses
			// NULL values. This also simplifies decode logic...
			fh.WriteLine("// FusesMask values");
			fh.WriteLine("enum EnumFusesMask : uint32_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					FusesMask
				FROM
					Chips
				WHERE
					FusesMask IS NOT NULL
				ORDER BY 1
			";
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				++cnt;
				last = String.Format("kFMask_{0:x2}", row.FusesMask);
				fh.WriteLine(Utils.BeatifyEnum("\t{0},\t// {1}", last, row.FusesMask));
			}
			fh.WriteLine("\tkFMask_Last_ = {0}", last);
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}
		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			DeclareFuseEnum(fh, conn);
			DeclareFuseMaskEnum(fh, conn);
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
		}

		void DefineFuseFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Decodes the 'fuse' field
ALWAYS_INLINE static uint8_t DecodeFuse(EnumFuses v)
{
	return v == kFuse_None ? kNoFuse : (uint8_t)v;
}
");
		}
		void DefineFuseMaskFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Decodes the 'fuse mask' field
ALWAYS_INLINE static uint8_t DecodeFuseMask(EnumFusesMask v)
{
	// Table map to solve the 'fuse mask' field
	static constexpr uint8_t from_enum_to_fuse_mask_val[] =
	{
");
			string sql = @"
				SELECT DISTINCT
					FusesMask
				FROM
					Chips
				WHERE
					FusesMask IS NOT NULL
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine("\t\t0x{0:x2},", row.FusesMask);
			}
			fh.Write(@"	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_fuse_mask_val) == EnumFusesMask::kFMask_Last_ + 1, ""enum range does not match table"");

	// Resolves using the table
	return from_enum_to_fuse_mask_val[v];
}
");
		}
		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			DefineFuseFunclets(fh, conn);
			DefineFuseMaskFunclets(fh, conn);
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

