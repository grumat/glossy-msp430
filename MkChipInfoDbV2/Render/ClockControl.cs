using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class ClockControl : IRender
	{
		static Dictionary<string, string> Db2Cpp = new Dictionary<string, string>()
		{
			{ "GCC_NONE", "kGccNone" },
			{ "GCC_STANDARD", "kGccStandard" },
			{ "GCC_STANDARD_I", "kGccStandardI" },
			{ "GCC_EXTENDED", "kGccExtended" },
		};
		static public string Map(string k)
		{
			return Db2Cpp[k];
		}

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			string sql = @"
				SELECT DISTINCT
					ClockControl
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (!Db2Cpp.ContainsKey(row.ClockControl))
					throw new InvalidDataException(String.Format("Table ClockControl contains a value '{0}' that is not expected!", row.ClockControl));
			}
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine(@"// Clock type supported by device
enum EnumClockControl : uint16_t
{
	kGccNone
	, kGccStandard
	, kGccStandardI
	, kGccExtended
};
");
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

