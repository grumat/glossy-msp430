using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;

namespace UnitTest
{
	class MemBlock
	{
		public memoryType mem_type_;
		public UInt32 mem_start_;
		public UInt32 mem_size_;
	}

	internal class Tests
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		// Creates the Test suite
		public Tests(IComm comm)
		{
			comm_ = comm;
		}

		// Process the desired test
		public bool DoTest(int num)
		{
			//WaitAck();
			switch(num)
			{
			case 1:
				return Test1();
			case 2:
				return Test2();
			}
			return false;
		}

		private void WriteMemCompatible(UInt32 addr, Span<byte> buffer)
		{
			StringBuilder sb = new StringBuilder("M");
			sb.Append(addr.ToString("X"));
			sb.Append(',');
			sb.Append(buffer.Length.ToString("X"));
			sb.Append(':');
			foreach(byte b in buffer)
			{
				sb.Append(b.ToString("X2"));
			}
			comm_.Send(sb.ToString());
		}

		// Receive a standard response string
		private bool GetReponseString(out String msg)
		{
			// Decode and unescape the stream
			var res = rcv_.ReceiveString(comm_, out msg);
			// A NAK means that request was malformed
			if (res == GdbInData.State.nak)
			{
				Utility.WriteLine("  NAK");
				return false;
			}
			// Target may have stopped responding
			if (res == GdbInData.State.timeout)
			{
				if (!String.IsNullOrEmpty(msg))
					Utility.WriteLine("  {0}", msg);
				Utility.WriteLine("  TIMEOUT");
				return false;
			}
			// Accept response even if checksum is bad
			comm_.SendAck();
			// Tests permanently rejects an error
			if (res == GdbInData.State.chksum)
			{
				if (!String.IsNullOrEmpty(msg))
					Utility.WriteLine("  {0}", msg);
				Utility.WriteLine("  BAD CHECKSUM");
				return false;
			}
			// Packet is OK
			return true;
		}

		private bool FinalConfirmation(string msg, string wanted)
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

		/// Collects the Feature strings
		/// These are key/value pairs separated by ';'
		private void DecodeFeats(String msg)
		{
			// Clear features array
			feats_.Clear();
			// Split into separate groups
			String[] toks = msg.Split(';');
			foreach (String s in toks)
			{
				// "<key>=<value>" pair?
				if (s.IndexOf('=') >= 0)
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

		private bool IsFeatureSupported(string feat, string? val = null)
		{
			if (!feats_.ContainsKey(feat))
				return false;
			if(val == null)
				return true;
			return val.Equals(feats_[feat]);
		}

		// A step to query supported features
		private bool GetFeatures()
		{
			Utility.WriteLine("SUPPORTED FEATURES");
			// Send default GDB8 query
			comm_.Send("qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+");
			// Return message
			String msg;
			if(!GetReponseString(out msg))
				return false;
			Utility.WriteLine("  \"{0}\"", msg);
			// Just decode and collect results
			DecodeFeats(msg);
			// Just print them
			foreach (KeyValuePair<string, string> entry in feats_)
				Utility.WriteLine("  {0} {1}", entry.Key, entry.Value);
			// Specific target found. Reclassify because of different behavior
			if (comm_.GetPlatform() == Platform.gdbproxy
				&& IsFeatureSupported("PacketSize", "4000"))
			{
				CommTcp? ct = comm_ as CommTcp;
				if (ct != null)
					ct.platform_ = Platform.gdb_agent;
			}
			return true;
		}

		/// Checks how target respond to unknown request
		private bool GetReplyMode()
		{
			Utility.WriteLine("REPLY MODE FOR UNKNOWN PACKETS");
			// This should always be handled as invalid packet0
			comm_.Send("vMustReplyEmpty");
			// Store response for unknown queries
			if (!GetReponseString(out resp_unkn_))
				return false;
			// Empty response should be default for normal targets
			bool bRes = FinalConfirmation(resp_unkn_, "");
			// Confirm sending a "damn invalid request"
			Utility.Write("  Simulating an invalid request... ");
			comm_.Send("vDamnInvalidRequest");
			// Store response for unknown queries
			String msg;
			if (!GetReponseString(out msg))
				return false;
			// Print result
			if(msg != resp_unkn_)
			{
				Utility.WriteLine("ERROR!");
				return false;
			}
			else if(bRes)
			{
				Utility.WriteLine("OK");
				return true;
			}
			else
			{
				Utility.WriteLine("ERROR: Both invalid packets have same reply, but invalid for this protocol...");
				return true;
			}
		}

		/// Sets extended protocol mode
		private bool SetExtendedMode()
		{
			Utility.WriteLine("SET EXTENDED MODE");
			// Sends request
			comm_.Send("!");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "OK";
			switch (comm_.GetPlatform())
			{
			case Platform.gdbproxy:
				// GDB proxy does not support this mode
				wanted = "";
				break;
			default:
				break;
			}
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool SetThreadForSubsequentOperation(int n)
		{
			Utility.WriteLine("SET THREAD FOR SUBSEQUENT OPERATION");
			// Sends request
			comm_.Send(String.Format("Hg{0}", n));
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "OK";
#if DEACTIVATED
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "1";
				break;
			default:
				break;
			}
#endif
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool GetTracePointStatus()
		{
			Utility.WriteLine("GET TRACEPOINT STATUS");
			// Sends request
			comm_.Send("qTStatus");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "";
#if DEACTIVATED
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "1";
				break;
			default:
				break;
			}
#endif
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool GetReasonTheTargetHalted()
		{
			Utility.WriteLine("REASON THE TARGET HALTED");
			comm_.Send("?");
			String msg;
			if (!GetReponseString(out msg))
				return false;
			Utility.WriteLine("  {0}", msg);

			List<String> errs = new List<string>();
			if (!msg.StartsWith("T05")
				&& !msg.StartsWith("S05"))
			{
				errs.Append("Expected state is T05 (SIGTRAP)");
			}
			Utility.WriteLine("  S={0}", msg.Substring(1, 2));
			if (msg.First() == 'T')
			{
				use32bits_ = false;
				String[] toks = msg.Substring(3).Split(';');
				if (toks.Length == 0)
				{
					// Should never happen!
					Debug.Assert(false);
					errs.Append("Invalid register dump format");
				}
				else
				{
					foreach (String tok in toks)
					{
						if (String.IsNullOrEmpty(tok))
							continue;
						String[] kv = tok.Split(':');
						if (kv.Length != 2)
						{
							errs.Append("Register value should be separated by ':'");
							continue;
						}
						uint reg = 0;
						if (!uint.TryParse(kv[0], out reg)
							|| reg < 0
							|| reg > 15)
						{
							errs.Append(String.Format("Invalid register number ({0})", reg));
							continue;
						}
						bool f32Bit = (kv[1].Length > 4);
						use32bits_ |= f32Bit;
						uint val;
						if (!uint.TryParse(kv[1], NumberStyles.HexNumber, null, out val))
						{
							errs.Append("Could not decode register value");
							return false;
						}
						if (f32Bit)
							val = Utility.SwapUint32(val);
						else
							val = Utility.SwapUint16((UInt16)val);
						regs_[reg] = val;
						Utility.WriteLine("  R{0}=0x{1:x4}", reg, val);
					}
				}
			}
			foreach (String e in errs)
				Utility.WriteLine("  {0}", e);
			return (errs.Count == 0);
		}

		private bool GetThreadInfo()
		{
			Utility.WriteLine("GET THREAD INFO");
			// Sends request
			comm_.Send("qfThreadInfo");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "";
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "m0";
				break;
			default:
				break;
			}
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool GetThreadInfoRTOS()
		{
			Utility.WriteLine("OBTAIN THREAD INFORMATION FROM RTOS");
			// Sends request
			comm_.Send("qL1160000000000000000");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "";
#if DEACTIVATED
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "1";
				break;
			default:
				break;
			}
#endif
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool GetCurrentThreadID()
		{
			Utility.WriteLine("GET CURRENT THREAD ID");
			// Sends request
			comm_.Send("qC");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "QC0";
#if DEACTIVATED
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "1";
				break;
			default:
				break;
			}
#endif
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool QueryRemoteAttached()
		{
			Utility.WriteLine("QUERY IF REMOTE IS ATTACHED");
			// Sends request
			comm_.Send("qAttached");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "";
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "1";
				break;
			default:
				break;
			}
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool GetSectionOffsets()
		{
			Utility.WriteLine("GET SECTION OFFSETS");
			// Sends request
			comm_.Send("qOffsets");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			String wanted = "";
#if DEACTIVATED
			switch (comm_.GetPlatform())
			{
			case Platform.gdb_agent:
				wanted = "1";
				break;
			default:
				break;
			}
#endif
			// Human readable result
			return FinalConfirmation(msg, wanted);
		}

		private bool GetRegisterValues()
		{
			Utility.WriteLine("GET REGISTER VALUES");
			// Sends request
			comm_.Send("g");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			use32bits_ = (msg.Length >= 128);
			int r = 0;
			StringBuilder sb = new StringBuilder();
			foreach (char ch in msg)
			{
				if (r == 16)
				{
					Utility.WriteLine("  ERROR! More than 16 register were returned!");
					return false;
				}
				sb.Append(ch);
				if(use32bits_ && sb.Length == 8)
				{
					uint val = Utility.SwapUint32(uint.Parse(sb.ToString(), NumberStyles.HexNumber));
					Utility.WriteLine("  R{0,-2} = 0x{1:X5}", r, val);
					regs_[r++] = val;
					sb.Clear();
				}
				else if (!use32bits_ && sb.Length == 4)
				{
					UInt16 val = Utility.SwapUint16(UInt16.Parse(sb.ToString(), NumberStyles.HexNumber));
					Utility.WriteLine("  R{0,-2} = 0x{1:X4}", r, val);
					regs_[r++] = val;
					sb.Clear();
				}
			}
			if(r != 16)
			{
				Utility.WriteLine("  ERROR! 16 register values are expected!");
				return false;
			}
			return true;
		}

		private bool GetMemoryMap()
		{
			Utility.WriteLine("GET MEMORY MAP");
			// Sends request
			comm_.Send("qXfer:memory-map:read::0,18a");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			// This test depends on connection features
			if(!IsFeatureSupported("qXfer:memory-map:read", "+"))
				return FinalConfirmation(msg, "");
			// TODO: 'm' also needs to be handled here
			if (!msg.StartsWith('l'))
			{
				Utility.WriteLine(msg);
				Utility.WriteLine("  ERROR! The 'type' (first char) of the reply is unsupported!");
				return false;
			}
			msg = msg.Substring(1);
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
			if(mmap.memory.Length == 0)
			{
				Utility.WriteLine(msg);
				Utility.WriteLine("  ERROR! No memory blocks found!");
				return false;
			}
			Utility.WriteLine("  Type    Start   Size");
			Utility.WriteLine("  ====================");
			foreach (memory m in mmap.memory)
			{
				MemBlock memBlock = new MemBlock();
				memBlock.mem_type_ = m.type;
				if(!Utility.ConvertUint32C(m.start, out memBlock.mem_start_))
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
					, "0x"+memBlock.mem_start_.ToString("X4")
					, memBlock.mem_size_
					);
				mem_blocks_.Add(memBlock);
			}

			return true;
		}

		private bool ReadFlashBenchmark()
		{
			/*
			BENCHMARKS:
				gdb-proxy++:
					TI MSP-FET:
						default:	17.85 kB/s
						slow:		34.98 kB/s
						medium:		49.20 kB/s
						fast:		53.13 kB/s
					TI MSP-FET430UIF
						<fixed>:	14.71 kB/s
					OLIMEX MSP430-JTAG-TINY-V2
						<fixed>:	30.24 kB/s
				gdb-agent-console:
					TI MSP-FET:
						default:	49,17 kB/s
					TI MSP-FET430UIF
						<fixed>:	14.71 kB/s
			*/
			Utility.WriteLine("READ FLASH BENCHMARK");
			UInt32 flash = 0;
			MemBlock? memBlock = null;
			if (mem_blocks_.Count == 0)
			{
				Utility.WriteLine("  WARNING! No support for memory map: assuming a 4 kB Part");
				// Assume parts that we are testing has 4kB or more of flash memory
				memBlock = new MemBlock();
				memBlock.mem_type_ = memoryType.flash;
				memBlock.mem_start_ = 0xF000;
				memBlock.mem_size_ = 0x1000;
			}
			else
			{
				foreach (MemBlock m in mem_blocks_)
				{
					if ((m.mem_type_ == memoryType.flash
						|| m.mem_type_ == memoryType.rom)
						&& m.mem_size_ > flash)
					{
						memBlock = m;
						flash = m.mem_size_;  // maximize size
					}
				}
				if (memBlock == null)
				{
					Utility.WriteLine("  ERROR! Failed to locate a Flash block on the memory map!");
					return false;
				}
			}
			Utility.WriteLine("  Using FLASH/ROM at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			Stopwatch sw = Stopwatch.StartNew();
			UInt32 total = 0;
			flash = 0;
			while (true)
			{
				if(sw.ElapsedMilliseconds > 3000)
				{
					sw.Stop();
					break;
				}
				UInt32 blk = memBlock.mem_size_ - flash;
				if (blk > 256)
					blk = 256;
				comm_.Send(String.Format("m{0:X},{1:X}", memBlock.mem_start_ + flash, blk));
				// Get response string and discard
				String msg;
				if (!GetReponseString(out msg))
					return false;
				if(msg.StartsWith('E')
					&& msg.Length == 3)
				{
					return FinalConfirmation(msg, "<hex data>");
				}
				// Next iteration
				total += blk;
				flash += blk;
				if (flash == memBlock.mem_size_)
					flash = 0;
			}
			long elapsed = sw.ElapsedMilliseconds;
			if (elapsed == 0)
				elapsed = 1;    // ensure no avoid division by 0
			Utility.WriteLine("  Read Performance: {0:0.00} kB/s", (double)(1000 * total) / (elapsed * 1024));
			return true;
		}

		private bool TestRamWrite()
		{
			/*
			BENCHMARKS:
			TI MSP-FET:
				default:	9.38 kB/s
				slow:		16.72 kB/s
				medium:		28.9 kB/s
				fast:		33.22 kB/s
			TI MSP-FET430UIF
				<fixed>:	9.99 kB/s
			*/
			Utility.WriteLine("TEST RAM WRITE");
			UInt32 ram = 0;
			MemBlock? memBlock = null;
			if (mem_blocks_.Count == 0)
			{
				Utility.WriteLine("  WARNING! No support for memory map: assuming a 256 bytes part");
				// Assume parts that we are testing has 256B of RAM
				memBlock = new MemBlock();
				memBlock.mem_type_ = memoryType.ram;
				memBlock.mem_start_ = 0x1100;
				memBlock.mem_size_ = 0x100;
			}
			else
			{
				foreach (MemBlock m in mem_blocks_)
				{
					if (m.mem_type_ == memoryType.ram
						&& m.mem_size_ > ram)
					{
						memBlock = m;
						ram = m.mem_size_;  // maximize size
					}
				}
				if (memBlock == null)
				{
					Utility.WriteLine("  ERROR! Failed to locate a RAM block on the memory map!");
					return false;
				}
			}
			Utility.WriteLine("  Using RAM at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			byte[] buffer = new byte[memBlock.mem_size_];
			Random rnd = new Random(1234);
			rnd.NextBytes(buffer);
			Stopwatch sw = Stopwatch.StartNew();
			ram = 0;
			while (ram < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - ram;
				if (blk > 256)
					blk = 256;
				WriteMemCompatible(memBlock.mem_start_ + ram, new Span<byte>(buffer, (int)ram, (int)blk));
				// Get response
				String msg;
				if (!GetReponseString(out msg))
					return false;
				if (msg != "OK")
				{
					Utility.WriteLine("  ERROR! Unexpected reply: {0}", msg);
					return false;
				}
				// Next iteration
				ram += blk;
			}
			sw.Stop();
			long elapsed = sw.ElapsedMilliseconds;
			if (elapsed == 0)
				elapsed = 1;	// ensure no avoid division by 0
			Utility.WriteLine("  Write Performance: {0:0.00} kB/s", (double)(1000 * memBlock.mem_size_) / (elapsed * 1024));
			return true;
		}

		// Test 1 simulates connection phase of GDB
		private bool Test1()
		{
			if (!GetFeatures())
				return false;
			if (!GetReplyMode())
				return false;
			if (!SetExtendedMode())
				return false;
			if (!SetThreadForSubsequentOperation(0))
				return false;
			if (!GetTracePointStatus())
				return false;
			if (!GetReasonTheTargetHalted())
				return false;
			if (!GetThreadInfo())
				return false;
			if (!GetThreadInfoRTOS())
				return false;
			if (!SetThreadForSubsequentOperation(-1))
				return false;
			if (!GetCurrentThreadID())
				return false;
			if (!QueryRemoteAttached())
				return false;
			if (!GetSectionOffsets())
				return false;
			if (!GetRegisterValues())
				return false;
			if (!GetThreadInfoRTOS())
				return false;
			if (!GetMemoryMap())
				return false;
			if (!ReadFlashBenchmark())
				return false;
			if (!TestRamWrite())
				return false;
			return true;
		}

		// TODO: Currently a playground
		private bool Test2()
		{
			Utility.WriteLine("TEST STATE");
			comm_.Send("?");
			String msg;
			if (!GetReponseString(out msg))
				return false;
			Utility.WriteLine("  {0}", msg);
			List<String> errs = new List<string>();
			bool fTmode = msg.StartsWith("T05");
			if (!fTmode
				&& !msg.StartsWith("S05"))
			{
				errs.Append("Expected state is T05 (SIGTRAP)");
			}
			Utility.WriteLine("  S={0}", msg.Substring(1,2));
			if (fTmode) 
			{
				use32bits_ = false;
				String[] toks = msg.Substring(3).Split(';');
				if(toks.Length == 0)
				{
					// Should never happen!
					Debug.Assert(false);
					errs.Append("Invalid register dump format");
				}
				else
				{
					foreach (String tok in toks)
					{
						if (String.IsNullOrEmpty(tok))
							continue;
						String[] kv = tok.Split(':');
						if (kv.Length != 2)
						{
							errs.Append("Register value should be separated by ':'");
							continue;
						}
						uint reg = 0;
						if (!uint.TryParse(kv[0], out reg)
							|| reg < 0
							|| reg > 15)
						{
							errs.Append(String.Format("Invalid register number ({0})", reg));
							continue;
						}
						bool f32Bit = (kv[1].Length > 4);
						use32bits_ |= f32Bit;
						uint val;
						if (!uint.TryParse(kv[1], NumberStyles.HexNumber, null, out val))
						{
							errs.Append("Could not decode register value");
							return false;
						}
						if (f32Bit)
							val = Utility.SwapUint32(val);
						else
							val = Utility.SwapUint16((UInt16)val);
						regs_[reg] = val;
						Utility.WriteLine("  R{0}=0x{1:x4}", reg, val);
					}
				}
			}
			foreach (String e in errs)
				Utility.WriteLine("  {0}", e);
			return (errs.Count == 0);
		}

		protected IComm comm_;
		protected GdbInData rcv_ = new GdbInData();
		protected uint?[] regs_ = new uint?[16];
		protected bool use32bits_ = false;
		protected Dictionary<string, string> feats_ = new Dictionary<string, string>();
		protected String resp_unkn_ = "";
		protected List<MemBlock> mem_blocks_ = new List<MemBlock>();
	}
}
