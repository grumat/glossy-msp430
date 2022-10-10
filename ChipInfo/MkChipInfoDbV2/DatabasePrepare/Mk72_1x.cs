using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk72_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '72_10',
					RefTo = '72_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5726', 
						'MSP430FR5727',
						'MSP430FR5728',
						'MSP430FR5729',
						'MSP430FR5736',
						'MSP430FR5737',
						'MSP430FR5738',
						'MSP430FR5739'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_51'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '72_11',
					RefTo = '72_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR5720', 
						'MSP430FR5721', 
						'MSP430FR5730', 
						'MSP430FR5731'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_59'
						)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
