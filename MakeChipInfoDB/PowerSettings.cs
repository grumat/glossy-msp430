//#define TEST

using System;
using System.Collections.Generic;
using System.IO;

namespace MakeChipInfoDB
{
	class PowerSettings
	{
		public string Id;
		public uint TestRegMask;
		public uint TestRegDefault;
		public uint TestRegEnableLpm5;
		public uint TestRegDisableLpm5;
		public ushort TestReg3VMask;
		public ushort TestReg3VDefault;
		public ushort TestReg3VEnableLpm5;
		public ushort TestReg3VDisableLpm5;

		public PowerSettings(powerSettingsType o, ref PowerSettingsTable tab)
		{
			if (o.@ref != null)
			{
				// Merge ref immediately
				string k = MyUtils.MkIdentifier(o.@ref);
				Copy(tab.Items[k]);
			}
			if (o.id != null)
				Id = MyUtils.MkIdentifier(o.id);
			if (!string.IsNullOrEmpty(o.testRegMask))
				TestRegMask = Convert.ToUInt32(o.testRegMask, 16);
			if(!string.IsNullOrEmpty(o.testRegDefault))
				TestRegDefault = Convert.ToUInt32(o.testRegDefault, 16);
			if(!string.IsNullOrEmpty(o.testRegEnableLpm5))
				TestRegEnableLpm5 = Convert.ToUInt32(o.testRegEnableLpm5, 16);
			if(!string.IsNullOrEmpty(o.testRegDisableLpm5))
				TestRegDisableLpm5 = Convert.ToUInt32(o.testRegDisableLpm5, 16);
			if(!string.IsNullOrEmpty(o.testReg3VMask))
				TestReg3VMask = Convert.ToUInt16(o.testReg3VMask, 16);
			if(!string.IsNullOrEmpty(o.testReg3VDefault))
				TestReg3VDefault = Convert.ToUInt16(o.testReg3VDefault, 16);
			if(!string.IsNullOrEmpty(o.testReg3VEnableLpm5))
				TestReg3VEnableLpm5 = Convert.ToUInt16(o.testReg3VEnableLpm5, 16);
			if(!string.IsNullOrEmpty(o.testReg3VDisableLpm5))
				TestReg3VDisableLpm5 = Convert.ToUInt16(o.testReg3VDisableLpm5, 16);
			if (o.id == null)
				Patch(ref tab);
			else
				tab.AddItem(this);
		}

		void Copy(PowerSettings o)
		{
			Id = o.Id;
			TestRegMask = o.TestRegMask;
			TestRegDefault = o.TestRegDefault;
			TestRegEnableLpm5 = o.TestRegEnableLpm5;
			TestRegDisableLpm5 = o.TestRegDisableLpm5;
			TestReg3VMask = o.TestReg3VMask;
			TestReg3VDefault = o.TestReg3VDefault;
			TestReg3VEnableLpm5 = o.TestReg3VEnableLpm5;
			TestReg3VDisableLpm5 = o.TestReg3VDisableLpm5;
		}

		void Patch(ref PowerSettingsTable pwr)
		{
			// This is an adaptation for the database stand v3.15.1.1.
			// It could break for newer databases with new devices.
			// The adaptation was made to reduce the total flash size around
			// 500 bytes.
			if (TestReg3VDefault == 0
				&& TestReg3VDisableLpm5 == 0x4020
				&& TestReg3VEnableLpm5 == 0x4020
				&& TestReg3VMask == 0x4020
				&& TestRegDefault == 0x10000
				&& TestRegDisableLpm5 == 0x18
				&& TestRegEnableLpm5 == 0x18
				&& TestRegMask == 0x10018
				)
			{
				Id = "slau445";
				pwr.AddItem(this);
			}

		}

		public void DoHFile(TextWriter fh, string varname)
		{
			fh.WriteLine("static constexpr PowerSettings {0} =", varname);
			fh.WriteLine("{");
			fh.WriteLine("\t0x{0:x},", TestRegMask);
			fh.WriteLine("\t0x{0:x},", TestRegDefault);
			fh.WriteLine("\t0x{0:x},", TestRegEnableLpm5);
			fh.WriteLine("\t0x{0:x},", TestRegDisableLpm5);
			fh.WriteLine("\t0x{0:x},", TestReg3VMask);
			fh.WriteLine("\t0x{0:x},", TestReg3VDefault);
			fh.WriteLine("\t0x{0:x},", TestReg3VEnableLpm5);
			fh.WriteLine("\t0x{0:x},", TestReg3VDisableLpm5);
			fh.WriteLine("};");
			fh.WriteLine();
		}
	}

	class PowerSettingsTable
	{
		public Dictionary<string, PowerSettings> Items = new Dictionary<string, PowerSettings>();

		public void AddItem(PowerSettings o)
		{
			if (!Items.ContainsKey(o.Id))
				Items.Add(o.Id, o);
		}

		public void DoHFile(TextWriter fh)
		{
			/*
			** This validates the current state of this table.
			** Because the relation of this table is very simple, it is bound in code, based on
			** the SLAU number (the family User's Guide).
			** Thus if this validation fails you will need to review these rules and fix them.
			*/
			if (Items.Count != 4)
				throw new InvalidDataException("Database changed for PowerSettings. The current port will probably fail.");
			fh.WriteLine("// PowerSettings for member of the SLAU208, SLAU259 and SLAU367 families");
			Items["Default_Xv2"].DoHFile(fh, "PwrSlau208");
			fh.WriteLine("static constexpr PowerSettings PwrSlau259 = PwrSlau208;");
			fh.WriteLine();
			fh.WriteLine("// PowerSettings for member of the SLAU367 family");
			Items["Xv2_Fram_FRx9"].DoHFile(fh, "PwrSlau367");
			fh.WriteLine("static constexpr PowerSettings PwrSlau378 = PwrSlau208;");
			fh.WriteLine();
			fh.WriteLine("// PowerSettings for member of the SLAU445 family");
			Items["slau445"].DoHFile(fh, "PwrSlau445");
			fh.WriteLine(@"
// Decodes a power settings record based on the User's Guide number
ALWAYS_INLINE static const PowerSettings *DecodePowerSettings(FamilySLAU family)
{
	switch (family)
	{
	case kSLAU208:
	case kSLAU259:
	case kSLAU378:
		return &PwrSlau208;
	case kSLAU367:
		return &PwrSlau367;
	case kSLAU445:
		return &PwrSlau445;
	default:
		return NULL;
	}
}

");
		}
	}
}
