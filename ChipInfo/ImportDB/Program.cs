using System;

namespace ImportDB
{
	class Program
	{
		static void Main(string[] args)
		{
			Console.WriteLine("ImportDB");
			if (args.Length != 3)
			{
				Console.WriteLine("ERROR: Invalid Command line!");
				Console.WriteLine("USAGE: ImportDB <xml-folder> <csv_slas> <sqlite-file>");
				Console.WriteLine("WHERE:");
				Console.WriteLine("    <xml-folder>  : folder containing TI MSP430 XML database");
				Console.WriteLine("    <csv_slas>    : CSV file created by ScrapeDataSheet tool");
				Console.WriteLine("    <sqlite-file> : output sqlite database file");
			}
			else
			{
				try
				{
					Console.WriteLine("Importing MSP430 XML database from '{0}'...", args[0]);
					XmlDatabase.DoImport(args[0]);
					Console.WriteLine("Importing MSP430 data-sheet data from '{0}'...", args[1]);
					Slas.AllDevices.DoImport(args[1]);
					Console.WriteLine("Validating imported data...");
					CreateDatabase db = new CreateDatabase();
					db.ValidateDatabases();
					Console.WriteLine("Creating SQLite database '{0}'...", args[2]);
					db.MakeSqlite(args[2]);
				}
				catch (Exception ex)
				{
					Console.WriteLine(ex.Message);
				}
			}
		}
	}
}
