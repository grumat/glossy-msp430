using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk41_2x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '41_200',
					RefTo = '41_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_10')
					AND Memories2.PartNumber IN (
						'MSP430F2410',
						'MSP430G2x55'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_201',
					RefTo = '41_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_11')
					AND Memories2.PartNumber IN (
						'MSP430F2416',
						'MSP430F2616'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_202',
					RefTo = '41_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_12')
					AND Memories2.PartNumber IN (
						'MSP430F2419',
						'MSP430F2619'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_203',
					RefTo = '41_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_15')
					AND Memories2.PartNumber IN (
						'MSP430F248',
						'MSP430F2481'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_204',
					RefTo = '41_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_13', 'kBlkRam2_2')
					AND Memories2.PartNumber IN (
						'MSP430F2417',
						'MSP430F2617'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_205',
					RefTo = '41_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_14', 'kBlkRam2_2')
					AND Memories2.PartNumber IN (
						'MSP430F2418',
						'MSP430F2618'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_206',
					RefTo = '41_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_17')
					AND Memories2.PartNumber IN (
						'MSP430F235',
						'MSP430F2350'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '41_207',
					RefTo = '41_101'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_16')
					AND Memories2.PartNumber IN (
						'MSP430F2370'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
