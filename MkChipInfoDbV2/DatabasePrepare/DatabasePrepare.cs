using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.Data;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		public static void Prepare(SqliteConnection conn)
		{
			using (var xact = conn.BeginTransaction(IsolationLevel.ReadUncommitted))
			{
				using var cmd = new SqliteCommand();
				cmd.Connection = conn;
				cmd.Transaction = xact;
				PrepareMemoryLayouts(cmd);
				PrepareMemoryBlocks(cmd);
				MkMemBlockTables(cmd);
				xact.Commit();
			}
			using (var xact = conn.BeginTransaction(IsolationLevel.ReadUncommitted))
			{
				using var cmd = new SqliteCommand();
				cmd.Connection = conn;
				cmd.Transaction = xact;
				MkMemGroup_Common(cmd);	// CPU and EEM are valid for ALL parts
				MkA0_0(cmd);
				Mk70_0(cmd);
				Mk71_0(cmd);
				Mk72_0(cmd);
				Mk60_0(cmd);
				Mk61_0(cmd);
				Mk50_0(cmd);
				Mk73_0(cmd);
				Mk51_0(cmd);
				Mk52_0(cmd);
				Mk53_0(cmd);
				Mk54_0(cmd);
				Mk40_0(cmd);
				Mk41_0(cmd);
				Mk42_0(cmd);
				Mk43_0(cmd);
				Mk44_0(cmd);
				Mk30_0(cmd);
				Mk31_0(cmd);
				//
				MkA0_10(cmd);
				Mk70_10(cmd);
				Mk70_11(cmd);
				Mk70_2x(cmd);
				Mk71_1x(cmd);
				Mk72_1x(cmd);
				Mk61_1x(cmd);
				Mk50_1x(cmd);
				Mk54_1x(cmd);
				Mk40_1x(cmd);
				Mk40_2x(cmd);
				Mk41_1x(cmd);
				Mk41_2x(cmd);
				Mk42_1x(cmd);
				Mk43_1x(cmd);
				Mk43_2x(cmd);
				Mk43_3x(cmd);
				Mk44_1x(cmd);
				Mk30_1x(cmd);
				Mk30_2x(cmd);
				Mk31_1x(cmd);
				//
				PrepareMemories(cmd);
				xact.Commit();
			}
		}
	}
}
