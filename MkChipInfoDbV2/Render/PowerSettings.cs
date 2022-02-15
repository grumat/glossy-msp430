using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class PowerSettings : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			HashSet<string> repeat = new HashSet<string>();
			string sql = @"
				SELECT DISTINCT
					UsersGuide, PowerSettings
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (repeat.Contains(row.UsersGuide))
					throw new InvalidDataException("UsersGuide/PowerSettings pair is not consistent and invalidates current logic!");
				repeat.Add(row.UsersGuide);
				if (row.UsersGuide == "SLAU367")
				{
					if (row.PowerSettings == null)
						throw new InvalidDataException("'SLAU367' Users Guide invalidates current algorithm logic!");
				}
				else if (row.UsersGuide == "SLAU445")
				{
					if (row.PowerSettings == null)
						throw new InvalidDataException("'SLAU445' Users Guide invalidates current algorithm logic!");
				}
				else if(row.PowerSettings != null)
					throw new InvalidDataException(String.Format("'{0}' Users Guide invalidates current algorithm logic!", row.UsersGuide));
			}
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Extra PowerSettings records
struct ALIGNED PowerSettings
{
	uint32_t test_reg_mask_;			// 0
	uint32_t test_reg_default;			// 4
	uint32_t test_reg_enable_lpm5_;		// 8
	uint32_t test_reg_disable_lpm5_;	// 12
	uint16_t test_reg3v_mask_;			// 16
	uint16_t test_reg3v_default;		// 18
	uint16_t test_reg3v_enable_lpm5_;	// 20
	uint16_t test_reg3v_disable_lpm5_;	// 22
};										// Structure size = 24 bytes

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			string sql = @"
				SELECT DISTINCT
					TestRegMask,
					TestRegDefault,
					TestRegEnableLpm5,
					TestRegDisableLpm5,
					TestReg3VMask,
					TestReg3VDefault,
					TestReg3VEnableLpm5,
					TestReg3VDisableLpm5,
					UsersGuide
				FROM
					PowerSettingsPacked p1 
						INNER JOIN Chips c1 
						WHERE (p1.SettingsPK == c1.PowerSettings)
				ORDER BY
					UsersGuide
			";
			foreach (var row in conn.Query(sql))
			{
				fh.WriteLine("// PowerSettings for family {0}", row.UsersGuide);
				fh.WriteLine("static constexpr PowerSettings pwr_{0} =", row.UsersGuide);
				fh.WriteLine("{");
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X8},\t// test_reg_mask_", row.TestRegMask));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X8},\t// test_reg_default", row.TestRegDefault));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X8},\t// test_reg_enable_lpm5_", row.TestRegEnableLpm5));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X8},\t// test_reg_disable_lpm5_", row.TestRegDisableLpm5));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X4},\t// test_reg3v_mask_", row.TestReg3VMask));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X4},\t// test_reg3v_default", row.TestReg3VDefault));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X4},\t// test_reg3v_enable_lpm5_", row.TestReg3VEnableLpm5));
				fh.WriteLine(Utils.BeatifyEnum("\t0x{0:X4}\t// test_reg3v_disable_lpm5_", row.TestReg3VDisableLpm5));
				fh.WriteLine("};");
				fh.WriteLine();
			}
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Decodes a power settings record based on the User's Guide number
ALWAYS_INLINE static const PowerSettings *DecodePowerSettings(EnumSlau family)
{
	if (family == kSLAU367)
		return &pwr_SLAU367;
	if (family == kSLAU445)
		return &pwr_SLAU445;
	return NULL;
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

