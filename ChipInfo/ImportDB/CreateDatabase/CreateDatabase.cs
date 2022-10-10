using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Serialization;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateTables(SqliteConnection conn)
		{
			using var cmd = new SqliteCommand();
			cmd.Connection = conn;
			CreateSlasTables(cmd);
			CreateChipsTables(cmd);
			CreateEmmTimersTables(cmd);
			CreateEmmClocksTables(cmd);
			CreateFeaturesTables(cmd);
			CreateExtFeaturesTables(cmd);
			CreatePowerSettingsTables(cmd);
			CreateMemoryLayoutTables(cmd);
		}

		void CreateViews(SqliteConnection conn)
		{
			using var cmd = new SqliteCommand();
			cmd.Connection = conn;
			CreateMemoryLayoutViews(cmd);
		}

		public void ValidateDatabases()
		{
			foreach (deviceType part in XmlDatabase.AllInfos.Devices)
			{
				if (String.IsNullOrEmpty(part.description))
					continue;
				Slas.DatasheetAttr slas = Slas.AllDevices.LocateEntry(part.description);
				if (slas == null)
				{
					//throw new NotSupportedException(String.Format("Part number '{0}' has no match in the CSV database", xml_partname));
					Console.WriteLine("  Part number '{0}' has no match in the CSV database", part.description);
				}
			}
		}

		public void MakeSqlite(string fname)
		{
			if (File.Exists(fname))
			{
				Console.WriteLine("  Deleting previous file instance");
				File.Delete(fname);
			}
			Console.WriteLine("  Connecting to new database");
			var connectionString = new SqliteConnectionStringBuilder()
			{
				DataSource = fname,
				Mode = SqliteOpenMode.ReadWriteCreate
			}.ToString();
			SqliteConnection conn = new SqliteConnection(connectionString);
			conn.Open();
			Console.WriteLine("  Creating tables");
			CreateTables(conn);
			Console.WriteLine("  Creating views");
			CreateViews(conn);
			using (var xact = conn.BeginTransaction(IsolationLevel.ReadUncommitted))
			{
				Console.Write("  Loading Flash Timing data... ");
				ExportSlasTiming(conn);
				Console.Write("  Loading Device data... ");
				int cnt = 0;
				foreach (deviceType part in XmlDatabase.AllInfos.Devices)
				{
					if (String.IsNullOrEmpty(part.description))
						continue;
					++cnt;
					Model.Chips rec = new Model.Chips();
					rec.Fill(part);
					rec.EemTimers = ExportPackedEmmTimers(conn, rec);
					rec.PowerSettings = ExportPackedPowerSettings(conn, rec);
					ExportChips(conn, rec);
					ExportEmmTimers(conn, rec);
					ExportEmmClocks(conn, rec);
					ExportFeatures(conn, rec);
					ExportExtFeatures(conn, rec);
					ExportPowerSettings(conn, rec);
					ExportMemoryLayout(conn, rec);
				}
				Console.WriteLine("{0} devices added.", cnt);

				Console.WriteLine("  Committing data");
				xact.Commit();
			}
			conn.Close();
		}
	}
}
