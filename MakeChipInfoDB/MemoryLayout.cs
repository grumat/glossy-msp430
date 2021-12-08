using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MakeChipInfoDB
{
	class MemoryLayout
	{
		public string Id;
		public string Ref;
		public List<Tuple<string, string>> MemoryFields = new List<Tuple<string, string>>();

		public MemoryLayout(memoryLayoutType node, string lid, MemoryLayouts lays, ref Memories col)
		{
			Id = lid;
			if (node.@ref != null)
				Ref = lays.ResolveID(GetIdentifier(node.@ref));
			if (node.memory != null)
			{
				foreach (var ff in node.memory)
				{
					string clas = Enums.UnMapMemoryNameType(ff.name);
					string mid = Memory.GetIdentifier(lid + '_' + clas);
					Memory o = new Memory(ff, col.MemAliases_);
					if (o.IsPure())
						MemoryFields.Add(Tuple.Create(clas, o.Ref));
					else
					{
						o.Id = mid;
						mid = col.AddItem(o);
						MemoryFields.Add(Tuple.Create(clas, mid));
					}
				}
			}
		}

		public bool Equals(MemoryLayout o)
		{
			if (Ref != o.Ref
				|| MemoryFields.Count != o.MemoryFields.Count)
				return false;
			// Compare lists
			foreach (Tuple<string, string> l in MemoryFields)
			{
				bool found = false;
				foreach (Tuple<string, string> r in o.MemoryFields)
				{
					if (l.Item1 == r.Item1
						&& l.Item2 == r.Item2)
					{
						found = true;
						break;
					}
				}
				if (found == false)
					return false;
			}
			return true;
		}

		public static string GetIdentifier(string id)
		{
			return "kLyt" + MyUtils.MkIdentifier(id);
		}

		public void DoHFile(TextWriter fh, Memories mems)
		{
			fh.Write("\t{ " + MemoryFields.Count.ToString() + ",\t");
			if (Ref != null)
				fh.WriteLine(Ref + " },");
			else
				fh.WriteLine("kLytNone },");
			foreach (var i in MemoryFields)
			{
				int tmp = mems.ToIdx(i.Item2);
				fh.WriteLine("\t\t{{kClas{0}, {1}}},", i.Item1, tmp);
			}
		}
	}

	class MemoryLayouts
	{
		public Dictionary<string, MemoryLayout> Mems_ = new Dictionary<string, MemoryLayout>();
		public Dictionary<string, string> MemAliases_ = new Dictionary<string, string>();
		public List<string> Phys_ = new List<string>();

		public string AddItem(MemoryLayout o)
		{
			var match = Mems_.FirstOrDefault(x => o.Equals(x.Value));
			if (match.Key != null)
			{
				MemAliases_.Add(o.Id, match.Key);
				return match.Key;
			}
			else
			{
				Mems_.Add(o.Id, o);
				Phys_.Add(o.Id);
				return o.Id;
			}
		}

		public string ResolveID(string id)
		{
			if (MemAliases_.ContainsKey(id))
				return MemAliases_[id];
			return id;
		}

		public string AppendDeviceLayout(memoryLayoutType node, string id, ref Memories mems)
		{
			string lid = null;
			if (node.id != null)
				lid = MemoryLayout.GetIdentifier(node.id);
			else if(id != null)
				lid = MemoryLayout.GetIdentifier(id);
			else
				throw new InvalidDataException("Unable to determine an id for element");
			MemoryLayout m = new MemoryLayout(node, lid, this, ref mems);
			//m.Id = lid;
			return AddItem(m);
		}

		public void DoHFile(TextWriter fh, Memories mems)
		{
			fh.WriteLine();
			fh.WriteLine("enum LytIndexes : uint8_t");
			fh.WriteLine("{");
			foreach (var n in Phys_)
				fh.WriteLine("\t" + n + ",");
			fh.WriteLine("\tkLytNone = 255");
			fh.WriteLine("};");
			fh.WriteLine();
			fh.WriteLine("static constexpr const MemoryLayoutBlob msp430_lyt_set[] =");
			fh.WriteLine("{");
			for (int i = 0; i < Phys_.Count; ++i)
			{
				string n = Phys_[i];
				MemoryLayout o = Mems_[n];
				fh.WriteLine("\t// {0}: {1}", i, n);
				o.DoHFile(fh, mems);
			}
			fh.Write(@"};


ALWAYS_INLINE static const MemoryLayoutInfo *GetLyt(uint8_t idx)
{
	uint32_t c = 0;
	// Iterate the list
	for (uint8_t i = 0; i < idx; ++i)
		c += msp430_lyt_set[c].low_ + 1;	// +1 for the header entry
	return (const MemoryLayoutInfo *)&msp430_lyt_set[c];
}

");
		}
	}
}
