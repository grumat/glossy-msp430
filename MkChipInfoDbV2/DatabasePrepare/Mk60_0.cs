using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk60_0(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				DROP TABLE IF EXISTS TmpParts;
				CREATE TEMP TABLE TmpParts AS
				SELECT DISTINCT PartNumber
				FROM
					Memories2 AS m1
				WHERE EXISTS (
						SELECT * 
						FROM Memories2 AS m2 
						WHERE m1.PartNumber = m2.PartNumber 
							AND m2.BlockId = 'kBlkBsl_1'
							 AND m2.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkInfo_2'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkMain_21'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkPeripheral16bit_0'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkPeripheral8bit_0'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkRam_4'
							 AND m3.MemGroup IS NULL
						)
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '60_0'
				FROM
					TmpParts t1
				WHERE
					Memories2.PartNumber = t1.PartNumber
					AND Memories2.BlockId IN 
						(
							'kBlkBsl_1', 
							'kBlkInfo_2', 
							'kBlkMain_21', 
							'kBlkPeripheral16bit_0', 
							'kBlkPeripheral8bit_0',
							'kBlkRam_4'
						)
				";
			cmd.ExecuteNonQuery();
		}
	}
}
