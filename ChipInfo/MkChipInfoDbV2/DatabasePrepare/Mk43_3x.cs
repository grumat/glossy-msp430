using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk43_3x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '43_300',
					RefTo = '43_201'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_0', 'kBlkMain_12')
					AND Memories2.PartNumber IN (
						'MSP430FG4619'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_301',
					RefTo = '43_201'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_0', 'kBlkMain_11')
					AND Memories2.PartNumber IN (
						'MSP430FG4616'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_302',
					RefTo = '43_201'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_2', 'kBlkMain_13')
					AND Memories2.PartNumber IN (
						'MSP430FG4617'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '43_303',
					RefTo = '43_201'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_2', 'kBlkMain_14')
					AND Memories2.PartNumber IN (
						'MSP430FG4618'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
