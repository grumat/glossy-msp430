using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void MkA0_0(SqliteCommand cmd)
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
							AND m2.BlockId = 'kBlkTinyRam_0'
							 AND m2.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkBootCode_3'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkBsl_3'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkLeaPeripheral_0'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkLeaRam_1'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkPeripheral16bit1_0'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkPeripheral16bit2_1'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkPeripheral16bit3_0'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkUssPeripheral_0'
							 AND m3.MemGroup IS NULL
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkRam_8'
							 AND m3.MemGroup IS NULL
						)
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = 'A0_0'
				FROM
					TmpParts t1
				WHERE
					Memories2.PartNumber = t1.PartNumber
					AND Memories2.BlockId IN 
						(
							'kBlkTinyRam_0', 
							'kBlkBootCode_3', 
							'kBlkBsl_3', 
							'kBlkLeaPeripheral_0', 
							'kBlkLeaRam_1',
							'kBlkPeripheral16bit1_0',
							'kBlkPeripheral16bit2_1',
							'kBlkPeripheral16bit3_0',
							'kBlkUssPeripheral_0',
							'kBlkRam_8',
							-- Overrides
							'kBlkMain_71'
						)
				";
			cmd.ExecuteNonQuery();
		}
	}
}
