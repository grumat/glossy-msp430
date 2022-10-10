using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk30_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '30_100',
					RefTo = '30_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkBsl_0', 
						'kBlkLcd_1', 
						-- Overrides
						'kBlkRam2_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '30_0' OR m2.RefTo = '30_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkBsl_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkLcd_1'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkRam2_0', 'kBlkRam2_2')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_101',
					RefTo = '30_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkBsl_0', 
						'kBlkLcd_2', 
						-- Overrides
						'kBlkMain_3'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '30_0' OR m2.RefTo = '30_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkBsl_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkLcd_2'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_102',
					RefTo = '30_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_2',
						'kBlkMain_18'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '30_0' OR m2.RefTo = '30_0')
					)
					AND NOT EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkBsl_0'
					)
					AND NOT EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkLcd_2', 'kBlkLcd_1')
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
							AND m2.BlockId = 'kBlkMain_18'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_103',
					RefTo = '30_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_2',
						'kBlkMain_17'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '30_0' OR m2.RefTo = '30_0')
					)
					AND NOT EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkBsl_0'
					)
					AND NOT EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkLcd_2', 'kBlkLcd_1')
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
							AND m2.BlockId = 'kBlkMain_17'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_104',
					RefTo = '30_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_1',
						'kBlkMain_19'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '30_0' OR m2.RefTo = '30_0')
					)
					AND NOT EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkBsl_0'
					)
					AND NOT EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkLcd_2', 'kBlkLcd_1')
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
							AND m2.BlockId = 'kBlkMain_19'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
