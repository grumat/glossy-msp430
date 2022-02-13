using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Issue1377 : IRender
	{
		public static string Map(long k)
		{
			return k != 0 ? "k1377" : "kNo1377";
		}

		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Device has an issue with the JTAG MailBox peripheral
/*!
grumat: Was unable to locate the issue documentation. Checked Errata datasheets
and candidates could be: EEM6, EEM13, JTAG17
Note that XML logic does not matches these errata sheets. For example: MSP430F5438
is the single variant in the family that is not tagged with 1377 issue, but its
errata-sheet is just identical to MSP430F5418. XML may also be the issue.
*/
enum EnumIssue1377 : uint16_t
{
	kNo1377
	, k1377
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

