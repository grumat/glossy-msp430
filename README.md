# Black Magic Probe - MSP430 Edition

## Primary Goal ##
This project aims to create a tool similar to the [Black Magic Probe](https://github.com/blacksphere/blackmagic) seen on the ARM processor, but for the Texas Instruments MSP430 MCU line.

The project is roughly based on the Daniel Beer's **mspdebug** but with many mix-ins from the **slau320aj - MSP430™ Programming With the JTAG Interface** and
the **MSP Debug Stack 3.15.1.1**.

New coding is performed in C++ with a self tailored *template library*.

The development is made using the outstanding **VisualGDB** and **Visual Studio Community 2019**, which IMHO is currently the best development combination available for embedded development. Unfortunately *VisualGDB* is not a free product but it is worth every buck I paid. So a *makefile* alternative is planned for those that only wants to build the binaries.

## What is the Black Magic Probe ##

In its own words:

> The Black Magic Probe is a modern, in-application debugging tool for embedded microprocessors. It allows you see what is going on 'inside' an application running on an embedded microprocessor while it executes. It is able to control and examine the state of the target microprocessor using a JTAG or Serial Wire Debugging (SWD) port and on-chip debug logic provided by the microprocessor. The probe connects to a host computer using a standard USB interface. The user is able to control exactly what happens using the GNU source level debugging software, GDB.

### The Special Motivation ###

The first impression of such a project, is the "Yet another On Chip Debugger". But this is a simplistic view. If you evaluate interfaces such as the MSP430.dll you will notice that the amount of operations that these interface is capable is too simplistic and fine grained.

So there are lots of USB protocol communication, which indeed is of a half-duplex nature and does not suits well the send and receive of packets with alternating direction. USB typically likes more unidirectional data flow.

This adds latencies between every short messages. GDB on the other side uses a higher level protocol and the JTAG dongle requires more embedded intelligence to fully atend a request, reducing latencies in the USB line.

So you will expect faster responses and that is what you experience if you compare BMP with most of the existing solutions.

## Differences of the Black Magic Probe - MSP430 Edition ##

Keeping the idea of On Chip Debugger, the following specifications where added or changed to accomplish the MSP430 version:
- JTAG pinouts using TI 14-pin standard.
- Operating voltages lower than 3.3V, which is very common on MSP430 designs.
- Instead of SWD, the MSP430 uses the Spy-By-Wire as an alternate protocol.

Sumarizing, these are the features:
- Targets MSP430 Families: MSP430F1xx, MSP430F2xx, MSP430Gxxx, MSP430F4xx, MSP430F5xx, MSP430FRxxx.
- Connects to the target processor using the JTAG or Spy-By Wire (SBW) interfaces.
- Provides full debugging functionality, including: flash memory breakpoints, memory and register examination, flash memory programming, etc.
- Interface to the host computer is a standard USB CDC ACM device (virtual serial port), which does not require special drivers on Linux or OS X.
- Implements the GDB extended remote debugging protocol for seamless integration with the GNU debugger and other GNU development tools.
- Implements USB DFU class for easy firmware upgrade as updates become available.
- Works with Windows, Linux and Mac environments.

With the gdb partnership, the BMP/MSP430 allows you to:

- Load your application into the target Flash memory, FRAM or RAM.
- Single step through your program.
- Run your program in real-time and halt on demand.
- Examine and modify CPU registers and memory.
- Obtain a call stack backtrace.
- Set up to 8 hardware assisted breakpoints.


## Hardware Platform ##

Two robust hardware alternative were developed to allow one for a professional looking solution and, most important, support for flexible target supply voltages.

Current alternatives are:

- A prototype option using the Blue-pill. This is the best option to those that want to experiment the project. Please note that this option limits the supply voltage to 3.3V, which is good for usual proto-boards, but many other devices will operate in lower levels.  
![BluePill-BMP,png](Hardware/BluePill-BMP/images/BluePill-BMP.png)  
[Details for this option can be found here](Hardware/BluePill-BMP/README.md).

- BMPMSP: a dedicated hardware that suits into a nice form factor.  
![MSPBMP.png](Hardware/MSPBMP/images/MSPBMP.png)
[Details here](Hardware/MSPBMP/README.md).

- BMPMSP2-stick: Same as above but more compact.  
![MSPBMP2-stick.png](Hardware/MSPBMP2-stick/images/MSPBMP2-stick.png)  
[Details here](Hardware/MSPBMP2-stick/README.md).


Note that for all three options you will find a KiCad project on the repo.

Last but not least, it is planed to support ST-Link V2 clones using the clone hardware provided with them, but at the cost of some important features:
- no support to power supply voltages other than 3.3V
- no standard MSP JTAG connector
- no UART function.

So, a low cost option is possible to make tests before going to a more featured but costly option.

## Target Prototype Boards ##

Besides the series of affordable targets of the *LaunchPad series*, some larger targets requires prototype boards for the development of the firmware.

On the repo you will find KiCad prototypes for:

- Generic legacy MSP430 board, for many variants, such as MSP4301611, MSP430F249 and many others. This board has pads for dual pinouts and instead of an MCU at the top, an alternative pinout is found at the bottom compatible with th MSP430G2955 and compatible variants. [Details here](Hardware/Target_Proto_Boards/MSP_Proto/README.md).  
![MSP_Proto.png](Hardware/Target_Proto_Boards/MSP_Proto/images/MSP_Proto.png)

- Another option is designed for a newer model using MSP430FR2476. [Details here](Hardware/Target_Proto_Boards/FR2476/README.md).  
![FR2476.png](Hardware/Target_Proto_Boards/FR2476/images/FR2476.png)

## Secondary Goal ##

New coding is performed in C++ with a self tailored *template library* and it serves to demonstrate how modern C++ can outperform C development with the use of the ``constexpr`` keyword.

The ``constexpr`` keyword sits like a *glove* to embedded development as most peripheral register addresses are pure constants.

So very complex logic decisions are simplified wherever possible during compile time for every ``constexpr`` in the template library and unnecessary code are automatically remved and the final result is just a couple of lines.

