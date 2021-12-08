using System.IO;

namespace MakeChipInfoDB
{
	class DieInfo
	{
		public string McuVer_ = "0x0000";
		public string McuSub_ = "0x0000";
		public string McuRev_ = "0x00";
		public string McuFab_ = "0x00";
		public string McuSelf_ = "0x0000";
		public string McuCfg_ = "0x00";
		public string McuFuse_ = "0x0";

		public void Merge(Device dev)
		{
			if (dev.Version != null)
				McuVer_ = "0x" + ((uint)dev.Version).ToString("x4");
			if (dev.SubVersion != null)
				McuSub_ = "0x" + ((uint)dev.SubVersion).ToString("x4");
			if (dev.Revision != null)
				McuRev_ = "0x" + ((uint)dev.Revision).ToString("x2");
			if (dev.Fab != null)
				McuFab_ = "0x" + ((uint)dev.Fab).ToString("x2");
			if (dev.Self != null)
				McuSelf_ = "0x" + ((uint)dev.Self).ToString("x4");
			if (dev.Config != null)
				McuCfg_ = "0x" + ((uint)dev.Config).ToString("x2");
			if (dev.Fuses != null)
				McuFuse_ = "0x" + ((uint)dev.Fuses).ToString("x");
		}

		public void DoHFile(StreamWriter fh, string id)
		{
			fh.Write(
			string.Format("\t{{{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7} }},\n"
				, id
				, McuVer_
				, McuSub_
				, McuRev_
				, McuFab_
				, McuSelf_
				, McuCfg_
				, McuFuse_
				)
			);
		}
	}
}
