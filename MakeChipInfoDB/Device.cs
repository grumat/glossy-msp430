using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace MakeChipInfoDB
{
	class Device
	{
		public string Id;
		public string Ref;
		public string Name;
		public uint? Version = null;
		public uint? SubVersion = null;
		public uint? Revision = null;
		public uint? Config = null;
		public ConfigMask MConfig = ConfigMask.kCfgNoMask;
		public uint? Self = null;
		public uint? Fab = null;
		public uint? Fuses = null;
		public FusesMask MFuses = FusesMask.kFuseNoMask;
		public uint? ActivationKey = null;
		public string Psa;
		public BitSize Bits = BitSize.kNullBitSize;
		public CpuArchitecture Arch = CpuArchitecture.kNullArchitecture;
		public EemType2 Eem;
		public bool QuickMemRead;
		public bool ClrExtFeat = false;
		public bool Issue1377;
		public ClockControl Clock = ClockControl.kGccNone;
		public string Lay;

		internal string ResolveClashId(string base_id, List<Device> devs)
		{
			string id;
			foreach (char ch in "!abcdefghijklmnopqrstuvwxyz")
			{
				if (ch != '!')
					id = base_id + '_' + ch;
				else
					id = base_id;
				bool collision = false;
				foreach (var d in devs)
				{
					if (d.Id == id)
					{
						collision = true;
						break;
					}
				}
				if (!collision)
					return id;
			}
			throw new InvalidDataException("Cannot solve id in node " + Name);
		}

		internal string ResolveClashIdByRef(List<Device> devs)
		{
			if(Ref == null)
				throw new InvalidDataException("Cannot solve id in node " + Name);
			Device refd = null;
			foreach (Device d in devs)
			{
				if (d.Id == Ref)
				{
					refd = d;
					break;
				}
			}
			if(refd == null)
				throw new InvalidDataException("Cannot solve id in node " + Name);
			return ResolveClashId(refd.Id, devs);
		}

		public Device(deviceType o, List<Device> devs, ref MemoryLayouts lays
			, ref Memories mems, ref Features feats, ref ExtFeatures xfeats
			, ref ClockInfos clks)
		{
			if (o.id != null)
				Id = GetIdentifier(o.id);
			if (o.@ref != null)
				Ref = GetIdentifier(o.@ref);
			Name = o.description;
			if (o.idCode != null)
			{
				Version = o.idCode.version != null ? Convert.ToUInt32(o.idCode.version, 16) : null;
				SubVersion = o.idCode.subversion != null ? Convert.ToUInt32(o.idCode.subversion, 16) : null;
				Revision = o.idCode.revision != null ? Convert.ToUInt32(o.idCode.revision, 16) : null;
				Config = o.idCode.config != null ? Convert.ToUInt32(o.idCode.config, 16) : null;
				Self = o.idCode.self != null ? Convert.ToUInt32(o.idCode.self, 16) : null;
				Fab = o.idCode.fab != null ? Convert.ToUInt32(o.idCode.fab, 16) : null;
				Fuses = o.idCode.fuses != null ? Convert.ToUInt32(o.idCode.fuses, 16) : null;
				ActivationKey = o.idCode.activationKey != null ? Convert.ToUInt32(o.idCode.activationKey, 16) : null;
			}
			if (o.idMask != null)
			{
				uint tmp;
				if (o.idMask.version != null)
				{
					tmp = Convert.ToUInt32(o.idMask.version, 16);
					if (tmp != 0xFFFF)
						throw new InvalidDataException("Version mask support limited to 0xFFFF for current tool (version/" + Id.ToString() + ")");
				}
				if (o.idMask.subversion != null)
				{
					tmp = Convert.ToUInt32(o.idMask.subversion, 16);
					if (tmp != 0xFFFF)
						throw new InvalidDataException("Subversion mask support limited to 0xFFFF for current tool (subversion/" + Id.ToString() + ")");
				}
				if (o.idMask.revision != null)
				{
					tmp = Convert.ToUInt32(o.idMask.revision, 16);
					if (tmp != 0xFF)
						throw new InvalidDataException("revision mask support limited to 0xFF for current tool (revision/" + Id.ToString() + ")");
				}
				if (o.idMask.config != null)
				{
					tmp = Convert.ToUInt32(o.idMask.config, 16);
					if (tmp != 0x7F && tmp != 0xFF)
						throw new InvalidDataException("config mask support limited to 0xFF for current tool (config/" + Id.ToString() + ")");
					MConfig = Enums.ResolveConfigMask(tmp);
				}
				if (o.idMask.self != null)
				{
					tmp = Convert.ToUInt32(o.idMask.self, 16);
					if (tmp != 0xFFFF)
						throw new InvalidDataException("Self mask support limited to 0xFFFF for current tool (self/" + Id.ToString() + ")");
				}
				if (o.idMask.fab != null)
				{
					tmp = Convert.ToUInt32(o.idMask.fab, 16);
					if (tmp != 0xFF)
						throw new InvalidDataException("Fab mask support limited to 0xFF for current tool (revision/" + Id.ToString() + ")");
				}
				if (o.idMask.fuses != null)
				{
					tmp = Convert.ToUInt32(o.idMask.fuses, 16);
					MFuses = Enums.ResolveFusesMask(tmp);
				}
			}
			if (o.psaSpecified)
				Psa = Enum.GetName(typeof(psaType), o.psa);
			if (o.bitsSpecified)
			{
				switch (((uint)o.bits))
				{
				case 8:
					Bits = BitSize.k8;
					break;
				case 16:
					Bits = BitSize.k16;
					break;
				case 20:
					Bits = BitSize.k20;
					break;
				default:
					throw new InvalidDataException("Bit size not supported");
				}
			}
			if (o.architectureSpecified)
				Arch = Enums.ResolveCpuArch(o.architecture);
			if (o.eemSpecified)
				Eem = Enums.ResolveEemType(o.eem);
			if (o.features != null)
			{
				Feature tmp = new Feature(o.features, feats.Items);
				QuickMemRead = tmp.QuickMemRead;
			}
			if (o.extFeatures != null)
			{
				ExtFeature tmp = new ExtFeature(o.extFeatures, xfeats.Items);
				if (tmp.IsEmpty)
					ClrExtFeat = true;
				else
					Issue1377 = tmp._1377 == true;
			}
			if (o.clockInfo != null)
			{
				ClockInfo tmp = new ClockInfo(o.clockInfo, clks.Items);
				Clock = tmp.ClockControl;
			}
			if (Id == null)
			{
				if (Name != null)
					Id = ResolveClashId("kMcu_" + MyUtils.MkIdentifier(Name), devs);
				else
					Id = ResolveClashIdByRef(devs);
			}
			if (o.memoryLayout != null)
			{
				Lay = lays.AppendDeviceLayout(o.memoryLayout, Id, ref mems);
			}
		}

		public static string GetIdentifier(string id)
		{
			return "kMcu_" + MyUtils.MkIdentifier(id);
		}

		internal static string extract_lo(uint w)
		{
			w = w & 0xFF;
			return "0x" + w.ToString("x");
		}
		internal static string extract_hi(uint w)
		{
			w = (w >> 8) & 0xFF;
			return "0x" + w.ToString("x");
		}

		private void PutBoolWithMask(TextWriter fh, uint? val, string t, string f)
		{
			StringBuilder s = new StringBuilder("\t\t, ");
			if (val != null)
			{
				s.Append(t);
				int tabs = 6 - ((s.Length - 2) / 4);
				if (tabs == 0)
					tabs = 1;
				s.Append('\t', tabs);
#if TEST
				s.Append("// 0x");
				s.Append(((uint)val).ToString("x"));
#else
				if (val == 0)
					s.Append("// 0");
				else
				{
					s.Append("// 0x");
					s.Append(((uint)val).ToString("x"));
				}
#endif
			}
			else
				s.Append(f);
			s.Append(Environment.NewLine);
			fh.Write(s.ToString());
		}

		public int DoHFile(TextWriter fh, int i, Devices devs)
		{
			int compress = 0;
			int cfg_cnt = 0;
			fh.WriteLine(String.Format("\t// {0}: Part number: {1}", Id, Name ?? "None"));
			fh.WriteLine(String.Format("\t{{ // {0}", i));
			if (Name != null)
			{
				string cn = MyUtils.ChipNameCompress(Name);
				compress = Name.Length - cn.Length;
				fh.WriteLine("\t\t\"" + cn + "\"");
			}
			else
				fh.WriteLine("\t\tNULL");
			if (Lay != null)
				fh.WriteLine("\t\t, " + Lay);
			else
				fh.WriteLine("\t\t, kLytNone");
			fh.WriteLine("\t\t, " + Enum.GetName(typeof(EemType2), Eem));
			fh.WriteLine("\t\t, " + Enum.GetName(typeof(ClockControl), Clock));
			fh.WriteLine("\t\t, " + (ClrExtFeat ? "kClrExtFeat" : "kNoClrExtFeat"));
			fh.WriteLine("\t\t, " + (Issue1377 ? "k1377" : "kNo1377"));
			fh.WriteLine("\t\t, " + (QuickMemRead ? "kQuickMemRead" : "kNoQuickMemRead"));
			if (Ref != null)
				fh.WriteLine(String.Format("\t\t, {0}\t\t\t\t\t// base: {1}", devs.IndexOf(Ref) + 1, Ref));
			else
				fh.WriteLine("\t\t, 0");
			PutBoolWithMask(fh, Fab, "kUseFab", "kNoFab");
			PutBoolWithMask(fh, Fuses, "kUseFuses", "kNoFuses");
			fh.WriteLine("\t\t, " + Enum.GetName(typeof(CpuArchitecture), Arch));
			if (Psa != null)
				fh.WriteLine("\t\t, k" + Psa);
			else
				fh.WriteLine("\t\t, kNullPsaType");
			if (Version != null)
				fh.WriteLine("\t\t, 0x" + ((uint)Version).ToString("x"));
			else
				fh.WriteLine("\t\t, NO_MCU_ID0");
			if (SubVersion != null)
			{
				fh.WriteLine("\t\t, " + extract_lo((uint)SubVersion));
				fh.WriteLine("\t\t, " + extract_hi((uint)SubVersion));
				cfg_cnt += 2;
			}
			if (Self != null)
			{
				fh.WriteLine("\t\t, " + extract_lo((uint)Self));
				fh.WriteLine("\t\t, " + extract_hi((uint)Self));
				cfg_cnt += 2;
			}
			if (Revision != null)
			{
				fh.WriteLine("\t\t, 0x" + ((uint)Revision).ToString("x"));
				cfg_cnt += 1;
			}
			if (Config != null)
			{
				fh.WriteLine("\t\t, 0x" + ((uint)Config).ToString("x"));
				cfg_cnt += 1;
			}
			if (Fab != null)
			{
				fh.WriteLine("\t\t, 0x" + ((uint)Fab).ToString("x"));
				cfg_cnt += 1;
			}
			if (Fuses != null)
			{
				fh.WriteLine("\t\t, 0x" + ((uint)Fuses).ToString("x"));
				cfg_cnt += 1;
			}
			if (cfg_cnt > 4)
				throw new InvalidDataException("Too many identification bytes for " + Id);
			while (cfg_cnt < 4)
			{
				fh.WriteLine("\t\t, EMPTY_INFO_SLOT");
				cfg_cnt += 1;
			}
			//
			fh.WriteLine("\t\t, " + Enum.GetName(typeof(FusesMask), MFuses));
			fh.WriteLine("\t\t, " + Enum.GetName(typeof(ConfigMask), MConfig));
			//
			PutBoolWithMask(fh, SubVersion, "kUseSubversion", "kNoSubversion");
			PutBoolWithMask(fh, Self, "kUseSelf", "kNoSelf");
			PutBoolWithMask(fh, Revision, "kUseRevision", "kNoRevision");
			PutBoolWithMask(fh, Config, "kUseConfig", "kNoConfig");
			//
			fh.WriteLine("\t},");

			return compress;
		}
	}

	class Devices
	{
		public List<Device> Items_ = new List<Device>();

		public void AddItem(Device o)
		{
			Items_.Add(o);
		}

		public int IndexOf(string id)
		{
			for (int i = 0; i < Items_.Count; ++i)
			{
				var n = Items_[i];
				if (n.Id == id)
					return i;
			}
			throw new InvalidDataException("Failed to find device id: " + id);
		}

		public void DoHFile(TextWriter fh)
		{
			StringBuilder senum = new StringBuilder();
			int compress = 0;
			int cnt = 0;
			fh.WriteLine();
			fh.WriteLine("// Device table, indexed by the McuIndexes enumeration");
			fh.WriteLine("static constexpr const Device msp430_mcus_set[] =");
			fh.WriteLine("{");
			for (int i = 0; i < Items_.Count; ++i)
			{
				var n = Items_[i];
				senum.Append("\t" + n.Id + "," + Environment.NewLine);
				compress += n.DoHFile(fh, i, this);
				if (n.Name != null)
					cnt += 1;
			}
			fh.WriteLine("};");
			fh.WriteLine();
			fh.WriteLine("enum McuIndexes : uint16_t");
			fh.WriteLine("{");
			fh.WriteLine(senum);
			fh.WriteLine("};");
			fh.WriteLine();

			Console.WriteLine("Compressed {0} bytes from part names", compress);

			List<string> ids = Items_.OrderBy(q => q.Name).Select(x => x.Id).ToList();

			fh.WriteLine();
			fh.WriteLine("// Supported MCU Table");
			fh.WriteLine("constexpr const DeviceList all_msp430_mcus =");
			fh.WriteLine("{");
			fh.WriteLine("\t" + cnt.ToString());

			foreach (string i in ids)
			{
				if (!String.IsNullOrEmpty(i))
				{
					var n = Items_.FirstOrDefault(x => x.Id == i);
					if (!String.IsNullOrEmpty(n.Name))
					{
						StringBuilder tmp = new StringBuilder("\t, ");
						tmp.Append(n.Id);
						tmp.Append('\t', ((29 - n.Id.Length) / 4));
						//
						tmp.Append("\t// ");
						tmp.Append(n.Name);
						//
						if (n.Version != null)
							tmp.AppendFormat(" 0x{0:x}", n.Version);
						else
							tmp.Append(" None");
						//
						uint? v2 = n.Revision ?? n.SubVersion ?? n.Config;
						if (v2 == 0)
							tmp.Append(":0");
						else if (v2 != null)
							tmp.AppendFormat(":0x{0:x}", v2);
						else
							tmp.Append(":?");
						//
						fh.WriteLine(tmp.ToString());
					}
				}
			}
			fh.WriteLine("};");
			fh.WriteLine();

			fh.Write(@"
#ifdef OPT_IMPLEMENT_TEST_DB

struct PartInfo
{
	McuIndexes	i_refd_;
	uint16_t	mcu_ver_;
	uint16_t	mcu_sub_;
	uint8_t		mcu_rev_;
	uint8_t		mcu_fab_;
	uint16_t	mcu_self_;
	uint8_t		mcu_cfg_;
	uint8_t		mcu_fuse_;
};

static constexpr const PartInfo all_part_codes[] =
{
");

			HashSet<string> combo = new HashSet<string>();
			foreach (string i in ids)
			{
				if (!String.IsNullOrEmpty(i))
				{
					var n = Items_.FirstOrDefault(x => x.Id == i);
					if (!String.IsNullOrEmpty(n.Name))
					{
						DieInfo di = new DieInfo();
						Fill(ref di, n);
						di.DoHFile(fh, n.Id);
						combo.Add(di.McuSub_);
					}
				}
			}

			fh.WriteLine("};");
			fh.WriteLine();
			fh.WriteLine("#endif	// OPT_IMPLEMENT_TEST_DB");

			Console.WriteLine("Total Version record: {0}", combo.Count);
		}
		internal void Fill(ref DieInfo di, Device dev)
		{
			if (dev.Ref != null)
			{
				var n = Items_.FirstOrDefault(x => x.Id == dev.Ref);
				if (n == null)
					throw new InvalidDataException("Cannot resolve device ID = " + dev.Ref);
				Fill(ref di, n);
			}
			di.Merge(dev);
		}
	}
}
