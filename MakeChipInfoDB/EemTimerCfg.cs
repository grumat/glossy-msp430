using System.Collections.Generic;
using System.IO;

namespace MakeChipInfoDB
{
	class EemTimerCfg
	{
		public string Id;
		public EemTimerCfgRaw EemTims = new EemTimerCfgRaw();

		public EemTimerCfg(eemTimersType o, Dictionary<string, EemTimerCfg> refs, ref EemTimers ref2)
		{
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(refs[k]);
				if (o.eemTimer != null)
				{
					EemTims.ClearHits(o.eemTimer);
				}
			}
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if (o.eemTimer != null)
			{
				// Assign new ids
				EemTims.Set(o.eemTimer, ref ref2);
			}
		}

		void Copy(EemTimerCfg o)
		{
			EemTims.Copy(o.EemTims);
		}
	}

	class EemTimerCfgs
	{
		public Dictionary<string, EemTimerCfg> Items = new Dictionary<string, EemTimerCfg>();

		public void AddItem(EemTimerCfg o)
		{
			if (!Items.ContainsKey(o.Id))
				Items.Add(o.Id, o);
		}
	}
}
