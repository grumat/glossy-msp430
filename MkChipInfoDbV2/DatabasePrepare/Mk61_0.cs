using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk61_0(SqliteCommand cmd)
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
							AND m2.BlockId = 'kBlkBsl_3'
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkBootCode_5')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m4 
						WHERE m1.PartNumber = m4.PartNumber 
							AND m4.BlockId = 'kBlkInfo_8')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m5 
						WHERE m1.PartNumber = m5.PartNumber 
							AND m5.BlockId = 'kBlkPeripheral16bit_1')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m6 
						WHERE m1.PartNumber = m6.PartNumber 
							AND m6.BlockId = 'kBlkPeripheral16bit1_1')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m6 
						WHERE m1.PartNumber = m6.PartNumber 
							AND m6.BlockId = 'kBlkTinyRam_0')
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '61_0'
				FROM
					TmpParts t1
				WHERE
					Memories2.PartNumber = t1.PartNumber
					AND Memories2.BlockId IN (
						'kBlkBsl_3', 
						'kBlkBootCode_5', 
						'kBlkInfo_8', 
						'kBlkPeripheral16bit_1', 
						'kBlkPeripheral16bit1_1',
						'kBlkTinyRam_0',
						-- Overrides
						'kBlkMain_68',
						'kBlkRam_7'
					)
				";
			cmd.ExecuteNonQuery();
		}
	}
}
