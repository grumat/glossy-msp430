using System.IO;
using System.Text;

namespace MkChipInfoDbV2.Render
{
	class Utils
	{
		static public string BeatifyEnum(string txt, params object []args)
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

		static public uint BitsRequired(uint cnt)
		{
			if (cnt <= 2)
				return 1;
			if (cnt <= 4)
				return 2;
			if (cnt <= 8)
				return 3;
			if (cnt <= 16)
				return 4;
			if (cnt <= 32)
				return 5;
			if (cnt <= 64)
				return 6;
			if (cnt <= 128)
				return 7;
			if (cnt <= 256)
				return 8;
			throw new InvalidDataException("Not expected value count");
		}
	}
}

