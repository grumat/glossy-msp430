using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk42_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '42_100',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_3', 'kBlkMain_2')
					AND Memories2.PartNumber IN (
						'MSP430F1611'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_101',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_2')
					AND Memories2.PartNumber IN (
						'MSP430F148',
						'MSP430F168'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_102',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_1', 'kBlkMain_1')
					AND Memories2.PartNumber IN (
						'MSP430F1612'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_103',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_1', 'kBlkMain_3')
					AND Memories2.PartNumber IN (
						'MSP430F1610'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_104',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_1', 'kBlkMain_6')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '42_0' OR m2.RefTo = '42_0')
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
							AND m2.BlockId = 'kBlkMain_6'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_105',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_1', 'kBlkMain_7')
					AND Memories2.PartNumber IN (
						'MSP430F11x1',
						'MSP430F11x1A'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_106',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_3', 'kBlkMain_3')
					AND Memories2.PartNumber IN (
						'MSP430F147',
						'MSP430F157',
						'MSP430F167'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_107',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_3', 'kBlkMain_4')
					AND Memories2.PartNumber IN (
						'MSP430F156'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '42_108',
					RefTo = '42_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_2', 'kBlkMain_5')
					AND Memories2.PartNumber IN (
						'MSP430F135',
						'MSP430F155'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
