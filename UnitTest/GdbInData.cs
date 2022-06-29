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
			timeout = -2,	// Timeout error
			chksum = -1,	// Packet has checksum error
			nak = 0,		// Response to last request was NAK
			ok = 1,			// String received and checksum is OK
		}

		/// Receives a string from the GDB input stream
		public State ReceiveString(IComm comm, out String result)
		{
			// Get next char
			int ch = comm.Get();
			// Hopefully the request was acknowledged
			if (ch != '+')
			{
				// NAK received!
				if(ch == '-')
				{
					logger.Debug("<- NAK");
					// Clear return value
					result = "";
					// Request message was rejected
					return State.nak;
				}
				// A start of frame without an ACK is always accepted
				if (ch != '$')
				{
					// Timeout?
					if (ch < 0)
					{
						result = "";
						logger.Debug("Timeout waiting for response!");
						return State.timeout;
					}
					logger.Debug("<- '{0}' unexpected, throwing exception", (char)ch);
					throw new Exception("No ACK and packet response does not start with '$'");
				}
			}
			else
			{
				logger.Debug("<- ACK");
				ch = comm.Get();    // hopefully start of frame...
			}
			// Start of frame?
			if (ch != '$')
			{
				// Timeout?
				if (ch < 0)
				{
					result = "";
					logger.Debug("Timeout waiting for response!");
					return State.timeout;
				}
				logger.Debug("<- '{0}' unexpected, throwing exception", (char)ch);
				throw new Exception("Packet response should start with '$'");
			}

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
				// End of frame mark?
				if (ch == '#')
				{
					// Two more bytes forms the checksum
					ch = comm.Get();
					// Timeout?
					if (ch < 0)
						break;
					// Upper nibble of hex value
					int hex = Utility.MkHex((char)ch) << 4;
					ch = comm.Get();
					// Timeout?
					if (ch < 0)
						break;
					// Lower hex nibble
					hex += Utility.MkHex((char)ch);
					// Copy collected bytes to string buffer
					result = sb.ToString();
					logger.Debug("${0}#{1,X2}", result, hex);
					chksum &= 0xFF;
					// Compare checksums to return result
					if (chksum != hex)
					{
						logger.Debug("Computed checksum: {0,X2}", chksum);
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
			logger.Debug("Timeout while receiving packet: '${0}'", result);
			// Returns all bytes that we got, but inform the timeout event
			return State.timeout;
		}
	}
}
