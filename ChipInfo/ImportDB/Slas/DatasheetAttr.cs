using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using CsvHelper;
using CsvHelper.Configuration;

namespace ImportDB.Slas
{
	public class DatasheetAttr
	{
		public string Manual { get; set; }
		public string Chip { get; set; }
		// Family Users Guide
		public string FamilyUsersGuide { get; set; }
		public decimal? VccPgmMin { get; set; }
		public decimal? VccPgmMax { get; set; }
		public UInt16? fFtgMin { get; set; }
		public UInt16? fFtgMax { get; set; }
		public Byte? tWord { get; set; }
		public Byte? tCMErase { get; set; }
		public Byte? tBlock0 { get; set; }
		public Byte? tBlockI { get; set; }
		public Byte? tBlockN { get; set; }
		public UInt16? tMassErase { get; set; }
		public UInt16? tSegErase { get; set; }
		public Byte? fTck2_2V { get; set; }
		public Byte? fTck3V { get; set; }
		public Byte? fSbw { get; set; }
		public Byte? fSbwEn { get; set; }
		public Byte? fSbwRet { get; set; }
	}

	public enum EnumFlashTiming
	{
		None,
		FlashTimingGen1,
		FlashTimingGen2a,
		FlashTimingGen2b,
	}

	public class FlashTiming
	{
		public EnumFlashTiming Id { get; set; }
		public UInt16? tMassErase { get; set; }
		public UInt16? tSegErase { get; set; }
		public Byte? tWord { get; set; }
		public Byte? tCMErase { get; set; }

		public FlashTiming(EnumFlashTiming id, UInt16? mass_erase, UInt16? seg_erase, Byte? word, Byte? tcmerase)
		{
			Id = id;
			tMassErase = mass_erase;
			tSegErase = seg_erase;
			tWord = word;
			tCMErase = tcmerase;
		}

		public static string MkFlashTimingKey(UInt16? mass_erase, UInt16? seg_erase, Byte? word, Byte? tcmerase)
		{
			// Ignore tCMErase as this is directly bound to tMassErase (note SLAS383E pg.38 probably contains a type there. See contradiction on comment (2) on that table)
			return String.Format("{0}.{1}.{2}", mass_erase, seg_erase, word);
		}
		public static string MkFlashTimingKey(DatasheetAttr attr)
		{
			if (attr == null)
				return MkFlashTimingKey(null, null, null, null);
			return MkFlashTimingKey(attr.tMassErase, attr.tSegErase, attr.tWord, attr.tCMErase);
		}
		public string MkKey()
		{
			return MkFlashTimingKey(tMassErase, tSegErase, tWord, tCMErase);
		}
	}

	class AllDevices
	{
		public static List<DatasheetAttr> Infos;
		public static Dictionary<string, FlashTiming> Timings = new Dictionary<string, FlashTiming>();

		public static void DoImport(string fname)
		{
			CsvConfiguration config = new CsvConfiguration(CultureInfo.InvariantCulture)
			{
				Mode = CsvMode.Escape,
				Delimiter = "\t"
			};
			Console.WriteLine("  Reading Device timing specifications");
			using (FileStream fs = new FileStream(fname, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
			using (StreamReader reader = new StreamReader(fs))
			using (var csv = new CsvReader(reader, config))
			{
				Infos = new List<DatasheetAttr>(csv.GetRecords<DatasheetAttr>());
			}
			// The static timing table
			Console.WriteLine("  Building Flash-Timing parameters for Family F1xx, F2xx and F4xx");
			MkTimingIdentity();
		}

		static void MkTimingIdentity()
		{
			/*
			** This table is very stable. It will be hard-coded and protected by exceptions
			** in the case a future database upgrade changes rules. Firmware will hard code
			** constants.
			** This will possibly never happen, since table applies to legacy families using
			** CPU and CPUX architectures. It is *not* used in CPUXv2, which includes an 
			** embedded clock generator for flash operations.
			*/
			FlashTiming tmp = new FlashTiming(EnumFlashTiming.None, null, null, null, null);
			Timings.Add(tmp.MkKey(), tmp);
			tmp = new FlashTiming(EnumFlashTiming.FlashTimingGen1, 5297, 4819, 35, 200);
			Timings.Add(tmp.MkKey(), tmp);
			tmp = new FlashTiming(EnumFlashTiming.FlashTimingGen2a, 10593, 4819, 30, 20);
			Timings.Add(tmp.MkKey(), tmp);
			tmp = new FlashTiming(EnumFlashTiming.FlashTimingGen2b, 10593, 9628, 25, 20);
			Timings.Add(tmp.MkKey(), tmp);
			// Validation
			foreach (var p in Infos)
			{
				if (!Timings.ContainsKey(FlashTiming.MkFlashTimingKey(p)))
					throw new InvalidDataException("Database upgrade changed Flash timings on static data. Please review!");
			}
		}

		// This is required since TI database contains part numbers with wildcards or even
		// confidential and unreleased parts.
		static Dictionary<string, string> RemapPartName = new Dictionary<string, string>()
		{
			{ "MSP430F11x1", "MSP430F1121A" },
			{ "MSP430F11x1A", "MSP430F1121A" },
			{ "MSP430F11x2", "MSP430F1132" },
			{ "MSP430F12x", "MSP430F123" },
			{ "MSP430F12x2/F11x2", "MSP430F1232" },
			{ "F20x1_G2x0x_G2x1x", "MSP430F2011" },
			{ "F20x2_G2x2x_G2x3x", "MSP430F2012" },
			{ "MSP430F20x3", "MSP430F2013" },
			{ "MSP430F21x1", "MSP430F2131" },
			{ "MSP430F2272_G2744", "MSP430F2272" },
			{ "MSP430F2252_G2544", "MSP430F2252" },
			{ "MSP430F2232_G2444", "MSP430F2232" },
			{ "MSP430G2xx2", "MSP430G2452" },
			{ "MSP430G2xx3", "MSP430G2533" },
			{ "MSP430G2x55", "MSP430G2955" },
			{ "MSP430F41x", "MSP430F417" },
			{ "MSP430F42x0", "MSP430F4270" },
			{ "MSP430FG42x0", "MSP430FG4270" },
			{ "MSP430F4230", "MSP430F423A" },	// not sure...
			{ "MSP430AFE250", "MSP430AFE251" },
			{ "MSP430AFE230", "MSP430AFE231" },
			{ "MSP430AFE220", "MSP430AFE221" },
			{ "MSP430FW42x/F41x", "MSP430FW429" },
			{ "MSP430F43x", "MSP430F439" },
			{ "MSP430FG43x_F43x", "MSP430FG439" },
			{ "MSP430FE42x2", "MSP430FE4272" },
			{ "MSP430F44x", "MSP430F449" },
			{ "MSP430F5213", "MSP430F5214" },
			{ "MSP430F5218", "MSP430F5219" },
			{ "MSP430F5223", "MSP430F5222" },
			{ "MSP430F5228", "MSP430F5229" },
			{ "MSP430SL5438A", "MSP430F5438A" },
			{ "MSP430F6722", "MSP430F6723" },
			{ "MSP430F6732", "MSP430F6733" },
			{ "MSP430FR5929", "MSP430FR5922" },
			{ "MSP430I204x_I203x_I202x", "MSP430I2041" },
			{ "RF430F5175", "RF430FRL154H" },	// confidential part..
			{ "RF430F5155", "RF430FRL154H" },	// confidential part..
			{ "RF430F5144", "RF430FRL154H" },	// confidential part..
		};
		// Binds XML part number to CSV database, as a couple of cases needs special handling
		public static DatasheetAttr LocateEntry(string xml_partname)
		{
			if (RemapPartName.ContainsKey(xml_partname))
				xml_partname = RemapPartName[xml_partname];
			var hit = Infos.FirstOrDefault(x => x.Chip == xml_partname);
			return hit;
		}

		public static EnumFlashTiming LocateFlashTimings(string xml_partname)
		{
			if(xml_partname == null)
				return Timings[FlashTiming.MkFlashTimingKey(null)].Id;
			DatasheetAttr tmp = LocateEntry(xml_partname);
			return Timings[FlashTiming.MkFlashTimingKey(tmp)].Id;
		}
	}

}
