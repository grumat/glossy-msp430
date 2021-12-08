using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace MakeChipInfoDB
{

	// Reduced structure version of MemoryType
	public class Memory
	{
		public string Id;
		public string Ref;
		public MemoryTypeType2 Type = MemoryTypeType2.kNullMemType;
		public BitSize Bits = BitSize.kNullBitSize;
		public AddressStart Start = AddressStart.kStart_None;
		public BlockSize Size = BlockSize.kSize_None;
		public SegSize SegmentSize = SegSize.kSeg_None;
		public uint Banks;
		public bool Mapped;		// this should be nullable, but it costs 1 bit and seems not to affect
		public MemAccessType AccessType = MemAccessType.kNullMemAccess;
		public bool AccessMpu;

		public bool Equals(Memory o)
		{
			if (o == null)
				return false;
			return Ref == o.Ref
				&& Start == o.Start
				&& Size == o.Size
				&& Type == o.Type
				&& Bits == o.Bits
				&& Banks == o.Banks
				&& Mapped == o.Mapped
				&& AccessType == o.AccessType
				&& AccessMpu == o.AccessMpu
				;
		}

		public bool IsPure()
		{
			if (String.IsNullOrEmpty(Ref))
				return false;
			return Type == MemoryTypeType2.kNullMemType
				&& Bits == BitSize.kNullBitSize
				&& Start == AddressStart.kStart_None
				&& Size == BlockSize.kSize_None
				&& Banks == 0
				&& Mapped == false
				&& AccessType == MemAccessType.kNullMemAccess
				&& AccessMpu == false;
		}

		public static string GetIdentifier(string id)
		{
			return "kMem_" + MyUtils.MkIdentifier(id);
		}

		public Memory(MemoryType o, Dictionary<string, string> refmap)
		{
			if(o.id != null)
				Id = GetIdentifier(o.id);
			if(o.@ref != null)
			{
				string r = GetIdentifier(o.@ref);
				if (refmap.ContainsKey(r))
					Ref = refmap[r];
				else
					Ref = r;
			}
			else
				Ref = o.@ref;
			Start = o.start == null ? AddressStart.kStart_None : Enums.ResolveAddress(Convert.ToUInt32(o.start, 16));
			Size = o.size == null ? BlockSize.kSize_None : Enums.ResolveBlockSize(Convert.ToUInt32(o.size, 16));
			SegmentSize = o.size == null ? SegSize.kSeg_None : Enums.ResolveSegSize(Convert.ToUInt32(o.segmentSize, 16));
			//
			if (o.typeSpecified)
			{
				switch (o.type)
				{
				case MemoryTypeType.Flash:
					Type = MemoryTypeType2.kFlash;
					break;
				case MemoryTypeType.Ram:
					Type = MemoryTypeType2.kRam;
					break;
				case MemoryTypeType.Register:
					Type = MemoryTypeType2.kRegister;
					break;
				case MemoryTypeType.Rom:
					Type = MemoryTypeType2.kRom;
					break;
				}
			}
			else
				Type = MemoryTypeType2.kNullMemType;
			// 
			if (o.bitsSpecified)
			{
				switch ((int)o.bits)
				{
				case 8:
					Bits = BitSize.k8;
					break;
				case 16:
					Bits = BitSize.k16;
					break;
				case 20:
					Bits = BitSize.k20;
					break;
				default:
					throw new InvalidDataException("Invalid BitSize = " + o.bits.ToString());
				}
			}
			else
				Bits = BitSize.kNullBitSize;
			//
			if (o.banksSpecified)
				Banks = (uint)o.banks;
			else
				Banks = 0;
			//
			if (o.mappedSpecified)
				Mapped = o.mapped != Bool.@false;
			else
				Mapped = false;
			//
			AccessMpu = o.memoryAccess != null && o.memoryAccess.mpuSpecified && o.memoryAccess.mpu != Bool.@false;
			// 
			if (o.memoryAccess != null)
			{
				switch (o.memoryAccess.type)
				{
				case memoryAccessClassType.LockableRamMemoryAccess:
					AccessType = MemAccessType.kLockableRamMemoryAccess;
					break;
				case memoryAccessClassType.BootcodeRomAccess:
					AccessType = MemAccessType.kBootcodeRomAccess;
					break;
				case memoryAccessClassType.RegisterAccess5xx:
					AccessType = MemAccessType.kRegisterAccess5xx;
					break;
				case memoryAccessClassType.InformationFlashAccess:
					AccessType = MemAccessType.kInformationFlashAccess;
					break;
				case memoryAccessClassType.FlashMemoryAccess2ByteAligned:
					AccessType = MemAccessType.kFlashMemoryAccess2ByteAligned;
					break;
				case memoryAccessClassType.BslFlashAccess:
					AccessType = MemAccessType.kBslFlashAccess;
					break;
				case memoryAccessClassType.FramMemoryAccessBase:
					AccessType = MemAccessType.kFramMemoryAccessBase;
					break;
				case memoryAccessClassType.FramMemoryAccessFRx9:
					AccessType = MemAccessType.kFramMemoryAccessFRx9;
					break;
				case memoryAccessClassType.UsbRamAccess:
					AccessType = MemAccessType.kUsbRamAccess;
					break;
				case memoryAccessClassType.BslRomAccess:
					AccessType = MemAccessType.kBslRomAccess;
					break;
				case memoryAccessClassType.BslRomAccessGR:
					AccessType = MemAccessType.kBslRomAccessGR;
					break;
				case memoryAccessClassType.TinyRandomMemoryAccess:
					AccessType = MemAccessType.kTinyRandomMemoryAccess;
					break;
				}
			}
			else
				AccessType = MemAccessType.kNullMemAccess;
		}

		public void DoHfile(TextWriter fh, int i_ref, string ref_name)
		{
			fh.Write("\t\t" + i_ref.ToString());
			if (ref_name != null)
				fh.Write("\t\t\t\t// " + ref_name);
			fh.WriteLine();
			//
			fh.WriteLine("\t\t, " + Banks);
			//
			fh.WriteLine("\t\t, " + Enums.MakeAddressKey(Start));
			fh.WriteLine("\t\t, " + Enums.MakeBlockSizeKey(Size));
			fh.WriteLine("\t\t, " + Enums.MakeMemoryTypeTypeKey(Type));
			fh.WriteLine("\t\t, " + Enums.MakeBitSizeKey(Bits));
			fh.WriteLine("\t\t, " + Mapped.ToString().ToLower());
			fh.WriteLine("\t\t, " + AccessMpu.ToString().ToLower());
			fh.WriteLine("\t\t, " + Enums.MakeMemAccessTypeKey(AccessType));
		}
	}

	public class Memories
	{
		public Dictionary<string, Memory> Mems_ = new Dictionary<string, Memory>();
		public Dictionary<string, string> MemAliases_ = new Dictionary<string, string>();
		public List<string> Phys_ = new List<string>();

		public string  AddItem(Memory o)
		{
			var match = Mems_.FirstOrDefault(x => o.Equals(x.Value));
			if (match.Key != null)
			{
				MemAliases_.Add(o.Id, match.Key);
				return match.Key;
			}
			else
			{
				Mems_.Add(o.Id, o);
				Phys_.Add(o.Id);
				return o.Id;
			}
		}

		public Memory Find(string key)
		{
			if (MemAliases_.ContainsKey(key))
				key = MemAliases_[key];
			if (Mems_.ContainsKey(key))
				return Mems_[key];
			return null;
		}

		internal int ToIdx(string r)
		{
			for (int i = 0; i < Phys_.Count; ++i)
			{
				if (Phys_[i] == r)
					return i;
			}
			throw new InvalidDataException("Key '" + r + "' was not found on collection");
		}

		public void DoHFile(TextWriter stream)
		{
			StringBuilder the_enum = new StringBuilder();
			stream.WriteLine("static constexpr const MemoryInfo all_mem_infos[] =");
			stream.WriteLine("{");
			for (int i = 0; i < Phys_.Count; ++i)
			{
				string n = Phys_[i];
				Memory o = Mems_[n];
				the_enum.Append('\t' + n + "," + Environment.NewLine);
				stream.WriteLine(String.Format("\t{{\t// {1} [{0}]", i + 1, n));
				if (o.Ref != null)
					o.DoHfile(stream, ToIdx(o.Ref) + 1, o.Ref);
				else
					o.DoHfile(stream, 0, null);
				stream.WriteLine("\t},");
			}
			stream.WriteLine("};");
			stream.WriteLine();
			stream.WriteLine("enum MemIndexes");
			stream.WriteLine("{");
			stream.Write(the_enum.ToString());
			stream.WriteLine("};");
		}
	}
}
