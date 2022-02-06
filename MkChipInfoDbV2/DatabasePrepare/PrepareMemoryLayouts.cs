
using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void PrepareMemoryLayouts(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumSegSize;
				CREATE VIEW EnumSegSize AS
				SELECT DISTINCT
					printf('kSeg_0x%x', SegmentSize) AS Value,
					SegmentSize As Integral
				FROM
					Memories
				ORDER BY
					SegmentSize
				";
			cmd.ExecuteNonQuery();

			cmd.CommandText = @"
				DROP VIEW IF EXISTS EnumAddressSize;
				CREATE VIEW EnumAddressSize AS
				SELECT DISTINCT
					printf('kBlk_%05x_%05x_%03x_%x', Start, Size, SegmentSize, Banks) AS Value,
					Start,
					Size,
					SegmentSize,
					Banks
				FROM
					Memories
				ORDER BY
					Start,
					Size,
					SegmentSize,
					Banks
				";
			cmd.ExecuteNonQuery();

			// Helps to calculate SHIFT for Start
			cmd.CommandText = @"
				DROP VIEW IF EXISTS StartShift;
				CREATE VIEW StartShift AS
				SELECT DISTINCT
					Start,
					printf('%05x', Start) as a_integral,
					(Start & 0xFFF) == Start as a_seg0,
					printf('%05x', Start & 0xFFF) as a_shift0, 
					(((Start >> 4) & 0xFFF) << 4) == Start as a_seg4,
					printf('%05x', (Start>>4) & 0xFFF) as a_shift4,
					(((Start >> 8) & 0xFFF) << 8) == Start as a_seg8,
					printf('%05x', (Start>>8) & 0xFFF) as a_shift8,
					(((Start >> 12) & 0xFFF) << 12) == Start as a_seg12,
					printf('%05x', (Start>>12) & 0xFFF) as a_shift12
				FROM
					Memories
				ORDER BY Start
";
			cmd.ExecuteNonQuery();

			// Helps to calculate SHIFT for Size
			cmd.CommandText = @"
				DROP VIEW IF EXISTS SizeShift;
				CREATE VIEW SizeShift AS
				SELECT DISTINCT
					Size,
					printf('%05x', Size) as s_integral,
					(Size & 0x1FF) == Size as s_seg0,
					printf('%05x', Size & 0x1FF) as s_shift0, 
					(((Size >> 4) & 0x1FF) << 4) == Size as s_seg4,
					printf('%05x', (Size>>4) & 0x1FF) as s_shift4,
					(((Size >> 8) & 0x1FF) << 8) == Size as s_seg8,
					printf('%05x', (Size>>8) & 0x1FF) as s_shift8,
					(((Size >> 12) & 0x1FF) << 12) == Size as s_seg12,
					printf('%05x', (Size>>12) & 0x1FF) as s_shift12
				FROM
					Memories
				ORDER BY Size
";
			cmd.ExecuteNonQuery();
		}
	}
}
