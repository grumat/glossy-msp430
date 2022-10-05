using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Runtime.Intrinsics.Arm;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using System.Reflection;
using static System.Net.WebRequestMethods;

namespace UnitTest
{
	internal partial class Tests : TestsBase
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
			switch (num)
			{
			case 1:
				return Test1();
			case 2:
				return Test2();
			case 100:
				return GetFeatures();
			case 110:
				return GetReplyMode();
			case 120:
				return StartNoAckMode();
			case 130:
				return SetExtendedMode();
			case 140:
				return SetThreadForSubsequentOperation(0);
			case 141:
				return SetThreadForSubsequentOperation(-1);
			case 150:
				return GetTracePointStatus();
			case 160:
				return GetReasonTheTargetHalted();
			case 170:
				return GetThreadInfo();
			case 180:
				return GetThreadInfoRTOS();
			case 190:
				return GetCurrentThreadID();
			case 200:
				return QueryRemoteAttached();
			case 210:
				return GetSectionOffsets();
			case 220:
				return GetRegisterValues();
			case 221:
				return CompareRegisterValues();
			case 230:
				return GetMemoryMap(true);
			case 240:
				return TestRamWriteDiverse();
			case 250:
				return TestRlePackets();
			case 260:
				return TestRamWrite();
			case 270:
				return BenchmarkRamWrite();
			case 280:
				return ReadFlashBenchmark();
			case 290:
				return ReadPeripherals();
			case 400:
				return BackupInfoMemory();
			case 401:
				return VerifyInfoMemory();
			case 402:
				return RestoreInfoMemory();
			case 410:
				return EraseFlash();
			case 420:
				return VerifyFlashErased();
			case 430:
				return TestFlashWrite();
			case 9999:
				return Detach();
			}
			Console.WriteLine("INVALID TEST NUMBER");
			return false;
		}

		public static void List()
		{
			Console.WriteLine("TEST LIST");
			Console.WriteLine("1   : General GDB v7 test (non-destructive)");
			Console.WriteLine("2   : Erase All Flash test (destructive)");
			Console.WriteLine("100 : Supported features");
			Console.WriteLine("110 : Reply mode for unknown packets");
			Console.WriteLine("120 : Start No ACK mode");
			Console.WriteLine("130 : Set extended mode");
			Console.WriteLine("140 : Set thread 0 for subsequent operation");
			Console.WriteLine("141 : Set thread -1 for subsequent operation");
			Console.WriteLine("150 : Get tracepoint status");
			Console.WriteLine("160 : Reason the target halted");
			Console.WriteLine("170 : Get thread info");
			Console.WriteLine("180 : Obtain thread information from RTOS");
			Console.WriteLine("190 : Get current thread id");
			Console.WriteLine("200 : Query if remote is attached");
			Console.WriteLine("210 : Get section offsets");
			Console.WriteLine("220 : Get register values");
			Console.WriteLine("221 : Compare register values (with last get)");
			Console.WriteLine("230 : Get memory map");
			Console.WriteLine("240 : Test RAM write mixed patterns");
			Console.WriteLine("250 : Test RLE response packets");
			Console.WriteLine("260 : Test RAM write");
			Console.WriteLine("270 : Benchmark RAM write");
			Console.WriteLine("280 : Read flash benchmark");
			Console.WriteLine("290 : Read Peripherals");
			Console.WriteLine("400 : Backup Info Memory");
			Console.WriteLine("401 : Verify Info Memory");
			Console.WriteLine("402 : Restore Info Memory");
			Console.WriteLine("410 : Erase Flash Memory");
			Console.WriteLine("420 : Verify if flash is erased");
			Console.WriteLine("430 : Test flash write");
			Console.WriteLine("9999: Detach target");
		}

		// A step to query supported features
		private bool GetFeatures()
		{
			Utility.WriteLine("SUPPORTED FEATURES");
			// Send default GDB v7 query
			/*
			** Note that a dirty hack exists here: the 'swbreak+' feature is used to distiguish
			** two different versions of the GCC. The legacy open source mspgcc compiler does
			** not have this feature. msp430-gdbproxy and glossy-msp430 uses this to switch 
			** internally between 32-bit or 16-bit registers. RSP does not provide a way to 
			** identify GDB neither middleware.
			*/
			comm_.Send("qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+");
			// Return message
			String msg;
			if (!GetReponseString(out msg))
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
					ct.platform_ = Platform.gdb_agent;	// sorry, no other way to distinguish middleware...
			}
			return true;
		}

		private bool Detach()
		{
			Utility.WriteLine("DETACH TARGET");
			// Send default GDB v7 query
			comm_.Send("D");
			// Return message
			String msg;
			if (!GetReponseString(out msg))
				return false;
			Utility.WriteLine("  \"{0}\"", msg);
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
			if (msg != resp_unkn_)
			{
				Utility.WriteLine("ERROR!");
				return false;
			}
			else if (bRes)
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

		private bool StartNoAckMode()
		{
			if (feats_.Count == 0
				&& !GetFeatures())
				return false;
			Utility.WriteLine("START NO ACK MODE");
			if (!IsFeatureSupported("QStartNoAckMode"))
			{
				Utility.WriteLine("  WARNING! Target does not support 'QStartNoAckMode'; Performance degradation is expected!");
				return true;
			}
			// This should always be handled as invalid packet0
			comm_.Send("QStartNoAckMode");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			comm_.AckMode = false;
			return true;
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
				if (use32bits_ && sb.Length == 8)
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
			if (r != 16)
			{
				Utility.WriteLine("  ERROR! 16 register values are expected!");
				return false;
			}
			return true;
		}

		private bool CompareRegisterValues()
		{
			Utility.WriteLine("COMPARE REGISTER VALUES");
			// Sends request
			comm_.Send("g");
			// Get response string
			String msg;
			if (!GetReponseString(out msg))
				return false;
			bool dirty = false;
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
				if (use32bits_ && sb.Length == 8)
				{
					uint? rval = regs_[r];
					if (!rval.HasValue)
						rval = uint.MaxValue;
					uint val = Utility.SwapUint32(uint.Parse(sb.ToString(), NumberStyles.HexNumber));
					if (val != rval)
					{
						Utility.WriteLine("  WARNING! R{0,-2} = 0x{1:X5} (was 0x{2:X5})", r, val, rval);
						dirty = true;
					}
					regs_[r++] = val;
					sb.Clear();
				}
				else if (!use32bits_ && sb.Length == 4)
				{
					uint? rval = regs_[r];
					if (!rval.HasValue)
						rval = uint.MaxValue;
					UInt16 val = Utility.SwapUint16(UInt16.Parse(sb.ToString(), NumberStyles.HexNumber));
					if (val != rval)
					{
						Utility.WriteLine("  WARNING! R{0,-2} = 0x{1:X4} (was 0x{2:X4})", r, val, rval);
						dirty = true;
					}
					regs_[r++] = val;
					sb.Clear();
				}
			}
			if (r != 16)
			{
				Utility.WriteLine("  ERROR! 16 register values are expected!");
				return false;
			}
			if(dirty)
				Utility.WriteLine("  FAILED!");
			else
				Utility.WriteLine("  OK");
			return true;
		}

		protected uint?[] regs_ = new uint?[16];
		protected bool use32bits_ = false;
		protected String resp_unkn_ = "";
	}
}
