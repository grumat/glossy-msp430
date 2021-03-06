using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml.Serialization;
using System.Linq;

namespace MakeChipInfoDB
{

	class XmlManager
	{
		public IdCodes IdCodes_ = new IdCodes();
		public EemTimers EemTimers_ = new EemTimers();
		public EemTimerCfgs EemTimerCfgs_ = new EemTimerCfgs();
		public EemTimerDB EemTimerDB_ = new EemTimerDB();
		public Features Feats_ = new Features();
		public ExtFeatures ExtFeats_ = new ExtFeatures();
		public ClockInfos Clocks_ = new ClockInfos();
		public Memories Mems_ = new Memories();
		public MemoryLayouts Lyts_ = new MemoryLayouts();
		public PowerSettingsTable Pwr_ = new PowerSettingsTable();
		public Devices Devs_ = new Devices();
		/// <summary>
		/// Reads a single MSP430 XML file 
		/// </summary>
		/// <param name="fname"></param>
		/// <returns></returns>
		static DeviceInformation ReadSingleXml(string fname)
		{
			XmlSerializer serializer = new XmlSerializer(typeof(DeviceInformation));
			StreamReader reader = new StreamReader(fname, Encoding.Latin1);
			DeviceInformation devs = (DeviceInformation)serializer.Deserialize(reader);
			reader.Close();
			return devs;
		}

#if TEST
		void MergeXml(DeviceInformation info)
		{
			Db_.ClockInfos.AddRange(info.ClockInfos);
			Db_.Devices.AddRange(info.Devices);
			Db_.Eems.AddRange(info.Eems);
			Db_.EemClocks.AddRange(info.EemClocks);
			Db_.EemTimerItems.AddRange(info.EemTimerItems);
			Db_.EemTimers.AddRange(info.EemTimers);
			Db_.ExtFeatures.AddRange(info.ExtFeatures);
			Db_.Features.AddRange(info.Features);
			Db_.FuncletMaps.AddRange(info.FuncletMaps);
			Db_.FunctionMaps.AddRange(info.FunctionMaps);
			Db_.IdCodes.AddRange(info.IdCodes);
			Db_.IdMasks.AddRange(info.IdMasks);
			Db_.Imports.AddRange(info.Imports);
			Db_.Memories.AddRange(info.Memories);
			Db_.MemoryLayouts.AddRange(info.MemoryLayouts);
			Db_.PowerSettings.AddRange(info.PowerSettings);
			Db_.VoltageInfos.AddRange(info.VoltageInfos);
		}
#endif

		public void LoadXml(string fname)
		{
			DeviceInformation info = ReadSingleXml(fname);
			CompactMemoryType(info);
		}

		void CompactMemoryType(DeviceInformation info)
		{
			foreach (var idcode in info.IdCodes)
			{
				IdCode toadd = new IdCode(idcode, IdCodes_.Items);
				IdCodes_.AddItem(toadd);
			}
			foreach (var tt in info.EemTimerItems)
			{
				EemTimer toadd = new EemTimer(tt, EemTimers_.Items);
				EemTimers_.AddItem(toadd);
			}
			foreach (var tim in info.EemTimers)
			{
				EemTimerCfg toadd = new EemTimerCfg(tim, EemTimerCfgs_.Items, ref EemTimers_);
				EemTimerCfgs_.AddItem(toadd);
			}
			foreach (var feat in info.Features)
			{
				Feature toadd = new Feature(feat, Feats_.Items);
				Feats_.AddItem(toadd);
			}
			foreach (var ef in info.ExtFeatures)
			{
				ExtFeature toadd = new ExtFeature(ef, ExtFeats_.Items);
				ExtFeats_.AddItem(toadd);
			}
			foreach (var gcc in info.ClockInfos)
			{
				ClockInfo toadd = new ClockInfo(gcc, Clocks_.Items, ref EemTimerCfgs_, ref EemTimers_);
				Clocks_.AddItem(toadd);
			}
			// Copy elements in hierarchical order
			foreach (var m in info.Memories)
			{
				Memory toadd = new Memory(m, Mems_.MemAliases_);
				Mems_.AddItem(toadd);
			}
			foreach (memoryLayoutType lyt in info.MemoryLayouts)
			{
				string lid = MemoryLayout.GetIdentifier(lyt.id);
				MemoryLayout toadd = new MemoryLayout(lyt, lid, Lyts_, ref Mems_);
				Lyts_.AddItem(toadd);
			}
			foreach (powerSettingsType pwr in info.PowerSettings)
			{
				PowerSettings toadd = new PowerSettings(pwr, ref Pwr_);
			}
			foreach (deviceType dev in info.Devices)
			{
				Device toadd = new Device(dev, Devs_, ref Lyts_, ref Mems_, ref Feats_
					, ref ExtFeats_, ref Clocks_, ref IdCodes_, ref EemTimerCfgs_, ref EemTimers_
					, ref EemTimerDB_, ref Pwr_);
				Devs_.AddItem(toadd);
			}
		}
	}
}
