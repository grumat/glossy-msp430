using System;
using System.Collections.Generic;
using System.Linq;

namespace ImportDB.Model
{
	class Memory
	{
		public string PartNumber { get; set; }
		public string MemoryName { get; set; }
		public string MemoryType { get; set; }
		public Byte? Bits { get; set; }
		public UInt32? Start { get; set; }
		public UInt32? Size { get; set; }
		public UInt32? SegmentSize { get; set; }
		public Byte? Banks { get; set; }
		public bool? Mapped { get; set; }
		public bool Mpu { get; set; }
		public UInt64? Mask { get; set; }
		public bool Protectable { get; set; }
		public string AccessType { get; set; }
		public UInt16? WpAddress { get; set; }
		public UInt16? WpBits { get; set; }
		public UInt16? WpMask { get; set; }
		public UInt16? WpPwd { get; set; }


		public void Fill(MemoryType rec)
		{
			if (!String.IsNullOrEmpty(rec.@ref))
			{
				var @base = XmlDatabase.AllInfos.Memories.First(x => x.id == rec.@ref);
				Fill(@base);
			}
			if (rec.nameSpecified)
				MemoryName = Enum.GetName(typeof(MemoryNameType), rec.name);
			if (rec.typeSpecified)
				MemoryType = Enum.GetName(typeof(MemoryTypeType), rec.type);
			if (rec.bitsSpecified)
				Bits = Convert.ToByte(rec.bits);
			if (!String.IsNullOrEmpty(rec.start))
				Start = Convert.ToUInt32(rec.start, 16);
			if (!String.IsNullOrEmpty(rec.size))
				Size = Convert.ToUInt32(rec.size, 16);
			if (!String.IsNullOrEmpty(rec.segmentSize))
				SegmentSize = Convert.ToUInt32(rec.segmentSize, 16);
			if (rec.banksSpecified)
				Banks = Convert.ToByte(rec.banks);
			if (rec.mappedSpecified)
				Mapped = rec.mapped != Bool.@false;
			if (!String.IsNullOrEmpty(rec.mask))
				Mask = Convert.ToUInt64(rec.mask, 16);
			if (rec.protectableSpecified)
				Protectable = rec.protectable != Bool.@false;
			if (rec.memoryAccess != null)
			{
				// Current implementation does not use nesting
				if (!String.IsNullOrEmpty(rec.memoryAccess.id) || !String.IsNullOrEmpty(rec.memoryAccess.@ref))
					throw new NotSupportedException("Nesting found for MemoryAccess definitions");
				AccessType = Enum.GetName(typeof(memoryAccessClassType), rec.memoryAccess.type);
				if (rec.memoryAccess.mpuSpecified)
					Mpu = rec.memoryAccess.mpu != Bool.@false;
				else
					Mpu = false;
				if (rec.memoryAccess.writeProtection != null)
				{
					WriteProtectionType wp = rec.memoryAccess.writeProtection;
					// Current implementation does not use nesting
					if (!String.IsNullOrEmpty(wp.id) || !String.IsNullOrEmpty(wp.@ref))
						throw new NotSupportedException("Nesting found for WriteProtectionType definitions");
					if (!String.IsNullOrEmpty(wp.address))
						WpAddress = Convert.ToUInt16(wp.address, 16);
					if (!String.IsNullOrEmpty(wp.bits))
						WpBits = Convert.ToUInt16(wp.bits, 16);
					if (!String.IsNullOrEmpty(wp.mask))
						WpMask = Convert.ToUInt16(wp.mask, 16);
					if (!String.IsNullOrEmpty(wp.pwd))
						WpPwd = Convert.ToUInt16(wp.pwd, 16);
				}
			}
		}
	}

	class MemoryLayout
	{
		public List<Memory> Memories;

		public void Fill(memoryLayoutType rec)
		{
			if (rec.@ref != null)
			{
				var @base = XmlDatabase.AllInfos.MemoryLayouts.First(x => x.id == rec.@ref);
				Fill(@base);
			}
			if (rec.memory != null)
			{
				if (Memories == null)
					Memories = new List<Memory>();
				foreach (var t in rec.memory)
				{
					Memory itm = new Memory();
					itm.Fill(t);
					if (itm.MemoryName != null)
					{
						Memory t2 = Memories.FirstOrDefault(x => x.MemoryName == itm.MemoryName);
						if (t2 != null)
							Memories.RemoveAll(x => x.MemoryName == itm.MemoryName);
					}
					Memories.Add(itm);
				}
			}
		}
	}
}
