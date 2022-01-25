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
			new Memories(),
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
				foreach (var r in Engines)
					r.OnDeclareConsts(fh, Conn);
				foreach (var r in Engines)
					r.OnDeclareEnums(fh, Conn);
				foreach (var r in Engines)
					r.OnDeclareStructs(fh, Conn);
				for(int i = Engines.Count-1; i >= 0; --i)
				{
					var r = Engines[i];
					r.OnEpilogue(fh, Conn);
				}
			}
		}
	}
}
