using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	internal partial class Tests : TestsBase
	{
		private ImageAddress LoadRegsFunclet(UInt16 r1, UInt16 r4, UInt16 r5, UInt16 r6, UInt16 r7, UInt16 r8
			, UInt16 r9, UInt16 r10, UInt16 r11, UInt16 r12, UInt16 r13, UInt16 r14, UInt16 r15)
		{
			byte[] data = new byte[26];
			data[0] = (byte)r1;
			data[1] = (byte)(r1 >> 8);
			data[2] = (byte)r4;
			data[3] = (byte)(r4 >> 8);
			data[4] = (byte)r5;
			data[5] = (byte)(r5 >> 8);
			data[6] = (byte)r6;
			data[7] = (byte)(r6 >> 8);
			data[8] = (byte)r7;
			data[9] = (byte)(r7 >> 8);
			data[10] = (byte)r8;
			data[11] = (byte)(r8 >> 8);
			data[12] = (byte)r9;
			data[13] = (byte)(r9 >> 8);
			data[14] = (byte)r10;
			data[15] = (byte)(r10 >> 8);
			data[16] = (byte)r11;
			data[17] = (byte)(r11 >> 8);
			data[18] = (byte)r12;
			data[19] = (byte)(r12 >> 8);
			data[20] = (byte)r13;
			data[21] = (byte)(r13 >> 8);
			data[22] = (byte)r14;
			data[23] = (byte)(r14 >> 8);
			data[24] = (byte)r15;
			data[25] = (byte)(r15 >> 8);
			return TransferFuncletRam("LoadRegs.bin", data);
		}

		private bool LoadRegsFunclet()
		{
			UInt16[] refs = new UInt16[]
			{
				0x0101,
				0x0404,
				0x0505,
				0x0606,
				0x0707,
				0x0808,
				0x0909,
				0x0a0a,
				0x0b0b,
				0x0c0c,
				0x0d0d,
				0x0e0e,
				0x0f0f,
			};
			Utility.WriteLine("RUN LOADREGS FUNCLET");
			ImageAddress img = LoadRegsFunclet(refs[0], refs[1], refs[2], refs[3]
				, refs[4], refs[5], refs[6], refs[7]
				, refs[8], refs[9], refs[10], refs[11]
				, refs[12]
				);
			// PC and R12 are start address and 1st parameter
			regs_[0] = img.start_;
			regs_[12] = img.r12args_;
			//
			if (!img.IsOk()
				|| !UpdateRegisters(true)
				|| !SetBreakPoint(img.start_ + 6)
				|| !Continue(img.start_)
				)
				return false;
			// Now CPU is paused...
			//if(!GetRegisterValues())
			//	return false;
			uint pc = img.start_ + 6;
			for(int i = 0; i < 13; ++i)
			{
				regs_[0] = pc;
				if (i >= 1)
				{
					pc += 4;
					// Algorithm order is R1, R3..R11, R13..R15, R12
					if (i == 1)
						regs_[i] = refs[0];
					else if (i < 10)
						regs_[i + 2] = refs[i - 1];
					else
						regs_[i + 3] = refs[i];
				}
				else
					pc += 2;
				if (!CompareRegisterValues(true)
					|| !Step()
					)
					return false;
			}
			regs_[0] = pc;
			regs_[12] = refs[9];
			if (!CompareRegisterValues(true))
				return false;
			Utility.WriteLine("  SUCCESS!");
			return true;
		}

		// Test 3 runs a simple funclet
		private bool Test3()
		{
			// 100
			if (!GetFeatures())
				return false;
			bool res =
				// 110
				GetReplyMode()
				// 120
				&& StartNoAckMode()
				// 130
				&& SetExtendedMode()
				// 140
				&& SetThreadForSubsequentOperation(0)
				// 140
				&& SetThreadForSubsequentOperation(-1)
				// 220
				&& GetRegisterValues()
				// 500
				&& ClearRegisters()
				// 510
				&& LoadRegsFunclet()
				;

			if (!Detach())
				res = false;
			return res;
		}
	}
}
