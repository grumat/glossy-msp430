# Glossy MSP430 - A Remote GDB Debugger for MSP430

## Primary Goal

This project aims to create a tool similar to the 
[Black Magic Probe](https://github.com/blacksphere/blackmagic) found on 
the ARM processor, but for the Texas Instruments MSP430 MCU line.

The project is roughly based on the Daniel Beer's **mspdebug** but with 
many mix-ins from the **slau320aj - MSP430™ Programming With the JTAG Interface** 
and the **MSP Debug Stack 3.15.1.1**.

Coding is performed in C++ with a self tailored *template library* acting as a *Hardware Abstraction Layer*, refactoring C code if necessary.

The development is made using the outstanding **VisualGDB** and 
**Visual Studio Community 2022**, which IMHO is currently the best 
development combination available for embedded development. Unfortunately 
*VisualGDB* is not a free product but it is worth every buck I paid for. 
So a *makefile* alternative is planned for those that only wants to build 
the binaries.

## What is the Glossy MSP430

The **Glossy MSP430** is a modern, in-application debugging tool for 
embedded microprocessors. It implements the 
[GDB Remote Serial Protocol](https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html), 
which allows you see what is going on *inside* an application running on 
an embedded **MSP430 microprocessor** while it executes. It is able to 
control and examine the state of the target microprocessor using a 
**JTAG** 
or **Spy-Bi-Wire** (SBW) port and on-chip debug logic provided by the 
microprocessor. The probe connects to a host computer using a standard 
USB interface. The user is able to control exactly what happens using the 
GNU source level debugging software, GDB.

### The Special Motivation

The first impression of such a project, is the "Yet another On Chip 
Debugger". But this is a very simplistic view. If you evaluate standard 
MSP430 interfaces, the typical *MSP430.dll+probe combo*, you will notice 
that the amount of operations that these interface is capable to do in 
each request is too simplistic and fine grained.

This ends up in lots of small USB packets, which indeed uses a 
half-duplex principle for the communication. The Windows driver 
stack-based structure, may be extremely flexible, but at the cost of 
performance, it ends that you cannot saturate an USB bus without using 
huge data payloads. Fact is that a 12Mb USB link will constantly idle in 
the current *MSP430.dll* architecture. This adds latencies between every 
short messages.

GDB on the other side uses a higher level protocol and the JTAG probe 
requires more embedded intelligence to fully attend a request, reducing 
the number of in and outs to complete a request.  
As a result, faster responses are expected and that is exactly what you experience with the referred solution for the ARM platform.

Another important point: This is a my hobby project and no boss or time 
schedules are priorities here. I am reading and reviewing all STM32 
hardware documentation, to extract all possible performance for this 
application. For example, bit banging is the fastest way to develop a 
JTAG/SBW protocol and currently I've already developed a JTAG 
implementation based on a combination of SPI+DMA+Timer to generate the 
**TMS signal logic** and the result of this work added the following benefits:
- Transmit a request at **9 MHz** using a STM32F103 controller 
(**72MHz / 8 = 9MHz**). This is at least double the speed of the best 
case scenario for bit-banging (MSP430 JTAG interface limits this 
interface to **10MHz**).
- Easy possibility to select lower speed to **4.5**, **2.25**, **1.13** 
and **0.56 MHz** improve connection when using lengthy cables or lower 
power supplies.
- On newer STM32 variants it is possible to reach the **10 MHz** limit by choosing MCU clocks of **80** or **160 MHz**.
- Possibility to use DMA to feed data to the SPI device while MCU is decoding the next request, which shortens the latency between requests.

The wiki contains details on this work.


## Differences of the Glossy MP430 and the Black Magic Probe

The following specifications where added or changed to accomplish the 
MSP430 version:
- JTAG pin-outs using TI 14-pin standard.
- Instead of SWD, the MSP430 uses the Spy-Bi-Wire as a two wire protocol.
- The **TRACESWO** is not present on a MSP430 MCU.
- Programmable variable JTAG transfer speed.

Summarizing, these are the features:
- Targets almost 500 parts from the MSP430 families: MSP430F1xx, 
MSP430F2xx, MSP430Gxxx, MSP430F4xx, MSP430F5xx, MSP430FRxxx. (completed)
- Connects to the target processor using the JTAG interfaces. (completed)
- Connects to the target processor using the Spy-Bi-Wire (SBW) 
interfaces. (TBD)
- Provides full debugging functionality, including: flash memory 
breakpoints, memory and register examination, flash memory programming, 
etc. (partially functional)
- Interface to the host computer is a standard USB CDC ACM device 
(virtual serial port), which does not require special drivers on Windows, 
Linux or OS X. (TBD)
- Implements the GDB extended remote debugging protocol for seamless 
integration with the GNU debugger and other GNU development tools. (almost complete)
- Implements USB DFU class for easy firmware upgrade as updates become 
available. (TBD)
- Works with Windows, Linux and Mac environments.

With the gdb partnership, the *Glossy MSP430* allows you to:

- Load your application into the target Flash memory, FRAM or RAM.
- Single step through your program.
- Run your program in real-time and halt on demand.
- Examine and modify CPU registers and memory.
- Obtain a call stack backtrace.
- Set up to 8 hardware assisted breakpoints.


## List of Projects in the VS2022 Solution

This repository has a VS2022 solution which contains various projects, described next.

### bmt

A template library for the STM32 MCU that uses advanced C++ techniques to develop an highly optimized code having the flexibility of a "HAL" environment, but the same performance of a bare metal firmware that has hard-coded direct register access.

### EraseDCO

Note: This will be probably removed.

### EraseDCOX

Note: This will be probably removed.

### EraseXv2

This is a MSP430 *funclet* responsible for erasing flash on MSP430 
CPUXv2 parts, which integrates dedicated timing circuits for the Flash 
memory.

### ExtractChipInfo

A very simple C project to extract the TI database from the 
**MSP Debug Stack 3.15.1.1** source code. This is actually a trick where 
a zip file is encoded into a C array definition -- typical result of a 
Bin2C tool. This zip contains a set of XML files that forms a MSP430 
device information database.

The result can be extracted using a conventional ZIP tool. The result was expanded into the **MSP430-devices** folder.

> Note that there are errors in the **xsd** XML definition and this 
> extracted instance has these corrections.

### GDB430

This is the firmware, based on the VisualGDB plugin using at it's core a gcc ARM compiler. It is planned to develop make-files, so one have the possibility to compile it in a Unix system.

### ImportDB

This is a C# project that decodes the XML files and produces a SQLite 
database with the contents of the XML. SQL is far more practical for the 
data-mining work that was made to create the chip database.

Not all information was imported from the XML. The work was focused on the requirements for the JTAG debugging interface.

Another source of data is the data extracted with the **ScrapeDatasheet** C# project. There we find flash timing information required for the Flash routines, not provided by the usual TI database.

### MakeChipInfoDB

Old Chip DB generation tool. **Deprecated**.

### MkChipInfoDbV2

This is a refactored version of the **MakeChipInfoDB** tool. Database 
organization was improved and now it produces a more compact version than 
of the initial tool.

It is a C# tool that produces a C++ include file to be used by the 
**GDB430** firmware to locate MSP430 part numbers and attributes 
necessary for the JTAG operation. The firmware loads identification 
bytes from the connected chip and queries the database. Part 
number, memory sizes, chip errata and other interface properties are 
retrieved as a result, so that the behavior of the JTAG connection can be 
fine tuned, according to the inspected MSP430 chip.

A general command line guide to build the **Chip Info** database consists 
of the following steps:
```
> ScrapeDataSheet
> ImportDB "ExtractChipInfo\MSP430-devices\devices" "ScrapeDataSheet\Results\All Data.csv" "ImportDB\Results\results.db"
> MkChipInfoDbV2 "ImportDB\Results\results.db" "GDB430\ChipInfoDB.h"
```

> Please note that the **ScrapeDataSheet** tool contains hard-coded paths 
> and should be correctly compiled for a specific installation before one 
> is able to run the steps described before.  
> Note that the repository already has a copy of the results of this tool 
> because one of the requirements is to download **all** data-sheets from 
> TI website.


### ScrapeDataSheet

This is a C# tool that scapes the PDF data-sheet of all MSP430 devices 
and produces a CSV list of some of its attributes.

To use this tool you need to download all data-sheets from the TI web 
site and edit the source code with the path to locate them.

A scrapping is performed to locate the Flash Memory JTAG and SBW 
specifications to produce the final result.  
A copy of the current results was already added to the repository, so no 
one will require to do it by himself.  
See: [ScrapeDataSheet/Results/All Data.csv](ScrapeDataSheet/Results/All%20Data.csv)

This result is used to produce the chip database using the 
**MkChipInfoDbV2** tool.

The valuable information extracted here a specially the JTAG and SBW max 
clock speeds and the Flash timings for Erase and Write operations. Note 
that the original TI database does not provide this information, since 
MSP-FET uses a DCO clock timing tool to install a *funclet* to generate 
the 450 kHz clock frequency required for the flash operation.

We opted to drive this clock using the TCLK JTAG input, as the STM32 can 
very easily generate a stable frequency at that pin. For this to work, we 
need specs for individual MCU parts.


## Other Repository Folders

When cloning the repository, some other folders will be created. The 
contents is briefly described next.

### FuncletsInterface

A folder with include files to be shared between ARM firmware and MSP430 
*funclets*.

### Hardware

A folder to store all hardware projects. These projects are developed 
using KiCad 6. Please note the dedicated topic below which describes them 
with more details.


## Hardware Platform

> Please note that current development state is very preliminary and  
> focused on one primary objectives: Performance of the JTAG link.
> Although most of the infra-structure for the debugger was already 
> ported integration of them are still pending. Also, SBW support is
> pending, which is another performance optimization task, quite more 
> difficult to accomplish due to the multiplexed nature of the data line.

Two robust hardware alternative were developed to allow one for a 
professional looking solution and, most important, support for flexible 
target supply voltages.

Current alternatives are:

- A prototype option using the Blue-pill or Black-Pill. This is currently 
the best option for those that want to experiment the project. Please 
note that this option limits the supply voltage to 3.3V, which is good 
for usual prototype boards, but many other devices will operate in lower levels.  
![BlackPill-BMP-fs8.png](Hardware/BlackPill-BMP/images/BlackPill-BMP-fs8.png)  
[Details for this option can be found here](Hardware/BlackPill-BMP/README.md).

- Conceptually similar to the previous prototype, a board was developed 
to be used with a couple of Nucleo-32 boards. This is a new move caused 
by the chip mangle, since the old STM32F103 is now a rarity and I don't 
want additional delays when fiddling with clones. Hardware for newer 
parts are also more robust and faster. Although this prototype board is 
ready and running here, I am still concentrated in my initial proposal 
and later I will move definitively to newer hardware as I already have 
bought these MCU parts.
![L432KC-fs8.png](Hardware/L432KC/images/L432KC-fs8.png)  
[Details for this option can be found here](Hardware/L432KC/README.md).

- BMPMSP: a preliminary physical prototype that suits into a nice form 
factor. The schematics has evolved since that release and a new board 
will be produced for the final product.  
![MSPBMP.png](Hardware/MSPBMP/images/MSPBMP.png)
[Details here](Hardware/MSPBMP/README.md).

- BMPMSP2-stick: Same as above but more compact.  
![MSPBMP2-stick.png](Hardware/MSPBMP2-stick/images/MSPBMP2-stick.png)  
[Details here](Hardware/MSPBMP2-stick/README.md).


Note that for all four options you will find a KiCad project on the repo.

Last but not least, it is planed to support ST-Link V2 clones using the clone hardware provided with them, but at the cost of some important features:
- no support to power supply voltages other than 3.3V
- no standard MSP JTAG connector
- UART function requires a hardware mod.

So, a low cost option is possible to make tests before going to a more featured but costly option.



## Target Prototype Boards

Besides the series of affordable targets of the *LaunchPad series* from 
TI, some other targets requires prototype boards for the development of 
the firmware, specially in this phase where I am aiming the JTAG 
protocol.

On the repo you will find KiCad prototypes for:

- Generic legacy MSP430 board, for many variants, such as **MSP4301611**, 
**MSP430F249** and many others. This board has pads for dual pin-outs and 
instead of a single option with the MCU at the top, an alternative 
pin-out is also found at the bottom. These optional pins are compatible 
with the MSP430G2955 and other variants.  
![MSP_Proto.png](Hardware/Target_Proto_Boards/MSP_Proto/images/MSP_Proto.png)  
[Details here](Hardware/Target_Proto_Boards/MSP_Proto/README.md).

- Another option is designed for a newer model using **MSP430FR2476**.  
![FR2476.png](Hardware/Target_Proto_Boards/FR2476/images/FR2476.png)  
[Details here](Hardware/Target_Proto_Boards/FR2476/README.md).

- The third option is designed for the **MSP430F5418** and its siblings.  
![F5418.png](Hardware/Target_Proto_Boards/F5418/images/F5418.png)  
[Details here](Hardware/Target_Proto_Boards/F5418/README.md).


## Secondary Goal

New coding is performed in C++ with a self tailored *template library* 
and it serves to demonstrate that modern C++ can easily outperform C 
development with the use of the ``constexpr`` keyword.

The ``constexpr`` keyword sits like a *glove* to embedded development as 
most peripheral register addresses are pure constants.

So very complex logic decisions are simplified wherever possible during compile time for every ``constexpr`` in the template library and unnecessary code are automatically removed and the final result is just a couple of lines.

