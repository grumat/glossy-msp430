using System.Collections.Generic;

namespace MakeChipInfoDB
{
	class ClockInfo
	{
		public string Id;
		public ClockControl ClockControl = ClockControl.kGccNone;

		public ClockInfo(clockInfoType o, Dictionary<string, ClockInfo> refs)
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
		}

		void Copy(ClockInfo o)
		{
			ClockControl = o.ClockControl;
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
