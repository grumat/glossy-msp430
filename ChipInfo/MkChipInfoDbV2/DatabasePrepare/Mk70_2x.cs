using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk70_2x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '70_20',
					RefTo = '70_10'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_65'
						)
					AND EXISTS (
						SELECT * 
					FROM 
						Memories2 AS m2 
					WHERE 
						Memories2.PartNumber = m2.PartNumber 
						AND m2.MemGroup = '70_10');
				UPDATE
					Memories2
				SET 
					MemGroup = '70_21',
					RefTo = '70_11'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_65'
						)
					AND EXISTS (
						SELECT * 
					FROM 
						Memories2 AS m2 
					WHERE 
						Memories2.PartNumber = m2.PartNumber 
						AND m2.MemGroup = '70_11');
				UPDATE
					Memories2
				SET 
					MemGroup = '70_22',
					RefTo = '70_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_65'
						)
					AND EXISTS (
						SELECT * 
					FROM 
						Memories2 AS m2 
					WHERE 
						Memories2.PartNumber = m2.PartNumber 
						AND m2.MemGroup = '70_0');
				";
			cmd.ExecuteNonQuery();
		}
	}
}
