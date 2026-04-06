using System.Collections.Generic;
using System.Linq;
using Tabula;
using Tabula.Detectors;
using UglyToad.PdfPig;
using UglyToad.PdfPig.Content;
using UglyToad.PdfPig.Core;
using UglyToad.PdfPig.DocumentLayoutAnalysis;
using UglyToad.PdfPig.DocumentLayoutAnalysis.PageSegmenter;
using UglyToad.PdfPig.DocumentLayoutAnalysis.WordExtractor;

namespace ScrapeDataSheet
{
	static class MyPdfWords
	{
		public static IEnumerable<TextBlock> Extract(PdfDocument document, int pg_num)
		{
			Page page = document.GetPage(pg_num);
			List<Word> words = NearestNeighbourWordExtractor.Instance.GetWords(page.Letters).ToList();	// extract words
			if (pg_num == 1)
				return RecursiveXYCut.Instance.GetBlocks(words);										// extract blocks and lines
			return DefaultPageSegmenter.Instance.GetBlocks(words);										// extract blocks and lines
		}

		public static (PageArea, IReadOnlyList<TableRectangle>) GetTables(PdfDocument document, int pg_num)
		{
			PageArea page = ObjectExtractor.ExtractPage(document, pg_num);

			// detect candidate table zones
			SimpleNurminenDetectionAlgorithm detector = new SimpleNurminenDetectionAlgorithm();
			IReadOnlyList<TableRectangle> regions = detector.Detect(page);
			return (page, regions);
		}
	}
}
