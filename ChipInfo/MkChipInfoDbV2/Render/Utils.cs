using System;
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
		static public string BeatifyEnum2(string txt, params object[] args)
		{
			int col2 = 36;
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
			if (cnt <= 512)
				return 9;
			if (cnt <= 1024)
				return 10;
			if (cnt <= 2048)
				return 11;
			if (cnt <= 4096)
				return 12;
			throw new InvalidDataException("Not expected value count");
		}

		static public string GetLargestPrefix(string s1, string s2)
		{
			StringBuilder buf = new StringBuilder();
			int maxs = s1.Length < s2.Length ? s1.Length : s2.Length;
			for (int i = 0; i < maxs; ++i)
			{
				char ch = s1[i];
				if (ch == s2[i])
					buf.Append(ch);
				else
					break;
			}
			return buf.ToString();
		}

		static public string GetIdentifier(string s)
		{
			StringBuilder buf = new StringBuilder();
			foreach (char ch in s)
			{
				if (buf.Length == 0)
				{
					if (char.IsLetter(ch) || ch == '_')
						buf.Append(ch);
					else
						buf.Append('_');
				}
				else
				{
					if (char.IsLetterOrDigit(ch) || ch == '_')
						buf.Append(ch);
					else
						buf.Append('_');
				}
			}
			return buf.ToString();
		}

		static public string BareHex2(long? v)
		{
			return v != null ? String.Format("{0:x2}", v) : "--";
		}

		static public string BareHex4(long? v)
		{
			return v != null ? String.Format("{0:x4}", v) : "----";
		}
	}

	public class SymTable
	{
		StringBuilder Tab = new StringBuilder();
		public int AddString(string s)
		{
			int pos = Tab.Length;
			Tab.Append(s);
			Tab.Append('|');
			return pos;
		}

		public void RenderTable(TextWriter fh, string var_name)
		{
			fh.WriteLine("static constexpr const char {0}[] =", var_name);
			fh.WriteLine("{");
			bool start = true;
			foreach (char ch in Tab.ToString())
			{
				if (start)
				{
					start = false;
					fh.Write('\t');
				}
				if (ch == '|')
				{
					fh.WriteLine("'\\0',");
					start = true;
				}
				else
					fh.Write("'{0}', ", ch);
			}
			fh.WriteLine("};");
			fh.WriteLine();
		}
	}
}

