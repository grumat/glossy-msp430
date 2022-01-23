using Dapper;
using Microsoft.Data.Sqlite;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateChipsTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE Chips (
					PartNumber TEXT NOT NULL,
					Psa TEXT NOT NULL,
					Bits INTEGER NOT NULL,
					Architecture TEXT NOT NULL,
					DataSheet TEXT NOT NULL,
					UsersGuide TEXT NOT NULL,
					EemType TEXT NOT NULL,
					Version INTEGER NOT NULL,
					VersionMask INTEGER,
					Subversion INTEGER,
					SubversionMask INTEGER,
					Revision INTEGER,
					RevisionMask INTEGER,
					MaxRevision INTEGER,
					MaxRevisionMask INTEGER,
					Fab INTEGER,
					FabMask INTEGER,
					Self INTEGER,
					SelfMask INTEGER,
					Config INTEGER,
					ConfigMask INTEGER,
					Fuses INTEGER,
					FusesMask INTEGER,
					ActivationKey INTEGER,
					ActivationKeyMask INTEGER,
					ClockControl TEXT NOT NULL,
					EemTimers INTEGER,
					VccMin INTEGER NOT NULL,
					VccMax INTEGER NOT NULL,
					VccFlashMin INTEGER,
					VccSecureMin INTEGER,
					TestVpp INTEGER NOT NULL,
					QuickMemRead INTEGER NOT NULL,
					StopFllDbg INTEGER NOT NULL,
					Issue1377 INTEGER NOT NULL,
					Jtag INTEGER NOT NULL,
					PowerSettings INTEGER
				)
			";
			cmd.ExecuteNonQuery();
		}

		void ExportChips(SqliteConnection conn, Model.Chips rec)
		{
			conn.Execute(
				@"INSERT INTO Chips (
					PartNumber,
					Psa,
					Bits,
					Architecture,
					DataSheet,
					UsersGuide,
					EemType,
					Version,
					VersionMask,
					Subversion,
					SubversionMask,
					Revision,
					RevisionMask,
					MaxRevision,
					MaxRevisionMask,
					Fab,
					FabMask,
					Self,
					SelfMask,
					Config,
					ConfigMask,
					Fuses,
					FusesMask,
					ActivationKey,
					ActivationKeyMask,
					ClockControl,
					EemTimers,
					VccMin,
					VccMax,
					VccFlashMin,
					VccSecureMin,
					TestVpp,
					QuickMemRead,
					StopFllDbg,
					Issue1377,
					Jtag,
					PowerSettings
				) VALUES (
					@PartNumber,
					@Psa,
					@Bits,
					@Architecture,
					@DataSheet,
					@UsersGuide,
					@EemType,
					@Version,
					@VersionMask,
					@Subversion,
					@SubversionMask,
					@Revision,
					@RevisionMask,
					@MaxRevision,
					@MaxRevisionMask,
					@Fab,
					@FabMask,
					@Self,
					@SelfMask,
					@Config,
					@ConfigMask,
					@Fuses,
					@FusesMask,
					@ActivationKey,
					@ActivationKeyMask,
					@ClockControl,
					@EemTimers,
					@VccMin,
					@VccMax,
					@VccFlashMin,
					@VccSecureMin,
					@TestVpp,
					@QuickMemRead,
					@StopFllDbg,
					@Issue1377,
					@Jtag,
					@PowerSettings
				)"
				, rec);
		}
	}
}
