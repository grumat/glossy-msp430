using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.IO;

namespace MkChipInfoDbV2.Render
{
	class Memories : IRender
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
			fh.Write(@"/*
** Describes a memory block
**
** Following information from original DB can be ignored:
**    Bits: Only exception is Peripheral8bit
**    Mpu: Original DB is inconsistent
**    Mapped: All true except CPU and EEM are mapped
*/
/*struct MemoryInfo
{
	// Start address or kNoMemStart
	AddressStart estart_ : 6;		// 1
	// Size of block (ignored for kNoMemStart)
	BlockSize esize_ : 6;
	// Total memory banks
	uint16_t banks_ : 3;
	// Total memory banks
	SegmentSize segs_ : 3;
};*/

");
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

