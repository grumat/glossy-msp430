using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
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
					String[] kv = s.Split('=', 1);
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
				Utility.WriteLine("  {1} {0}", entry.Key, entry.Value);
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
			if(String.IsNullOrEmpty(resp_unkn_))
			{
				resp_unkn_ = "";
				Utility.WriteLine("  <empty response>");
			}
			else
				Utility.WriteLine("  {0}", resp_unkn_);
			// So this is really invalid!!!
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
			else
			{
				Utility.WriteLine("OK");
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
			// GDB proxy does not support this mode
			if(comm_.GetPlatform() == Platform.gdbproxy)
			{
				if (msg != "")
				{
					Utility.WriteLine("  {0} BAD RESPONSE", msg);
					return false;
				}
				else
					Utility.WriteLine("  <unsupported by platform>");
			}
			// glossy-msp should accept mode
			else if (msg != "OK")
			{
				Utility.WriteLine("  {0} BAD RESPONSE", msg);
				return false;
			}
			else
				Utility.WriteLine("  <unsupported by platform>");
			return true;
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
			if (msg != "OK")
			{
				Utility.WriteLine("  BAD RESPONSE: {0}", msg);
				return false;
			}
			Utility.WriteLine("  " + msg);
			return true;
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
			if (msg != "")
			{
				Utility.WriteLine("  UNEXPECTED RESPONSE: {0}", msg);
				return false;
			}
			Utility.WriteLine("  OK <unsupported>");
			return true;
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
	}
}
