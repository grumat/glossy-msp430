using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void MkMemGroup_Common(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '_AllParts_'
				WHERE
					Memories2.BlockId IN ('kBlkCpu_0', 'kBlkEem_0')
				";
			cmd.ExecuteNonQuery();
		}
	}
}
