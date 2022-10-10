using Dapper;
using Microsoft.Data.Sqlite;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateExtFeaturesTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE ExtFeatures (
					PartNumber TEXT NOT NULL,
					Tmr INTEGER NOT NULL,
					Jtag INTEGER NOT NULL,
					Dtc INTEGER NOT NULL,
					Sync INTEGER NOT NULL,
					Instr INTEGER NOT NULL,
					Issue1377 INTEGER NOT NULL,
					PsaCh INTEGER NOT NULL,
					NoEemLpm INTEGER NOT NULL
				)
			";
			cmd.ExecuteNonQuery();
		}

		void ExportExtFeatures(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.ExtFeatures_ != null)
			{
				rec.ExtFeatures_.PartNumber = rec.PartNumber;
				conn.Execute(
					@"INSERT INTO ExtFeatures (
								PartNumber,
								Tmr,
								Jtag,
								Dtc,
								Sync,
								Instr,
								Issue1377,
								PsaCh,
								NoEemLpm
							) VALUES (
								@PartNumber,
								@Tmr,
								@Jtag,
								@Dtc,
								@Sync,
								@Instr,
								@Issue1377,
								@PsaCh,
								@NoEemLpm
							)", rec.ExtFeatures_);
			}
		}
	}
}
