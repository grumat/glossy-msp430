//#define SINGLE_FILE

using CsvHelper;
using CsvHelper.Configuration;
using System;
using System.Globalization;
using System.IO;
using static ScrapeDataSheet.ExtractFlashData;

namespace ScrapeDataSheet
{
	class Program
	{
		static void Main(string[] args)
		{
			string base_path = @"D:\[CDCache]\[Electronics]\[Embedded]\[µC]\Texas Instruments\MSP430\DataSheet";
			CsvConfiguration config = new CsvConfiguration(CultureInfo.InvariantCulture)
			{
				Mode = CsvMode.Escape,
				Delimiter = "\t"
			};
			Console.WriteLine("Scrape TI Datasheet (Flash Info)");
			using (FileStream fs = new FileStream("Flash.csv", FileMode.Create, FileAccess.Write, FileShare.ReadWrite))
			using (StreamWriter writer = new StreamWriter(fs))
			using (var csv = new CsvWriter(writer, config))
			{
#if SINGLE_FILE
				string[] files = {
					@"D:\[CDCache]\[Electronics]\[Embedded]\[µC]\Texas Instruments\MSP430\DataSheet\MSP430FR5xx Family\slase66c - Datasheet - MSP430fr597x(1), MSP430fr592x(1), MSP430fr587x(1).pdf",
				};
#else
				string[] files = Directory.GetFiles(base_path, "*.pdf", SearchOption.AllDirectories);
#endif
				csv.WriteHeader<DocProperties>();
				csv.NextRecord();
				foreach (string test_file in files)
				{
					ExtractFlashData scrapper = new ExtractFlashData();
					DocProperties info = scrapper.ScrapePdf(test_file, base_path);
					if (info != null)
					{
						string[] chips = info.Chip.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
						Array.Sort(chips, StringComparer.InvariantCulture);
						foreach (string chip in chips)
						{
							info.Chip = chip;
							csv.WriteRecord(info);
							csv.NextRecord();
						}
					}
					else
					{
						writer.Flush();
						fs.Flush();
					}
				}
			}
		}
	}
}
