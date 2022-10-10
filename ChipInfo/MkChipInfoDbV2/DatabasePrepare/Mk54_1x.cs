using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk54_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '45_100',
					RefTo = '54_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_61')
					AND Memories2.PartNumber IN (
						'MSP430FR2310'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
