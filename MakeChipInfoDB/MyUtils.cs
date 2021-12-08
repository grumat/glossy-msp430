using System;
using System.Collections.Generic;
using System.Text;

namespace MakeChipInfoDB
{
	class MyUtils
	{
		public static string MkIdentifier(string s)
		{
			StringBuilder res = new StringBuilder();
			foreach (var ch in s)
			{
				if (res.Length == 0)
				{
					if (char.IsLetter(ch) || ch == '_')
						res.Append(ch);
					else
						res.Append('_');
				}
				else if(char.IsLetterOrDigit(ch) || ch == '_')
					res.Append(ch);
				else
					res.Append('_');
			}
			return res.ToString();
		}

		internal static Tuple<string, char>[] PrefixTab_ = {
			Tuple.Create("MSP430SL5438A", 'a' )
			, Tuple.Create("MSP430FE42",  'b')
			, Tuple.Create("MSP430AFE2",  'c')
			, Tuple.Create("RF430FRL15",  'd')
			, Tuple.Create("MSP430F67",   'e')
			, Tuple.Create("MSP430FG4",   'f')
			, Tuple.Create("MSP430FG6",   'g')
			, Tuple.Create("MSP430FR2",   'h')
			, Tuple.Create("MSP430FR4",   'i')
			, Tuple.Create("MSP430FR5",   'j')
			, Tuple.Create("MSP430FR6",   'k')
			, Tuple.Create("MSP430F1",    'l')
			, Tuple.Create("MSP430F2",    'm')
			, Tuple.Create("MSP430F4",    'n')
			, Tuple.Create("MSP430F5",    'o')
			, Tuple.Create("MSP430F6",    'p')
			, Tuple.Create("MSP430FW",    'q')
			, Tuple.Create("MSP430G2",    'r')
			, Tuple.Create("MSP430C",     's')
			, Tuple.Create("MSP430L",     't')
			, Tuple.Create("RF430F5",     'u')
			, Tuple.Create("CC430F",      'v')
			, Tuple.Create("MSP430",      'w')
		};

		public static string ChipNameCompress(string s)
		{
			foreach (var t in PrefixTab_)
			{
				if (s.StartsWith(t.Item1))
				{
					string p2 = s.Substring(t.Item1.Length);
					return t.Item2 + p2;
				}
			}
			return s;
		}
	}
}
