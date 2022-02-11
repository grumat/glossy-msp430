using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Eem : IRender
	{
		static Dictionary<string, string> Db2Cpp = new Dictionary<string, string>()
		{
			{ "EMEX_LOW", "kEmexLow" },
			{ "EMEX_MEDIUM", "kEmexMedium" },
			{ "EMEX_HIGH", "kEmexHigh" },
			{ "EMEX_EXTRA_SMALL_5XX", "kEmexExtraSmall5xx" },
			{ "EMEX_SMALL_5XX", "kEmexSmall5xx" },
			{ "EMEX_LARGE_5XX", "kEmexLarge5xx" },
		};

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			string sql = @"
				SELECT DISTINCT
					EemType
				FROM
					Chips
				ORDER BY 1
			";
			foreach (var row in conn.Query(sql))
			{
				if (!Db2Cpp.ContainsKey(row.EemType))
					throw new InvalidDataException(String.Format("Table EemType contains a value '{0}' that is not expected!"));
			}

		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Embedded Emulation Module (EEM) type
enum EnumEemType : uint8_t
{
	kEmexLow
	, kEmexMedium
	, kEmexHigh
	, kEmexExtraSmall5XX
	, kEmexSmall5XX
	, kEmexMedium5XX		// not used
	, kEmexLarge5XX
	, kEemUpper_ = kEmexLarge5XX
};	// 7 values

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

