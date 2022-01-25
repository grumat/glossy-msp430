using Microsoft.Data.Sqlite;
using System.Data;

namespace MkChipInfoDbV2
{
	public class DatabasePrepare
	{
		static void PrepareMemories(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemoryNames;
				CREATE VIEW EnumMemoryNames AS
				SELECT DISTINCT
					'kMkey' || MemoryName AS Name
				FROM
					Memories
				GROUP BY
					MemoryName
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemoryTypes;
				CREATE VIEW EnumMemoryTypes AS
				SELECT DISTINCT
					'kMtyp' || MemoryType AS Type
				FROM
					Memories
				GROUP BY
					MemoryType
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumMemAccessType;
				CREATE VIEW EnumMemAccessType AS
				SELECT DISTINCT
					'kAcc' || AccessType AS Type
				FROM
					Memories
				WHERE
					AccessType != 'None'
				GROUP BY
					AccessType
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumAddressStart;
				CREATE VIEW EnumAddressStart AS
				SELECT DISTINCT
					printf('kStart_0x%x', Start) AS Value,
					Start As Integral
				FROM
					Memories
				GROUP BY
					Start
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumBlockSize;
				CREATE VIEW EnumBlockSize AS
				SELECT DISTINCT
					printf('kSize_0x%x', Size) AS Value,
					Size As Integral
				FROM
					Memories
				GROUP BY
					Size
				";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumSegSize;
				CREATE VIEW EnumSegSize AS
				SELECT DISTINCT
					printf('kSeg_0x%x', SegmentSize) AS Value,
					SegmentSize As Integral
				FROM
					Memories
				GROUP BY
					SegmentSize
				";
			cmd.ExecuteNonQuery();
		}

		public static void Prepare(SqliteConnection conn)
		{

			using (var xact = conn.BeginTransaction(IsolationLevel.ReadUncommitted))
			{
				using var cmd = new SqliteCommand();
				cmd.Connection = conn;
				cmd.Transaction = xact;
				PrepareMemories(cmd);
				xact.Commit();
			}
		}
	}
}
