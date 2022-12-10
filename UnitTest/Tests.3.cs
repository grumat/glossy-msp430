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
		/// This creates a data structure having values to be loaded into CPU registers
		private ImageAddress LoadRegsFunclet(UInt16 r1, UInt16 r4, UInt16 r5, UInt16 r6, UInt16 r7, UInt16 r8
			, UInt16 r9, UInt16 r10, UInt16 r11, UInt16 r12, UInt16 r13, UInt16 r14, UInt16 r15)
		{
			byte[] data = new byte[26];
			// PC, R2, R3 cannot be changed
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
			// Loads funclet and data structure into RAM; the return object contains address information
			return TransferFuncletRam("LoadRegs.bin", data);
		}

		/// Loads unique data values into general registers and checks breakpoint + step-over feature
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
			// Loads funclet and array data into RAM
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
				|| !UpdateRegisters(true)			// update modified CPU registers
				|| !SetBreakPoint(img.start_ + 6)	// address of 'MOV.W 0(R12),R1' instruction 
				|| !Continue(img.start_)			// run the program (until breakpoint hits)
				)
				return false;
			// Now CPU is paused...
			uint pc = img.start_ + 6;
			// Steps over all MOV.W n(R12),Rx instructions and checks progress of register values
			for(int i = 0; i < 13; ++i)
			{
				// New expected PC value
				regs_[0] = pc;
				if (i >= 1)
				{
					// PC points to next instruction
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
					pc += 2;	// First move is optimized by assembler

				if (!CompareRegisterValues(true)	// checks expectations
					|| !Step()						// step into next instruction
					)
					return false;
			}
			// Final register state
			regs_[0] = pc;
			regs_[12] = refs[9];
			// Perform last check
			if (!CompareRegisterValues(true))
				return false;
			// Test is OK
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
