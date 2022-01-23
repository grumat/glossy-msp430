
using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ImportDB
{
	partial class CreateDatabase
	{
		void CreateEmmTimersTables(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				CREATE TABLE EemTimers (
					PartNumber TEXT NOT NULL,
					Name TEXT,
					Value INTEGER,
					DefaultStop BOOLEAN,
					[Index] INTEGER
				)
			";
			cmd.ExecuteNonQuery();
			cmd.CommandText = @"
				CREATE TABLE EemTimersPacked (
					TimerPK INTEGER NOT NULL,
					[Index] INTEGER,
					Value INTEGER,
					Name TEXT
				)
			";
			cmd.ExecuteNonQuery();
		}

		Dictionary<string, int> PackedEmmTimers = new Dictionary<string, int>();
		int? ExportPackedEmmTimers(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.EemTimers_ != null && rec.EemTimers_.EemTimers != null)
			{
				StringBuilder buf = new StringBuilder();
				foreach (var t in rec.EemTimers_.EemTimers.OrderBy(x => x.Index))
				{
					buf.Append(String.Format("{0}.{1}", t.Index, t.Value));
				}
				if (PackedEmmTimers.ContainsKey(buf.ToString()))
					return PackedEmmTimers[buf.ToString()];
				int pk = PackedEmmTimers.Count;
				foreach (var t in rec.EemTimers_.EemTimers)
				{
					conn.Execute(
						@"INSERT INTO EemTimersPacked (
								TimerPK,
								[Index],
								Value,
								Name
							) VALUES (
								@TimerPK,
								@Index,
								@Value,
								@Name
							)",
							new
							{
								TimerPK = pk,
								Index = t.Index,
								Value = t.Value,
								Name = t.Id ?? t.Name
							}
						);
				}
				PackedEmmTimers.Add(buf.ToString(), pk);
				return pk;
			}
			return null;
		}
		void ExportEmmTimers(SqliteConnection conn, Model.Chips rec)
		{
			if (rec.EemTimers_ != null && rec.EemTimers_.EemTimers != null)
			{
				foreach (var t in rec.EemTimers_.EemTimers)
				{
					t.PartNumber = rec.PartNumber;
					if (t.DefaultStop == null)
						t.DefaultStop = false;
					conn.Execute(
						@"INSERT INTO EemTimers (
								PartNumber,
								Name,
								Value,
								DefaultStop,
								[Index]
							) VALUES (
								@PartNumber,
								@Name,
								@Value,
								@DefaultStop,
								@Index
							)", t);
				}
			}
		}
	}
}

