using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk61_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '61_10',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5847', 
						'MSP430FR5857', 
						'MSP430FR5867', 
						'MSP430FR5947', 
						'MSP430FR5957', 
						'MSP430FR5967'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_72',
						'kBlkRam_6'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '61_11',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5870', 
						'MSP430FR5970', 
						'MSP430FR6820', 
						'MSP430FR6870', 
						'MSP430FR6920', 
						'MSP430FR6970'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_72'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '61_12',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5848', 
						'MSP430FR5858', 
						'MSP430FR5868', 
						'MSP430FR5948', 
						'MSP430FR5958', 
						'MSP430FR5968', 
						'MSP430FR5986' 
						)
					AND Memories2.BlockId IN (
						'kBlkMain_67'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '61_13',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5889', 
						'MSP430FR5989', 
						'MSP430FR6879', 
						'MSP430FR6889', 
						'MSP430FR6979', 
						'MSP430FR6989'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_70'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '61_14',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5888', 
						'MSP430FR5988', 
						'MSP430FR6888', 
						'MSP430FR6928', 
						'MSP430FR6988'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_69'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '61_15',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5962'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_65',
						'kBlkRam_10'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '61_16',
					RefTo = '61_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5964'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_66',
						'kBlkRam_10'
						)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
