using Dapper;
using Microsoft.Data.Sqlite;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateEmmClocksTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE EemClocks (
					PartNumber TEXT NOT NULL,
					[Index] INTEGER,
					Value TEXT
				)
			";
			cmd.ExecuteNonQuery();
		}

		void ExportEmmClocks(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.EemClocks_ != null && rec.EemClocks_.EemClocks != null)
			{
				foreach (var t in rec.EemClocks_.EemClocks)
				{
					t.PartNumber = rec.PartNumber;
					conn.Execute(
						@"INSERT INTO EemClocks (
								PartNumber,
								[Index],
								Value
							) VALUES (
								@PartNumber,
								@Index,
								@Value
							)", t);
				}
			}
		}
	}
}
