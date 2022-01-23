using System;
using System.Linq;

namespace ImportDB.Model
{
	class Voltages
	{
		public UInt16? VccMin { get; set; }
		public UInt16? VccMax { get; set; }
		public UInt16? VccFlashMin { get; set; }
		public UInt16? VccSecureMin { get; set; }
		public bool? TestVpp { get; set; }

		public void Fill(voltageInfoType src)
		{
			if (!string.IsNullOrEmpty(src.@ref))
			{
				var @base = XmlDatabase.AllInfos.VoltageInfos.First(x => x.id == src.@ref);
				Fill(@base);
			}
			if (src.vccMinSpecified)
				VccMin = src.vccMin;
			if (src.vccMaxSpecified)
				VccMax = src.vccMax;
			if (src.vccFlashMinSpecified && src.vccFlashMin != 0)
				VccFlashMin = src.vccFlashMin;
			if (src.vccSecureMinSpecified && src.vccSecureMin != 0)
				VccSecureMin = src.vccSecureMin;
			if (src.testVppSpecified)
				TestVpp = src.testVpp != Bool.@false;
		}
	}
}
