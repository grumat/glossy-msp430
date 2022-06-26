using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	internal class GdbInData
	{
		public enum State
		{
			timeout = -2,	// Timeout error
			chksum = -1,	// Packet has checksum error
			nak = 0,		// Response was NAK
			ok = 1,			// String received and checksum is OK
		}

		public State ReceiveString(IComm comm, out String result)
		{
			StringBuilder sb = new StringBuilder();
			int ch = comm.Get();
			if (ch != '+')
			{
				if(ch == '-')
				{
					result = "";
					return State.nak;
				}
				if (ch != '$')
					throw new Exception("No ACK and packet response does not start with '$'");
			}
			else
				ch = comm.Get();
			if (ch != '$')
			{
				throw new Exception("Packet response should start with '$'");
			}
			int last = 0;
			int chksum = 0;
			while(true)
			{
				ch = comm.Get();
				if(ch < 0)
					break;			       // timeout
				if (ch == '#')
				{
					ch = comm.Get();
					if (ch < 0)
						break;              // timeout
					int hex = Utility.MkHex(ch) << 4;
					ch = comm.Get();
					if (ch < 0)
						break;              // timeout
					hex += Utility.MkHex(ch);
					result = sb.ToString();
					return (hex == (chksum & 0xff)) ? State.ok : State.chksum;
				}
				if (ch == '}')
				{
					chksum += ch;
					ch = comm.Get();
					if(ch < 0)
						break;				// timeout
					ch = ch ^ 0x20;
				}
				else if(ch == '*')
				{
					// Run length encoding
					ch = comm.Get();
					if (ch < 0)
						break;              // timeout
					ch -= 29;
					for(int i = 0; i < ch; ++i)
						sb.Append((char)last);
					last = 0;
					continue;
				}
				chksum += ch;
				sb.Append((char)ch);
				last = ch;
			}
			// All errors land here
			result = sb.ToString();
			return State.timeout;
		}
	}
}
