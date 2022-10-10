using Dapper;
using Microsoft.Data.Sqlite;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateFeaturesTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE Features (
					PartNumber TEXT NOT NULL,
					ClockSystem TEXT NOT NULL,
					Lcfe INTEGER NOT NULL,
					QuickMemRead INTEGER NOT NULL,
					I2C INTEGER NOT NULL,
					StopFllDbg INTEGER NOT NULL,
					HasFram INTEGER NOT NULL
				)
			";
			cmd.ExecuteNonQuery();
		}

		void ExportFeatures(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.Features_ != null)
			{
				rec.Features_.PartNumber = rec.PartNumber;
				conn.Execute(
					@"INSERT INTO Features (
								PartNumber,
								ClockSystem,
								Lcfe,
								QuickMemRead,
								I2C,
								StopFllDbg,
								HasFram
							) VALUES (
								@PartNumber,
								@ClockSystem,
								@Lcfe,
								@QuickMemRead,
								@I2C,
								@StopFllDbg,
								@HasFram
							)", rec.Features_);
			}
		}
	}
}
