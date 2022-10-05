using NLog.Targets.Wrappers;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;

namespace UnitTest
{
	internal partial class Tests : TestsBase
	{
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
				case Platform.glossy_msp:
				case Platform.gdb_agent:
					wanted = "m0";
					break;
				default:
					break;
			}
			// Human readable result
			if (!FinalConfirmation(msg, wanted))
				return false;
			// Continue request type
			comm_.Send("qsThreadInfo");
			// Get response string
			if (!GetReponseString(out msg))
				return false;
			wanted = "l";
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
			switch (comm_.GetPlatform())
			{
				case Platform.gdb_agent:
					if (msg == "" || msg == "1")
						wanted = msg;
					break;
				default:
					break;
			}
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

		private bool GetMemoryMap(bool forced = false)
		{
			Utility.WriteLine("GET MEMORY MAP");
			// Sends request
			comm_.Send("qXfer:memory-map:read::0,18a");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			// This test depends on connection features
			if (!forced && !IsFeatureSupported("qXfer:memory-map:read", "+"))
				return FinalConfirmation(msg, "");
			// TODO: 'm' also needs to be handled here
			if (!msg.StartsWith('l'))
			{
				Utility.WriteLine(msg);
				Utility.WriteLine("  ERROR! The 'type' (first char) of the reply is unsupported!");
				return false;
			}
			Utility.WriteLine("  WARNING! Although this command is useful MSP430 GDB misses XML support and cannot interpret its results.");
			ParseMemoryMapXml(msg.Substring(1));
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
						fast:		53.55 kB/s
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
				if (sw.ElapsedMilliseconds > 3000)
				{
					sw.Stop();
					break;
				}
				UInt32 blk = memBlock.mem_size_ - flash;
				if (blk > 256)
					blk = 256;
				if (!ReadMemCompatible(memBlock.mem_start_ + flash, blk, null))
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
				int len = i + 5;
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
				if (!WriteMemCompatible(memBlock.mem_start_ + ram, new Span<byte>(buf_out, (int)ram, (int)blk)))
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
				fast:		33.38 kB/s
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

		private bool ReadPeripherals()
		{
			Utility.WriteLine("READ PERIPHERALS");
			List<PeriphReg> regs = GetHwRegisters();
			if (regs.Count == 0)
			{
				Utility.WriteLine("  ERROR! Chip DB does not contain rules for this test!");
				return false;
			}
			foreach (PeriphReg reg in regs)
			{
				byte[] buf_cmp = new byte[2] { 0, 0};
				if (reg.bus == PeriphBus.Bus8bit
					|| reg.bus == PeriphBus.Bus8and16bit)
				{
					ReadMemCompatible(reg.addr, 1, new Span<byte>(buf_cmp, 0, 1));
					if (buf_cmp[0] != reg.value)
					{
						Utility.WriteLine("  ERROR! Failed to Read Register '{0}'. Got 0x{1:X2} instead of 0x{2:X2}!"
							, reg.name
							, buf_cmp[0]
							, reg.value);
						return false;
					}
					buf_cmp[0] = 0;
				}
				if (reg.bus == PeriphBus.Bus16bit
					|| reg.bus == PeriphBus.Bus8and16bit)
				{
					ReadMemCompatible(reg.addr, 2, new Span<byte>(buf_cmp, 0, 2));
					UInt16 tmp = (UInt16)(buf_cmp[0] + (buf_cmp[1] << 8));
					if (tmp != reg.value)
					{
						Utility.WriteLine("  ERROR! Failed to Read Register '{0}'. Got 0x{1:X2} instead of 0x{2:X2}!"
							, reg.name
							, tmp
							, reg.value);
						return false;
					}
					buf_cmp[0] = 0;
					buf_cmp[1] = 0;
				}
			}
			Utility.WriteLine("  Verification PASSED!");
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
			if (!StartNoAckMode())
				return false;
			// 130
			if (!SetExtendedMode())
				return false;
			// 140
			if (!SetThreadForSubsequentOperation(0))
				return false;
			// 150
			if (!GetTracePointStatus())
				return false;
			// 160
			if (!GetReasonTheTargetHalted())
				return false;
			// 170
			if (!GetThreadInfo())
				return false;
			// 180
			if (!GetThreadInfoRTOS())
				return false;
			// 141
			if (!SetThreadForSubsequentOperation(-1))
				return false;
			// 190
			if (!GetCurrentThreadID())
				return false;
			// 200
			if (!QueryRemoteAttached())
				return false;
			// 210
			if (!GetSectionOffsets())
				return false;
			// 220
			if (!GetRegisterValues())
				return false;
			// 180
			if (!GetThreadInfoRTOS())
				return false;
			// 230
			if (!GetMemoryMap())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 240
			if (!TestRamWriteDiverse())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 250
			if (comm_.HasRle && !TestRlePackets())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 260
			if (!TestRamWrite())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 270
			if (!BenchmarkRamWrite())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 280
			if (!ReadFlashBenchmark())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 290
			if (!ReadPeripherals())
				return false;
			// 221
			if (!CompareRegisterValues())
				return false;
			// 9999
			if (!Detach())
				return false;
			return true;
		}
	}
}
