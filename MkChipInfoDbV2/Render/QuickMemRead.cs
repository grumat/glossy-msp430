using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class QuickMemRead : IRender
	{
		public static string Map(long k)
		{
			return k != 0 ? "kQuickMemRead" : "kNoQuickMemRead";
		}

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine(@"// Device supports quick memory read
enum EnumQuickMemRead : uint16_t
{
	kNoQuickMemRead
	, kQuickMemRead
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

