using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.Linq;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateMemoryLayoutTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE Memories (
					PartNumber TEXT NOT NULL,
					MemoryName TEXT NOT NULL,
					MemoryType TEXT NOT NULL,
					Bits INTEGER,
					Start INTEGER,
					Size INTEGER,
					SegmentSize INTEGER,
					Banks INTEGER,
					Mapped INTEGER,
					Mpu INTEGER,
					Mask INTEGER,
					Protectable INTEGER,
					AccessType TEXT,
					WpAddress INTEGER,
					WpBits INTEGER,
					WpMask INTEGER,
					WpPwd INTEGER
				)
			";
			cmd.ExecuteNonQuery();
		}

		void CreateMemoryLayoutViews(SqliteCommand cmd)
		{
#if DISABLE_THIS
			cmd.CommandText = @"
				CREATE VIEW AllMemTypes AS 
				SELECT DISTINCT
					MemoryType, 
					Bits, 
					Start, 
					Size, 
					SegmentSize, 
					Banks, 
					Mapped, 
					Mpu, 
					Protectable, 
					AccessType, 
					WpAddress, 
					WpBits, 
					WpMask, 
					WpPwd
				FROM
					Memories
				ORDER BY 1, 2, 3, 4
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE VIEW AllMemTypesPacked AS 
				SELECT DISTINCT
					MemoryType, 
					Bits, 
					Start, 
					Size, 
					SegmentSize, 
					Banks,
					Mapped,
					Mpu,
					Protectable,
					AccessType
				FROM
					Memories
				ORDER BY 1, 2, 3, 4
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE VIEW AllBootCode AS 
				SELECT DISTINCT
					MemoryType, 
					Bits, 
					Start, 
					Size, 
					SegmentSize, 
					Banks, 
					Mapped, 
					Protectable, 
					AccessType
				FROM
					Memories
				WHERE
					MemoryName = ""BootCode""
				ORDER BY 1, 2, 3, 4
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE VIEW AllMainMem AS 
				SELECT DISTINCT
					MemoryType, 
					Bits, 
					Start, 
					Size, 
					SegmentSize, 
					Banks, 
					AccessType
				FROM
					Memories
				WHERE
					MemoryName == 'Main'
				ORDER BY 1, 2, 3, 4
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE VIEW ChipsMainMem AS 
				SELECT
					PartNumber,
					MemoryName,
					MemoryType, 
					Bits, 
					Start, 
					Size, 
					SegmentSize, 
					Banks, 
					AccessType
				FROM
					Memories
				WHERE
					MemoryName == 'Main'
				ORDER BY 1, 2, 3, 4
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE VIEW MemoryNamesCount AS 
				SELECT
					MemoryName,
					Count(*)
				FROM
					Memories
				GROUP BY
					MemoryName
				";
			cmd.ExecuteNonQuery();
#endif
		}

		HashSet<string> AddedParts_ = new HashSet<string>();
		void ExportMemoryLayout(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.MemoryLayout_ != null)
			{
				// Different part revisions may have different JTAG ID. So add just once
				if (AddedParts_.Contains(rec.PartNumber))
					return;
				AddedParts_.Add(rec.PartNumber);
				foreach (var m in rec.MemoryLayout_.Memories.OrderBy(x => x.MemoryName))
				{
					m.PartNumber = rec.PartNumber;
					/*
					** Reuse AccessType member which is not used in F1xx, F2xx and F4xx families
					** Here we write Flash clock specs for these parts
					*/
					if (m.MemoryName == "Main" && m.MemoryType == "Flash")
						m.AccessType = Enum.GetName(typeof(Slas.EnumFlashTiming)
							, Slas.AllDevices.LocateFlashTimings(rec.PartNumber));
					else if (String.IsNullOrEmpty(m.AccessType))
						m.AccessType = Enum.GetName(typeof(Slas.EnumFlashTiming)
							, Slas.AllDevices.LocateFlashTimings(null));
					conn.Execute(
						@"INSERT INTO Memories (
								PartNumber,
								MemoryName,
								MemoryType,
								Bits,
								Start,
								Size,
								SegmentSize,
								Banks,
								Mapped,
								Mpu,
								Mask,
								Protectable,
								AccessType,
								WpAddress,
								WpBits,
								WpMask,
								WpPwd
							) VALUES (
								@PartNumber,
								@MemoryName,
								@MemoryType,
								@Bits,
								@Start,
								@Size,
								@SegmentSize,
								@Banks,
								@Mapped,
								@Mpu,
								@Mask,
								@Protectable,
								@AccessType,
								@WpAddress,
								@WpBits,
								@WpMask,
								@WpPwd
							)", m);
				}
			}
		}
	}
}
