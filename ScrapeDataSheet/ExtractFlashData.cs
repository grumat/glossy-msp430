using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using Tabula;
using Tabula.Detectors;
using Tabula.Extractors;
using UglyToad.PdfPig;
using UglyToad.PdfPig.Content;
using UglyToad.PdfPig.DocumentLayoutAnalysis;
using UglyToad.PdfPig.DocumentLayoutAnalysis.PageSegmenter;
using UglyToad.PdfPig.DocumentLayoutAnalysis.ReadingOrderDetector;
using UglyToad.PdfPig.DocumentLayoutAnalysis.WordExtractor;

namespace ScrapeDataSheet
{
	class ColNums
	{
		public int ctc = 0;
		public int cvcc = 0;
		public int cmin = 0;
		public int ctyp = 0;
		public int cmax = 0;
		public int cunit = 0;
	};

	class ExtractFlashData
	{
		static int[] Buckets = new int [100];
		public class Cluster
		{
			public decimal Value;
			public UInt32 Count;

			public Cluster(decimal val)
			{
				Value = val;
				Count = 1;
			}
		};

		public class DocProperties
		{
			public string Manual { get; set; }
			public string Chip { get; set; }
			// Family Users Guide
			public string FamilyUsersGuide { get; set; }
			public decimal? VccPgmMin { get; set; }
			public decimal? VccPgmMax { get; set; }
			public UInt32? fFtgMin { get; set; }
			public UInt32? fFtgMax { get; set; }
			public UInt32? tWord { get; set; }
			public UInt32? tBlock0 { get; set; }
			public UInt32? tBlockI { get; set; }
			public UInt32? tBlockN { get; set; }
			public UInt32? tMassErase { get; set; }
			public UInt32? tSegErase { get; set; }
			public UInt32? fTck2_2V { get; set; }
			public UInt32? fTck3V { get; set; }
			public UInt32? fSbw { get; set; }
			public UInt32? fSbwEn { get; set; }
			public UInt32? fSbwRet { get; set; }
		}

		protected Tuple<double, double> FindClusters(IEnumerable<TextBlock> orderedBlocks)
		{
			List<Cluster> clusters = new List<Cluster>();
			List<Cluster> rowh = new List<Cluster>();
			foreach (var block in orderedBlocks)
			{
				foreach (var line in block.TextLines)
				{
					decimal left = Math.Round(Convert.ToDecimal(line.BoundingBox.Left), 1);
					decimal height = Math.Round(Convert.ToDecimal(line.BoundingBox.Height), 1);
					if (height == 0m)
						height = Math.Round(Convert.ToDecimal(block.BoundingBox.Height / block.TextLines.Count), 1);
					bool add = true;
					for (int i = 0; i < clusters.Count; ++i)
					{
						if (clusters[i].Value == left)
						{
							clusters[i].Count++;
							add = false;
							break;
						}
						if (clusters[i].Value > left)
						{
							clusters.Insert(i, new Cluster(left));
							add = false;
							if (clusters.Count > 3)
								clusters.RemoveAt(clusters.Count - 1);
							break;
						}
					}
					if (add && clusters.Count < 3)
					{
						clusters.Add(new Cluster(left));
					}
					// Row heights
					add = true;
					for (int i = 0; i < rowh.Count; ++i)
					{
						if (rowh[i].Value == height)
						{
							rowh[i].Count++;
							add = false;
							break;
						}
					}
					if (add)
						rowh.Add(new Cluster(height));
				}
			}
			if (clusters.Count == 0)
				return null;
			// Left margin
			double v1;
			if (clusters.Count >=3 && clusters[2].Value - clusters[0].Value < 5.0m)
				v1 = Convert.ToDouble(clusters[2].Value);
			else if (clusters.Count >= 2 && clusters[0].Count < clusters[1].Count)
				v1 = Convert.ToDouble(clusters[1].Value) + 0.1;
			else
				v1 = Convert.ToDouble(clusters[0].Value) + 0.1;
			// Default text height
			UInt32 m = rowh.Max(k => k.Count);
			var v = rowh.Where(x => x.Count == m).FirstOrDefault();
			return new Tuple<double, double>(v1, Convert.ToDouble(v.Value));
		}

		Regex Slau_ = new Regex(@".*(SLAU\d{3,4}[A-Z]?).*");
		(Regex, string)[] UGNames_ = new (Regex, string)[]
		{
				(new Regex(".*MSP430x1xx Family User.s Guide.*"), "SLAU049"),
				(new Regex(".*MSP430F2xx, MSP430G2xx Family User.s Guide.*"), "SLAU144"),
				(new Regex(".*MSP430x2xx Family User.s Guide.*"), "SLAU144"),
				(new Regex(".*MSP430i2xx Family User.s Guide.*"), "SLAU335"),
				(new Regex(".*MSP430x4xx Family User.s Guide.*"), "SLAU056"),
				(new Regex(".*MSP430F5xx and MSP430F6xx Family User.s Guide.*"), "SLAU208"),
				(new Regex(".*MSP430FR4xx and MSP430FR2xx Family User.s Guide.*"), "SLAU445"),
				(new Regex(".*MSP430FR57xx Family User.s Guide.*"), "SLAU272"),
				(new Regex(".*MSP430FR58xx, MSP430FR59xx,? and MSP430FR6xx Family User.s Guide.*"), "SLAU367"),
				(new Regex(".*CC430 Family User.s Guide.*"), "SLAU259"),
		};

		internal bool DoUG_(string prev, string what, ref DocProperties info)
		{
			string total = prev + ' ' + what;
			Match match = Slau_.Match(total);
			if (match.Success)
			{
				info.FamilyUsersGuide = match.Groups[1].Value;
				return true;
			}
			foreach (var tup in UGNames_)
			{
				if (tup.Item1.IsMatch(total))
				{
					info.FamilyUsersGuide = tup.Item2;
					return true;
				}
			}
			return false;
		}

		Regex ChipModel_ = new Regex(@".*((?:MSP|CC|RF)430(?:C|F|G|I|L|FE|FG|FR|FW|SL|AFE)\d{3,5}A?)(.*)?");
		Regex ChipModel_2 = new Regex(@".*(RF430FRL\d{3}H)(.*)?");

		internal void DoChips_(string what, ref List<string> chips)
		{
			char[] seps = { ' ', '\t', '\r', '\n' };
			string[] toks = what.Split(seps, StringSplitOptions.RemoveEmptyEntries);
			foreach (string tok in toks)
			{
				Match match = ChipModel_.Match(tok);
				if (match.Success)
				{
					string tmp = match.Groups[1].Value;
					// Sequences with datasheet patters shall be ignored
					if (match.Groups.Count > 2
						&& match.Groups[2].Value.StartsWith('x'))
						continue;
					if (!chips.Contains(tmp))
						chips.Add(tmp);
				}
				else if (tok == "MSP430TCH5E")
				{
					if (!chips.Contains(tok))
						chips.Add(tok);
				}
				else
				{
					match = ChipModel_2.Match(tok);
					if (match.Success)
					{
						string tmp = match.Groups[1].Value;
						// Sequences with datasheet patters shall be ignored
						if (match.Groups.Count > 2
							&& match.Groups[2].Value.StartsWith('x'))
							continue;
						if (!chips.Contains(tmp))
							chips.Add(tmp);
					}
				}
			}
		}

		protected void GetUserGuide(PdfDocument document, int pg_num, ref DocProperties info, ref List<string> chips)
		{
			IEnumerable<TextBlock> orderedBlocks = MyPdfWords.Extract(document, pg_num);
			foreach (var block in orderedBlocks)
			{
				string prev = "";
				foreach (var line in block.TextLines)
				{
					string what = line.Text.ToString();
					DoChips_(what, ref chips);
					if (info.FamilyUsersGuide == null)
					{
						DoUG_(prev, what, ref info);
					}
					prev = what;
				}
			}
		}

		protected DocProperties GetDatasheet(PdfDocument document)
		{
			Regex Slas = new Regex(@".*(SLAS(?:\d{3,4}|E\d{2}|E[A-Z]\d)[A-Z]?).*");

			DocProperties info = new DocProperties();
			List<string> chips = new List<string>();
			IEnumerable<TextBlock> orderedBlocks = MyPdfWords.Extract(document, 1);

			bool any_data = false;
			foreach (var block in orderedBlocks)
			{
				string prev = "";
				foreach (var line in block.TextLines)
				{
					string what = line.Text.ToString();
					DoChips_(what, ref chips);
					if (info.Manual == null)
					{
						Match match = Slas.Match(what);
						if (match.Success)
						{
							info.Manual = match.Groups[1].Value;
							any_data = true;
							continue;
						}
					}
					if (info.FamilyUsersGuide == null)
					{
						if (DoUG_(prev, what, ref info))
						{
							any_data = true;
							continue;
						}
					}
					prev = what;
				}
			}
			if (any_data == false)
				return null;
			if (info.FamilyUsersGuide == null || chips.Count < 4)
			{
				int pg_num = 2;
				GetUserGuide(document, pg_num, ref info, ref chips);
				while ((info.FamilyUsersGuide == null || chips.Count == 0) && pg_num < 5)
				{
					++pg_num;
					GetUserGuide(document, pg_num, ref info, ref chips);
				}
			}
			info.Chip = String.Join(", ", chips);
			return info;
		}

		protected bool HasFlashMemory(PdfDocument document, int pg_num)
		{
			Regex fmem = new Regex(@"^(?:table\s)?(?:[\d.\-]{3,9}\s+)?flash\s+memory(?:\s\(.*\))?$");

			IEnumerable<TextBlock> orderedBlocks = MyPdfWords.Extract(document, pg_num);

			Tuple<double, double> sizes = FindClusters(orderedBlocks);

			if (sizes == null)
				return false;

			foreach (var block in orderedBlocks)
			{
				foreach (var line in block.TextLines)
				{
					if (line.BoundingBox.Height - sizes.Item2 > 1.3 // Title Height
						|| line.BoundingBox.Height == 0)            // some files fails here...
					{
						if (fmem.IsMatch(line.Text.ToLower()))
							return true;
					}
				}
			}
			return false;
		}

		Regex RexJtag_ = new Regex(@"^(?:table\s)?(?:[\d.\-]{3,9}\s+)?(?:(?:jtag,?\s+)|(?:4\-wire,?\s+)|(?:and\s+)|(?:spy\-bi\-wire\s+)|(?:\(sbw\)\s+)){1,4}interface(?:\s.*)?$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant | RegexOptions.Compiled);
		Regex RexJtag2_ = new Regex(@"^(?:[\d.\-]{3,9}\s+)?jtag$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant | RegexOptions.Compiled);

		protected bool HasJtag(PdfDocument document, int pg_num)
		{
			IEnumerable<TextBlock> orderedBlocks = MyPdfWords.Extract(document, pg_num);

			Tuple<double, double> sizes = FindClusters(orderedBlocks);

			if (sizes == null)
				return false;

			foreach (var block in orderedBlocks)
			{
				foreach (var line in block.TextLines)
				{
					if (line.BoundingBox.Height - sizes.Item2 > 1.3 // Title Height
						|| line.BoundingBox.Height == 0)            // some files fails here...
					{
						string what = line.Text.ToLower();
						if (RexJtag_.IsMatch(what)
							|| RexJtag2_.IsMatch(what))
							return true;
					}
				}
			}
			return false;
		}

		internal static string MkId(string key)
		{
			StringBuilder buf = new StringBuilder();
			foreach (char ch in key)
			{
				if (!char.IsWhiteSpace(ch))
					buf.Append(ch);
			}
			return buf.ToString().ToLower();
		}

		static bool IsFlashTime(string s)
		{
			if (s == "µs"
				|| s == "\u03bcs"
				|| s == "us"
				|| s == "ms"
				|| s == "ns")
			{
				return true;
			}
			return false;
		}

		protected bool GetFlashParams(PdfDocument document, int pg_num, ref DocProperties doc)
		{
			(PageArea page, List<TableRectangle> regions) = MyPdfWords.GetTables(document, pg_num);

			IExtractionAlgorithm ea = new BasicExtractionAlgorithm();
			foreach (var region in regions)
			{
				List<Table> tables = ea.Extract(page.GetArea(region.BoundingBox)); // take first candidate area
				foreach (var table in tables)
				{
					bool table_found = false;
					for(int i = 0; i < table.Rows.Count; ++i)
					{
						IReadOnlyList<Cell> row = table.Rows[i];
						if (row.Count >= 7)
						{
							int c1 = row.Count - 4;
							int c2 = row.Count - 3;
							int c3 = row.Count - 2;
							int c4 = row.Count - 1;
							string key = MkId(row[0].GetText());
							if (String.IsNullOrEmpty(key))
								continue;
							/*
							 * This handles old PDF files where informed font height is nearly 0.
							 */
							if ((key == "f" || key == "t" || key == "v")
								&& row[0].BoundingBox.Height <= 2.0)	// overlapping areas surely fails
							{
								int j = i + 1;
								if (i < table.Rows.Count)
								{
									IReadOnlyList<Cell> r2 = table.Rows[j];
									key += MkId(r2[0].GetText());
									++i;
								}
							}
							if (key == "vcc(pgm/" 
								|| key == "vcc(pgm/erase)"
								|| key == "dvcc(pgm,erase)"
								|| key == "dvcc(pgm/erase)")
							{
								table_found = true;
								string vmin = row[c1].GetText();
								string vmax = row[c3].GetText();
								if (String.IsNullOrEmpty(vmin))
								{
									IReadOnlyList<Cell> next = table.Rows[i + 1];
									vmin = next[c1].GetText();
									vmax = next[c3].GetText();
									++i;
								}
								doc.VccPgmMin = Convert.ToDecimal(vmin, CultureInfo.InvariantCulture);
								doc.VccPgmMax = Convert.ToDecimal(vmax, CultureInfo.InvariantCulture);
							}
							else if (key == "fftg")
							{
								table_found = true;
								doc.fFtgMin = Convert.ToUInt32(row[c1].GetText());
								doc.fFtgMax = Convert.ToUInt32(row[c3].GetText());
							}
							else if (key == "tword")
							{
								table_found = true;
								// CPUXv2!!
								if (!IsFlashTime(row[c4].GetText()))
									doc.tWord = Convert.ToUInt32(row[c2].GetText());
							}
							else if (key == "tblock,0")
							{
								table_found = true;
								// CPUXv2!!
								if (!IsFlashTime(row[c4].GetText()))
									doc.tBlock0 = Convert.ToUInt32(row[c2].GetText());
							}
							else if (key == "tblock,1-63" || key == "tblock,1\u201363")
							{
								table_found = true;
								if (!IsFlashTime(row[c4].GetText()))
									doc.tBlockI = Convert.ToUInt32(row[c2].GetText());
							}
							else if (key == "tblock,end")
							{
								table_found = true;
								if (!IsFlashTime(row[c4].GetText()))
									doc.tBlockN = Convert.ToUInt32(row[c2].GetText());
							}
							else if (key == "tmasserase")
							{
								table_found = true;
								if (!IsFlashTime(row[c4].GetText()))
									doc.tMassErase = Convert.ToUInt32(row[c2].GetText());
							}
							else if (key == "tsegerase")
							{
								table_found = true;
								if (!IsFlashTime(row[c4].GetText()))
									doc.tSegErase = Convert.ToUInt32(row[c2].GetText());
							}
						}
					}
					// table found
					if (table_found)
						return true;
				}
			}
			return false;
		}

		protected bool GetJtagParams(PdfDocument document, int pg_num, ref DocProperties doc)
		{
			(PageArea page, List<TableRectangle> regions) = MyPdfWords.GetTables(document, pg_num);

			bool data_found = false;
			IExtractionAlgorithm ea = new BasicExtractionAlgorithm();
			foreach (var region in regions)
			{
				List<Table> tables = ea.Extract(page.GetArea(region.BoundingBox)); // take first candidate area
				foreach (var table in tables)
				{
					ColNums cols = new ColNums();

					int hdr = Math.Min(4, table.Rows.Count);
					for (int i = 0; i < hdr; ++i)
					{
						IReadOnlyList<Cell> row = table.Rows[i];
						IReadOnlyList<Cell> next = table.Rows[i+1];
						for (int col = 0; col < row.Count; ++col)
						{
							Cell cell = row[col];
							string tmp = MkId(cell.GetText());
							if (String.IsNullOrEmpty(tmp))
								continue;
							double sz = (10.0 / 14.0) * cell.TextElements[0].TextElements[0].FontSize;
							Cell cdown = next[col];
							string tmp2 = MkId(cdown.GetText());
							if (!String.IsNullOrEmpty(tmp2)
								&& (cell.Bottom - cdown.Bottom) <= sz)
							{
								tmp += tmp2;
							}
							if (tmp == "parameter")
								hdr = i;    // got the header line
							else if (tmp == "testconditions")
								cols.ctc = col;
							else if (tmp == "vcc" || tmp == "vddb")
								cols.cvcc = col;
							else if (tmp == "min")
								cols.cmin = col;
							else if (tmp == "typ"
								|| tmp == "nom")
								cols.ctyp = col;
							else if (tmp == "max")
								cols.cmax = col;
							else if (tmp == "unit")
								cols.cunit = col;
						}
					}
					if (cols.cvcc == 0)
						cols.cvcc = cols.ctc;
					if (cols.cvcc != 0
						&& cols.cmin != 0
						&& cols.ctyp != 0
						&& cols.cmax != 0
						&& cols.cunit != 0)
					{
						for (int i = hdr + 1; i < table.Rows.Count; ++i)
						{
							IReadOnlyList<Cell> row = table.Rows[i];
							if (row.Count >= 7)
							{
								string key = MkId(row[0].GetText());
								if (String.IsNullOrEmpty(key))
									continue;

								if (key == "ftck"
									|| key == "fftck"       // extraction engine double letters sometimes
									|| key == "ftckftck")
								{
									data_found = true;
									string svcc = MkId(row[cols.cvcc].GetText());
									if (String.IsNullOrEmpty(svcc)
										&& !String.IsNullOrEmpty(MkId(row[cols.cunit].GetText())) )
									{
										// rare, but sometimes column alignment fails
										string tmp = MkId(row[cols.cvcc - 1].GetText());
										if (!String.IsNullOrEmpty(tmp) 
											&& tmp.EndsWith('v')
											&& Char.IsDigit(tmp[0]))
										{
											svcc = tmp;
										}
									}
									if (svcc == "2.2v" || svcc == "2v" || svcc == "1.5v")
									{
										doc.fTck2_2V = Convert.ToUInt32(row[cols.cmax].GetText());
										if (i + 1 < table.Rows.Count)
										{
											IReadOnlyList<Cell> next = table.Rows[i + 1];
											string svcc2 = MkId(next[cols.cvcc].GetText());
											// Note that some data-sheets don't publish 3V values
											if (svcc == "3v" || svcc == "3.0v")
												doc.fTck3V = Convert.ToUInt32(next[cols.cmax].GetText());
										}
									}
									else if (svcc == "3v" || svcc == "3.0v")
									{
										doc.fTck3V = Convert.ToUInt32(row[cols.cmax].GetText());
										IReadOnlyList<Cell> prev = table.Rows[i - 1];
										if (String.IsNullOrEmpty(prev[0].GetText()))
											doc.fTck2_2V = Convert.ToUInt32(prev[cols.cmax].GetText());
									}
									else if (svcc == "2v,3v" || svcc == "2.0v,3.0v")
									{
										doc.fTck3V = doc.fTck2_2V = Convert.ToUInt32(row[cols.cmax].GetText());
									}
									else if (String.IsNullOrEmpty(svcc))
									{
										IReadOnlyList<Cell> prev = table.Rows[i - 1];
										string svcc2 = MkId(prev[cols.cvcc].GetText());
										if (svcc2 == "2.2v" || svcc2 == "2v")
											doc.fTck2_2V = Convert.ToUInt32(prev[cols.cmax].GetText());
										IReadOnlyList<Cell> next = table.Rows[i + 1];
										svcc2 = MkId(next[cols.cvcc].GetText());
										if (svcc2 == "3v" || svcc2 == "3.0v")
											doc.fTck3V = Convert.ToUInt32(next[cols.cmax].GetText());
									}
									else
									{
										throw new InvalidDataException("Malformed Table");
									}
								}
								else if (key == "fsbw")
								{
									string smhz = MkId(row[cols.cunit].GetText());
									if (smhz == "mhz")
									{
										data_found = true;
										doc.fSbw = Convert.ToUInt32(row[cols.cmax].GetText());
									}
								}
								else if (key == "tsbw,en")
								{
									string sunit = MkId(row[cols.cunit].GetText());
									if (IsFlashTime(sunit))
									{
										data_found = true;
										doc.fSbwEn = Convert.ToUInt32(row[cols.cmax].GetText());
									}
								}
								else if (key == "tsbw,ret"
									|| key == "tsbw,rst")
								{
									string sunit = MkId(row[cols.cunit].GetText());
									if (IsFlashTime(sunit))
									{
										data_found = true;
										doc.fSbwRet = Convert.ToUInt32(row[cols.cmin].GetText());
									}
								}
								// Handles SLAS368G which produces height letter == 0 and disturbs all stuff
								else if (key == "f" || key == "ff")
								{
									// BAD, BAD, BAD, but I don't want to loose more time on a
									// better solution. Perfect is the enemy of good...
									IReadOnlyList<Cell> next = table.Rows[i + 1];
									key = MkId(next[0].GetText());
									if (key == "tck")
									{
										data_found = true;
										IReadOnlyList<Cell> prev = table.Rows[i - 1];
										doc.fTck2_2V = Convert.ToUInt32(prev[cols.cmax].GetText());
										next = table.Rows[i + 2];
										doc.fTck3V = Convert.ToUInt32(next[cols.cmax].GetText());
									}
								}
							}
						}
					}
				}
				if (data_found)
					return true;
			}
			return false;
		}

		protected void UpdateBucket(int pg, PdfDocument document)
		{
			int idx = pg * 100 / document.NumberOfPages;
			++Buckets[idx];
		}

		HashSet<string> HasFram = new HashSet<string>()
		{
			"SLAU272",
			"SLAU321",
			"SLAU367",
			"SLAU445",
			"SLAU506",
		};
		protected bool ScanChipParameters(PdfDocument document, ref DocProperties doc)
		{
			int p0 = 4 * document.NumberOfPages / 100;		// 0
			int p1 = 10 * document.NumberOfPages / 100;		// 2/3 = 0.7
			int p2 = 13 * document.NumberOfPages / 100;		// 56/11 = 2.4
			int p3 = 24 * document.NumberOfPages / 100;		// 2/7 = 0.3
			int p4 = 31 * document.NumberOfPages / 100;		// 175/19 = 9.2
			int p5 = 50 * document.NumberOfPages / 100;		// 27/5 = 5.4
			int p6 = 55 * document.NumberOfPages / 100;		// 105/17 = 6.2
			int p7 = 72 * document.NumberOfPages / 100;		// 0
			int p8 = 98 * document.NumberOfPages / 100;		// 0
			var ranges = new(int, int)[]
			{
				(p4, p5),	// Quote = 9.2
				(p6, p7),	// Quote = 6.2
				(p5, p6),	// Quote = 5.4
				(p2, p3),	// Quote = 2.4
				(p1, p2),	// Quote = 0.7
				(p3, p4),	// Quote = 0.3
				(p0, p1),	// Quote = 0
				(p7, p8),	// Quote = 0
			};
			bool found_flash = HasFram.Contains(doc.FamilyUsersGuide);
			bool found_jtag = false;
			foreach (var range in ranges)
			{
				for (int pg = range.Item1; pg < range.Item2; ++pg)
				{
					if (found_flash == false && HasFlashMemory(document, pg))
					{
						UpdateBucket(pg, document);
						Console.Write("  Found 'Flash Memory' candidate on page: {0}... ", pg);
						if (GetFlashParams(document, pg, ref doc))
						{
							Console.WriteLine("OK!");
							found_flash = true;
						}
						else
							Console.WriteLine("No table found!");
					}
					if (found_jtag == false && HasJtag(document, pg))
					{
						UpdateBucket(pg, document);
						Console.Write("  Found 'JTAG interface' candidate on page: {0}... ", pg);
						if (GetJtagParams(document, pg, ref doc))
						{
							Console.WriteLine("OK!");
							if (HasJtag(document, pg + 1))
							{
								Console.WriteLine("  Found 'JTAG interface continued...' candidate on page: {0}... ", pg+1);
								GetJtagParams(document, pg+1, ref doc);
							}
							found_jtag = true;
						}
						else
							Console.WriteLine("No table found!");
					}
					if (found_flash && found_jtag)
						return true;
				}
			}
			if(found_flash == false)
				Console.WriteLine("  FAILURE TO LOCATE FLASH DATA!");
			if (found_jtag == false)
				Console.WriteLine("  FAILURE TO LOCATE JTAG DATA!");
			return false;
		}

		internal void PutChipList(string allchips)
		{
			StringBuilder buf = new StringBuilder();
			string[] chips = allchips.Split(", ");
			int lim = 65;
			foreach (string chip in chips)
			{
				if (buf.Length != 0)
					buf.Append(", ");
				if (buf.Length + chip.Length > lim)
				{
					Console.WriteLine(buf.ToString());
					buf.Clear();
					buf.Append(' ', 14);
					lim = 77;
				}
				buf.Append(chip);
			}
			buf.Append(')');
			Console.WriteLine(buf.ToString());
		}

		static public void PutHitStats()
		{
			// Make mean values
			int sum = 0;
			for (int i = 0; i < Buckets.Length; ++i)
				sum += Buckets[i];
			for (int i = 0; i < Buckets.Length; ++i)
				Buckets[i] = Buckets[i] * 800 / sum;
			// Draw histogram
			for (int i = 0; i < Buckets.Length; ++i)
			{
				string bar = new string('*', Buckets[i]);
				Console.WriteLine("{0,2} {1}", i, bar);
			}
		}

		public DocProperties ScrapePdf(string pdf_file, string base_path)
		{
			Console.Write("Trying file '{0}'... ", Path.GetRelativePath(base_path, pdf_file) );
			using (PdfDocument document = PdfDocument.Open(pdf_file, new ParsingOptions() { ClipPaths = true }))
			{
				DocProperties info = GetDatasheet(document);
				if (info == null || info.Manual == null)
				{
					Console.WriteLine("Not a datasheet!");
					return null;
				}
				Console.WriteLine("OK!");
				Console.Write("  '{0}' (", info.Manual);
				PutChipList(info.Chip);
				ScanChipParameters(document, ref info);
				return info;
			}
		}
	}
}
