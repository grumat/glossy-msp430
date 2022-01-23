using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml.Serialization;

namespace ImportDB
{
	class XmlDatabase
	{
		public static DeviceInformation AllInfos;

		static void ImportXml(string fname)
		{
			XmlSerializer serializer = new XmlSerializer(typeof(DeviceInformation));
			StreamReader reader = new StreamReader(fname, Encoding.Latin1);
			DeviceInformation devs = (DeviceInformation)serializer.Deserialize(reader);
			reader.Close();
			if (AllInfos == null)
				AllInfos = devs;
			else
				MergeImport(devs);
		}

		static void MergeImport(DeviceInformation devs)
		{
			AllInfos.ClockInfos.AddRange(devs.ClockInfos);
			AllInfos.Devices.AddRange(devs.Devices);
			AllInfos.Eems.AddRange(devs.Eems);
			AllInfos.EemClocks.AddRange(devs.EemClocks);
			AllInfos.EemTimerItems.AddRange(devs.EemTimerItems);
			AllInfos.EemTimers.AddRange(devs.EemTimers);
			AllInfos.ExtFeatures.AddRange(devs.ExtFeatures);
			AllInfos.Features.AddRange(devs.Features);
			AllInfos.FuncletMaps.AddRange(devs.FuncletMaps);
			AllInfos.FunctionMaps.AddRange(devs.FunctionMaps);
			AllInfos.IdCodes.AddRange(devs.IdCodes);
			AllInfos.IdMasks.AddRange(devs.IdMasks);
			AllInfos.Imports.AddRange(devs.Imports);
			AllInfos.Memories.AddRange(devs.Memories);
			AllInfos.MemoryLayouts.AddRange(devs.MemoryLayouts);
			AllInfos.PowerSettings.AddRange(devs.PowerSettings);
			AllInfos.VoltageInfos.AddRange(devs.VoltageInfos);
			// This is weird design... But inner elements can be referenced from other scopes
			// This kinda wrap data layers!
			foreach (var r in devs.ClockInfos)
			{
				if(r.eemTimers != null && !String.IsNullOrEmpty(r.eemTimers.id))
					AllInfos.EemTimers.Add(r.eemTimers);
			}
			foreach (var r in devs.Devices)
			{
				if (r.idCode != null && !String.IsNullOrEmpty(r.idCode.id))
					AllInfos.IdCodes.Add(r.idCode);
				if (r.idMask != null && !String.IsNullOrEmpty(r.idMask.id))
					AllInfos.IdMasks.Add(r.idMask);
				if (r.clockInfo != null)
				{
					if(r.clockInfo.eemTimers != null && !String.IsNullOrEmpty(r.clockInfo.eemTimers.id))
						AllInfos.EemTimers.Add(r.clockInfo.eemTimers);
					if (!String.IsNullOrEmpty(r.clockInfo.id))
						AllInfos.ClockInfos.Add(r.clockInfo);
				}
				if (r.memoryLayout != null && !String.IsNullOrEmpty(r.memoryLayout.id))
					AllInfos.MemoryLayouts.Add(r.memoryLayout);
			}
		}

		static HashSet<string> clash_ = new HashSet<string>()
		{
			"defaults.xml", "p401x.xml", "legacy.xml"
		};

		public static void DoImport(string xml_folder)
		{
			Console.WriteLine("  Importing 'defaults.xml'");
			string deffile = Path.Combine(xml_folder, "defaults.xml");
			ImportXml(deffile);
			string[] all_files = Directory.GetFiles(xml_folder);
			Array.Sort(all_files, StringComparer.Ordinal);
			foreach (string fname in all_files)
			{
				// Just only xml files
				if (Path.GetExtension(fname).ToLower() != ".xml")
					continue;
				Console.WriteLine("  Importing '{0}'", Path.GetFileName(fname));
				// Skip files that are already loaded or outside of the interested scope
				if (clash_.Contains(Path.GetFileName(fname)))
					continue;
				// Add to import database
				ImportXml(fname);
			}
		}

	}
}
