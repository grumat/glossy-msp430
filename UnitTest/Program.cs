using System;

namespace UnitTest
{
	internal class Program
	{
		/// Prints usage instruction for the tool
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

		/// Main program
		static void Main(string[] args)
		{
			// Two arguments are required
			if(args.Length != 2)
			{
				Usage();
			}
			try
			{
				// Test number is the second argument
				int tnum;
				// Evaluate and validate second argument
				if (!int.TryParse(args[1], out tnum))
					throw new Exception("Invalid test number");

				// Port number is the first argument
				int port;
				IComm comm;
				// Try numeric value, for localhost network
				if (int.TryParse(args[0], out port))
				{
					/*
					Connection Example with gdb-proxy++:
						msp430-gdbproxy --tcpport=2000 --keepalive --verbose --32bitregs --iface=jtag
					*/
					comm = new CommTcp(port);
				}
				else
				{
					// TODO: Serial communication with glossy MSP430
					throw new Exception("NOT IMPLEMENTED!");
				}
				// Create test suite
				Tests t = new Tests(comm);
				// Execute test by number
				t.DoTest(tnum);
			}
			catch(Exception e)
			{
				// Generic error display
				Console.Error.WriteLine(e.ToString());
			}
		}
	}
}
