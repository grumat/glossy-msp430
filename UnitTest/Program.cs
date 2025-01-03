﻿using NLog;
using System;
using System.Diagnostics;

namespace UnitTest
{
	internal class Program
	{
		private static Logger logger = LogManager.GetCurrentClassLogger();

		/// Prints usage instruction for the tool
		static void Usage()
		{
			Console.WriteLine("UnitTest Tool 1.0");
			Console.WriteLine("=================");
			Console.WriteLine("Tool that performs unit test on GDB Remote Serial Protocol (RSP) for a MSP430 chip.");
			Console.WriteLine("USAGE:");
			Console.WriteLine("    UnitTest <port> <chip> <test_num> [<test_num> ...]");
			Console.WriteLine("    UnitTest <COMn> <chip> <test_num> [<test_num> ...]");
			Console.WriteLine("WHERE:");
			Console.WriteLine("    <port>     : The localhost TCP port number");
			Console.WriteLine("    <COMn>     : The serial port");
			Console.WriteLine("    <test_num> : Unit test number");
			Console.WriteLine("    <chip>     : Chip Number (see ChipDB.xml)");
			Console.WriteLine("INFORMATIONAL:");
			Console.WriteLine("    UnitTest list");
			Console.WriteLine("        Displays a list of the available tests");
			Console.WriteLine("    UnitTest mcus");
			Console.WriteLine("        Displays a list of the available MSP430 MCUs templates");
		}

		/// Main program
		static void Main(string[] args)
		{
			// Two arguments are required
			if (args.Length < 3)
			{
				if (args.Length == 1)
				{
					if(args[0] == "list")
						Tests.List();
					else if (args[0] == "mcu" || args[0] == "mcus")
						Tests.ListMcus();
				}
				else
					Usage();
			}
			else
			{
				try
				{
					logger.Debug("Starting UnitTest");

					// Port number is the first argument
					int port;
					IComm comm;
					// Try numeric value, for localhost network
					if (int.TryParse(args[0], out port))
					{
						/*
						Command line example: 2000 1 MSP430F1611

						Connection Example with gdb-proxy++:
							msp430-gdbproxy --tcpport=2000 --keepalive --32bitregs --iface=jtag
						*/
						comm = new CommTcp(port);
					}
					else
					{
						/*
						Command line example: COM3 220 MSP430F1611
						*/
						comm = new CommSerial(args[0]);
					}
					// Create test suite
					Tests t = new Tests(comm, args[1]);
					// Execute test by number
					Stopwatch sw = Stopwatch.StartNew();
					for (int i = 2; i < args.Length; ++i)
					{
						// Test number is the second argument
						int tnum;
						// Evaluate and validate second argument
						if (!int.TryParse(args[i], out tnum))
							throw new Exception("Invalid test number");
						t.DoTest(tnum);
					}
					long ellapsed = sw.ElapsedMilliseconds;
					Utility.WriteLine("\nTOTAL TEST DURATION: {0:0.000} s", (double)(ellapsed)/1000.0);
				}
				catch (Exception e)
				{
					// Generic error display
					Console.Error.WriteLine(e.ToString());
					logger.Error(e.ToString());
				}
				NLog.LogManager.Shutdown();
			}
		}
	}
}
