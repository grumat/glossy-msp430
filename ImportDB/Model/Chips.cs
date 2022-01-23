using System;
using System.Linq;

namespace ImportDB.Model
{
	class Chips
	{
		public string PartNumber { get; set; }
		public string Psa { get; set; }
		public int Bits { get; set; }
		public string Architecture { get; set; }
		public string DataSheet { get; set; }
		public string UsersGuide { get; set; }
		public string EemType { get; set; }

		// Fields from idCodeType
		public UInt16? Version { get; set; }
		public UInt16? VersionMask { get; set; }
		public UInt16? Subversion { get; set; }
		public UInt16? SubversionMask { get; set; }
		public Byte? Revision { get; set; }
		public Byte? RevisionMask { get; set; }
		public Byte? MaxRevision { get; set; }
		public Byte? MaxRevisionMask { get; set; }
		public Byte? Fab { get; set; }
		public Byte? FabMask { get; set; }
		public UInt16? Self { get; set; }
		public UInt16? SelfMask { get; set; }
		public Byte? Config { get; set; }
		public Byte? ConfigMask { get; set; }
		public Byte? Fuses { get; set; }
		public Byte? FusesMask { get; set; }
		public UInt32? ActivationKey { get; set; }
		public UInt32? ActivationKeyMask { get; set; }

		// Clock and timers
		public string ClockControl { get; set; }
		public Timers EemTimers_;
		public int? EemTimers { get; set; }
		public Clocks EemClocks_;

		// Voltages
		public UInt16? VccMin { get; set; }
		public UInt16? VccMax { get; set; }
		public UInt16? VccFlashMin { get; set; }
		public UInt16? VccSecureMin { get; set; }
		public bool? TestVpp { get; set; }

		// Features
		public Features Features_;
		public bool QuickMemRead { get; set; }
		public bool StopFllDbg { get; set; }

		// Ext Features
		public ExtFeatures ExtFeatures_;
		public bool Issue1377 { get; set; }
		public bool Jtag { get; set; }

		// Power Settings
		public PowerSettings PowerSettings_;
		public int? PowerSettings { get; set; }

		// Memory Layout
		public MemoryLayout MemoryLayout_;

		public void Fill(deviceType part)
		{
			if (!string.IsNullOrEmpty(part.@ref))
			{
				var @base = XmlDatabase.AllInfos.Devices.First(x => x.id == part.@ref);
				Fill(@base);
			}
			PartNumber = part.description;
			if (part.psaSpecified)
				Psa = Enum.GetName(typeof(psaType), part.psa);
			if (part.bitsSpecified)
				Bits = (int)part.bits;
			if (part.architectureSpecified)
				Architecture = Enum.GetName(typeof(ArchitectureType), part.architecture);
			if (part.eemSpecified)
				EemType = Enum.GetName(typeof(EemType), part.eem);
			if (part.idCode != null)
				FillId(part.idCode);
			if (part.clockInfo != null)
				Fill(part.clockInfo);
			if (part.voltageInfo != null)
				Fill(part.voltageInfo);
			if (part.features != null)
				Fill(part.features);
			if (part.extFeatures != null)
				Fill(part.extFeatures);
			if (part.powerSettings != null)
				Fill(part.powerSettings);
			if (part.memoryLayout != null)
				Fill(part.memoryLayout);
			if (!String.IsNullOrEmpty(PartNumber))
			{
				Slas.DatasheetAttr sheet = Slas.AllDevices.LocateEntry(PartNumber);
				if (sheet != null)
				{
					DataSheet = sheet.Manual;
					UsersGuide = sheet.FamilyUsersGuide;
				}
			}
		}

		void FillId(idCodeType top)
		{
			IdCode rec = new IdCode();
			rec.Fill(top);
			if (rec.Version != null)
				Version = rec.Version;
			if (rec.Subversion != null)
				Subversion = rec.Subversion;
			if (rec.Revision != null)
				Revision = rec.Revision;
			if (rec.MaxRevision != null)
				MaxRevision = rec.MaxRevision;
			if (rec.Fab != null)
				Fab = rec.Fab;
			if (rec.Self != null)
				Self = rec.Self;
			if (rec.Config != null)
				Config = rec.Config;
			if (rec.Fuses != null)
				Fuses = rec.Fuses;
			if (rec.ActivationKey != null)
				ActivationKey = rec.ActivationKey;
		}

		void FillIdMask(idCodeType top)
		{
			IdCode rec = new IdCode();
			rec.FillMask(top);
			if (rec.Version != null)
				Version = rec.Version;
			if (rec.Subversion != null)
				Subversion = rec.Subversion;
			if (rec.Revision != null)
				Revision = rec.Revision;
			if (rec.MaxRevision != null)
				MaxRevision = rec.MaxRevision;
			if (rec.Fab != null)
				Fab = rec.Fab;
			if (rec.Self != null)
				Self = rec.Self;
			if (rec.Config != null)
				Config = rec.Config;
			if (rec.Fuses != null)
				Fuses = rec.Fuses;
			if (rec.ActivationKey != null)
				ActivationKey = rec.ActivationKey;
		}

		void Fill(clockInfoType top)
		{
			ClockInfo rec = new ClockInfo();
			rec.Fill(top);
			if (rec.ClockControl != null)
				ClockControl = rec.ClockControl;
			// Timers are only relevant on the CPUXv2
			if (rec.EemTimers != null && Architecture == "CpuXv2")
				EemTimers_ = rec.EemTimers;
			// Clocks
			if (rec.EemClocks != null)
			{
				EemClocks_ = rec.EemClocks;
				// Clear info
				if (EemClocks_.EemClocks == null || EemClocks_.EemClocks.Count == 0)
					EemClocks_ = null;
			}
		}

		void Fill(voltageInfoType top)
		{
			Voltages rec = new Voltages();
			rec.Fill(top);
			if (rec.VccMin != null)
				VccMin = rec.VccMin;
			if (rec.VccMax != null)
				VccMax = rec.VccMax;
			if (rec.VccFlashMin != null)
				VccFlashMin = rec.VccFlashMin;
			if (rec.VccSecureMin != null)
				VccSecureMin = rec.VccSecureMin;
			if (rec.TestVpp != null)
				TestVpp = rec.TestVpp;
		}

		void Fill(featuresType top)
		{
			Features_ = new Features();
			Features_.Fill(top);
			QuickMemRead = Features_.QuickMemRead;
			StopFllDbg = Features_.StopFllDbg;
		}

		void Fill(extFeaturesType top)
		{
			ExtFeatures_ = new ExtFeatures();
			ExtFeatures_.Fill(top);
			Issue1377 = ExtFeatures_.Issue1377;
			Jtag = ExtFeatures_.Jtag;
		}

		void Fill(powerSettingsType top)
		{
			PowerSettings_ = new PowerSettings();
			PowerSettings_.Fill(top);
			if (PowerSettings_.IsClear())
				PowerSettings_ = null;
		}

		void Fill(memoryLayoutType top)
		{
			MemoryLayout_ = new MemoryLayout();
			MemoryLayout_.Fill(top);
		}
	}
}
