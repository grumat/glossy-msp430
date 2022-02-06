using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk44_0(SqliteCommand cmd)
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
							AND m2.BlockId = 'kBlkBootCode_5'
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkBsl_4')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m4 
						WHERE m1.PartNumber = m4.PartNumber 
							AND m4.BlockId = 'kBlkInfo_6')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m5 
						WHERE m1.PartNumber = m5.PartNumber 
							AND m5.BlockId = 'kBlkPeripheral16bit_2')
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '44_0'
				FROM
					TmpParts t1
				WHERE
					Memories2.PartNumber = t1.PartNumber
					AND Memories2.BlockId IN ('kBlkBootCode_5', 'kBlkBsl_4', 'kBlkInfo_6', 'kBlkPeripheral16bit_2')
				";
			cmd.ExecuteNonQuery();
		}
	}
}
