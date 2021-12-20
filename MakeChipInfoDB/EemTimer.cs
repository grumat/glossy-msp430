using System;
using System.Collections.Generic;

namespace MakeChipInfoDB
{
	class EemTimer
	{
		public string Id;
		public uint? Index;
		public string Name;
		public byte? Value;
		public bool? DefaultStop;

		public EemTimer(eemTimerType o, Dictionary<string, EemTimer> refs)
		{
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(refs[k]);
			}
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if (o.indexSpecified)
				Index = decimal.ToUInt32(o.index);
			if (o.name != null)
				Name = o.name;
			if (o.value != null)
				Value = Convert.ToByte(o.value, 16);
			if(o.defaultStopSpecified)
				DefaultStop = o.defaultStop != Bool.@false;
		}

		void Copy(EemTimer o)
		{
			if (o.Id != null)
				Id = o.Id;
			if (o.Index != null)
				Index = o.Index;
			if (o.Name != null)
				Name = o.Name;
			if (o.Value != null)
				Value = o.Value;
			if (o.DefaultStop != null)
				DefaultStop = o.DefaultStop;
		}

		public bool Equals(EemTimer o)
		{
			// fields that are commented out does not affect firmware behavior
			return Index == o.Index
				//&& Name == o.Name
				&& Value == o.Value
				//&& DefaultStop == o.DefaultStop
				;
		}
	}

	class EemTimers
	{
		public Dictionary<string, EemTimer> Items = new Dictionary<string, EemTimer>();

		public void AddItem(EemTimer o)
		{
			if (!Items.ContainsKey(o.Id))
				Items.Add(o.Id, o);
		}
	}
}
