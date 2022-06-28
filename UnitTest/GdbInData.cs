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
					// Clear return value
					result = "";
					// Request message was rejected
					return State.nak;
				}
				// A start of frame without an ACK is always accepted
				if (ch != '$')
					throw new Exception("No ACK and packet response does not start with '$'");
			}
			else
				ch = comm.Get();	// hopefully start of frame...
			// Start of frame?
			if (ch != '$')
			{
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
					// Compare checksums to return result
					return (hex == (chksum & 0xff)) ? State.ok : State.chksum;
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
					// Get count
					ch = comm.Get();
					// Timeout?
					if (ch < 0)
						break;
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
			// Returns all bytes that we got, but inform the timeout event
			return State.timeout;
		}
	}
}
