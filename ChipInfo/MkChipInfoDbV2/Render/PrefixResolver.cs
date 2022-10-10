using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MkChipInfoDbV2.Render
{
	class PrefixResolver : IRender
	{
		Dictionary<string, int> AllPrefixes = new Dictionary<string, int>();
		static List<Tuple<string, string>> PrefSlau = new List<Tuple<string, string>>();

		int MaxLen = 0;
		Dictionary<string, string> ClassifyParts(SqliteConnection conn)
		{
			Dictionary<string, string> all_parts = new Dictionary<string, string>();
			string sql = @"
				SELECT DISTINCT
					PartNumber, UsersGuide
				FROM
					Chips
				ORDER BY 1
			";
			string last = "";
			foreach (var row in conn.Query(sql))
			{
				string k1 = row.UsersGuide + '.' + row.PartNumber;
				string prefix = Utils.GetLargestPrefix(last, k1);
				if (prefix.Length >= 15)
				{
					bool ends_ok = Char.IsDigit(prefix[prefix.Length - 1]);
					if(ends_ok)
					{
						if (AllPrefixes.ContainsKey(prefix))
							AllPrefixes[prefix] += 1;
						else
							AllPrefixes[prefix] = 2;
					}
				}
				last = k1;
				if (MaxLen == 0 || MaxLen < last.Length)
					MaxLen = last.Length;
				all_parts.Add(row.PartNumber, row.UsersGuide);
			}
			return all_parts;
		}

		void Level1_Merge()
		{
			string[] groups = AllPrefixes.Keys.ToArray();
			Array.Sort(groups);
			for (int kl = MaxLen; kl > 16; --kl)
			{
				bool modified = true;
				int last_i = 1;
				while (modified)
				{
					modified = false;
					for (int i = last_i; !modified && i < groups.Length; ++i)
					{
						string k1 = groups[i - 1];
						string k2 = groups[i];
						if (k1.Length > kl
							&& k1.Length == k2.Length)
						{
							string k3 = Utils.GetLargestPrefix(k1, k2);
							if (k3.Length > 15)
							{
								bool ends_ok = Char.IsDigit(k3[k3.Length-1]);
								if (ends_ok && k3.Length >= k1.Length - 1)
								{
									modified = true;
									last_i = i;
									HashSet<string> keys = new HashSet<string>() { k1, k2 };
									for (int j = i + 1; j < groups.Length; ++j)
									{
										k1 = groups[j];
										if (k1.StartsWith(k3))
											keys.Add(k1);
									}
									int cnt = 0;
									foreach (string k in keys)
									{
										cnt += AllPrefixes[k];
										AllPrefixes.Remove(k);
									}
									if (AllPrefixes.ContainsKey(k3))
										AllPrefixes[k3] += cnt;
									else
										AllPrefixes[k3] = cnt;
									groups = AllPrefixes.Keys.ToArray();
									Array.Sort(groups);
								}
							}
						}
					}
				}
			}
		}
		void Level2_Merge()
		{
			string[] groups = AllPrefixes.Where(x => x.Value < 15).Select(k => k.Key).ToArray();
			Array.Sort(groups);
			foreach (string k1 in groups)
			{
				// Locate largest match in current elements
				string knew = k1;
				int klen = 0;
				foreach (string k2 in AllPrefixes.Where(x => x.Key != k1).Select(k => k.Key))
				{
					if (k2.Length > klen && k1.StartsWith(k2))
					{
						knew = k2;
						klen = k2.Length;
					}
				}
				// Match found
				if (klen != 0)
				{
					AllPrefixes[knew] += AllPrefixes[k1];
					AllPrefixes.Remove(k1);
				}
			}
		}
		static Tuple<string, string> GetBestKey(string part_name)
		{
			foreach (var t in PrefSlau)
			{
				if (part_name.StartsWith(t.Item1))
				{
					return t;
				}
			}
			return null;
		}
		static int? GetBestKeyIdx(string part_name)
		{
			for (int i = 0; i < PrefSlau.Count; ++i)
			{
				var t = PrefSlau[i];
				if (part_name.StartsWith(t.Item1))
				{
					return i;
				}
			}
			return null;
		}
		int mycomp_(Tuple<string, string> t1, Tuple<string, string> t2)
		{
			int d = t2.Item1.Length - t1.Item1.Length;
			if (d == 0)
			{
				d = t2.Item1.CompareTo(t1.Item1);
			}
			return d;
		}
		void AssociateUsersGuide(Dictionary<string, string> parts)
		{
			PrefSlau.Add(Tuple.Create("MSP430", "SLAU144"));
			foreach (var k in AllPrefixes.Keys)
			{
				string[] toks = k.Split('.', 2);
				PrefSlau.Add(Tuple.Create(toks[1], toks[0]));
			}
			PrefSlau.Sort(mycomp_);
		}
		void CollectBlackSheeps(Dictionary<string, string> parts)
		{
			foreach (var p in parts)
			{
				Tuple<string, string> t = GetBestKey(p.Key);
				if (t == null)
				{
					PrefSlau.Add(Tuple.Create(p.Key, p.Value));
					PrefSlau.Sort(mycomp_);
				}
				else
				{
					string slau = t.Item2;
					if (slau != p.Value)
					{
						PrefSlau.Add(Tuple.Create(p.Key, p.Value));
						PrefSlau.Sort(mycomp_);
					}
				}
			}
		}
		public void OnPrologue(TextWriter fh, SqliteConnection conn)
		{
			Dictionary<string, string> all_parts = ClassifyParts(conn);
			Level1_Merge();
			Level2_Merge();
			AssociateUsersGuide(all_parts);
			CollectBlackSheeps(all_parts);
			AllPrefixes.Clear();
		}

		public void OnDeclareConsts(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareEnums(TextWriter fh, SqliteConnection conn)
		{
		}

		public void OnDeclareStructs(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"// Part name prefix resolver (First byte of name_) and TI SLAU number
struct PrefixResolver
{
	// Chip part number prefix
	uint8_t prefix_;				// 0

	// TI User's guide
	EnumSlau family_;				// 4
};									// Structure size = 5 bytes

");
		}

		public void OnDefineData(TextWriter fh, SqliteConnection conn)
		{
			SymTable symtab = new SymTable();

			fh.WriteLine("// Decompress titles using a char to string map");
			fh.WriteLine("static constexpr const PrefixResolver msp430_part_name_prefix[] =");
			fh.WriteLine("{");
			fh.WriteLine("\t// Ordered from larger string to shorter");
			fh.WriteLine("\t// This lookup table also associates exact Users Guide");
			foreach (var t in PrefSlau)
			{
				int idx = symtab.AddString(t.Item1) / 2;
				fh.WriteLine("\t{{ {0,3}, k{1} }},\t// {2}", idx, t.Item2, t.Item1);
			}
			fh.WriteLine("\t// First char of all Device names have to be subtracted from '0'");
			fh.WriteLine("\t// this offset value forms an index to this lookup table.");
			fh.WriteLine("\t// The prefix_ member should be copied to a buffer to hold the part");
			fh.WriteLine("\t// name and concatenated to the remaining chars found on the specific");
			fh.WriteLine("\t// Device name.");
			fh.WriteLine("\t// Users Guide mandates how Flash operations will be performed and other");
			fh.WriteLine("\t// stuff.");
			fh.WriteLine("};");
			fh.WriteLine();

			symtab.RenderTable(fh, "sym_tab_prefix");
		}

		public void OnDefineFunclets(TextWriter fh, SqliteConnection conn)
		{
			fh.Write(@"
// Utility to decompress the chip Part number
ALWAYS_INLINE static void DecompressChipName(char *t, const char *s)
{
	// Offset in symbol table
	const size_t p = msp430_part_name_prefix[*s - '0'].prefix_ * 2UL;
	// Position in symbol table
	const char *f = sym_tab_prefix + p;
	// Since index was divided by 2 we may hit the terminator of the previous string
	if (*f == 0)
		++f;
	// Copy prefix...
	while (*f)
		*t++ = *f++;
	// Position suffix
	++s;
	// Copy string or append suffix
	while (*s)
		*t++ = *s++;
	*t = 0;
}

// Utility to retrieve TI's User's Guide SLAU number
ALWAYS_INLINE static EnumSlau MapToChipToSlau(const char *s)
{
	return msp430_part_name_prefix[*s - '0'].family_;
}
");
		}

		public void OnEpilogue(TextWriter fh, SqliteConnection conn)
		{
		}

		static public string EncodeChipName(string part_name)
		{
			int? idx = GetBestKeyIdx(part_name);
			if (idx == null)
				throw new InvalidDataException("Chip not in database. Are you passing a part number of the database?");
			char k = (char)((int)'0' + idx);
			var t = PrefSlau[(int)idx];
			return k.ToString() + part_name.Substring(t.Item1.Length);
		}
	}
}

