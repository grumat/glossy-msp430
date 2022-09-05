using NLog;
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace UnitTest
{
	internal class CommSerial : IComm
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		/// Opens a connection on the glossy-msp430 device
		public CommSerial(string port)
		{
			HasRle = true;
			// Setup 115200 baud rate
			port_.PortName = port;
			port_.BaudRate = 115200;
			port_.Parity = Parity.None;
			port_.StopBits = StopBits.One;
			port_.DataBits = 8;
			port_.Handshake = Handshake.None;
			port_.ReadTimeout = 10000;
			port_.WriteTimeout = 500;
			// Open port
			port_.Open();
			port_.DiscardInBuffer();
			port_.DiscardOutBuffer();
			// Wait until JTAG connection happens
			Thread.Sleep(500);
			// A starting ACK is required
			SendAck();
		}
		/// Destructor
		~CommSerial()
		{
			port_.Close();
		}
		/// Identifies platform; used to trim target behavior
		public Platform GetPlatform()
		{
			return Platform.glossy_msp;
		}
		/// Sends an ACK, confirming the last reception is valid
		public void SendAck()
		{
			port_.Write("+");
			logger.Debug("-> ACK");
		}
		/// Sends a NAK
		public void SendNak()
		{
			port_.Write("-");
			logger.Debug("-> NAK");
		}
		/// Sends a message to the GDB. The message will be escaped by this method before transmitting
		public int Send(string msg)
		{
			GdbOutBuffer proto = new GdbOutBuffer(msg);
			// Send the data through the socket.
			msg = proto.MakePacket();
			logger.Debug("-> " + msg);
			port_.Write(msg);
			return msg.Length;
		}
		/// Retrieves a raw single byte from the input stream
		public int Get()
		{
			return port_.ReadByte();
		}
		public bool AckMode { get; set; }
		public bool HasRle { get; set; }

		protected SerialPort port_ = new SerialPort();
	}
}
