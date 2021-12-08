using System.Collections.Generic;

namespace MakeChipInfoDB
{
	class ExtFeature
	{
		public string Id;
		public bool? Tmr;
		public bool? Jtag;
		public bool? Dtc;
		public bool? Sync;
		public bool? Instr;
		public bool? _1377 = null;
		public bool? Psach;
		public bool? EemInaccessibleInLPM;

		public ExtFeature(extFeaturesType o, Dictionary<string, ExtFeature> refs)
		{
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(refs[k]);
			}
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if (o.tmrSpecified)
				Tmr = o.tmr != Bool.@false;
			if (o.jtagSpecified)
				Jtag = o.jtag != Bool.@false;
			if (o.dtcSpecified)
				Dtc = o.dtc != Bool.@false;
			if (o.syncSpecified)
				Sync = o.sync != Bool.@false;
			if (o.instrSpecified)
				Instr = o.instr != Bool.@false;
			if (o._1377Specified)
				_1377 = o._1377 != Bool.@false;
			if (o.psachSpecified)
				Psach = o.psach != Bool.@false;
			if (o.eemInaccessibleInLPMSpecified)
				EemInaccessibleInLPM = o.eemInaccessibleInLPM != Bool.@false;
		}

		public bool IsEmpty
		{
			get
			{
				return Id == null
					&& Tmr == null
					&& Jtag == null
					&& Dtc == null
					&& Sync == null
					&& Instr == null
					&& _1377 == null
					&& Psach == null
					&& EemInaccessibleInLPM == null
					;
			}
		}

		void Copy(ExtFeature o)
		{
			Id = o.Id;
			Tmr = o.Tmr;
			Jtag = o.Jtag;
			Dtc = o.Dtc;
			Sync = o.Sync;
			Instr = o.Instr;
			_1377 = o._1377;
			Psach = o.Psach;
			EemInaccessibleInLPM = o.EemInaccessibleInLPM;
		}
}

	class ExtFeatures
	{
		public Dictionary<string, ExtFeature> Items = new Dictionary<string, ExtFeature>();

		public void AddItem(ExtFeature o)
		{
			Items.Add(o.Id, o);
		}
	}
}
