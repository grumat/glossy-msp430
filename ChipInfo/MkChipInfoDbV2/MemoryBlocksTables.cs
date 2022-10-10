using System;
using System.Text;

namespace MkChipInfoDbV2
{
	public class MemoryBlocksTables
	{
		public string BlockId { get; set; }
		public string MemoryKey { get; set; }
		public string MemoryType { get; set; }
		public string AccessType { get; set; }
		public bool Protectable { get; set; }
		public string MemLayout { get; set; }
		public string MemWrProt { get; set; }

		public string MkHash()
		{
			StringBuilder buf = new StringBuilder();
			buf.Append(MemoryKey);
			buf.Append(MemoryType);
			buf.Append(AccessType);
			buf.Append(Protectable.ToString());
			buf.Append(MemLayout);
			buf.Append(MemWrProt);
			using (System.Security.Cryptography.MD5 md5 = System.Security.Cryptography.MD5.Create())
			{
				byte[] inputBytes = System.Text.Encoding.ASCII.GetBytes(buf.ToString());
				byte[] hashBytes = md5.ComputeHash(inputBytes);
				return BitConverter.ToString(hashBytes).Replace("-", string.Empty).ToLower();
			}
		}
	}
}
