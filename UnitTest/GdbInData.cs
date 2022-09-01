using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	/// A class to handle GDB input stream
	internal class GdbInData
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		/// Results for ReceiveString()
		public enum State
		{
			timeout = -3,	// Timeout error
			proto = -2,		// Invalid protocol format
			chksum = -1,	// Packet has checksum error
			nak = 0,		// Response to last request was NAK
			ok = 1,			// String received and checksum is OK
		}

		/// Receives a string from the GDB input stream
		public State ReceiveString(IComm comm, out String result)
		{
			String raw;
			return ReceiveString(comm, out result, out raw);
		}
		/// Receives a string from the GDB input stream
		public State ReceiveString(IComm comm, out String result, out String raw)
		{
			StringBuilder rawsb = new StringBuilder();
			// Get next char
			int ch = comm.Get();
			// GDB sends ACK on first connection, which should restart ACK mode
			if (ch == '+')
				comm.AckMode = true;
			// no ACK mode, speeds protocol up
			if (comm.AckMode)
			{
				// Hopefully the request was acknowledged
				if (ch != '+')
				{
					// NAK received!
					if (ch == '-')
					{
						logger.Debug("<- NAK");
						raw = ch.ToString();
						// Clear return value
						result = "";
						// Request message was rejected
						return State.nak;
					}
					// A start of frame without an ACK is always accepted
					if (ch != '$')
					{
						result = "";
						// Timeout?
						if (ch < 0)
						{
							raw = "";
							logger.Debug("*** Timeout waiting for response!");
							return State.timeout;
						}
						raw = ch.ToString();
						logger.Error(String.Format("<- '{0}' unexpected", (char)ch));
						logger.Warn("No ACK and packet response does not start with '$'");
						return State.proto;
					}
				}
				else
				{
					logger.Debug("<- ACK");
					ch = comm.Get();    // hopefully start of frame...
				}
			}
			// Start of frame?
			if (ch != '$')
			{
				result = "";
				// Timeout?
				if (ch < 0)
				{
					logger.Debug("*** Timeout waiting for response!");
					raw = rawsb.ToString();
					return State.timeout;
				}
				rawsb.Append((char)ch);
				logger.Debug(String.Format("<- '{0}' unexpected", (char)ch));
				logger.Warn("Packet response should start with '$'");
				raw = rawsb.ToString();
				return State.proto;
			}
			rawsb.Append((char)ch);

			// A string buffer to build the string
			StringBuilder sb = new StringBuilder();
			int last = 0;
			int chksum = 0;
			// Retrieve bytes loop
			while(true)
			{
				// Get next char from input
				ch = comm.Get();
				// Timeout?
				if(ch < 0)
					break;
				rawsb.Append((char)ch);
				// End of frame mark?
				if (ch == '#')
				{
					// Two more bytes forms the checksum
					ch = comm.Get();
					// Timeout?
					if (ch < 0)
						break;
					rawsb.Append((char)ch);
					// Upper nibble of hex value
					int hex = Utility.MkHex((char)ch) << 4;
					ch = comm.Get();
					// Timeout?
					if (ch < 0)
						break;
					rawsb.Append((char)ch);
					// Lower hex nibble
					hex += Utility.MkHex((char)ch);
					// Copy collected bytes to string buffer
					result = sb.ToString();
					raw = rawsb.ToString();
					logger.Debug("<- " + raw);
					chksum &= 0xFF;
					// Compare checksums to return result
					if (chksum != hex)
					{
						logger.Debug(String.Format("*** *Computed checksum: {0:X2}", chksum));
						return State.chksum;
					}
					return State.ok;
				}
				// Escape char?
				if (ch == '}')
				{
					// Also checksum escape char
					chksum += ch;
					// Next char to unescape
					ch = comm.Get();
					// Timeout?
					if(ch < 0)
						break;
					rawsb.Append((char)ch);
					// Unescape char
					ch = ch ^ 0x20;
				}
				// Run Length encoding?
				else if(ch == '*')
				{
					// Also checksum RLE char
					chksum += ch;
					// Get count
					ch = comm.Get();
					// Timeout?
					if (ch < 0)
						break;
					rawsb.Append((char)ch);
					chksum += ch;
					// Remove offset to obtain the count
					ch -= 29;
					// Repeat last char according to the count byte
					for(int i = 0; i < ch; ++i)
						sb.Append((char)last);
					// Clear last char and restart loop
					last = 0;
					continue;
				}
				// Compute simple checksum
				chksum += ch;
				// Accumulate char
				sb.Append((char)ch);
				// Record last char for RLE feature
				last = ch;
			}
			// All timeout errors lands here!
			result = sb.ToString();
			raw = rawsb.ToString();
			logger.Debug(String.Format("*** Timeout while receiving packet: '${0}'", raw));
			// Returns all bytes that we got, but inform the timeout event
			return State.timeout;
		}
	}
}
