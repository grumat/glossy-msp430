using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk50_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '50_10',
					RefTo = '50_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR2533'
						)
					AND Memories2.BlockId IN (
						'kBlkRam_18'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '50_11',
					RefTo = '50_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR2532'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_55',
						'kBlkRam_17'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '50_12',
					RefTo = '50_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN (
						'MSP430FR2632'
						)
					AND Memories2.BlockId IN (
						'kBlkMain_55',
						'kBlkRam_18'
						)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
