using System.IO;

namespace MakeChipInfoDB
{
	class DieInfo
	{
		public string McuVer_ = "0xffff";	// 391
		public string McuSub_ = "0xffff";	// 3
		public string McuRev_ = "0xff";		// 5
		public string McuFab_ = "0xff";		// 2
		public string McuSelf_ = "0xffff";	// 2
		public string McuCfg_ = "0xff";		// 8
		public string McuFuse_ = "0xff";	// 20

		public void Merge(Device dev)
		{
			if (dev.Version != null)
				McuVer_ = "0x" + ((uint)dev.Version).ToString("x4");
			McuSub_ = "0x" + Enums.EnumToValue(dev.SubVersion).ToString("x4");
			McuRev_ = "0x" + Enums.EnumToValue(dev.Revision).ToString("x2");
			McuFab_ = "0x" + Enums.EnumToValue(dev.Fab).ToString("x2");
			McuSelf_ = "0x" + Enums.EnumToValue(dev.Self).ToString("x4");
			McuCfg_ = "0x" + Enums.EnumToValue(dev.Config).ToString("x2");
			McuFuse_ = "0x" + Enums.EnumToValue(dev.Fuses).ToString("x2");
		}

		public void DoHFile(TextWriter fh, string id)
		{
			fh.WriteLine("\t{{{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7} }},"
				, id
				, McuVer_
				, McuSub_
				, McuRev_
				, McuFab_
				, McuSelf_
				, McuCfg_
				, McuFuse_
			);
		}
	}
}
