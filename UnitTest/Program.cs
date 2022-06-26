using System;

namespace UnitTest
{
	internal class Program
	{
		static void Usage()
		{
			Console.WriteLine("UnitTest Tool 1.0");
			Console.WriteLine("=================");
			Console.WriteLine("Tool that performs unit test on GDB Remote Serial Protocol (RSP) for a MSP430 chip.");
			Console.WriteLine("USAGE:");
			Console.WriteLine("    UnitTest <port> <test_num>");
			Console.WriteLine("    UnitTest <COMn> <test_num>");
			Console.WriteLine("WHERE:");
			Console.WriteLine("    <port>     : The localhost TCP port number");
			Console.WriteLine("    <COMn>     : The serial port");
			Console.WriteLine("    <test_num> : Unit test number");
		}
		static void Main(string[] args)
		{
			if(args.Length != 2)
			{
				Usage();
			}
			try
			{
				int tnum;
				if (!int.TryParse(args[1], out tnum))
					throw new Exception("Invalid test number");

				int port;
				IComm comm;
				if (int.TryParse(args[0], out port))
				{
					/*
					Example: (gdb-proxy++)
					msp430-gdbproxy --tcpport=2000 --keepalive --verbose --32bitregs --iface=jtag
					*/
					comm = new CommTcp(port);
				}
				else
				{
					throw new Exception("NOT IMPLEMENTED!");
				}
				Tests t = new Tests(comm);
				t.DoTest(tnum);
			}
			catch(Exception e)
			{
				Console.Error.WriteLine(e.ToString());
			}
		}
	}
}
