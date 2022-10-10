using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk44_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '44_100',
					RefTo = '44_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_17', 'kBlkMain_54')
					AND Memories2.PartNumber IN (
						'MSP430FR2032',
						'MSP430FR4132'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '44_101',
					RefTo = '44_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_18', 'kBlkMain_52')
					AND Memories2.PartNumber IN (
						'MSP430FR2033',
						'MSP430FR4133'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '44_102',
					RefTo = '44_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_16', 'kBlkMain_58')
					AND Memories2.PartNumber IN (
						'MSP430FR4131'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
