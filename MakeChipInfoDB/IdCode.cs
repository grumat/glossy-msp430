using System;
using System.Collections.Generic;

namespace MakeChipInfoDB
{
	class IdCode
	{
		public string Id;
		public uint? Version = null;
		public SubversionEnum SubVersion = SubversionEnum.kSubver_None;
		public RevisionEnum Revision = RevisionEnum.kRev_None;
		public FabEnum Fab = FabEnum.kFab_None;
		public SelfEnum Self = SelfEnum.kSelf_None;
		public ConfigEnum Config = ConfigEnum.kCfg_None;
		public FusesEnum Fuses = FusesEnum.kFuse_None;
		public uint? ActivationKey = null;

		public IdCode(idCodeType o, Dictionary<string, IdCode> refs)
		{
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(refs[k]);
			}
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if(o.version != null)
				Version = Convert.ToUInt32(o.version, 16);
			if (o.subversion != null)
				SubVersion = Enums.ResolveSubversion(o.subversion);
			if (o.revision != null)
				Revision = Enums.ResolveRevision(o.revision);
			if (o.config != null)
				Config = Enums.ResolveConfig(o.config);
			if (o.self != null)
				Self = Enums.ResolveSelf(o.self);
			if (o.fab != null)
				Fab = Enums.ResolveFab(o.fab);
			if (o.fuses != null)
				Fuses = Enums.ResolveFuses(o.fuses);
			if (o.activationKey != null)
				ActivationKey = Convert.ToUInt32(o.activationKey, 16);
		}

		void Copy(IdCode o)
		{
			Version = o.Version;
			SubVersion = o.SubVersion;
			Revision = o.Revision;
			Fab = o.Fab;
			Self = o.Self;
			Config = o.Config;
			Fuses = o.Fuses;
			ActivationKey = o.ActivationKey;
		}
	}

	class IdCodes
	{
		public Dictionary<string, IdCode> Items = new Dictionary<string, IdCode>();

		public void AddItem(IdCode o)
		{
			Items.Add(o.Id, o);
		}
	}
}
