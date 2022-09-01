using NLog;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace UnitTest
{
	internal class CommTcp : IComm
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		/// Opens a connection on the localhost for the given port to access gdbproxy-like interface
		public CommTcp(int port)
		{
			// Localhost
			IPAddress ipAddress = IPAddress.Parse("127.0.0.1");
			// IP+port
			IPEndPoint remoteEP = new IPEndPoint(ipAddress, port);
			// Create a TCP/IP  socket.  
			sender_ = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			// The first connection attempt sets a 10 seconds timeout as MSP-FET will be very slow to setup VCC
			sender_.ReceiveTimeout = 10000;
			// This connects TCP; gdbproxy also connects to MSP430.dll and initializes MCU
			sender_.Connect(remoteEP);
			// Expecting an exception, if connect fails; why is object not initialized?
			Debug.Assert(sender_.RemoteEndPoint != null);
			// UI Feedback
			Console.WriteLine("Socket connected to {0}", sender_.RemoteEndPoint.ToString());
			logger.Debug("Socket connected to " + sender_.RemoteEndPoint.ToString());
			// Wait until JTAG connection happens
			Thread.Sleep(2000);
			// A starting ACK is required
			SendAck();
		}
		/// Destructor
		~CommTcp()
		{
			// Disconnect send/rcv streams
			sender_.Shutdown(SocketShutdown.Both);
			// Close socket
			sender_.Close();
		}
		/// Identifies platform; used to trim target behavior
		public Platform GetPlatform()
		{
			return platform_;
		}
		/// Sends an ACK, confirming the last reception is valid
		public void SendAck()
		{
			sender_.Send(new byte[] { (byte)'+' });
			logger.Debug("-> ACK");
		}
		/// Sends a NAK
		public void SendNak()
		{
			sender_.Send(new byte[] { (byte)'-' });
			logger.Debug("-> NAK");
		}
		/// Sends a message to the GDB. The message will be escaped by this method before transmitting
		public int Send(string msg)
		{
			GdbOutBuffer proto = new GdbOutBuffer(msg);
			// Send the data through the socket.
			msg = proto.MakePacket();
			logger.Debug("-> " + msg);
			return sender_.Send(Encoding.ASCII.GetBytes(msg));
		}
		/// Retrieves a raw single byte from the input stream
		public int Get()
		{
			// All buffered data retrieved?
			if(max_ <= pos_)
			{
				try
				{
					// Receive next buffer
					max_ = sender_.Receive(rcvbuf_);
					// After first attempt reduce timeout
					if (sender_.ReceiveTimeout > 2000)
						sender_.ReceiveTimeout = 2000;
				}
				catch(SocketException e)
				{
					// Rethrow if not a timeout
					if (e.ErrorCode != 10060)
						throw;
					// Clear buffer
					max_ = 0;
					pos_ = 0;
					// No char received
					return -1;
				}
				// Set index to buffer head
				pos_ = 0;
			}
			// Retrieve current byte and increment index
			return rcvbuf_[pos_++];
		}
		public bool AckMode { get; set; }
		public Platform platform_ = Platform.gdbproxy;
		// The socket object
		protected Socket sender_;
		// A tall buffer to receive data
		protected byte[] rcvbuf_ = new byte[4096];
		// Total bytes on buffer
		protected int max_ = 0;
		// Current buffer index
		protected int pos_ = 0;
	}
}

