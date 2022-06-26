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
		public CommTcp(int port)
		{
			IPAddress ipAddress = IPAddress.Parse("127.0.0.1");
			IPEndPoint remoteEP = new IPEndPoint(ipAddress, port);
			// Create a TCP/IP  socket.  
			sender_ = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			// First attempt needs more time for VCC initialization
			sender_.ReceiveTimeout = 10000;
			sender_.Connect(remoteEP);
			if (sender_.RemoteEndPoint != null)
				Console.WriteLine("Socket connected to {0}", sender_.RemoteEndPoint.ToString());
			// Wait until JTAG connection happens
			Thread.Sleep(2000);
		}
		~CommTcp()
		{
			sender_.Shutdown(SocketShutdown.Both);
			sender_.Close();
		}
		public Platform GetPlatform()
		{
			return Platform.gdbproxy;
		}
		public void SendAck()
		{
			sender_.Send(new byte[] { (byte)'+' });
		}
		public void SendNack()
		{
			sender_.Send(new byte[] { (byte)'-' });
		}
		public int Send(string msg)
		{
			GdbOutBuffer proto = new GdbOutBuffer(msg);
			// Send the data through the socket.
			return sender_.Send(proto.MakePacket());
		}
		public int Get()
		{
			if(max_ <= pos_)
			{
				try
				{
					max_ = sender_.Receive(rcvbuf_);
					// After first attempt reduce timeout
					if (sender_.ReceiveTimeout == 10000)
						sender_.ReceiveTimeout = 2000;
				}
				catch(SocketException e)
				{
					if (e.ErrorCode != 10060)
						throw;
					max_ = 0;
					return -1;
				}
				pos_ = 0;
			}
			return rcvbuf_[pos_++];
		}
		protected Socket sender_;
		protected byte[] rcvbuf_ = new byte[4096];
		protected int max_ = 0;
		protected int pos_ = 0;
	}
}
