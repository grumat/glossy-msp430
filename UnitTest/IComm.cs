using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	enum Platform
	{
		gdbproxy = 0,
		glossy_msp,
	}
	internal interface IComm
	{
		public Platform GetPlatform();
		public void SendAck();
		public void SendNack();
		public int Send(string msg);
		public int Get();
	}
}
