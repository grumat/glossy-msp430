using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk40_1x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '40_100',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkMain_45'
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_101',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkRam_13'
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_44'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_102',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_30', 'kBlkMain_40')
					AND Memories2.PartNumber IN (
						'MSP430F5252',
						'MSP430F5253',
						'MSP430F5256',
						'MSP430F5257'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_103',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN (
						'kBlkRam_30', 
						'kBlkUsbRam_0',
						-- Overrides
						'kBlkMain_34'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_30'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_104',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_28', 'kBlkMain_26')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_26'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_105',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_28', 'kBlkMain_28')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_28'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_28'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_106',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_28', 'kBlkMain_27')
					AND Memories2.PartNumber IN (
						'MSP430F5213',
						'MSP430F5218',
						'MSP430F5223',
						'MSP430F5228'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_107',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_28', 'kBlkMain_33', 'kBlkUsbRam_0')
					AND Memories2.PartNumber IN (
						'MSP430F5513',
						'MSP430F5521',
						'MSP430F5522'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_108',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_8', 'kBlkMain_23')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_8'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_23', 'kBlkMain_24', 'kBlkMain_25', 'kBlkMain_22')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_109',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_29', 'kBlkMain_36', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_29'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_36', 'kBlkMain_37', 'kBlkMain_38', 'kBlkMain_34')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_110',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_7', 'kBlkMain_32')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_7'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_32', 'kBlkMain_42', 'kBlkMain_41', 'kBlkMain_47', 'kBlkMain_33')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_111',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_43')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_12'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_112',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_29')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_12'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_113',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_30')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_12'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_114',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_31')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_12'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_115',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_24', 'kBlkMain_32', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_24'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_32', 'kBlkMain_41', 'kBlkMain_39', 'kBlkMain_32', 'kBlkMain_46')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_116',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_6', 'kBlkMain_41')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_6'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_41', 'kBlkMain_46')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_117',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_4', 'kBlkRam_15', 'kBlkMain_33')
					AND Memories2.PartNumber IN (
						'CC430F5147',
						'CC430F6147'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_118',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam2_4', 'kBlkRam_14', 'kBlkMain_47')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam2_4'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_14'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_47', 'kBlkMain_42')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_119',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_10', 'kBlkMain_25')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_10'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_25'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_120',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_27', 'kBlkRam_25', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_27'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkUsbRam_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkRam_25', 'kBlkRam_26')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_121',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_27', 'kBlkMain_28', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_27'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkUsbRam_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_28', 'kBlkMain_34')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_122',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_40', 'kBlkRam_31')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_40'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_31'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_123',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_23', 'kBlkMain_46', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_23'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkUsbRam_0'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId IN ('kBlkMain_46', 'kBlkMain_32', 'kBlkMain_41', 'kBlkMain_39')
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_124',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_33', 'kBlkRam_9')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND (m2.MemGroup = '40_0' OR m2.RefTo = '40_0')
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_33'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_9'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_125',
					RefTo = '40_0'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.PartNumber IN (
						'MSP430FG6426'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
