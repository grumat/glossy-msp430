﻿using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;

namespace UnitTest
{
	internal class MemBlock
	{
		public MemoryTypeType mem_type_;
		public UInt32 mem_start_;
		public UInt32 mem_size_;
		public bool is_info_;

		public override bool Equals(Object? obj)
		{
			MemBlock? other = obj as MemBlock;
			if (other == null)
				return false;
			// FRAM shall be declared as RAM for GDB
			MemoryTypeType mt1 = mem_type_ == MemoryTypeType.fram ? MemoryTypeType.ram : mem_type_;
			MemoryTypeType mt2 = other.mem_type_ == MemoryTypeType.fram ? MemoryTypeType.ram : other.mem_type_;
			return mt1 == mt2
				&& mem_start_ == other.mem_start_
				&& mem_size_ == other.mem_size_
				&& is_info_ == other.is_info_;
		}

		public override int GetHashCode()
		{
			return (mem_type_.GetHashCode() << 24)
				^ (mem_start_.GetHashCode() << 16)
				^ mem_size_.GetHashCode();
		}
	}

	public enum PeriphBus
	{
		Bus8bit,
		Bus16bit,
		Bus8and16bit,
	}

	internal struct PeriphReg
	{
		// Name of the Hardware Register
		public String name;
		// Address of the register
		public UInt32 addr;
		// Reset value of the register
		public UInt16 value;
		// Reset value of the register
		public UInt16 mask;
		// Bus width for this register
		public PeriphBus bus;
	}

	internal class TestsBase
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		// Creates the Test suite
		public TestsBase(IComm comm, string chip)
		{
			comm_ = comm;

			chipdbtest db = ReadDB();
			chipdbtestChip? model = null;
			foreach (chipdbtestChip c in db.chip)
			{
				if (c.name == chip)
				{
					model = c;
					break;
				}
			}
			if(model == null
				|| model.memorymap == null)
				throw new Exception(String.Format("Cannot locate information for chip '{0}'", chip));
			chip_ = model;
			foreach (chipdbtestChipMemory m in model.memorymap)
			{
				MemBlock memBlock = new MemBlock();
				memBlock.mem_type_ = m.type;
				if (!Utility.ConvertUint32C(m.start, out memBlock.mem_start_))
				{
					throw new Exception(String.Format("ERROR! Convert value '{0}' to numeric", m.start));
				}
				if (!Utility.ConvertUint32C(m.length, out memBlock.mem_size_))
				{
					throw new Exception(String.Format("ERROR! Convert value '{0}' to numeric", m.length));
				}
				if (m.infoSpecified)
					memBlock.is_info_ = m.info;
				mem_blocks_.Add(memBlock);
			}
			if (model.testregisters != null)
			{
				foreach (chipdbtestChipRegister r in model.testregisters)
				{
					PeriphReg reg = new PeriphReg();
					reg.name = r.name;
					if (!Utility.ConvertUint32C(r.start, out reg.addr))
						throw new Exception(String.Format("ERROR! Convert value '{0}' to numeric", r.start));
					if (!Utility.ConvertUint16C(r.value, out reg.value))
						throw new Exception(String.Format("ERROR! Convert value '{0}' to numeric", r.value));
					switch (r.bus)
					{
					case RegBusType.Item8bit:
						reg.bus = PeriphBus.Bus8bit;
						break;
					case RegBusType.Item16bit:
						reg.bus = PeriphBus.Bus16bit;
						break;
					case RegBusType.both:
						reg.bus = PeriphBus.Bus8and16bit;
						break;
					}
					if (String.IsNullOrEmpty(r.mask))
						reg.mask = 0xFFFF;
					else if (!Utility.ConvertUint16C(r.mask, out reg.mask))
						throw new Exception(String.Format("ERROR! Convert value '{0}' to numeric", r.mask));

					periph_regs.Add(reg);
				}
			}
		}

		protected static string GetAppPath()
		{
			string strExeFilePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
			string? strWorkPath = Path.GetDirectoryName(strExeFilePath);
			if (strWorkPath == null)
				throw new Exception("Failed to retrieve application path");
			return strWorkPath;
		}

		protected static chipdbtest ReadDB()
		{
			string ship_db_path = Path.Combine(GetAppPath(), "ChipDB.xml");
			XmlSerializer ser = new XmlSerializer(typeof(chipdbtest));
			chipdbtest? db;
			using (XmlReader reader = XmlReader.Create(ship_db_path))
				db = (chipdbtest?)ser.Deserialize(reader);
			if (db == null)
				throw new Exception("Failed to parse ChipDB.xml");
			return db;
		}

		public static void ListMcus()
		{
			Console.WriteLine("MCU LIST");
			chipdbtest db = ReadDB();
			foreach (chipdbtestChip c in db.chip)
			{
				Console.WriteLine("  {0}", c.name);
				foreach (chipdbtestChipMemory m in c.memorymap)
					Console.WriteLine("    {0,-6} {1,6} {2, 7}", m.type, m.start, m.length);
			}
		}

		protected bool WriteMemCompatible(UInt32 addr, Span<byte> buffer)
		{
			StringBuilder sb = new StringBuilder("M");
			sb.Append(addr.ToString("x"));
			sb.Append(',');
			sb.Append(buffer.Length.ToString("x"));
			sb.Append(':');
			foreach (byte b in buffer)
			{
				sb.Append(b.ToString("x2"));
			}
			comm_.Send(sb.ToString());
			// Get response
			String msg;
			if (!GetReponseString(out msg))
				return false;
			if (msg != "OK")
			{
				Utility.WriteLine("  ERROR! Unexpected reply: {0}", msg);
				return false;
			}
			return true;
		}

		protected void SendMonitor(String msg)
		{
			StringBuilder sb = new StringBuilder("qRcmd,");
			foreach (char ch in msg)
				sb.Append(Convert.ToByte(ch).ToString("x2"));
			comm_.Send(sb.ToString());
		}

		// Receive a standard response string
		protected bool GetReponseString(out String msg, out String raw)
		{
			// Decode and unescape the stream
			var res = rcv_.ReceiveString(comm_, out msg, out raw);
			// A NAK means that request was malformed
			if (res == GdbInData.State.nak)
			{
				Utility.WriteLine("  NAK");
				return false;
			}
			// Target may have stopped responding
			if (res == GdbInData.State.timeout)
			{
				Utility.WriteLine("  {0}", raw);
				Utility.WriteLine("  TIMEOUT");
				return false;
			}
			// Target may respond invalid stuff
			if (res == GdbInData.State.proto)
			{
				Utility.WriteLine("  {0}", raw);
				Utility.WriteLine("  PROTOCOL ERROR");
				return false;
			}
			// Accept response even if checksum is bad
			if (comm_.AckMode)
			{
				comm_.SendAck();
			}
			// Tests permanently rejects an error
			if (comm_.AckMode == true && res == GdbInData.State.chksum)
			{
				if (!String.IsNullOrEmpty(msg))
					Utility.WriteLine("  {0}", msg);
				Utility.WriteLine("  BAD CHECKSUM");
				return false;
			}
			// Packet is OK
			return true;
		}
		protected bool GetReponseString(out String msg)
		{
			string raw;
			return GetReponseString(out msg, out raw);
		}

		protected bool FinalConfirmation(string msg, string wanted)
		{
			// Expected result?
			if (msg != wanted)
			{
				Utility.WriteLine("  UNEXPECTED RESPONSE: '{0}' - '{1}' was expected", msg, wanted);
				return false;
			}
			if (String.IsNullOrEmpty(msg))
				Utility.WriteLine("  OK: '<unsupported>'");
			else
				Utility.WriteLine("  OK: '{0}'", msg);
			return true;
		}

		protected bool ReadMemCompatible(UInt32 addr, UInt32 size, Span<byte> buffer)
		{
			comm_.Send(String.Format("m{0:x},{1:x}", addr, size));
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			if (msg.StartsWith('E')
				&& msg.Length == 3)
			{
				return FinalConfirmation(msg, "<hex data>");
			}
			if (buffer != null)
			{
				Debug.Assert(buffer.Length >= size);
				int pos = 0;
				// process all chars from return stream
				CharEnumerator it = msg.GetEnumerator();
				while (it.MoveNext())
				{
					// High nibble
					byte by = (byte)(Utility.MkHex(it.Current) << 4);
					// Take next nibble
					if (!it.MoveNext())
					{
						Utility.WriteLine("  ERROR! Returned Hex stream should be provided in HEX pairs. Odd count returned!");
						return false;
					}
					// Low nibble
					by += Utility.MkHex(it.Current);
					if (pos == size)
					{
						Utility.WriteLine("  ERROR! Returned Hex stream has more bytes than requested!");
						return false;
					}
					// Store into buffer
					buffer[pos++] = by;
				}
				if (pos < size)
				{
					Utility.WriteLine("  ERROR! Returned Hex stream has not enough bytes as requested!");
					return false;
				}
			}
			return true;
		}

		protected bool VerifyMemCompatible(UInt32 addr, Span<byte> @ref, bool chk_rle = false)
		{
			// Valid array required...
			Debug.Assert(@ref != null);
			// ...having at least one byte
			Debug.Assert(@ref.Length > 0);

			comm_.Send(String.Format("m{0:x},{1:x}", addr, @ref.Length));
			// Get response string and discard
			if (!GetReponseString(out string msg, out string raw))
				return false;
			if (msg.StartsWith('E')
				&& msg.Length == 3)
			{
				return FinalConfirmation(msg, "<hex data>");
			}
			// Check if hex stream uses lower-case letters
			if (lowcasewarn_ == false)
			{
				// Scan response for uppercase letters
				foreach (char c in msg)
				{
					if (Char.IsUpper(c))
					{
						// Lower-case has best compatibility, distinguishing from uppercase 'E'
						// used to report errors.
						Utility.WriteLine("  WARNING! Returned Hex stream contains Uppercase letters! Use lower case for best compatibility");
						lowcasewarn_ = true;
					}
				}
			}
			// Check escapes (expected response shall use RLE)
			if(chk_rle)
			{
				if (!raw.Contains('*'))
				{
					comm_.HasRle = false;
					Utility.WriteLine("  WARNING! Improve throughput using RLE feature of the protocol!");
				}
				else
					comm_.HasRle = true;
			}
			int pos = 0;
			// Compare all chars from return stream
			CharEnumerator it = msg.GetEnumerator();
			while (it.MoveNext())
			{
				// High nibble
				byte by = (byte)(Utility.MkHex(it.Current) << 4);
				// Take next nibble
				if (!it.MoveNext())
				{
					Utility.WriteLine("  ERROR! Returned Hex stream should be provided in HEX pairs. Odd count returned!");
					return false;
				}
				// Low nibble
				by += Utility.MkHex(it.Current);
				if (pos == @ref.Length)
				{
					Utility.WriteLine("  ERROR! Returned Hex stream has more bytes than requested!");
					return false;
				}
				// Verification
				if (@ref[pos] != by)
				{
					Utility.WriteLine("  VERIFICATION ERROR! Got 0x{0:X2} instead of 0x{1:X2} in address 0x{2:X4}"
						, by, @ref[pos], addr + pos);
					return false;
				}
				// next byte
				++pos;
			}
			if (pos < @ref.Length)
			{
				Utility.WriteLine("  ERROR! Returned Hex stream has not enough bytes as requested!");
				return false;
			}
			return true;
		}

		protected bool DecodeHexToString(out String res)
		{
			res = "";
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			if (msg.StartsWith('E')
				&& msg.Length == 3)
			{
				return FinalConfirmation(msg, "<hex data>");
			}
			StringBuilder sb = new StringBuilder();
			// Compare all chars from return stream
			CharEnumerator it = msg.GetEnumerator();
			while (it.MoveNext())
			{
				// High nibble
				byte by = (byte)(Utility.MkHex(it.Current) << 4);
				// Take next nibble
				if (!it.MoveNext())
				{
					Utility.WriteLine("  ERROR! Returned Hex stream should be provided in HEX pairs. Odd count returned!");
					return false;
				}
				// Low nibble
				by += Utility.MkHex(it.Current);
				sb.Append(Convert.ToChar(by));
			}
			res = sb.ToString();
			return true;
		}

		/// Collects the Feature strings
		/// These are key/value pairs separated by ';'
		protected void DecodeFeats(String msg)
		{
			// Clear features array
			feats_.Clear();
			// Split into separate groups
			String[] toks = msg.Split(';');
			foreach (String s in toks)
			{
				if(s.Length == 0)
				{
					Utility.WriteLine("  WARNING! Empty feature found or reply ends with ';'!");
				}
				// "<key>=<value>" pair?
				else if (s.IndexOf('=') >= 0)
				{
					// Split key and value
					String[] kv = s.Split('=', 2);
					// Store the key/value pair
					feats_[kv[0]] = kv[1];
				}
				else
				{
					// A simple key was found
					char l = s.Last();
					String k;
					// Boolean keys will have either '+' or '-' as suffix
					if ("+-".IndexOf(l) >= 0)
						k = s.Substring(0, s.Length - 1);
					else
					{
						// Defaults to '+' suffix
						l = '+';
						k = s;
					}
					// Store the key/value pair
					feats_[k] = l.ToString();
				}
			}
		}

		protected bool IsFeatureSupported(string feat, string? val = null)
		{
			if (!feats_.ContainsKey(feat))
				return false;
			if (val == null)
				return true;
			return val.Equals(feats_[feat]);
		}

		protected bool ParseMemoryMapXml(string msg)
		{
			XmlSerializer ser = new XmlSerializer(typeof(memorymap));
			memorymap? mmap;
			using (TextReader reader = new StringReader(msg))
				mmap = (memorymap?)ser.Deserialize(reader);

			if (mmap == null)
			{
				Utility.WriteLine(msg);
				Utility.WriteLine("  ERROR! Failed to convert XML!");
				return false;
			}
			if (mmap.memory.Length == 0)
			{
				Utility.WriteLine(msg);
				Utility.WriteLine("  ERROR! No memory blocks found!");
				return false;
			}
			HashSet<MemBlock> found = new HashSet<MemBlock>();
			Utility.WriteLine("  Type    Start   Size");
			Utility.WriteLine("  ====================");
			foreach (memory m in mmap.memory)
			{
				MemBlock memBlock = new MemBlock();
				memBlock.mem_type_ = (MemoryTypeType)Enum.Parse(typeof(MemoryTypeType), m.type.ToString(), true);
				if (!Utility.ConvertUint32C(m.start, out memBlock.mem_start_))
				{
					Utility.WriteLine("  ERROR! Convert value '{0}' to numeric", m.start);
					return false;
				}
				if (!Utility.ConvertUint32C(m.length, out memBlock.mem_size_))
				{
					Utility.WriteLine("  ERROR! Convert value '{0}' to numeric", m.length);
					return false;
				}
				Utility.WriteLine("  {0,-6} {1,6} {2,6}"
					, memBlock.mem_type_.ToString()
					, "0x" + memBlock.mem_start_.ToString("X4")
					, memBlock.mem_size_
					);
				// Check for repetition
				if (found.Contains(memBlock))
				{
					Utility.WriteLine("  WARNING! This memory map was already declared");
					return false;
				}
				found.Add(memBlock);
				// Locate on reference
				MemBlock? mm = null;
				foreach (MemBlock mb in mem_blocks_)
				{
					if(mb.Equals(memBlock))
					{
						mm = mb;
						break;
					}
				}
				if (mm == null)
				{
					Utility.WriteLine("  WARNING! This memory map does not match any of the tested chip");
					return false;
				}
			}
			foreach (MemBlock mb in mem_blocks_)
			{
				if (!found.Contains(mb))
				{
					Utility.WriteLine("  WARNING! Chip contains a memory map that was not declared by the response");
					Utility.WriteLine("    {0,-6} {1,6} {2,6}"
						, mb.mem_type_.ToString()
						, "0x" + mb.mem_start_.ToString("X4")
						, mb.mem_size_
						);
					return false;
				}
			}
			return true;
		}

		protected MemBlock? SelectInfoMemory()
		{
			MemBlock? block = null;
			if (mem_blocks_.Count == 0)
			{
				Utility.WriteLine("  WARNING! No support for memory map: assuming a legacy Part");
				// Assume parts that we are testing has 4kB or more of flash memory
				block = new MemBlock();
				block.mem_type_ = MemoryTypeType.flash;
				block.mem_start_ = 0x1000;
				block.mem_size_ = 0x100;
			}
			else
			{
				foreach (MemBlock m in mem_blocks_)
				{
					if ((m.mem_type_ == MemoryTypeType.flash || m.mem_type_ == MemoryTypeType.fram)
						&& m.is_info_)
					{
						block = m;
					}
				}
				if (block == null)
					Utility.WriteLine("  ERROR! Failed to locate an Info block on the memory map!");
			}
			return block;
		}

		protected MemBlock? SelectFlashMemory()
		{
			MemBlock? block = null;
			if (mem_blocks_.Count == 0)
			{
				Utility.WriteLine("  WARNING! No support for memory map: assuming a 4 kB Part");
				// Assume parts that we are testing has 4kB or more of flash memory
				block = new MemBlock();
				block.mem_type_ = MemoryTypeType.flash;
				block.mem_start_ = 0xF000;
				block.mem_size_ = 0x1000;
			}
			else
			{
				UInt32 flash = 0;
				foreach (MemBlock m in mem_blocks_)
				{
					if (m.mem_type_ == MemoryTypeType.flash
						&& m.mem_size_ > flash)
					{
						block = m;
						flash = m.mem_size_;  // maximize size
					}
				}
				// Try again for FRAM
				if (block == null)
				{
					foreach (MemBlock m in mem_blocks_)
					{
						if (m.mem_type_ == MemoryTypeType.fram
							&& m.mem_size_ > flash)
						{
							block = m;
							flash = m.mem_size_;  // maximize size
						}
					}
					// Try again for ROM
					if (block == null)
					{
						foreach (MemBlock m in mem_blocks_)
						{
							if (m.mem_type_ == MemoryTypeType.rom
								&& m.mem_size_ > flash)
							{
								block = m;
								flash = m.mem_size_;  // maximize size
							}
						}
					}
				}
				if (block == null)
					Utility.WriteLine("  ERROR! Failed to locate a Flash block on the memory map!");
			}
			return block;
		}

		protected MemBlock? SelectRamMemory()
		{
			MemBlock? block = null;
			if (mem_blocks_.Count == 0)
			{
				Utility.WriteLine("  WARNING! No support for memory map: assuming a 256 bytes part");
				// Assume parts that we are testing has 256B of RAM
				block = new MemBlock();
				block.mem_type_ = MemoryTypeType.ram;
				block.mem_start_ = 0x1100;
				block.mem_size_ = 0x100;
			}
			else
			{
				UInt32 ram = 0;
				foreach (MemBlock m in mem_blocks_)
				{
					if (m.mem_type_ == MemoryTypeType.ram
						&& m.mem_size_ > ram)
					{
						block = m;
						ram = m.mem_size_;  // maximize size
					}
				}
				if (block == null)
					Utility.WriteLine("  ERROR! Failed to locate a RAM block on the memory map!");
			}
			return block;
		}

		protected List<PeriphReg> GetHwRegisters()
		{
			return periph_regs;
		}

		public struct ImageAddress
		{
			public UInt32 start_;
			public UInt32 r12args_;
			public UInt32 size_;
			public bool installed_;
			public bool IsOk()
			{
				return installed_
					&& size_ != 0
					&& r12args_ != 0
					&& start_ != 0
					;
			}
			public ImageAddress()
			{
				start_ = 0;
				r12args_ = 0;
				size_ = 0;
				installed_ = false;
			}
		}

		/// Loads a binary image of a funclet followed by a raw data structure into RAM
		protected ImageAddress TransferFuncletRam(string file, byte[] r12ptr)
		{
			ImageAddress img = new ImageAddress();
			MemBlock? memBlock = SelectRamMemory();
			if (memBlock != null)
			{
				img.start_ = memBlock.mem_start_;
				string fp = Path.Combine(GetAppPath(), "TestFunclets", file);
				byte[] buffer = File.ReadAllBytes(fp);
				img.r12args_ = img.start_ + (UInt32)buffer.Length;
				img.size_ = (UInt32)(buffer.Length + r12ptr.Length);
				Utility.WriteLine("  Using RAM at 0x{0:X4} ({1} bytes)", img.start_, img.size_);
				buffer = Utility.Combine(buffer, r12ptr);
				img.installed_ = WriteMemCompatible(img.start_, buffer);
			}
			return img;
		}

		protected bool SetBreakPoint(UInt32 addr)
		{
			Utility.WriteLine("  Setting breakpoint at 0x{0:X4}", addr);
			comm_.Send(String.Format("Z1,{0:X},0", addr));
			String msg;
			if (!GetReponseString(out msg)
				|| msg != "OK")
				return false;
			return true;
		}

		protected bool ClrBreakPoint(UInt32 addr)
		{
			Utility.WriteLine("  Clearing breakpoint at 0x{0:X4}", addr);
			comm_.Send(String.Format("z1,{0:X},2", addr));
			String msg;
			if (!GetReponseString(out msg)
				|| msg != "OK")
				return false;
			return true;
		}

		private bool EvaluateBPHit(string msg)
		{
			uint code = 0;
			if (msg.Length >= 3)
			{
				if (msg.StartsWith('S'))
				{
					if (Utility.ConvertUint32C(msg.Substring(1), out code)
						&& code == 5)
						return true;
				}
				else if (msg.StartsWith('T'))
				{
					if (Utility.ConvertUint32C(msg.Substring(1, 2), out code)
						&& code == 5)
					{
						// TODO: decode registers
						return true;
					}
				}
			}
			Utility.WriteLine("  ERROR! Unexpected response '{0}'", msg);
			return false;
		}

		protected bool Continue(UInt32? addr = null)
		{
			Utility.WriteLine("  Continuing execution");
			if (addr == null)
				comm_.Send("c");
			else
				comm_.Send(String.Format("c{0:x}", addr));
			String msg;
			if (!GetReponseString(out msg)
				|| !EvaluateBPHit(msg))
				return false;
			return true;
		}

		protected bool Step()
		{
			Utility.WriteLine("  Step");
			comm_.Send("s");
			String msg;
			if (!GetReponseString(out msg)
				|| !EvaluateBPHit(msg))
				return false;
			return true;
		}

		protected IComm comm_;
		protected GdbInData rcv_ = new GdbInData();
		protected Dictionary<string, string> feats_ = new Dictionary<string, string>();
		protected List<MemBlock> mem_blocks_ = new List<MemBlock>();
		protected List<PeriphReg> periph_regs = new List<PeriphReg>();
		protected bool lowcasewarn_ = false;
		protected chipdbtestChip chip_;
		protected byte[]? info_backup_ = null;
		protected String? info_file_ = null;
	}
}
