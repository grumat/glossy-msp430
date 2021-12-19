using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MakeChipInfoDB
{
	public enum MemoryTypeType2
	{
		kNullMemType
		, kRegister
		, kFlash
		, kRam
		, kRom
	}
	// Bit size of the CPU or Peripheral
	public enum BitSize
	{
		kNullBitSize
		, k8
		, k16
		, k20
	};
	public enum MemAccessType
	{
		kNullMemAccess
		, kBootcodeRomAccess
		, kBslRomAccess
		, kBslRomAccessGR
		, kBslFlashAccess
		, kFlashMemoryAccess2ByteAligned
		, kInformationFlashAccess
		, kFramMemoryAccessBase
		, kFramMemoryAccessFRx9
		, kTinyRandomMemoryAccess
		, kLockableRamMemoryAccess
		, kUsbRamAccess
		, kRegisterAccess5xx
	};

	// Architecture of the CPU
	public enum CpuArchitecture
	{
		kNullArchitecture
		, kCpu
		, kCpuX
		, kCpuXv2
	};

	// Embedded Emulation Module (EEM) type
	public enum EemType2
	{
		kEmexNone
		, kEmexLow
		, kEmexMedium
		, kEmexHigh
		, kEmexExtraSmall5XX
		, kEmexSmall5XX
		, kEmexMedium5XX
		, kEmexLarge5XX
		, kEemMax_
	};

	public enum PsaEnum
	{
		kPsaNone
		, kPsaRegular
		, kPsaEnhanced
	}

	// Enumeration for the Sub-version field on device identification
	public enum SubversionEnum
	{
		kSubver_None = 3
		, kSubver_0000 = 0
		, kSubver_0001 = 1
	};

	public enum RevisionEnum
	{
		kRev_None       // 0xff
		, kRev_00       // 0x00
		, kRev_02       // 0x02
		, kRev_10       // 0x10
		, kRev_13       // 0x13
		, kRev_20       // 0x20
		, kRev_21       // 0x21
	};

	// Allowed values for Fab
	public enum FabEnum
	{
		kFab_None   // 0xff
		, kFab_40   // 0x40
	}

	// Self field possible values
	public enum SelfEnum
	{
		kSelf_None      // 0xffff
		, kSelf_0000    // 0x0000
	};

	// Allowed values for Config field
	public enum ConfigEnum
	{
		kCfg_None   // 0xff
		, kCfg_00   // 0x00
		, kCfg_01   // 0x01
		, kCfg_02   // 0x02
		, kCfg_03   // 0x03
		, kCfg_45   // 0x45
		, kCfg_47   // 0x47
		, kCfg_57   // 0x57
	};

	public enum FusesEnum
	{
		kFuse_None = 0x1f	// 0x1f
		, kFuse_00 = 0		// 0x00
		, kFuse_01			// 0x01
		, kFuse_02			// 0x02
		, kFuse_03			// 0x03
		, kFuse_04			// 0x04
		, kFuse_05			// 0x05
		, kFuse_06			// 0x06
		, kFuse_07			// 0x07
		, kFuse_08			// 0x08
		, kFuse_09			// 0x09
		, kFuse_0a			// 0x0a
		, kFuse_0b			// 0x0b
		, kFuse_0c			// 0x0c
		, kFuse_0d			// 0x0d
		, kFuse_0e			// 0x0e
		, kFuse_0f			// 0x0f
		, kFuse_10			// 0x10
		, kFuse_11			// 0x11
		, kFuse_12			// 0x12
		, kFuse_Upper = kFuse_12
	}

	// Types of fuse masks
	public enum FusesMask
	{
		kFuse1F			// 0x1F
		, kFuse0F		// 0xF
		, kFuse07		// 0x7
		, kFuse03		// 0x3
		, kFuse01		// 0x1
	};

	// Types of Config masks
	public enum ConfigMask
	{
		kCfgNoMask      // 0xFF
		, kCfg7F        // 0x7F
	};

	public enum AddressStart
	{
		kStart_None
		, kStart_0x0
		, kStart_0x6
		, kStart_0x20
		, kStart_0x90
		, kStart_0x100
		, kStart_0x200
		, kStart_0xa80
		, kStart_0xb00
		, kStart_0xc00
		, kStart_0xe00
		, kStart_0xf00
		, kStart_0x1000
		, kStart_0x1100
		, kStart_0x1800
		, kStart_0x1900
		, kStart_0x1a00
		, kStart_0x1c00
		, kStart_0x1c80
		, kStart_0x1e00
		, kStart_0x2000
		, kStart_0x2100
		, kStart_0x2380
		, kStart_0x2400
		, kStart_0x2500
		, kStart_0x2c00
		, kStart_0x3100
		, kStart_0x4000
		, kStart_0x4400
		, kStart_0x5c00
		, kStart_0x6000
		, kStart_0x6c00
		, kStart_0x8000
		, kStart_0xa000
		, kStart_0xa400
		, kStart_0xc000
		, kStart_0xc200
		, kStart_0xc400
		, kStart_0xe000
		, kStart_0xe300
		, kStart_0xf000
		, kStart_0xf100
		, kStart_0xf800
		, kStart_0xf840
		, kStart_0xf880
		, kStart_0xfc00
		, kStart_0xfe00
		, kStart_0xffe0
		, kStart_0xc0000
		, kStart_0xf0000
		, kStart_0xf8000
		, kStart_0xfac00
		, kStart_0xffc00
		, kStart_Max_
	};

	// Enumeration that specifies the size of a memory block
	public enum BlockSize
	{
		kSize_None
		, kSize_0x6
		, kSize_0xd
		, kSize_0x10
		, kSize_0x15
		, kSize_0x1a
		, kSize_0x20
		, kSize_0x40
		, kSize_0x60
		, kSize_0x80
		, kSize_0x100
		, kSize_0x200
		, kSize_0x300
		, kSize_0x400
		, kSize_0x500
		, kSize_0x600
		, kSize_0x760
		, kSize_0x780
		, kSize_0x7c0
		, kSize_0x7e0
		, kSize_0x800
		, kSize_0xa00
		, kSize_0xa60
		, kSize_0xe00
		, kSize_0xf00
		, kSize_0xf80
		, kSize_0xfe0
		, kSize_0x1000
		, kSize_0x1400
		, kSize_0x1c00
		, kSize_0x1d00
		, kSize_0x1800
		, kSize_0x2000
		, kSize_0x2800
		, kSize_0x3000
		, kSize_0x3c00
		, kSize_0x3e00
		, kSize_0x4000
		, kSize_0x5000
		, kSize_0x6000
		, kSize_0x8000
		, kSize_0xc000
		, kSize_0xbc00
		, kSize_0xdb00
		, kSize_0xdf00
		, kSize_0xe000
		, kSize_0xef00
		, kSize_0xfc00
		, kSize_0x10000
		, kSize_0x16f00
		, kSize_0x18000
		, kSize_0x1cf00
		, kSize_0x1df00
		, kSize_0x20000
		, kSize_0x30000
		, kSize_0x40000
		, kSize_0x1fc00
		, kSize_0x17c00
		, kSize_0x60000
		, kSize_0x80000
		, kSize_Max_
	};

	// Enumeration that specifies the size of a memory block
	public enum SegSize
	{
		kSeg_None
		, kSeg_0x1
		, kSeg_0x40
		, kSeg_0x80
		, kSeg_0x200
		, kSeg_0x400
	}

	public enum ClockControl
	{
		kGccNone
		, kGccStandard
		, kGccStandardI
		, kGccExtended
	}


	class Enums
	{
		public static AddressStart ResolveAddress(uint addr)
		{
			for (int i = 0; i < from_enum_to_address.Length; ++i)
			{
				if (from_enum_to_address[i] == addr)
					return (AddressStart)i;
			}
			throw new InvalidDataException("Address 0x" + addr.ToString("x5") + " is not in enumeration");
		}
		public static uint UnMapAddress(AddressStart i)
		{
			return from_enum_to_address[(int)i];
		}
		public static string MakeAddressKey(AddressStart i)
		{
			return Enum.GetName(typeof(AddressStart), i);
		}
		public static BlockSize ResolveBlockSize(uint addr)
		{
			for (int i = 0; i < from_enum_to_block_size.Length; ++i)
			{
				if (from_enum_to_block_size[i] == addr)
					return (BlockSize)i;
			}
			throw new InvalidDataException("Size=0x" + addr.ToString("x5") + " is not in enumeration");
		}
		public static uint UnMapBlockSize(BlockSize i)
		{
			return from_enum_to_block_size[(int)i];
		}
		public static string MakeBlockSizeKey(BlockSize i)
		{
			return Enum.GetName(typeof(BlockSize), i);
		}
		public static SegSize ResolveSegSize(uint addr)
		{
			for (int i = 0; i < from_enum_to_seg_size.Length; ++i)
			{
				if (from_enum_to_seg_size[i] == addr)
					return (SegSize)i;
			}
			throw new InvalidDataException("SegSize=0x" + addr.ToString("x5") + " is not in enumeration");
		}

		public static BitSize ResolveBitSize(decimal b)
		{
			switch ((uint)b)
			{
			case 0:
				return BitSize.kNullBitSize;
			case 8:
				return BitSize.k8;
			case 16:
				return BitSize.k16;
			case 20:
				return BitSize.k20;
			default:
				throw new InvalidDataException("Bit size not supported");
			}
		}

		public static CpuArchitecture ResolveCpuArch(ArchitectureType t)
		{
			switch (t)
			{
			case ArchitectureType.Cpu:
				return CpuArchitecture.kCpu;
			case ArchitectureType.CpuX:
				return CpuArchitecture.kCpuX;
			default:
				return CpuArchitecture.kCpuXv2;
			}
		}

		public static EemType2 ResolveEemType(EemType t)
		{
			switch (t)
			{
			case EemType.EMEX_LOW:
				return EemType2.kEmexLow;
			case EemType.EMEX_MEDIUM:
				return EemType2.kEmexMedium;
			case EemType.EMEX_HIGH:
				return EemType2.kEmexHigh;
			case EemType.EMEX_EXTRA_SMALL_5XX:
				return EemType2.kEmexExtraSmall5XX;
			case EemType.EMEX_SMALL_5XX:
				return EemType2.kEmexSmall5XX;
			case EemType.EMEX_MEDIUM_5XX:
				return EemType2.kEmexMedium5XX;
			case EemType.EMEX_LARGE_5XX:
				return EemType2.kEmexLarge5XX;
			default:
				return EemType2.kEmexNone;
			}
		}

		public static PsaEnum ResolvePsaType(psaType t)
		{
			switch (t)
			{
			case psaType.Regular:
				return PsaEnum.kPsaRegular;
			default:
				return PsaEnum.kPsaEnhanced;
			}
		}

		public static SubversionEnum ResolveSubversion(string sv)
		{
			if (sv == null)
				return SubversionEnum.kSubver_None;
			UInt16 v = Convert.ToUInt16(sv, 16);
			switch (v)
			{
			case 0x0000:
				return SubversionEnum.kSubver_0000;
			case 0x0001:
				return SubversionEnum.kSubver_0001;
			default:
				throw new InvalidDataException("Cannot resolve subversion enum = 0x" + v.ToString("x4"));
			}
		}

		public static UInt16 EnumToValue(SubversionEnum e)
		{
			if (e == SubversionEnum.kSubver_None)
				return 0xffff;
			return (UInt16)e;
		}

		public static RevisionEnum ResolveRevision(string sv)
		{
			if (sv == null)
				return RevisionEnum.kRev_None;
			UInt16 v = Convert.ToUInt16(sv, 16);
			switch (v)
			{
			case 0x00:
				return RevisionEnum.kRev_00;
			case 0x02:
				return RevisionEnum.kRev_02;
			case 0x10:
				return RevisionEnum.kRev_10;
			case 0x13:
				return RevisionEnum.kRev_13;
			case 0x20:
				return RevisionEnum.kRev_20;
			case 0x21:
				return RevisionEnum.kRev_21;
			default:
				throw new InvalidDataException("Cannot resolve revision enum = 0x" + v.ToString("x2"));
			}
		}

		public static UInt16 EnumToValue(RevisionEnum e)
		{
			return from_enum_to_revision[(uint)e];
		}

		public static FabEnum ResolveFab(string sv)
		{
			if (sv == null)
				return FabEnum.kFab_None;
			UInt16 v = Convert.ToUInt16(sv, 16);
			if (v == 0x40)
				return FabEnum.kFab_40;
			throw new InvalidDataException("Cannot resolve fab enum = 0x" + v.ToString("x2"));
		}

		public static UInt16 EnumToValue(FabEnum e)
		{
			return (UInt16)(e == FabEnum.kFab_None ? 0xff : 0x40);
		}

		public static SelfEnum ResolveSelf(string sv)
		{
			if (sv == null)
				return SelfEnum.kSelf_None;
			UInt16 v = Convert.ToUInt16(sv, 16);
			if (v == 0x0000)
				return SelfEnum.kSelf_0000;
			throw new InvalidDataException("Cannot resolve self enum = 0x" + v.ToString("x4"));
		}

		public static UInt16 EnumToValue(SelfEnum e)
		{
			return (UInt16)(e == SelfEnum.kSelf_None ? 0xffff : 0x0000);
		}

		public static ConfigEnum ResolveConfig(string sv)
		{
			if (sv == null)
				return ConfigEnum.kCfg_None;
			UInt16 v = Convert.ToUInt16(sv, 16);
			switch (v)
			{
			case 0x00:
				return ConfigEnum.kCfg_00;
			case 0x01:
				return ConfigEnum.kCfg_01;
			case 0x02:
				return ConfigEnum.kCfg_02;
			case 0x03:
				return ConfigEnum.kCfg_03;
			case 0x45:
				return ConfigEnum.kCfg_45;
			case 0x47:
				return ConfigEnum.kCfg_47;
			case 0x57:
				return ConfigEnum.kCfg_57;
			default:
				throw new InvalidDataException("Cannot resolve config enum = 0x" + v.ToString("x2"));
			}
		}

		public static UInt16 EnumToValue(ConfigEnum e)
		{
			return from_enum_to_config[(uint)e];
		}

		public static FusesEnum ResolveFuses(string sv)
		{
			if (sv == null)
				return FusesEnum.kFuse_None;
			UInt16 v = Convert.ToUInt16(sv, 16);
			if (v > (UInt16)FusesEnum.kFuse_Upper)
				throw new InvalidDataException("Cannot resolve fuses enum = 0x" + v.ToString("x2"));
			return (FusesEnum)v;
		}

		public static UInt16 EnumToValue(FusesEnum e)
		{
			return (UInt16)e;
		}

		public static FusesMask ResolveFusesMask(uint? v)
		{
			switch (v)
			{
			case 0x1f:
			case null:
				return FusesMask.kFuse1F;
			case 0xf:
				return FusesMask.kFuse0F;
			case 0x7:
				return FusesMask.kFuse07;
			case 0x3:
				return FusesMask.kFuse03;
			case 0x1:
				return FusesMask.kFuse01;
			default:
				throw new InvalidDataException("Cannot resolve fuse mask = 0x" + ((uint)v).ToString("x2"));
			}
		}

		public static ConfigMask ResolveConfigMask(uint? v)
		{
			switch (v)
			{
			case null:
				return ConfigMask.kCfgNoMask;
			case 0x7f:
				return ConfigMask.kCfg7F;
			default:
				throw new InvalidDataException("Cannot resolve config mask = 0x" + ((uint)v).ToString("x2"));
			}
		}

		public static string UnMapMemoryNameType(MemoryNameType i)
		{
			//return from_enum_name_type_to_string[(int)i];
			return Enum.GetName(typeof(MemoryNameType), i);
		}

		public static string MakeMemoryTypeTypeKey(MemoryTypeType2 i)
		{
			return Enum.GetName(typeof(MemoryTypeType2), i);
		}

		public static string MakeBitSizeKey(BitSize i)
		{
			return Enum.GetName(typeof(BitSize), i);
		}

		public static string MakeMemAccessTypeKey(MemAccessType i)
		{
			return Enum.GetName(typeof(MemAccessType), i);
		}

		static uint[] from_enum_to_address =
		{
			0x000FFFFF
			, 0x0
			, 0x6
			, 0x20
			, 0x90
			, 0x100
			, 0x200
			, 0xa80
			, 0xb00
			, 0xc00
			, 0xe00
			, 0xf00
			, 0x1000
			, 0x1100
			, 0x1800
			, 0x1900
			, 0x1a00
			, 0x1c00
			, 0x1c80
			, 0x1e00
			, 0x2000
			, 0x2100
			, 0x2380
			, 0x2400
			, 0x2500
			, 0x2c00
			, 0x3100
			, 0x4000
			, 0x4400
			, 0x5c00
			, 0x6000
			, 0x6c00
			, 0x8000
			, 0xa000
			, 0xa400
			, 0xc000
			, 0xc200
			, 0xc400
			, 0xe000
			, 0xe300
			, 0xf000
			, 0xf100
			, 0xf800
			, 0xf840
			, 0xf880
			, 0xfc00
			, 0xfe00
			, 0xffe0
			, 0xc0000
			, 0xf0000
			, 0xf8000
			, 0xfac00
			, 0xffc00
		};
		static uint[] from_enum_to_block_size =
		{
			0
			, 0x6
			, 0xd
			, 0x10
			, 0x15
			, 0x1a
			, 0x20
			, 0x40
			, 0x60
			, 0x80
			, 0x100
			, 0x200
			, 0x300
			, 0x400
			, 0x500
			, 0x600
			, 0x760
			, 0x780
			, 0x7c0
			, 0x7e0
			, 0x800
			, 0xa00
			, 0xa60
			, 0xe00
			, 0xf00
			, 0xf80
			, 0xfe0
			, 0x1000
			, 0x1400
			, 0x1c00
			, 0x1d00
			, 0x1800
			, 0x2000
			, 0x2800
			, 0x3000
			, 0x3c00
			, 0x3e00
			, 0x4000
			, 0x5000
			, 0x6000
			, 0x8000
			, 0xc000
			, 0xbc00
			, 0xdb00
			, 0xdf00
			, 0xe000
			, 0xef00
			, 0xfc00
			, 0x10000
			, 0x16f00
			, 0x18000
			, 0x1cf00
			, 0x1df00
			, 0x20000
			, 0x30000
			, 0x40000
			, 0x1fc00
			, 0x17c00
			, 0x60000
			, 0x80000
		};
		static uint[] from_enum_to_seg_size =
		{
			0
			, 0x1
			, 0x40
			, 0x80
			, 0x200
			, 0x400
		};
		static UInt16[] from_enum_to_revision =
		{
			0xff
			, 0x00
			, 0x02
			, 0x10
			, 0x13
			, 0x20
			, 0x21
		};
		static UInt16[] from_enum_to_config =
		{
			0xff
			, 0x00
			, 0x01
			, 0x02
			, 0x03
			, 0x45
			, 0x47
			, 0x57
		};
		static string[] from_enum_name_type_to_string =
		{
			"None",
			"Main",
			"Info",
			"Bsl",
			"Bsl2",
			"BootCode",
			"BootCode2",
			"Ram",
			"Ram2",
			"TinyRam",
			"UsbRam",
			"Lcd",
			"Cpu",
			"Eem",
			"Cpu",
			"Cpu1",
			"Peripheral8bit",
			"Peripheral16bit",
			"Peripheral16bit1",
			"Peripheral16bit2",
			"Peripheral16bit3",
			"IrVec",
			"Lib",
			"LeaPeripheral",
			"LeaRam",
			"MidRom",
			"UssPeripheral",
		};
}
}
