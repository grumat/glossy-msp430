using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnitTest
{
	enum Platform
	{
		gdbproxy = 0,	// gdbproxy++
		gdb_agent,	// TI GDB agent
		glossy_msp,		// Glossy MSP430
	}
	internal interface IComm
	{
		/// Identifies platform; used to trim target behavior
		public Platform GetPlatform();
		/// Sends an ACK, confirming the last reception is valid
		public void SendAck();
		/// Sends a NAK
		public void SendNak();
		/// Sends a message to the GDB. The message will be escaped by this method before transmitting
		public int Send(string msg);
		/// Retrieves a single raw byte from the input stream
		public int Get();
		/// Sets the No ACK mode
		public bool AckMode { get; set; }
	}
}
