using Microsoft.Data.Sqlite;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace MkChipInfoDbV2.Render
{
	class OutputFileRenderer
	{
		protected SqliteConnection Conn;
		protected List<IRender> Engines = new List<IRender>()
		{
			new OuterScope(),
			new MemoryLayout(),
			new MemoryBlocks(),
			new Memories(),
			new Psa(),
			new Cpu(),
			new Eem(),
			new Bits(),
			new Subversion(),
			new Devices(),
		};

		public OutputFileRenderer(SqliteConnection conn)
		{
			Conn = conn;
		}
		public void CreateFile(string fname)
		{
			using (TextWriter fh = new StreamWriter(fname, false, Encoding.Latin1))
			{
				foreach (var r in Engines)
					r.OnPrologue(fh, Conn);
				fh.WriteLine();
				fh.WriteLine();
				foreach (var r in Engines)
					r.OnDeclareConsts(fh, Conn);
				fh.WriteLine();
				fh.WriteLine();
				foreach (var r in Engines)
					r.OnDeclareEnums(fh, Conn);
				fh.WriteLine();
				fh.WriteLine();
				foreach (var r in Engines)
					r.OnDeclareStructs(fh, Conn);
				fh.WriteLine();
				fh.WriteLine();
				foreach (var r in Engines)
					r.OnDefineData(fh, Conn);
				fh.WriteLine();
				fh.WriteLine();
				foreach (var r in Engines)
					r.OnDefineFunclets(fh, Conn);
				fh.WriteLine();
				fh.WriteLine();
				for (int i = Engines.Count-1; i >= 0; --i)
				{
					var r = Engines[i];
					r.OnEpilogue(fh, Conn);
				}
				fh.WriteLine();
				fh.WriteLine();
			}
		}
	}
}
