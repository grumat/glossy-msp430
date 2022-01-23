using Dapper;
using Microsoft.Data.Sqlite;
using System.Collections.Generic;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreatePowerSettingsTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE PowerSettings (
					PartNumber TEXT NOT NULL,
					TestRegMask INTEGER,
					TestRegDefault INTEGER,
					TestRegEnableLpm5 INTEGER,
					TestRegDisableLpm5 INTEGER,
					TestReg3VMask INTEGER,
					TestReg3VDefault INTEGER,
					TestReg3VEnableLpm5 INTEGER,
					TestReg3VDisableLpm5 INTEGER
				)
			";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE TABLE PowerSettingsPacked (
					SettingsPK INTEGER NOT NULL,
					TestRegMask INTEGER,
					TestRegDefault INTEGER,
					TestRegEnableLpm5 INTEGER,
					TestRegDisableLpm5 INTEGER,
					TestReg3VMask INTEGER,
					TestReg3VDefault INTEGER,
					TestReg3VEnableLpm5 INTEGER,
					TestReg3VDisableLpm5 INTEGER
				)
			";
			cmd.ExecuteNonQuery();
		}

		void ExportPowerSettings(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.PowerSettings_ != null)
			{
				rec.PowerSettings_.PartNumber = rec.PartNumber;
				conn.Execute(
					@"INSERT INTO PowerSettings (
								PartNumber,
								TestRegMask,
								TestRegDefault,
								TestRegEnableLpm5,
								TestRegDisableLpm5,
								TestReg3VMask,
								TestReg3VDefault,
								TestReg3VEnableLpm5,
								TestReg3VDisableLpm5
							) VALUES (
								@PartNumber,
								@TestRegMask,
								@TestRegDefault,
								@TestRegEnableLpm5,
								@TestRegDisableLpm5,
								@TestReg3VMask,
								@TestReg3VDefault,
								@TestReg3VEnableLpm5,
								@TestReg3VDisableLpm5
							)", rec.PowerSettings_);
			}
		}

		Dictionary<string, int> PackedPowerSettings = new Dictionary<string, int>();
		int? ExportPackedPowerSettings(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.PowerSettings_ != null)
			{
				string key = rec.PowerSettings_.MkIdentity();
				if (PackedPowerSettings.ContainsKey(key))
					return PackedPowerSettings[key];
				int idx = PackedPowerSettings.Count;
				conn.Execute(
					@"INSERT INTO PowerSettingsPacked (
								SettingsPK,
								TestRegMask,
								TestRegDefault,
								TestRegEnableLpm5,
								TestRegDisableLpm5,
								TestReg3VMask,
								TestReg3VDefault,
								TestReg3VEnableLpm5,
								TestReg3VDisableLpm5
							) VALUES (
								@SettingsPK,
								@TestRegMask,
								@TestRegDefault,
								@TestRegEnableLpm5,
								@TestRegDisableLpm5,
								@TestReg3VMask,
								@TestReg3VDefault,
								@TestReg3VEnableLpm5,
								@TestReg3VDisableLpm5
							)"
					, new
					{
						SettingsPK = idx,
						TestRegMask = rec.PowerSettings_.TestRegMask,
						TestRegDefault = rec.PowerSettings_.TestRegDefault,
						TestRegEnableLpm5 = rec.PowerSettings_.TestRegEnableLpm5,
						TestRegDisableLpm5 = rec.PowerSettings_.TestRegDisableLpm5,
						TestReg3VMask = rec.PowerSettings_.TestReg3VMask,
						TestReg3VDefault = rec.PowerSettings_.TestReg3VDefault,
						TestReg3VEnableLpm5 = rec.PowerSettings_.TestReg3VEnableLpm5,
						TestReg3VDisableLpm5 = rec.PowerSettings_.TestReg3VDisableLpm5
					}
				);
				PackedPowerSettings.Add(key, idx);
				return idx;
			}
			return null;
		}
	}
}
