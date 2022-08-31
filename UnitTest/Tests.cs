using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace UnitTest
{
	internal class Tests : TestsBase
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		// Creates the Test suite
		public Tests(IComm comm, string chip)
			: base(comm, chip)
		{
		}

		// Process the desired test
		public bool DoTest(int num)
		{
			//WaitAck();
			switch(num)
			{
			case 1:
				return Test1();
			case 100:
				return GetFeatures();
			case 110:
				return GetReplyMode();
			case 120:
				return SetExtendedMode();
			case 130:
				return SetThreadForSubsequentOperation(0);
			case 131:
				return SetThreadForSubsequentOperation(-1);
			case 140:
				return GetTracePointStatus();
			case 150:
				return GetReasonTheTargetHalted();
			case 160:
				return GetThreadInfo();
			case 170:
				return GetThreadInfoRTOS();
			case 180:
				return GetCurrentThreadID();
			case 190:
				return QueryRemoteAttached();
			case 200:
				return GetSectionOffsets();
			case 210:
				return GetRegisterValues();
			case 220:
				return GetMemoryMap(true);
			case 230:
				return TestRamWriteDiverse();
			case 240:
				return TestRlePackets();
			case 250:
				return TestRamWrite();
			case 260:
				return BenchmarkRamWrite();
			case 270:
				return ReadFlashBenchmark();
			}
			Console.WriteLine("INVALID TEST NUMBER");
			return false;
		}

		public static void List()
		{
			Console.WriteLine("TEST LIST");
			Console.WriteLine("1   : General GDB v7 test");
			Console.WriteLine("100 : Supported features");
			Console.WriteLine("110 : Reply mode for unknown packets");
			Console.WriteLine("120 : Set extended mode");
			Console.WriteLine("130 : Set thread 0 for subsequent operation");
			Console.WriteLine("131 : Set thread -1 for subsequent operation");
			Console.WriteLine("140 : Get tracepoint status");
			Console.WriteLine("150 : Reason the target halted");
			Console.WriteLine("160 : Get thread info");
			Console.WriteLine("170 : Obtain thread information from RTOS");
			Console.WriteLine("180 : Get current thread id");
			Console.WriteLine("190 : Query if remote is attached");
			Console.WriteLine("200 : Get section offsets");
			Console.WriteLine("210 : Get register values");
			Console.WriteLine("220 : Get memory map");
			Console.WriteLine("230 : Test RAM write mixed patterns");
			Console.WriteLine("240 : Test RLE response packets");
			Console.WriteLine("250 : Test RAM write");
			Console.WriteLine("260 : Benchmark RAM write");
			Console.WriteLine("270 : Read flash benchmark");
		}

		// A step to query supported features
		private bool GetFeatures()
		{
			Utility.WriteLine("SUPPORTED FEATURES");
			// Send default GDB v7 query
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
			if(!IsFeatureSupported("QStartNoAckMode"))
			{
				Utility.WriteLine("  WARNING! Target does not support 'QStartNoAckMode'; Performance degradation is expected!");
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
						uint reg;
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

		private bool GetMemoryMap(bool forced=false)
		{
			Utility.WriteLine("GET MEMORY MAP");
			// Sends request
			comm_.Send("qXfer:memory-map:read::0,18a");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			// This test depends on connection features
			if(!forced && !IsFeatureSupported("qXfer:memory-map:read", "+"))
				return FinalConfirmation(msg, "");
			// TODO: 'm' also needs to be handled here
			if (!msg.StartsWith('l'))
			{
				Utility.WriteLine(msg);
				Utility.WriteLine("  ERROR! The 'type' (first char) of the reply is unsupported!");
				return false;
			}
			return ParseMemoryMapXml(msg.Substring(1));
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
			MemBlock? memBlock = SelectFlashMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using FLASH/ROM at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			Stopwatch sw = Stopwatch.StartNew();
			UInt32 total = 0;
			UInt32 flash = 0;
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
				if(!ReadMemCompatible(memBlock.mem_start_ + flash, blk, null))
					return false;
				// Next iteration
				total += blk;
				flash += blk;
				if (flash == memBlock.mem_size_)
					flash = 0;
			}
			long elapsed = sw.ElapsedMilliseconds;
			if (elapsed == 0)
				elapsed = 1;    // avoid division by 0
			Utility.WriteLine("  Read Performance: {0:0.00} kB/s", (double)(1000 * total) / (elapsed * 1024));
			return true;
		}

		/// This test writes pieces of data into RAM, aligned and unaligned, then it 
		/// retrieves results using other alignment to ensure writes aren't affected by
		/// alignment or size
		private bool TestRamWriteDiverse()
		{
			Utility.WriteLine("TEST RAM WRITE MIXED PATTERNS");
			MemBlock? memBlock = SelectRamMemory();
			if (memBlock == null)
				return false;

			const byte pattern = 0x44;
			uint @base = memBlock.mem_start_ + 16;
			// Fill reproducible random
			byte[] buf_out = new byte[30];
			Utility.WriteLine("  Using RAM at 0x{0:X4} ({1} bytes)", @base, buf_out.Length);

			// PHASE 1: Constant (RLE compatible)
			// A constant value for escape feature
			for (int i = 0; i < buf_out.Length; ++i)
				buf_out[i] = pattern;
			if (!WriteMemCompatible(@base, new Span<byte>(buf_out)))
				return false;
			// Verify if protocol uses Run-Length Encoding for better throughput
			if (!VerifyMemCompatible(@base, new Span<byte>(buf_out), true))
				return false;

			// PHASE 2: Variable sizes and alignment
			Random rnd = new Random(1234);
			rnd.NextBytes(buf_out);

			// If this fails, just choose another repetitive pattern
			// (0x00, 0x11, 0x22 ... 0xFF)
			// This ensures that previous RAM state is different during both tests
			Debug.Assert(Array.IndexOf(buf_out, pattern) < 0);

			// byte aligned
			if (!WriteMemCompatible(@base + 0, new Span<byte>(buf_out, 0, 1)))
				return false;
			// word unaligned
			if (!WriteMemCompatible(@base + 1, new Span<byte>(buf_out, 1, 2)))
				return false;
			// dword unaligned
			if (!WriteMemCompatible(@base + 3, new Span<byte>(buf_out, 3, 4)))
				return false;
			// qword unaligned
			if (!WriteMemCompatible(@base + 7, new Span<byte>(buf_out, 7, 8)))
				return false;
			// byte unaligned
			if (!WriteMemCompatible(@base + 15, new Span<byte>(buf_out, 15, 1)))
				return false;
			// word aligned
			if (!WriteMemCompatible(@base + 16, new Span<byte>(buf_out, 16, 2)))
				return false;
			// dword aligned
			if (!WriteMemCompatible(@base + 18, new Span<byte>(buf_out, 18, 4)))
				return false;
			// qword aligned
			if (!WriteMemCompatible(@base + 22, new Span<byte>(buf_out, 22, 8)))
				return false;

			// qword aligned
			if (!VerifyMemCompatible(@base + 0, new Span<byte>(buf_out, 0, 8)))
				return false;
			// dword aligned
			if (!VerifyMemCompatible(@base + 8, new Span<byte>(buf_out, 8, 4)))
				return false;
			// word aligned
			if (!VerifyMemCompatible(@base + 12, new Span<byte>(buf_out, 12, 2)))
				return false;
			// byte aligned
			if (!VerifyMemCompatible(@base + 14, new Span<byte>(buf_out, 14, 1)))
				return false;
			// qword unaligned
			if (!VerifyMemCompatible(@base + 15, new Span<byte>(buf_out, 15, 8)))
				return false;
			// dword unaligned
			if (!VerifyMemCompatible(@base + 23, new Span<byte>(buf_out, 23, 4)))
				return false;
			// word unaligned
			if (!VerifyMemCompatible(@base + 27, new Span<byte>(buf_out, 27, 2)))
				return false;
			// byte unaligned
			if (!VerifyMemCompatible(@base + 29, new Span<byte>(buf_out, 29, 1)))
				return false;

			Utility.WriteLine("  Verification PASSED!");
			return true;
		}

		private bool TestRlePackets()
		{
			Utility.WriteLine("TEST RLE RESPONSE PACKETS");
			MemBlock? memBlock = SelectRamMemory();
			if (memBlock == null)
				return false;

			uint @base = memBlock.mem_start_ + 16;
			byte[] buf_out = new byte[106];
			Utility.WriteLine("  Using RAM at 0x{0:X4} (up to {1} bytes)", @base, buf_out.Length);

			// Test all hex nibbles
			for (int i = 100; i >= 2; --i)
			{
				int len = i+5;
				logger.Debug("RLE test with {0}x'5' and {1}x'0'", i * 2, (len - i) * 2);
				for (int j = 0; j < i; ++j)
					buf_out[j] = 0x55;
				for (int j = i; j < len; ++j)
					buf_out[j] = 0x00;
				// Write a bunch of repetitive nibbles (even case)
				if (!WriteMemCompatible(@base, new Span<byte>(buf_out, 0, len)))
					return false;
				// Compare them, testing RLE, if available on firmware
				if (!VerifyMemCompatible(@base, new Span<byte>(buf_out, 0, len)))
				{
					Utility.WriteLine("A response packet with a series of {0}x '5' digits, followed by {1}x '0' caused errors", i * 2, (len - i) * 2);
					return false;
				}
				logger.Debug("RLE test with {0}x'5' and {1}x'0'", i * 2 + 1, (len - i) * 2 - 1);
				buf_out[i] = 0x50;
				// Write a bunch of repetitive nibbles (odd case)
				if (!WriteMemCompatible(@base, new Span<byte>(buf_out, 0, len)))
					return false;
				// Compare them, testing RLE, if available on firmware
				if (!VerifyMemCompatible(@base, new Span<byte>(buf_out, 0, len)))
				{
					Utility.WriteLine("A response packet with a series of {0}x '5' digits, followed by {1}x '0' caused errors", i * 2 + 1, (len - i) * 2 - 1);
					return false;
				}
			}
			Utility.WriteLine("  Verification PASSED!");
			return true;
		}

		private bool TestRamWrite()
		{
			Utility.WriteLine("TEST RAM WRITE");
			MemBlock? memBlock = SelectRamMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using RAM at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			byte[] buf_out = new byte[memBlock.mem_size_];
			Random rnd = new Random(1234);
			rnd.NextBytes(buf_out);
			UInt32 ram = 0;
			while (ram < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - ram;
				if (blk > 256)
					blk = 256;
				if(!WriteMemCompatible(memBlock.mem_start_ + ram, new Span<byte>(buf_out, (int)ram, (int)blk)))
					return false;
				// Next iteration
				ram += blk;
			}
			Utility.WriteLine("  VERIFICATION...");
			ram = 0;
			while (ram < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - ram;
				if (blk > 256)
					blk = 256;
				if (!VerifyMemCompatible(memBlock.mem_start_ + ram, new Span<byte>(buf_out, (int)ram, (int)blk)))
					return false;
				// Next iteration
				ram += blk;
			}
			Utility.WriteLine("  Verification PASSED!");
			return true;
		}

		private bool BenchmarkRamWrite()
		{
			/*
			BENCHMARKS:
			TI MSP-FET:
				fast:		33.29 kB/s
			TI MSP-FET430UIF
				<fixed>:	9.99 kB/s
			*/
			Utility.WriteLine("BENCHMARK RAM WRITE");
			MemBlock? memBlock = SelectRamMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using RAM at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			byte[] buf_out = new byte[memBlock.mem_size_];
			Random rnd = new Random(1234);
			rnd.NextBytes(buf_out);
			Stopwatch sw = Stopwatch.StartNew();
			UInt32 total = 0;
			while (true)
			{
				if (sw.ElapsedMilliseconds > 3000)
				{
					sw.Stop();
					break;
				}
				UInt32 ram = 0;
				while (ram < memBlock.mem_size_)
				{
					UInt32 blk = memBlock.mem_size_ - ram;
					if (blk > 256)
						blk = 256;
					if (!WriteMemCompatible(memBlock.mem_start_ + ram, new Span<byte>(buf_out, (int)ram, (int)blk)))
						return false;
					// Next iteration
					ram += blk;
					total += blk;
				}
			}
			long elapsed = sw.ElapsedMilliseconds;
			if (elapsed == 0)
				elapsed = 1;    // avoid division by 0
			Utility.WriteLine("  Write Performance: {0:0.00} kB/s", (double)(1000 * total) / (elapsed * 1024));
			return true;
		}

		// Test 1 roughly simulates connection phase of GDB
		private bool Test1()
		{
			// 100
			if (!GetFeatures())
				return false;
			// 110
			if (!GetReplyMode())
				return false;
			// 120
			if (!SetExtendedMode())
				return false;
			// 130
			if (!SetThreadForSubsequentOperation(0))
				return false;
			// 140
			if (!GetTracePointStatus())
				return false;
			// 150
			if (!GetReasonTheTargetHalted())
				return false;
			// 160
			if (!GetThreadInfo())
				return false;
			// 170
			if (!GetThreadInfoRTOS())
				return false;
			// 131
			if (!SetThreadForSubsequentOperation(-1))
				return false;
			// 180
			if (!GetCurrentThreadID())
				return false;
			// 190
			if (!QueryRemoteAttached())
				return false;
			// 200
			if (!GetSectionOffsets())
				return false;
			// 210
			if (!GetRegisterValues())
				return false;
			// 170
			if (!GetThreadInfoRTOS())
				return false;
			// 220
			if (!GetMemoryMap())
				return false;
			// 230
			if (!TestRamWriteDiverse())
				return false;
			// 240
			if (!TestRlePackets())
				return false;
			// 250
			if (!TestRamWrite())
				return false;
			// 260
			if (!BenchmarkRamWrite())
				return false;
			// 270
			if (!ReadFlashBenchmark())
				return false;
			return true;
		}

		protected uint?[] regs_ = new uint?[16];
		protected bool use32bits_ = false;
		protected String resp_unkn_ = "";
	}
}
