using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk43_2x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '43_200',
					RefTo = '43_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_3', 'kBlkMain_3')
					AND Memories2.PartNumber IN (
						'MSP430FW42x/F41x'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_201',
					RefTo = '43_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_4',
						-- Overrides
						'kBlkMain_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '43_100')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.BlockId = 'kBlkLcd_0')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_202',
					RefTo = '43_104'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_5'
					)
					AND Memories2.PartNumber IN (
						'MSP430F4250',
						'MSP430FG4250'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_203',
					RefTo = '43_104'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_3'
					)
					AND Memories2.PartNumber IN (
						'MSP430F42x0',
						'MSP430FG42x0'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
