using Microsoft.Data.Sqlite;

namespace MkChipInfoDbV2
{
	public partial class DatabasePrepare
	{
		static void Mk40_2x(SqliteCommand cmd)
		{
			cmd.CommandText = @"
				UPDATE
					Memories2
				SET 
					MemGroup = '40_200',
					RefTo = '40_100'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkRam_13'
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_45'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_201',
					RefTo = '40_103'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkMain_35'
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
					MemGroup = '40_202',
					RefTo = '40_103'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkMain_36'
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
					MemGroup = '40_203',
					RefTo = '40_104'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkUsbRam_0'
					AND Memories2.PartNumber = 'MSP430FG6625'
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_204',
					RefTo = '40_104'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkRam_11'
					AND Memories2.PartNumber = 'MSP430FG6425'
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_205',
					RefTo = '40_104'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_24', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_26'
							AND m2.MemGroup = '40_104'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_24'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_206',
					RefTo = '40_104'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_23', 'kBlkUsbRam_0')
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkMain_26'
							AND m2.MemGroup = '40_104'
					)
					AND EXISTS (
						SELECT * 
						FROM Memories2 m2 
						WHERE Memories2.PartNumber = m2.PartNumber
							AND m2.BlockId = 'kBlkRam_23'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_207',
					RefTo = '40_105'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkUsbRam_0'
					AND Memories2.PartNumber IN (
						'MSP430F5519',
						'MSP430F5528',
						'MSP430F5529',
						'MSP430FG6626'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_208',
					RefTo = '40_108'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkMain_24'
					AND Memories2.PartNumber IN (
						'MSP430F6724',
						'MSP430F6724A',
						'MSP430F6734',
						'MSP430F6734A'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_209',
					RefTo = '40_108'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkMain_25'
					AND Memories2.PartNumber IN (
						'MSP430F6725',
						'MSP430F6725A',
						'MSP430F6735',
						'MSP430F6735A'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_210',
					RefTo = '40_108'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId = 'kBlkMain_22'
					AND Memories2.PartNumber IN (
						'MSP430F6722',
						'MSP430F6732'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_211',
					RefTo = '40_109'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_37', 'kBlkMidRom_0', 'kBlkRam2_6')
					AND Memories2.PartNumber IN (
						'MSP430F5358',
						'MSP430F5658',
						'MSP430F6458',
						'MSP430F6658'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_212',
					RefTo = '40_109'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_38', 'kBlkMidRom_0', 'kBlkRam2_5')
					AND Memories2.PartNumber IN (
						'MSP430F5359',
						'MSP430F5659',
						'MSP430F6459',
						'MSP430F6659'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_213',
					RefTo = '40_109'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_34')
					AND Memories2.PartNumber IN (
						'MSP430F5336',
						'MSP430F6436'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_214',
					RefTo = '40_110'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_42')
					AND Memories2.PartNumber IN (
						'CC430F5135',
						'CC430F6125',
						'CC430F6135'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_215',
					RefTo = '40_110'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_41')
					AND Memories2.PartNumber IN (
						'MSP430F5151',
						'MSP430F5152'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_216',
					RefTo = '40_110'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_33')
					AND Memories2.PartNumber IN (
						'CC430F6126'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_217',
					RefTo = '40_110'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_47')
					AND Memories2.PartNumber IN (
						'CC430F5133'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_218',
					RefTo = '40_115'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_41')
					AND Memories2.PartNumber IN (
						'MSP430F5501',
						'MSP430F5505',
						'MSP430F5508'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_219',
					RefTo = '40_115'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_39')
					AND Memories2.PartNumber IN (
						'MSP430F5502',
						'MSP430F5506',
						'MSP430F5509'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_220',
					RefTo = '40_115'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_46')
					AND Memories2.PartNumber IN (
						'MSP430F5500',
						'MSP430F5504'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_221',
					RefTo = '40_116'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_46')
					AND Memories2.PartNumber IN (
						'MSP430F5131',
						'MSP430F5132'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_222',
					RefTo = '40_118'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_42')
					AND Memories2.PartNumber IN (
						'CC430F5125',
						'CC430F5145',
						'CC430F6145'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_223',
					RefTo = '40_120'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkRam_26')
					AND Memories2.PartNumber IN (
						'MSP430F5517',
						'MSP430F5526',
						'MSP430F5527'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_224',
					RefTo = '40_121'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_34')
					AND Memories2.PartNumber IN (
						'MSP430F5333',
						'MSP430F6433'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_225',
					RefTo = '40_123'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_32')
					AND Memories2.PartNumber IN (
						'MSP430F5310'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_226',
					RefTo = '40_123'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_41')
					AND Memories2.PartNumber IN (
						'MSP430F5308'
					)
				;
				UPDATE
					Memories2
				SET 
					MemGroup = '40_227',
					RefTo = '40_123'
				WHERE
					Memories2.MemGroup IS NULL
					AND Memories2.BlockId IN ('kBlkMain_39')
					AND Memories2.PartNumber IN (
						'MSP430F5309'
					)
				;
				";
			cmd.ExecuteNonQuery();
		}
	}
}
