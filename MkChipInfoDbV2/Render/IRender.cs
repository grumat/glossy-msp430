using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	interface IRender
	{
		void OnPrologue(TextWriter fh, SqliteConnection conn);
		void OnDeclareConsts(TextWriter fh, SqliteConnection conn);
		void OnDeclareEnums(TextWriter fh, SqliteConnection conn);
		void OnDeclareStructs(TextWriter fh, SqliteConnection conn);
		void OnEpilogue(TextWriter fh, SqliteConnection conn);
	}
}
