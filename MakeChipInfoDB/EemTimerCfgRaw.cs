using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MakeChipInfoDB
{
	class EemTimerCfgRaw
	{
		public EemTimer[] Timers = new EemTimer[32];

		public void ClearHits(eemTimerType[] eemt)
		{
			// Clear previously defined IDs
			foreach (var t in eemt)
			{
				if (t == null || t.id == null)
					continue;
				for (int i = 0; i < Timers.Length; ++i)
				{
					var tt = Timers[i];
					// a single id per set is allowed
					if (tt != null && tt.Id != null && tt.Id == t.id)
					{
						Timers[i] = null;
					}
				}
			}
		}

		public void Set(eemTimerType[] eemt, ref EemTimers ref2)
		{
			// Assign new ids
			foreach (var t in eemt)
			{
				EemTimer tt = new EemTimer(t, ref2.Items);
				if (tt.Id != null)
					ref2.AddItem(tt);
				if (tt.Index == null)
					throw new InvalidDataException("No index specified for timer " + tt.Name);
				Timers[(uint)tt.Index] = tt;
			}
		}

		public void Copy(EemTimerCfgRaw o)
		{
			for (uint i = 0; i < o.Timers.Length; ++i)
			{
				Timers[i] = o.Timers[i];
			}
		}

		public bool Equals(EemTimerCfgRaw o)
		{
			for (uint i = 0; i < Timers.Length; ++i)
			{
				EemTimer l = Timers[i];
				EemTimer r = o.Timers[i];
				if (l == null)
				{
					if (r != null)
						return false;
					else
						continue;
				}
				else if(r == null)
					return false;
				if (!l.Equals(r))
					return false;
			}
			return true;
		}
	}

	class EemTimerDB
	{
		public List<EemTimerCfgRaw> Tab_ = new List<EemTimerCfgRaw>();

		public int AddSingle(EemTimerCfgRaw o)
		{
			for (int i = 0; i < Tab_.Count; ++i)
			{
				if (o.Equals(Tab_[i]))
					return i;
			}
			Tab_.Add(o);
			return (Tab_.Count - 1);
		}

		public void DoHFile(TextWriter fh)
		{
			for (int i = 0; i < Tab_.Count; ++i)
			{
				var tim = Tab_[i].Timers;
				fh.WriteLine("static constexpr EemTimer emmTimer{0}[] =", i);
				fh.WriteLine("{");
				for (int j = 0; j < tim.Length; ++j)
				{
					// strange undocumented sort order found on uif source code;
					// probably to move WDT to the beginning
					int idx = (tim.Length - (j + 1)) & 0x0f;
					if (j >= 16)
						idx += 16;
					// Don't know what the hell this stuff is required on CpuXv2, totally
					// undocumented. Firmware point a HW register ETKEYSEL in 0x0110 and 
					// another ETCLKSEL in 0x11E. No trace on the data-sheet...
					// Legacy CPU and CpuX ignore them...
					var e = Tab_[i].Timers[idx];
					if (e != null)
						fh.WriteLine("\t{{0x{0:x2}, 0x{1:x2}}},\t// {2} - {3}", j, e.Value, idx, e.Id);
				}
				fh.WriteLine("\t{0, 0}");
				fh.WriteLine("};");
				fh.WriteLine();
			}
			fh.WriteLine("// Index to possible tables");
			fh.WriteLine("static constexpr const EemTimer *emmTimers[] =");
			fh.WriteLine("{");
			for (int i = 0; i < Tab_.Count; ++i)
			{
				fh.WriteLine("\temmTimer{0},", i);
			}
			fh.WriteLine("};");
			fh.Write(@"
// Retrieves the EemTimer table for a given EemTimerEnum or NULL
ALWAYS_INLINE const EemTimer *GetEemTimer(EemTimerEnum i)
{
	return i <= kEmmTimer_Upper_ ? emmTimers[i] : NULL;
};


");
		}
	}
}
