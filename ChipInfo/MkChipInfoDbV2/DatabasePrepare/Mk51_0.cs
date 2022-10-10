using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk51_0(SqliteCommand cmd)
		{
			cmd.ExecuteNonQuery();
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
							AND m2.BlockId = 'kBlkBootCode_0'
						)
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m3 
						WHERE m1.PartNumber = m3.PartNumber 
							AND m3.BlockId = 'kBlkBootCode2_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m4 
						WHERE m1.PartNumber = m4.PartNumber 
							AND m4.BlockId = 'kBlkMain_62')
					AND EXISTS (
						SELECT * 
						FROM Memories2 AS m5 
						WHERE m1.PartNumber = m5.PartNumber 
							AND m5.BlockId = 'kBlkRam_8')
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
					MemGroup = '51_0'
				FROM
					TmpParts t1
				WHERE
					Memories2.PartNumber = t1.PartNumber
					AND Memories2.BlockId IN ('kBlkBootCode_0', 'kBlkBootCode2_0', 'kBlkMain_62', 'kBlkRam_8', 'kBlkPeripheral16bit_2')
				";
			cmd.ExecuteNonQuery();
		}
	}
}
