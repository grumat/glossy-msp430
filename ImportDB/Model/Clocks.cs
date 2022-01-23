using System;
using System.Collections.Generic;
using System.Linq;

namespace ImportDB.Model
{
	class EemClock
	{
		public string PartNumber { get; set; }
		public Byte Index { get; set; }
		public string Value { get; set; }

		public void Fill(eemClockType rec)
		{
			Index = Convert.ToByte(rec.index);
			if (!String.IsNullOrEmpty(rec.Value))
				Value = rec.Value;
		}
	}

	class Clocks
	{
		public List<EemClock> EemClocks;

		public void Fill(eemClocksType rec)
		{
			if (rec.@ref != null)
			{
				var @base = XmlDatabase.AllInfos.EemClocks.First(x => x.id == rec.@ref);
				Fill(@base);
			}
			if (rec.eemClock != null)
			{
				foreach (var t in rec.eemClock)
				{
					EemClock itm = new EemClock();
					itm.Fill(t);
					if (EemClocks == null)
						EemClocks = new List<EemClock>();
					EemClock t2 = null;
					t2 = EemClocks.FirstOrDefault(x => x.Index == itm.Index);
					if (t2 != null)
					{
						EemClocks.RemoveAll(x => x.Index == t2.Index);
						// Merge
						if (itm.Value == null)
							itm.Value = t2.Value;
					}
					EemClocks.Add(itm);
				}
			}
			else if (EemClocks != null && String.IsNullOrEmpty(rec.id) && String.IsNullOrEmpty(rec.@ref))
			{
				EemClocks.Clear();
			}
		}

	}
}
