using Dapper;
using Microsoft.Data.Sqlite;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class FlashTimings : IRender
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
			fh.Write(@"
// Fixed Flash timings for Legacy devices
struct ALIGNED FlashTimings
{
	// Mass erase clock count
	uint16_t mass_erase_;
	// Segment erase clock count
	uint16_t seg_erase_;
	// Clocks for each word write
	uint16_t word_wr_;
};

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Flash devices Gen 1
static constexpr FlashTimings flash_timing_gen1 = { 5297, 4819, 35 };
// Flash devices Gen 1
static constexpr FlashTimings flash_timing_gen2a = { 10593, 4819, 30 };
// Flash devices Gen 1
static constexpr FlashTimings flash_timing_gen2b = { 10593, 9628, 25 };
");
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}
	}
}

