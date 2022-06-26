using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	internal class Utility
	{
		public static byte MkHex(int ch)
		{
			return (byte)(ch > 'Z'
				? ch - 'a' + 10
				: ch > '9'
				? ch - 'A' + 10
				: ch - '0');
		}

		public static uint SwapUint32(uint val)
		{
			byte[] bytes = BitConverter.GetBytes(val);
			Array.Reverse(bytes);
			return BitConverter.ToUInt32(bytes, 0);
		}

		public static UInt16 SwapUint16(UInt16 val)
		{
			byte[] bytes = BitConverter.GetBytes(val);
			Array.Reverse(bytes);
			return BitConverter.ToUInt16(bytes, 0);
		}
	}
}
