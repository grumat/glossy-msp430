using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.Data;

namespace MkChipInfoDbV2
{
	public class DatabasePrepare
	{
		static void PrepareMemoryLayouts(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumSegSize;
				CREATE VIEW EnumSegSize AS
				SELECT DISTINCT
					printf('kSeg_0x%x', SegmentSize) AS Value,
					SegmentSize As Integral
				FROM
					Memories
				ORDER BY
					SegmentSize
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumAddressSize;
				CREATE VIEW EnumAddressSize AS
				SELECT DISTINCT
					printf('kBlk_%05x_%05x_%03x_%x', Start, Size, SegmentSize, Banks) AS Value,
					Start,
					Size,
					SegmentSize,
					Banks
				FROM
					Memories
				ORDER BY
					Start,
					Size,
					SegmentSize,
					Banks
				";
			cmd.ExecuteNonQuery();

			// Helps to calculate SHIFT for Start
			cmd.CommandText = @"
				DROP VIEW IF EXISTS StartShift;
				CREATE VIEW StartShift AS
				SELECT DISTINCT
					Start,
					printf('%05x', Start) as a_integral,
					(Start & 0xFFF) == Start as a_seg0,
					printf('%05x', Start & 0xFFF) as a_shift0, 
					(((Start >> 4) & 0xFFF) << 4) == Start as a_seg4,
					printf('%05x', (Start>>4) & 0xFFF) as a_shift4,
					(((Start >> 8) & 0xFFF) << 8) == Start as a_seg8,
					printf('%05x', (Start>>8) & 0xFFF) as a_shift8,
					(((Start >> 12) & 0xFFF) << 12) == Start as a_seg12,
					printf('%05x', (Start>>12) & 0xFFF) as a_shift12
				FROM
					Memories
				ORDER BY Start
";
			cmd.ExecuteNonQuery();

			// Helps to calculate SHIFT for Size
			cmd.CommandText = @"
				DROP VIEW IF EXISTS SizeShift;
				CREATE VIEW SizeShift AS
				SELECT DISTINCT
					Size,
					printf('%05x', Size) as s_integral,
					(Size & 0x1FF) == Size as s_seg0,
					printf('%05x', Size & 0x1FF) as s_shift0, 
					(((Size >> 4) & 0x1FF) << 4) == Size as s_seg4,
					printf('%05x', (Size>>4) & 0x1FF) as s_shift4,
					(((Size >> 8) & 0x1FF) << 8) == Size as s_seg8,
					printf('%05x', (Size>>8) & 0x1FF) as s_shift8,
					(((Size >> 12) & 0x1FF) << 12) == Size as s_seg12,
					printf('%05x', (Size>>12) & 0x1FF) as s_shift12
				FROM
					Memories
				ORDER BY Size
";
			cmd.ExecuteNonQuery();
		}
		static Dictionary<string, int> BlkKeys = new Dictionary<string, int>();
		static Dictionary<string, string> Hash2BlkId = new Dictionary<string, string>();
		static void MkMemBlockTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				DROP TABLE IF EXISTS MemoryBlocksTables;
				CREATE TABLE MemoryBlocksTables (
					BlockId TEXT NOT NULL,
					MemoryKey TEXT NOT NULL,
					MemoryType TEXT NOT NULL,
					AccessType TEXT NOT NULL,
					Protectable INTEGER,
					MemLayout TEXT NOT NULL,
					MemWrProt TEXT NOT NULL
				);
				DROP TABLE IF EXISTS Memories2;
				CREATE TABLE Memories2 (
					PartNumber TEXT NOT NULL,
					BlockId TEXT NOT NULL
				);
				";
			cmd.ExecuteNonQuery();

			string sql = @"
				SELECT * FROM EnumMemoryBlocks
			";
			foreach (var row in cmd.Connection.Query<MemoryBlocksTables>(sql))
			{
				string hash = row.MkHash();
				string pk = "kBlk" + row.MemoryKey.Substring(5);
				if (BlkKeys.ContainsKey(pk))
					BlkKeys[pk] += 1;
				else
					BlkKeys[pk] = 0;
				row.BlockId = pk + '_' + BlkKeys[pk].ToString();
				cmd.Connection.Execute(@"
					INSERT INTO MemoryBlocksTables (
						BlockId,
						MemoryKey,
						MemoryType,
						AccessType,
						Protectable,
						MemLayout,
						MemWrProt
					) VALUES(
						@BlockId,
						@MemoryKey,
						@MemoryType,
						@AccessType,
						@Protectable,
						@MemLayout,
						@MemWrProt
					)", row);
				Hash2BlkId.Add(hash, row.BlockId);
			}
			sql = @"
				SELECT * FROM PartMemoryBlocks;
			";
			foreach (var row in cmd.Connection.Query(sql))
			{
				MemoryBlocksTables aux = new MemoryBlocksTables();
				aux.MemoryKey = row.MemoryKey;
				aux.MemoryType = row.MemoryType;
				aux.AccessType = row.AccessType;
				aux.Protectable = row.Protectable != 0;
				aux.MemLayout = row.MemLayout;
				aux.MemWrProt = row.MemWrProt;
				string hash = aux.MkHash();
				aux.BlockId = Hash2BlkId[hash];
				cmd.Connection.Execute(@"
					INSERT INTO Memories2 (
						PartNumber,
						BlockId
					) VALUES(
						@PartNumber,
						@BlockId
					)"
				, new {
					PartNumber = row.PartNumber,
					BlockId = aux.BlockId
				});
			}
		}
		static void PrepareMemoryBlocks(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemoryTypes;
				CREATE VIEW EnumMemoryTypes AS
				SELECT DISTINCT
					'kMtyp' || MemoryType AS Type
				FROM
					Memories
				ORDER BY
					MemoryType
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemAccessType;
				CREATE VIEW EnumMemAccessType AS
				SELECT DISTINCT
					'kAcc' || AccessType AS Type
				FROM
					Memories
				WHERE
					AccessType != 'None'
				ORDER BY
					AccessType
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumWriteProtection;
				CREATE VIEW EnumWriteProtection AS
				SELECT DISTINCT
					printf('kWp_%04x_%04x_%04x_%04x', coalesce(WpAddress, 0), coalesce(WpBits, 0), coalesce(WpMask, 0), coalesce(WpPwd, 0)) AS Wp,
					coalesce(WpAddress, 0) as WpAddress,
					coalesce(WpBits, 0) as WpBits,
					coalesce(WpMask, 0) as WpMask,
					coalesce(WpPwd, 0) as WpPwd
				FROM
					Memories
				ORDER BY 1
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemoryKeys;
				CREATE VIEW EnumMemoryKeys AS
				SELECT DISTINCT
					'kMkey' || MemoryName AS Name,
					MemoryName
				FROM
					Memories
				GROUP BY
					MemoryName
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemoryBlocks;
				CREATE VIEW EnumMemoryBlocks AS
				SELECT DISTINCT
					'kMkey' || MemoryName AS MemoryKey,
					'kMtyp' || MemoryType AS MemoryType,
					'kAcc' || AccessType AS AccessType,
					Protectable,
					printf('kBlk_%05x_%05x_%03x_%x', Start, Size, SegmentSize, Banks) AS MemLayout,
					printf('kWp_%04x_%04x_%04x_%04x', coalesce(WpAddress, 0), coalesce(WpBits, 0), coalesce(WpMask, 0), coalesce(WpPwd, 0)) AS MemWrProt
				FROM
					Memories
				ORDER BY 
					2, 3, 1, 4, 5
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS PartMemoryBlocks;
				CREATE VIEW PartMemoryBlocks AS
				SELECT DISTINCT
					PartNumber,
					'kMkey' || MemoryName AS MemoryKey,
					'kMtyp' || MemoryType AS MemoryType,
					'kAcc' || AccessType AS AccessType,
					Protectable,
					printf('kBlk_%05x_%05x_%03x_%x', Start, Size, SegmentSize, Banks) AS MemLayout,
					printf('kWp_%04x_%04x_%04x_%04x', coalesce(WpAddress, 0), coalesce(WpBits, 0), coalesce(WpMask, 0), coalesce(WpPwd, 0)) AS MemWrProt
				FROM
					Memories
				ORDER BY 
					1, 2, 3, 4, 5
				";
			cmd.ExecuteNonQuery();
		}
		static void PrepareMemories(SqliteCommand cmd)
		{
		}

		public static void Prepare(SqliteConnection conn)
		{

			using (var xact = conn.BeginTransaction(IsolationLevel.ReadUncommitted))
			{
				using var cmd = new SqliteCommand();
				cmd.Connection = conn;
				cmd.Transaction = xact;
				PrepareMemoryLayouts(cmd);
				PrepareMemoryBlocks(cmd);
				MkMemBlockTables(cmd);
				PrepareMemories(cmd);
				xact.Commit();
			}
		}
	}
}
