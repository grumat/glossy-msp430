using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	/// Class to group utility methods
	internal class Utility
	{
		/// Convert a hex digit into its numeric value, forming a nibble
		public static byte MkHex(char ch)
		{
			return (byte)(ch > 'Z'
				? ch - 'a' + 10
				: ch > '9'
				? ch - 'A' + 10
				: ch - '0');
		}

		/// Swap all bytes of a 4-byte unsigned integer value
		public static uint SwapUint32(uint val)
		{
			// Take underlying bytes
			byte[] bytes = BitConverter.GetBytes(val);
			// Reverse them
			Array.Reverse(bytes);
			// Reinterpret as unsigned 32-bit value
			return BitConverter.ToUInt32(bytes, 0);
		}

		/// Swap all bytes of a 2-byte unsigned integer value
		public static UInt16 SwapUint16(UInt16 val)
		{
			// Take underlying bytes
			byte[] bytes = BitConverter.GetBytes(val);
			// Reverse them
			Array.Reverse(bytes);
			// Reinterpret as unsigned 16-bit value
			return BitConverter.ToUInt16(bytes, 0);
		}
	}
}
