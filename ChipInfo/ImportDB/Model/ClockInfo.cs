using System;
using System.Linq;

namespace ImportDB.Model
{
	class ClockInfo
	{
		public string ClockControl { get; set; }

		public Timers EemTimers;

		public Clocks EemClocks;

		public void Fill(clockInfoType rec)
		{
			if (!String.IsNullOrEmpty(rec.@ref))
			{
				var @base = XmlDatabase.AllInfos.ClockInfos.First(x => x.id == rec.@ref);
				Fill(@base);
			}
			if (rec.clockControlSpecified)
				ClockControl = Enum.GetName(typeof(ClockControlType), rec.clockControl);
			if (rec.eemTimers != null)
			{
				if (EemTimers == null)
					EemTimers = new Timers();
				EemTimers.Fill(rec.eemTimers);
			}
			if (rec.eemClocks != null)
			{
				if (EemClocks == null)
					EemClocks = new Clocks();
				EemClocks.Fill(rec.eemClocks);
			}
		}
	}
}
