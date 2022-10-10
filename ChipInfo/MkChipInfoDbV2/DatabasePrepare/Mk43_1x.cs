using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk43_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '43_100',
					RefTo = '43_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkLcd_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '43_0' OR m2.RefTo = '43_0')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_101',
					RefTo = '43_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_3', 'kBlkMain_3')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '43_0' OR m2.RefTo = '43_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_3'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_3'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkLcd_1'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_102',
					RefTo = '43_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_2', 'kBlkMain_5')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '43_0' OR m2.RefTo = '43_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_2'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_5'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkLcd_1'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_103',
					RefTo = '43_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_4', 'kBlkMain_0')
					AND Memories2.PartNumber IN (
						'MSP430F44x',
						'MSP430FW429'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_104',
					RefTo = '43_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkLcd_2'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '43_0' OR m2.RefTo = '43_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_1'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkLcd_2'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
