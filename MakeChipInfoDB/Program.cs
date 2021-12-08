using System;
using System.IO;
using System.Xml.Serialization;

namespace MakeChipInfoDB
{
	class Program
	{

		static void Main(string[] args)
		{
			Console.WriteLine("MakeChipInfoDB");
			if (args.Length != 2)
			{
				Console.WriteLine("ERROR: Invalid Command line!");
				Console.WriteLine("USAGE: MakeChipInfoDB <xml-folder> <out-file>");
				Console.WriteLine("ERROR: Please specify path containing MSP430 device info XML files!");
			}
			else
				CreateDatabase.Do(args[0], args[1]);
		}
	}
}
