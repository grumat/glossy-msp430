using System;
using System.Linq;

namespace ImportDB.Model
{
	class IdCode
	{
		public UInt16? Version { get; set; }
		public UInt16? Subversion { get; set; }
		public Byte? Revision { get; set; }
		public Byte? MaxRevision { get; set; }
		public Byte? Fab { get; set; }
		public UInt16? Self { get; set; }
		public Byte? Config { get; set; }
		public Byte? Fuses { get; set; }
		public UInt32? ActivationKey { get; set; }

		void Fill_(idCodeType src)
		{
			if(!String.IsNullOrEmpty(src.version))
				Version = Convert.ToUInt16(src.version, 16);
			if (!String.IsNullOrEmpty(src.subversion))
				Subversion = Convert.ToUInt16(src.subversion, 16);
			if (!String.IsNullOrEmpty(src.revision))
				Revision = Convert.ToByte(src.revision, 16);
			if (!String.IsNullOrEmpty(src.maxRevision))
				MaxRevision = Convert.ToByte(src.maxRevision, 16);
			if (!String.IsNullOrEmpty(src.fab))
				Fab = Convert.ToByte(src.fab, 16);
			if (!String.IsNullOrEmpty(src.self))
				Self = Convert.ToUInt16(src.self, 16);
			if (!String.IsNullOrEmpty(src.config))
				Config = Convert.ToByte(src.config, 16);
			if (!String.IsNullOrEmpty(src.fuses))
				Fuses = Convert.ToByte(src.fuses, 16);
			if (!String.IsNullOrEmpty(src.activationKey))
				ActivationKey = Convert.ToUInt32(src.activationKey, 16);
		}

		public void Fill(idCodeType src)
		{
			if (!string.IsNullOrEmpty(src.@ref))
			{
				var @base = XmlDatabase.AllInfos.IdCodes.First(x => x.id == src.@ref);
				Fill(@base);
			}
			Fill_(src);
		}
		public void FillMask(idCodeType src)
		{
			if (!string.IsNullOrEmpty(src.@ref))
			{
				var @base = XmlDatabase.AllInfos.IdMasks.First(x => x.id == src.@ref);
				FillMask(@base);
			}
			Fill_(src);
		}
	}
}
