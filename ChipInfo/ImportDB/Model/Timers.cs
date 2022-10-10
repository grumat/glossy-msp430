using System;
using System.Collections.Generic;
using System.Linq;

namespace ImportDB.Model
{
	class EemTimer
	{
		public string PartNumber { get; set; }
		public string Id { get; set; }
		public string Name { get; set; }
		public Byte? Value { get; set; }
		public bool? DefaultStop { get; set; }
		public Byte? Index { get; set; }

		public void Fill(eemTimerType rec)
		{
			if (!String.IsNullOrEmpty(rec.@ref))
			{
				var @base = XmlDatabase.AllInfos.EemTimerItems.First(x => x.id == rec.@ref);
				Fill(@base);
			}
			if (!String.IsNullOrEmpty(rec.id))
				Id = rec.id;
			if (!String.IsNullOrEmpty(rec.name))
				Name = rec.name;
			if (!String.IsNullOrEmpty(rec.value))
				Value = Convert.ToByte(rec.value, 16);
			if (rec.defaultStopSpecified)
				DefaultStop = rec.defaultStop != Bool.@false;
			if (rec.indexSpecified)
				Index = (Byte)rec.index;
		}
	}

	class Timers
	{
		public List<EemTimer> EemTimers;

		public void Fill(eemTimersType rec)
		{
			if (rec.@ref != null)
			{
				var @base = XmlDatabase.AllInfos.EemTimers.First(x => x.id == rec.@ref);
				Fill(@base);
			}
			if (rec.eemTimer != null)
			{
				foreach (var t in rec.eemTimer)
				{
					EemTimer itm = new EemTimer();
					itm.Fill(t);
					if (EemTimers == null)
						EemTimers = new List<EemTimer>();
					EemTimer t2 = null;
					if (itm.Id != null)
						t2 = EemTimers.FirstOrDefault(x => x.Id == itm.Id);
					if (t2 == null && itm.Index != null)
						t2 = EemTimers.FirstOrDefault(x => x.Index == itm.Index);
					if (t2 != null)
					{
						EemTimers.RemoveAll(x => (x.Name == t2.Name && x.Index == t2.Index));
						// Merge
						if (itm.Name == null)
							itm.Name = t2.Name;
						if (itm.Value == null)
							itm.Value = t2.Value;
						if (itm.DefaultStop == null)
							itm.DefaultStop = t2.DefaultStop;
						if (itm.Index == null)
							itm.Index = t2.Index;
					}
					EemTimers.Add(itm);
				}
			}
			else if (EemTimers != null && String.IsNullOrEmpty(rec.id) && String.IsNullOrEmpty(rec.@ref))
			{
				EemTimers.Clear();
			}
		}
	}
}
