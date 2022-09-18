using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class TlvClash : IRender
	{
		public static string Map(long k)
		{
			return k != 0 ? "kTlvClash" : "kNoTlvClash";
		}

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.WriteLine(@"// Device contains TLV structure in InfoA segment
enum EnumTlvClash : uint16_t
{
	kNoTlvClash
	, kTlvClash
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

