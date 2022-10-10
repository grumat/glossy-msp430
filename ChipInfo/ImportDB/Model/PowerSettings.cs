using System;
using System.Linq;
using System.Text;

namespace ImportDB.Model
{
	class PowerSettings
	{
		public string PartNumber { get; set; }
		public UInt32? TestRegMask { get; set; }
		public UInt32? TestRegDefault { get; set; }
		public UInt32? TestRegEnableLpm5 { get; set; }
		public UInt32? TestRegDisableLpm5 { get; set; }
		public UInt32? TestReg3VMask { get; set; }
		public UInt32? TestReg3VDefault { get; set; }
		public UInt32? TestReg3VEnableLpm5 { get; set; }
		public UInt32? TestReg3VDisableLpm5 { get; set; }

		public void Fill(powerSettingsType src)
		{
			if (!string.IsNullOrEmpty(src.@ref))
			{
				var @base = XmlDatabase.AllInfos.PowerSettings.First(x => x.id == src.@ref);
				Fill(@base);
			}
			bool modified = false;
			if (!String.IsNullOrEmpty(src.testRegMask))
			{
				modified = true;
				TestRegMask = Convert.ToUInt32(src.testRegMask, 16);
			}
			if (!String.IsNullOrEmpty(src.testRegDefault))
			{
				modified = true;
				TestRegDefault = Convert.ToUInt32(src.testRegDefault, 16);
			}
			if (!String.IsNullOrEmpty(src.testRegEnableLpm5))
			{
				modified = true;
				TestRegEnableLpm5 = Convert.ToUInt32(src.testRegEnableLpm5, 16);
			}
			if (!String.IsNullOrEmpty(src.testRegDisableLpm5))
			{
				TestRegDisableLpm5 = Convert.ToUInt32(src.testRegDisableLpm5, 16);
				modified = true;
			}
			if (!String.IsNullOrEmpty(src.testReg3VMask))
			{
				modified = true;
				TestReg3VMask = Convert.ToUInt32(src.testReg3VMask, 16);
			}
			if (!String.IsNullOrEmpty(src.testReg3VDefault))
			{
				modified = true;
				TestReg3VDefault = Convert.ToUInt32(src.testReg3VDefault, 16);
			}
			if (!String.IsNullOrEmpty(src.testReg3VEnableLpm5))
			{
				modified = true;
				TestReg3VEnableLpm5 = Convert.ToUInt32(src.testReg3VEnableLpm5, 16);
			}
			if (!String.IsNullOrEmpty(src.testReg3VDisableLpm5))
			{
				modified = true;
				TestReg3VDisableLpm5 = Convert.ToUInt32(src.testReg3VDisableLpm5, 16);
			}
			// Clear empty tag
			if (!modified && String.IsNullOrEmpty(src.id) && String.IsNullOrEmpty(src.@ref))
			{
				Clear();
			}
		}
		void Clear()
		{
			TestRegMask = null;
			TestRegDefault = null;
			TestRegEnableLpm5 = null;
			TestRegDisableLpm5 = null;
			TestReg3VMask = null;
			TestReg3VDefault = null;
			TestReg3VEnableLpm5 = null;
			TestReg3VDisableLpm5 = null;
		}
		public bool IsClear()
		{
			return
				TestRegMask == null
				&& TestRegDefault == null
				&& TestRegEnableLpm5 == null
				&& TestRegDisableLpm5 == null
				&& TestReg3VMask == null
				&& TestReg3VDefault == null
				&& TestReg3VEnableLpm5 == null
				&& TestReg3VDisableLpm5 == null
				;
		}
		public string MkIdentity()
		{
			return String.Format("{0};{1};{2};{3}/{4};{5};{6};{7}"
				, TestRegMask
				, TestRegDefault
				, TestRegEnableLpm5
				, TestRegDisableLpm5
				, TestReg3VMask
				, TestReg3VDefault
				, TestReg3VEnableLpm5
				, TestReg3VDisableLpm5
				);
		}
	}
}
