using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void MkA0_10(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = 'A0_10',
					RefTo = 'A0_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND EXISTS (
						SELECT * 
					FROM 
						Memories2 AS m2 
					WHERE 
						Memories2.PartNumber = m2.PartNumber 
						AND m2.MemGroup = 'A0_0')
				";
			cmd.ExecuteNonQuery();
		}
	}
}
