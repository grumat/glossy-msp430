using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	internal partial class Tests : TestsBase
	{

		// Test 3 runs a simple funclet
		private bool Test3()
		{
			// 100
			if (!GetFeatures())
				return false;
			// 110
			if (!GetReplyMode())
				return false;
			// 120
			if (!StartNoAckMode())
				return false;
			// 220
			if (!GetRegisterValues())
				return false;


			bool res = true;
			if (!Detach())
				res = false;
			return res;
		}
	}
}
