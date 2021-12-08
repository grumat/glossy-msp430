using System;
using System.Collections.Generic;

namespace MakeChipInfoDB
{
	class Feature
	{
		public string Id;
		public string ClockSystem;
		public bool Lcfe;
		public bool QuickMemRead;
		public bool I2C;
		public bool StopFllDbg;
		public bool HasFram;

		public Feature(featuresType o, Dictionary<string, Feature> refs)
		{
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(refs[k]);
			}
			if(o.clockSystemSpecified)
				ClockSystem = Enum.GetName(typeof(clockSystemType), o.clockSystem);
			if (o.lcfeSpecified)
				Lcfe = o.lcfe != Bool.@false;
			if (o.quickMemReadSpecified)
				QuickMemRead = o.quickMemRead != Bool.@false;
			if (o.i2cSpecified)
				I2C = o.i2c != Bool.@false;
			if (o.stopFllDbgSpecified)
				StopFllDbg = o.stopFllDbg != Bool.@false;
			if (o.hasFramSpecified)
				HasFram = o.hasFram != Bool.@false;
		}

		void Copy(Feature o)
		{
			Id = o.Id;
			ClockSystem = o.ClockSystem;
			Lcfe = o.Lcfe;
			QuickMemRead = o.QuickMemRead;
			I2C = o.I2C;
			StopFllDbg = o.StopFllDbg;
			HasFram = o.HasFram;
		}
	}

	class Features
	{
		public Dictionary<string, Feature> Items = new Dictionary<string, Feature>();

		public void AddItem(Feature o)
		{
			Items.Add(o.Id, o);
		}
	}
}
