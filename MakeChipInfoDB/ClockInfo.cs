using System.Collections.Generic;

namespace MakeChipInfoDB
{
	class ClockInfo
	{
		public string Id;
		public ClockControl ClockControl = ClockControl.kGccNone;
		public EemTimerCfg TimerCfg;

		public ClockInfo(clockInfoType o, Dictionary<string, ClockInfo> refs, ref EemTimerCfgs ref1, ref EemTimers ref2)
		{
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(refs[k]);
			}
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if (o.clockControlSpecified)
				ClockControl = (ClockControl)o.clockControl;
			if (o.eemTimers != null)
			{
				if (TimerCfg == null)
					TimerCfg = new EemTimerCfg(o.eemTimers, ref1.Items, ref ref2);
				if (TimerCfg.Id != null)
					ref1.AddItem(TimerCfg);
			}
		}

		void Copy(ClockInfo o)
		{
			ClockControl = o.ClockControl;
			TimerCfg = o.TimerCfg;
		}
	}

	class ClockInfos
	{
		public Dictionary<string, ClockInfo> Items = new Dictionary<string, ClockInfo>();

		public void AddItem(ClockInfo o)
		{
			Items.Add(o.Id, o);
		}
	}
}
