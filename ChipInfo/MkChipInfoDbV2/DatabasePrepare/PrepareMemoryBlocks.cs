using Dapper;
using Microsoft.Data.Sqlite;
using System.Collections.Generic;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
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
					BlockId TEXT NOT NULL,
					MemGroup TEXT,
					RefTo TEXT
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
				, new
				{
					PartNumber = row.PartNumber,
					BlockId = aux.BlockId
				});
			}
			cmd.CommandText = @"
				CREATE INDEX IF NOT EXISTS IdxMem2BlkId 
				ON Memories2 (BlockId);
				CREATE INDEX IF NOT EXISTS IdxMem2PNum 
				ON Memories2 (PartNumber);
				";
			cmd.ExecuteNonQuery();
		}
	}
}
