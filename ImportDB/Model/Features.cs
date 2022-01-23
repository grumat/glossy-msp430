using System;
using System.Linq;

namespace ImportDB.Model
{
	class Features
	{
		public string PartNumber { get; set; }
		public string ClockSystem { get; set; }
		public bool Lcfe { get; set; }
		public bool QuickMemRead { get; set; }
		public bool I2C { get; set; }
		public bool StopFllDbg { get; set; }
		public bool HasFram { get; set; }

		public void Fill(featuresType src)
		{
			if (!string.IsNullOrEmpty(src.@ref))
			{
				var @base = XmlDatabase.AllInfos.Features.First(x => x.id == src.@ref);
				Fill(@base);
			}
			bool modified = false;
			if (src.clockSystemSpecified)
			{
				modified = true;
				ClockSystem = Enum.GetName(typeof(clockSystemType), src.clockSystem);
			}
			if (src.lcfeSpecified)
			{
				modified = true;
				Lcfe = src.lcfe != Bool.@false;
			}
			if (src.quickMemReadSpecified)
			{
				modified = true;
				QuickMemRead = src.quickMemRead != Bool.@false;
			}
			if (src.i2cSpecified)
			{
				modified = true;
				I2C = src.i2c != Bool.@false;
			}
			if (src.stopFllDbgSpecified)
			{
				modified = true;
				StopFllDbg = src.stopFllDbg != Bool.@false;
			}
			if (src.hasFramSpecified)
			{
				modified = true;
				HasFram = src.hasFram != Bool.@false;
			}
			if (!modified && String.IsNullOrEmpty(src.id) && String.IsNullOrEmpty(src.@ref))
				Clear();
		}

		void Clear()
		{
			ClockSystem = null;
			Lcfe = false;
			QuickMemRead = false;
			I2C = false;
			StopFllDbg = false;
			HasFram = false;
		}

		public bool IsClear()
		{
			return
				ClockSystem == null
				&& Lcfe == false
				&& QuickMemRead == false
				&& I2C == false
				&& StopFllDbg == false
				&& HasFram == false
				;
		}
	}
}
