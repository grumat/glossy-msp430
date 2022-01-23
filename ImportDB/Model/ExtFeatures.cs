using System;
using System.Linq;

namespace ImportDB.Model
{
	class ExtFeatures
	{
		public string PartNumber { get; set; }
		public bool Tmr { get; set; }
		public bool Jtag { get; set; }
		public bool Dtc { get; set; }
		public bool Sync { get; set; }
		public bool Instr { get; set; }
		public bool Issue1377 { get; set; }
		public bool PsaCh { get; set; }
		public bool NoEemLpm { get; set; }

		public void Fill(extFeaturesType src)
		{
			if (!string.IsNullOrEmpty(src.@ref))
			{
				var @base = XmlDatabase.AllInfos.ExtFeatures.First(x => x.id == src.@ref);
				Fill(@base);
			}
			bool modified = false;
			if (src.tmrSpecified)
			{
				modified = true;
				Tmr = src.tmr != Bool.@false;
			}
			if (src.jtagSpecified)
			{
				modified = true;
				Jtag = src.jtag != Bool.@false;
			}
			if (src.dtcSpecified)
			{
				modified = true;
				Dtc = src.dtc != Bool.@false;
			}
			if (src.syncSpecified)
			{
				modified = true;
				Sync = src.sync != Bool.@false;
			}
			if (src.instrSpecified)
			{
				modified = true;
				Instr = src.instr != Bool.@false;
			}
			if (src._1377Specified)
			{
				modified = true;
				Issue1377 = src._1377 != Bool.@false;
			}
			if (src.psachSpecified)
			{
				modified = true;
				PsaCh = src.psach != Bool.@false;
			}
			if (src.eemInaccessibleInLPMSpecified)
			{
				modified = true;
				NoEemLpm = src.eemInaccessibleInLPM != Bool.@false;
			}
			// Clear everything
			if (!modified && src.id == null && src.@ref == null)
			{
				Tmr = false;
				Jtag = false;
				Dtc = false;
				Sync = false;
				Instr = false;
				Issue1377 = false;
				PsaCh = false;
				NoEemLpm = false;
			}
		}
	}
}
