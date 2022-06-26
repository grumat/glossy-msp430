using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	internal class GdbOutBuffer
	{
		private StringBuilder buffer_ = new StringBuilder();

		public GdbOutBuffer()
		{ }
		public GdbOutBuffer(String msg)
		{
			buffer_.Append(msg);
		}

		public void PutChar(char ch)
		{
			buffer_.Append(ch);
		}
		public void Reset()
		{
			buffer_.Clear();
		}
		public int GetCheckSum()
		{
			int sum = 0;
			for (int i = 0; i < buffer_.Length; ++i)
			{
				char ch = buffer_[i];
				sum = (sum + ch) & 0xff;
			}
			return sum;
		}
		public byte[] MakePacket()
		{
			StringBuilder sb = new StringBuilder();
			sb.Append('$');
			UInt32 checksum = 0;
			for(int i = 0; i < buffer_.Length; ++i)
			{
				char ch = buffer_[i];
				if("#$}".IndexOf(ch) >= 0)
				{
					sb.Append('}');
					sb.Append(ch ^ 0x20);
					checksum += (UInt32)('}') + (UInt32)(ch ^ 0x20);
				}
				else
				{
					checksum += ch;
					sb.Append(ch);
				}
			}
			sb.Append('#');
			sb.Append(((byte)checksum).ToString("X2"));
			return Encoding.ASCII.GetBytes(sb.ToString());
		}
	}
}

