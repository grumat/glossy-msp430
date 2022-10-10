using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk30_2x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '30_200',
					RefTo = '30_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_12'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.MemGroup = '30_100'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_12'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_0'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_201',
					RefTo = '30_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_9'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.MemGroup = '30_100'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_9'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_0'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_202',
					RefTo = '30_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_13',
						'kBlkRam2_2'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.MemGroup = '30_100'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_13'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_2'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_203',
					RefTo = '30_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_14',
						'kBlkRam2_2'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.MemGroup = '30_100'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_14'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_2'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_204',
					RefTo = '30_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_15'
					)
					AND Memories2.PartNumber IN (
						'MSP430F4783',
						'MSP430F4784'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_205',
					RefTo = '30_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_2'
					)
					AND Memories2.PartNumber IN (
						'MSP430F478',
						'MSP430FG478'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_206',
					RefTo = '30_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_0'
					)
					AND Memories2.PartNumber IN (
						'MSP430F479',
						'MSP430FG479'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_207',
					RefTo = '30_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_5',
						'kBlkMain_8'
					)
					AND Memories2.PartNumber IN (
						'MSP430F4793',
						'MSP430F4794'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_208',
					RefTo = '30_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_2',
						'kBlkMain_5'
					)
					AND Memories2.PartNumber IN (
						'MSP430F4152'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '30_209',
					RefTo = '30_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_2',
						'kBlkMain_6'
					)
					AND Memories2.PartNumber IN (
						'MSP430F4132'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
