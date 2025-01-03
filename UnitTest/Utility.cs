﻿using NLog;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	/// Class to group utility methods
	internal class Utility
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

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

		protected static StringBuilder sb_ = new StringBuilder();

		public static void WriteLine(string format, params object?[] arg)
		{
			String msg = String.Format(format, arg);
			Console.WriteLine(msg);
			logger.Info(sb_.ToString() + msg);
			sb_.Clear();
		}

		public static void Write(string format, params object?[] arg)
		{
			String msg = String.Format(format, arg);
			Console.Write(msg);
			sb_.Append(msg);
		}

		public static bool ConvertUint32C(string txt, out UInt32 res)
		{
			if (txt.StartsWith("0x"))
				return UInt32.TryParse(txt.Substring(2), System.Globalization.NumberStyles.HexNumber, CultureInfo.InvariantCulture , out res);
			return UInt32.TryParse(txt, out res);
		}

		public static bool ConvertUint16C(string txt, out UInt16 res)
		{
			if (txt.StartsWith("0x"))
				return UInt16.TryParse(txt.Substring(2), System.Globalization.NumberStyles.HexNumber, CultureInfo.InvariantCulture, out res);
			return UInt16.TryParse(txt, out res);
		}

		public static byte[] Combine(params byte[][] arrays)
		{
			byte[] rv = new byte[arrays.Sum(a => a.Length)];
			int offset = 0;
			foreach (byte[] array in arrays)
			{
				System.Buffer.BlockCopy(array, 0, rv, offset, array.Length);
				offset += array.Length;
			}
			return rv;
		}

		public static string LittleEndianHex(UInt32 val, bool bUse32bits)
		{
			StringBuilder sb = new StringBuilder();
			sb.Append(((byte)val).ToString("x2"));
			sb.Append(((byte)(val >> 8)).ToString("x2"));
			if(bUse32bits)
			{
				sb.Append(((byte)(val >> 16)).ToString("x2"));
				sb.Append(((byte)(val >> 24)).ToString("x2"));
			}
			return sb.ToString();
		}
	}
}
