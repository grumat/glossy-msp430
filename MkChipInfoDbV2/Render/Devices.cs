using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace MkChipInfoDbV2.Render
{
	class RevInfo
	{
		public string NewName;
		public long Version;
		public long? Subversion;
		public long? Revision;
	}
	class Devices : IRender
	{
		Dictionary<string, List<RevInfo>> Clashes = new Dictionary<string, List<RevInfo>>();

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			// Locate name collisions and provide unique keys for them
			string sql = @"
				SELECT DISTINCT
					PartNumber, 
					Version, 
					Subversion, 
					Revision
				FROM
					Chips c1
				WHERE
					(ActivationKey IS NULL OR ActivationKey != 2774181210)
					AND EXISTS (
						SELECT * 
						FROM Chips c2 
						WHERE c1.PartNumber == c2.PartNumber 
							AND (c2.ActivationKey IS NULL OR c2.ActivationKey != 2774181210)
							AND c1.ROWID != c2.ROWID
						)
				ORDER BY
					1, 2, 3, 4
			";
			// Collect clashes
			foreach (var row in conn.Query(sql))
			{
				RevInfo info = new RevInfo();
				info.Version = row.Version;
				info.Subversion = row.Subversion;
				info.Revision = row.Revision;
				List<RevInfo> l = null;
				if (Clashes.ContainsKey(row.PartNumber))
				{
					l = Clashes[row.PartNumber];
				}
				else
				{
					l = new List<RevInfo>();
					Clashes[row.PartNumber] = l;
				}
				info.NewName = String.Format("{0}_{1}", row.PartNumber, l.Count);
				l.Add(info);
			}
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		string DecodePartName(dynamic row, out string details)
		{
			string last = null;
			string descr = null;
			if (Clashes.ContainsKey(row.PartNumber))
			{
				List<RevInfo> l = Clashes[row.PartNumber];
				foreach (RevInfo p in l)
				{
					if (p.Version == row.Version
						&& p.Subversion == row.Subversion
						&& p.Revision == row.Revision
						)
					{
						last = String.Format("kMcu_{0}", p.NewName);
						StringBuilder buf = new StringBuilder();
						buf.AppendFormat("{0}\tv{1:X4}", row.PartNumber, p.Version);
						if (p.Subversion != null)
							buf.AppendFormat("; s{0:X2}", p.Subversion);
						if (p.Revision != null)
							buf.AppendFormat("; r{0:X2}", p.Revision);
						descr = buf.ToString();
						break;
					}
				}
			}
			else
			{
				last = String.Format("kMcu_{0}", row.PartNumber);
				descr = row.PartNumber;
			}
			details = descr;
			return Utils.GetIdentifier(last);
		}
		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			/*
			** NOTE: At the moment ActivationKey is not used to differentiate a device
			** It may change in the future if some way is offered to specify it.
			*/
			fh.WriteLine("// Supported MCUs");
			fh.WriteLine("enum EnumMcu : uint16_t");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Architecture,
					PartNumber, 
					Version, 
					Subversion, 
					Revision,
					Fab,
					Self,
					Config,
					Fuses
				FROM
					Chips c1
				WHERE
					(ActivationKey IS NULL OR ActivationKey != 2774181210)
				ORDER BY
					1, 2, 3, 4, 5, 6, 7, 8, 9
			";
			string cpu = null;
			string cpuX = null;
			string cpuXv2 = null;
			string last = "";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				// Write a nice header
				if ((cnt & 0x1f) == 0)
					fh.WriteLine("\t//..............................//..Idx..Part Number.............Ver..Subv.Rv.Fa.Self.Cf.Fu");
				string descr = null;
				last = DecodePartName(row, out descr);
				// Locate thresholds
				if (cpu == null && row.Architecture == "Cpu")
					cpu = last;
				else if (cpuX == null && row.Architecture == "CpuX")
					cpuX = last;
				else if (cpuXv2 == null && row.Architecture == "CpuXv2")
					cpuXv2 = last;
				// print line
				fh.WriteLine(Utils.BeatifyEnum2("\t{0},\t// [{1,3}] {2,-23} {3} {4} {5} {6} {7} {8} {9}"
					, last
					, cnt
					, row.PartNumber
					, Utils.BareHex4(row.Version)
					, Utils.BareHex4(row.Subversion)
					, Utils.BareHex2(row.Revision)
					, Utils.BareHex2(row.Fab)
					, Utils.BareHex4(row.Self)
					, Utils.BareHex2(row.Config)
					, Utils.BareHex2(row.Fuses)
					));
				++cnt;
			}
			fh.WriteLine("\t// Legacy CPU architecture starts here");
			fh.WriteLine("\tkMcu_ = {0},", cpu);
			fh.WriteLine("\t// CpuX architecture starts here");
			fh.WriteLine("\tkMcuX_ = {0},", cpuX);
			fh.WriteLine("\t// CpuXv2 architecture starts here");
			fh.WriteLine("\tkMcuXv2_ = {0},", cpuXv2);
			fh.WriteLine("\t// The last valid item");
			fh.WriteLine("\tkMcu_Last_ = {0},", last);
			fh.WriteLine("\t// Value for a failing search operation");
			fh.WriteLine("\tkMcu_None = 0xffff");
			fh.WriteLine("}};\t// {0} values; {1} bits", cnt, Utils.BitsRequired(cnt));
			fh.WriteLine();
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Describes the device or common attributes of a device group
struct Device
{
	// Index on symbol table (result requires DecompressChipName() for complete part number)
	uint16_t name_sym_;						// 0

	// Main ID of the device
	uint32_t mcu_ver_ : 16;				// 2

	// Sub-version device identification
	EnumSubversion mcu_subv_ : 2;		// 4
	// Revision device identification
	EnumRevision mcu_rev_ : 3;
	// Config device identification
	EnumConfig mcu_cfg_ : 3;

	// The fuse value
	EnumFuses mcu_fuses_ : 5;			// 5
	// The fuses mask
	EnumFusesMask mcu_fuse_mask_ : 3;

	// Fab device identification
	EnumFab mcu_fab_ : 1;				// 6
	// Self device identification
	EnumSelf mcu_self_ : 1;
	// Type of clock required by device
	EnumClockControl clock_ctrl_: 2;
	// Stop FLL clock
	EnumStopFllDbg stop_fll_ : 1;
	// Embedded Emulation Module type
	EnumEemType eem_type_ : 3;

	// Memory Configuration
	EnumMemoryConfigs mem_config_;		// 7

	// EemTimers
	EnumEemTimers eem_timers_ : 6;		// 8
	// Issue 1377 with the JTAG MailBox
	EnumIssue1377 issue_1377_ : 1;
	// TLV can be lost when erasing InfoA
	EnumTlvClash tlv_clash_ : 1;
};
");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			SymTable symtab = new SymTable();

			fh.WriteLine("// Device table, indexed by the McuIndexes enumeration");
			fh.WriteLine("static constexpr const Device msp430_mcus_set[] =");
			fh.WriteLine("{");
			string sql = @"
				SELECT DISTINCT
					Architecture,
					PartNumber, 
					Version, 
					Subversion, 
					Revision,
					Fab,
					Self,
					Config,
					Fuses,
					Psa,
					ClockControl,
					EemType,
					Issue1377,
					TlvClash,
					FusesMask,
					(SELECT 
						MAX(MemGroup) 
						FROM Memories2 m2 
						WHERE m2.PartNumber == c1.PartNumber 
							AND m2.MemGroup != '_AllParts_'
					) AS MemGroup,
					EemTimers,
					StopFllDbg
				FROM
					Chips c1
				WHERE
					(ActivationKey IS NULL OR ActivationKey != 2774181210)
				ORDER BY
					1, 2, 3, 4, 5, 6, 7, 8, 9
			";
			uint cnt = 0;
			foreach (var row in conn.Query(sql))
			{
				string descr = null;
				string last = DecodePartName(row, out descr);
				fh.WriteLine("\t// {0}: Part number: {1}", last, descr);
				fh.WriteLine("\t{{ // {0}", cnt);
				int spos = symtab.AddString(PrefixResolver.EncodeChipName(row.PartNumber));
				fh.WriteLine("\t\t{0},", spos);
				//
				fh.WriteLine("\t\t0x{0:X4},", row.Version);
				//
				fh.WriteLine("\t\t{0},", Subversion.Map(row.Subversion));
				fh.WriteLine("\t\t{0},", Revision.Map(row.Revision));
				fh.WriteLine("\t\t{0},", Config.Map(row.Config));
				//
				fh.WriteLine("\t\t{0},", Fuses.Map(row.Fuses));
				fh.WriteLine("\t\t{0},", Fuses.Mask(row.FusesMask));
				//
				fh.WriteLine("\t\t{0},", Fab.Map(row.Fab));
				fh.WriteLine("\t\t{0},", Self.Map(row.Self));
				fh.WriteLine("\t\t{0},", ClockControl.Map(row.ClockControl));
				fh.WriteLine("\t\t{0},", StopFllDbg.Map(row.StopFllDbg));
				fh.WriteLine("\t\t{0},", Eem.Map(row.EemType));
				//
				fh.WriteLine("\t\tkMCfg_{0},", row.MemGroup);
				//
				fh.WriteLine("\t\t{0},", Eem.MapTimer(row.EemTimers));
				fh.WriteLine("\t\t{0},", Issue1377.Map(row.Issue1377));
				fh.WriteLine("\t\t{0},", TlvClash.Map(row.TlvClash));
				fh.WriteLine("\t},");
				++cnt;
			}
			fh.WriteLine("};");
			fh.WriteLine();

			symtab.RenderTable(fh, "symtab_part_names");
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine(@"
// Devices having GMERAS bit on FCTL1 register
ALWAYS_INLINE static bool HasGmeras(const Device &dev)
{
	return (dev.mcu_ver_ == 28660);
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
#ifdef OPT_IMPLEMENT_TEST_DB

#pragma pack(1)

struct PartInfo
{
	EnumMcu		i_refd_;
	uint16_t	mcu_ver_;
	uint16_t	mcu_sub_;
	uint8_t		mcu_rev_;
	uint8_t		mcu_fab_;
	uint16_t	mcu_self_;
	uint8_t		mcu_cfg_;
	uint8_t		mcu_fuse_;
};

#pragma pack()

static constexpr const PartInfo all_part_codes[] =
{
");
			string sql = @"
				SELECT DISTINCT
					PartNumber, 
					Version, 
					Subversion, 
					Revision,
					Fab,
					Self,
					Config,
					Fuses
				FROM
					Chips c1
				WHERE
					(ActivationKey IS NULL OR ActivationKey != 2774181210)
				ORDER BY
					1, 2, 3, 4, 5, 6, 7, 8
			";
			foreach (var row in conn.Query(sql))
			{
				string details;
				fh.WriteLine(Utils.BeatifyEnum2("\t{{ {0},\t0x{1:x4}, 0x{2:x4}, 0x{3:x2}, 0x{4:x2}, 0x{5:x4}, 0x{6:x2}, 0x{7:x2}, }},"
					, DecodePartName(row, out details)
					, row.Version
					, row.Subversion ?? 0xffff
					, row.Revision ?? 0xff
					, row.Fab ?? 0xff
					, row.Self ?? 0xffff
					, row.Config ?? 0xff
					, row.Fuses ?? 0x1f
					));
			}
			fh.Write(@"};

#endif	// OPT_IMPLEMENT_TEST_DB
");
		}
	}
}

