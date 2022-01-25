using System.Text;

namespace MkChipInfoDbV2.Render
{
	class Utils
	{
		static public string BeatifyEnum(string txt, params string []args)
		{
			int col2 = 32;
			txt = string.Format(txt, args);
			int tab = 0;
			int col = 0;
			StringBuilder buf = new StringBuilder();
			foreach (char ch in txt)
			{
				++col;
				if (ch == '\t')
				{
					++tab;
					while ((col % 4) != 0)
						++col;
					if (tab == 2)
					{
						while (col < col2)
						{
							if ((col % 4) == 0)
								buf.Append(ch);
							++col;
						}
					}
				}
				buf.Append(ch);
			}
			return buf.ToString();
		}
	}
}

