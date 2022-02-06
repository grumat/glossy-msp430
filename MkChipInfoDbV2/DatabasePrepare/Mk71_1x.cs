using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk71_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '71_10',
					RefTo = '71_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkMain_49', 
						'kBlkRam_21'
						)
					AND EXISTS (
						SELECT * 
					FROM 
						Memories2 AS m2 
					WHERE 
						Memories2.PartNumber = m2.PartNumber 
						AND m2.MemGroup = '71_0')
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '71_11',
					RefTo = '71_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN ('MSP430FR2153', 'MSP430FR2353')
					AND Memories2.BlockId IN (
						'kBlkMain_50', 
						'kBlkRam_18',
						'kBlkLib_2'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '71_12',
					RefTo = '71_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN ('MSP430FR2672')
					AND Memories2.BlockId IN (
						'kBlkMain_55', 
						'kBlkRam_18'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '71_13',
					RefTo = '71_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN ('MSP430FR2155', 'MSP430FR2355')
					AND Memories2.BlockId IN (
						'kBlkLib_2'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '71_14',
					RefTo = '71_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN ('MSP430FR2673')
					AND Memories2.BlockId IN (
						'kBlkMain_50'
						)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '71_15',
					RefTo = '71_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND PartNumber IN ('MSP430FR2675')
					AND Memories2.BlockId IN (
						'kBlkRam_20'
						)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
