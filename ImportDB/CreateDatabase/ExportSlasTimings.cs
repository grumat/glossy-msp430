using Dapper;
using Microsoft.Data.Sqlite;
using System;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateSlasTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE SlasTiming (
					Manual TEXT,
					Chip TEXT,
					FamilyUsersGuide  TEXT,
					VccPgmMin NUMERIC,
					VccPgmMax NUMERIC,
					fFtgMin INTEGER,
					fFtgMax INTEGER,
					tWord INTEGER,
					tBlock0 INTEGER,
					tBlockI INTEGER,
					tBlockN INTEGER,
					tMassErase INTEGER,
					tSegErase INTEGER,
					fTck2_2V INTEGER,
					fTck3V INTEGER,
					fSbw INTEGER,
					fSbwEn INTEGER,
					fSbwRet INTEGER
				)
			";
			cmd.ExecuteNonQuery();
		}
		void ExportSlasTiming(SqliteConnection conn)
		{
			foreach (var r in Slas.AllDevices.Infos)
			{
				conn.Execute(
					@"INSERT INTO SlasTiming (
						Manual,
						Chip,
						FamilyUsersGuide,
						VccPgmMin,
						VccPgmMax,
						fFtgMin,
						fFtgMax,
						tWord,
						tBlock0,
						tBlockI,
						tBlockN,
						tMassErase,
						tSegErase,
						fTck2_2V,
						fTck3V,
						fSbw,
						fSbwEn,
						fSbwRet
					) VALUES (
						@Manual,
						@Chip,
						@FamilyUsersGuide,
						@VccPgmMin,
						@VccPgmMax,
						@fFtgMin,
						@fFtgMax,
						@tWord,
						@tBlock0,
						@tBlockI,
						@tBlockN,
						@tMassErase,
						@tSegErase,
						@fTck2_2V,
						@fTck3V,
						@fSbw,
						@fSbwEn,
						@fSbwRet
					)
				", r);
			}
			Console.WriteLine("{0} records added.", Slas.AllDevices.Infos.Count);
		}
	}
}
