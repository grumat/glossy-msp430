using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk31_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '31_100',
					RefTo = '31_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_64'
					)
					AND Memories2.PartNumber IN (
						'MSP430FR2000'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '31_101',
					RefTo = '31_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_17',
						'kBlkMain_60'
					)
					AND Memories2.PartNumber IN (
						'MSP430FR2111'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '31_101',
					RefTo = '31_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_17',
						'kBlkMain_61'
					)
					AND Memories2.PartNumber IN (
						'MSP430FR2110'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
