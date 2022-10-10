using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk41_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '41_100',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_4', 'kBlkMain_16', 'kBlkRam2_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '41_0' OR m2.RefTo = '41_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_4'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN (
								'kBlkMain_10', 
								'kBlkMain_11', 
								'kBlkMain_12', 
								'kBlkMain_15', 
								'kBlkMain_16'
							)
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_101',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_4', 'kBlkMain_8')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '41_0' OR m2.RefTo = '41_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_4'
					)
					AND NOT EXISTS (	------ NOT ------
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN (
								'kBlkMain_8', 
								'kBlkMain_13', 
								'kBlkMain_14', 
								'kBlkMain_17', 
								'kBlkMain_16'
							)
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_102',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_2')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '41_0' OR m2.RefTo = '41_0')
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
							AND m2.BlockId IN (
								'kBlkMain_18'
							)
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_103',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_2', 'kBlkMain_17')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '41_0' OR m2.RefTo = '41_0')
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
							AND m2.BlockId IN (
								'kBlkMain_17'
							)
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_104',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_2', 'kBlkMain_19')
					AND Memories2.PartNumber IN (
						'MSP430F2122'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_105',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_3', 'kBlkMain_18')
					AND Memories2.PartNumber IN (
						'MSP430F233',
						'MSP430F2330'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_106',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_3', 'kBlkMain_16')
					AND Memories2.PartNumber IN (
						'MSP430F2272_G2744',
						'MSP430F2274'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_107',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_1')
					AND Memories2.PartNumber IN (
						'MSP430F21x1',
						'MSP430G2xx2'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_108',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_20', 'kBlkRam_0')
					AND Memories2.PartNumber IN (
						'F20x1_G2x0x_G2x1x',
						'F20x2_G2x2x_G2x3x',
						'MSP430F20x3'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_108',
					RefTo = '41_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_20', 'kBlkRam_1')
					AND Memories2.PartNumber IN (
						'MSP430F2112'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
