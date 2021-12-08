using System;
using System.Collections.Generic;
using System.IO;

namespace MakeChipInfoDB
{
	class CreateDatabase
	{
		public static int Do(string folder, string out_file)
		{
			XmlManager mng = new XmlManager();
			string deffile = Path.Combine(folder, "defaults.xml");
			mng.LoadXml(deffile);
			string[] all_files = Directory.GetFiles(folder);
			Array.Sort(all_files, StringComparer.Ordinal);
			foreach (string fname in all_files)
			{
				if (Path.GetExtension(fname).ToLower() != ".xml")
					continue;
				if (clash_.Contains(Path.GetFileName(fname)))
					continue;
				mng.LoadXml(fname);
			}

			Console.WriteLine();
			Console.WriteLine("Optimized {0} memory records", mng.Mems_.MemAliases_.Count);
			Console.WriteLine("Optimized {0} memory layout records", mng.Lyts_.MemAliases_.Count);

			FileRenderer render = new FileRenderer();
			render.WriteFile(out_file, mng);

			return 0;
		}
		static HashSet<string> clash_ = new HashSet<string>()
		{
			"defaults.xml", "p401x.xml", "legacy.xml"
		};
	}
}
