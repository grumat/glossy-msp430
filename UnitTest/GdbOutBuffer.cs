using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	/// Class to prepare a string to be sent to GDB RSP target
	internal class GdbOutBuffer
	{
		/// Buffer to store output string
		private StringBuilder buffer_ = new StringBuilder();

		/// Default Constructor
		public GdbOutBuffer()
		{ }
		/// Constructor with the string to transmit
		public GdbOutBuffer(String msg)
		{
			buffer_.Append(msg);
		}

		/// Appends a char to the buffer
		public void PutChar(char ch)
		{
			buffer_.Append(ch);
		}
		/// Resets buffer
		public void Reset()
		{
			buffer_.Clear();
		}
		/// Creates a packet good for transmission
		public String MakePacket()
		{
			// A buffer to store data
			StringBuilder sb = new StringBuilder();
			// Add frame start char
			sb.Append('$');
			// Checksum variable
			UInt32 checksum = 0;
			// Loop through all bytes
			for(int i = 0; i < buffer_.Length; ++i)
			{
				// Take the current char
				char ch = buffer_[i];
				// Needs to escape?
				if("#$}".IndexOf(ch) >= 0)
				{
					// Add escape char
					sb.Append('}');
					// Transform char, inverting a bit
					sb.Append(ch ^ 0x20);
					// checksum from both arguments
					checksum += (UInt32)('}') + (UInt32)(ch ^ 0x20);
				}
				else
				{
					// Simple checksum
					checksum += ch;
					// Copy to output buffer
					sb.Append(ch);
				}
			}
			// End of payload
			sb.Append('#');
			// Follow LSB of checksum, as hex digits
			sb.Append(((byte)checksum).ToString("X2"));
			// Convert to ASCII chars
			return sb.ToString();
		}
	}
}

