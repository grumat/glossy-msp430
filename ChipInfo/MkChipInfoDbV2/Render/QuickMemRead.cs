using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class QuickMemRead : IRender
	{
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine(@"// Devices that does not support Quick Mem Read
ALWAYS_INLINE static bool NoQuickMemRead(const Device &dev)
{
	return (dev.mcu_ver_ == 9201)
		|| (dev.mcu_ver_ == 5108)
		|| (dev.mcu_ver_ == 49298)
		;
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

