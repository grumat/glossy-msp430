# Glossy MSP430 - A Remote GDB Debugger for MSP430

## Primary Goal ##
This project aims to create a tool similar to the [Black Magic Probe](https://github.com/blacksphere/blackmagic) found on the ARM processor, but for the Texas Instruments MSP430 MCU line.

The project is roughly based on the Daniel Beer's **mspdebug** but with many mix-ins from the **slau320aj - MSP430™ Programming With the JTAG Interface** and
the **MSP Debug Stack 3.15.1.1**.

New coding is performed in C++ with a self tailored *template library* acting as a *Hardware Abstraction Layer *

The development is made using the outstanding **VisualGDB** and **Visual Studio Community 2019**, which IMHO is currently the best development combination available for embedded development. Unfortunately *VisualGDB* is not a free product but it is worth every buck I paid. So a *makefile* alternative is planned for those that only wants to build the binaries.

## What is the Glossy MSP430 ##

The **Glossy MSP430** is a modern, in-application debugging tool for embedded microprocessors. It implements the [GDB Remote Serial Protocol](https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html), which allows you see what is going on 'inside' an application running on an embedded MSP430 microprocessor while it executes. It is able to control and examine the state of the target microprocessor using a JTAG or Spy-Bi-Wire (SBW) port and on-chip debug logic provided by the microprocessor. The probe connects to a host computer using a standard USB interface. The user is able to control exactly what happens using the GNU source level debugging software, GDB.

### The Special Motivation ###

The first impression of such a project, is the "Yet another On Chip Debugger". But this is a simplistic view. If you evaluate standard MSP430 interfaces, the typical MSP430.dll+dongle you will notice that the amount of operations that these interface is capable to do in each request is too simplistic and fine grained.

This ends up in lots of small USB packets, which indeed uses a half-duplex principle for the communication. The Windows driver stack-based structure, may be extremely flexible, but at the cost of performance, it ends that you cannot saturate an USB bus without using huge data payloads. Fact is that a 12Mb USB link will constantly idle in the current MSP430.dll architecture. This adds latencies between every short messages.

GDB on the other side uses a higher level protocol and the JTAG dongle requires more embedded intelligence to fully attend a request, reducing latencies in the USB line.

As a result faster responses are expected and that is exactly what you experience with the referred solution for the ARM platform.


## Differences of the Glossy MP430 and the Black Magic Probe ##

Keeping the idea of On Chip Debugger, the following specifications where added or changed to accomplish the MSP430 version:
- JTAG pin-outs using TI 14-pin standard.
- Instead of SWD, the MSP430 uses the Spy-By-Wire as a two wire protocol.
- The **TRACESWO** is not present on a MSP430 MCU.
- Programmable JTAG transfer speed.

Summarizing, these are the features:
- Targets almost 500 parts from the MSP430 families: MSP430F1xx, MSP430F2xx, MSP430Gxxx, MSP430F4xx, MSP430F5xx, MSP430FRxxx.
- Connects to the target processor using the JTAG or Spy-By Wire (SBW) interfaces.
- Provides full debugging functionality, including: flash memory breakpoints, memory and register examination, flash memory programming, etc.
- Interface to the host computer is a standard USB CDC ACM device (virtual serial port), which does not require special drivers on Linux or OS X.
- Implements the GDB extended remote debugging protocol for seamless integration with the GNU debugger and other GNU development tools.
- Implements USB DFU class for easy firmware upgrade as updates become available.
- Works with Windows, Linux and Mac environments.

With the gdb partnership, the *Glossy MSP430* allows you to:

- Load your application into the target Flash memory, FRAM or RAM.
- Single step through your program.
- Run your program in real-time and halt on demand.
- Examine and modify CPU registers and memory.
- Obtain a call stack backtrace.
- Set up to 8 hardware assisted breakpoints.


## List of Projects in the VS2019 Solution

These repository has a VS2019 solution which contains various projects.

### bmt

A template library for the STM32 MCU that uses advanced C++ techniques to develop an highly optimized code having the flexibility of a "HAL" environment, but the same performance of a bare metal firmware that has hard-coded direct register access.

### EraseDCO

Note: This will be probably removed.

### EraseDCOX

Note: This will be probably removed.

### EraseXv2

This is a MSP430 *funclet* responsible for erasing flash in the parts that implements the CPUXv2, which integrates dedicated timing circuits for the Flash memory.

### ExtractChipInfo

A very simple C project to extract the TI database that is a zip file encoded in a C array definition -- typical result of a Bin2C tool. This zip contains a set of XML files that forms a MSP430 device information database.

The result can be extracted using a conventional ZIP tool. The result was expanded into the **MSP430-devices** folder. Note that there are errors in the **xsd** XML definition, usually character case and this extracted instance has these corrections.

### GDB430

This is the firmware, based on the VisualGDB plugin using at it's core a gcc ARM compiler. It is planned to develop make-files, so one have the possibility to compile it in a Unix system.

### ImportDB

This is a C# project that decodes the XML files and produces a SQLite database with the contents of the XML. QSL is far more practical for the data-mining work that was made to create the chip database.

Not all information was imported from the XML. The work was focused on the requirements for the JTAG debugging interface.

Another source of data is the data extracted with the **ScrapeDatasheet** C# project. There we find flash timing information required for the Flash routines, not provided by the TI database.

### MakeChipInfoDB

Old Chip DB generation tool. Deprecated.

### MkChipInfoDbV2

This is a refactored version of the **MakeChipInfoDB** tool. Database organization was improved and produced a more compact version than of the initial tool.

It is a C# tool that produces a C++ include file to be used by the **BMP430** firmware to locate MSP430 part numbers and attributes necessary for the JTAG operation. The firmware loads identification bytes from the connected chip and runs a query on the database. Part number, memory sizes, chip errata and other interface properties are retrieved as a result, accommodating the behavior of the JTAG connection, according to the inspected MSP430 chip.

### ScrapeDataSheet

This is a C# tool that scapes the PDF data-sheet of all MSP430 devices and produces a CSV list of some of its attributes.

To use this tool you need to download all data-sheets from the TI web site and edit the source code with the path to locate them.

A scrapping is performed to locate the Flash Memory JTAG and SBW specifications to produce the final result.  
A copy of the current results was already added to the repository, so no one will require to do it by himself.

This result is used to produce the chip database using the **MkChipInfoDbV2** tool.

The valuable information extracted here a specially the JTAG and SBW max clock speeds and the Flash timings for Erase and Write operations. Note that the original TI database does not provide this information, since MSP-FET uses a DCO clock timing tool to install a *funclet* to generate the 350 kHz clock frequency required for the flash operation.

We opted to drive this clock using the TCLK JTAG input, as the STM32 can very easily generate a stable frequency at that pin. This avoids spending time trimming the MSP430 clock speed. Another problem if you read the MSP-FET source code you will see that the *funclets* approach has also issues with some silicon bugs and devices with low RAM sizes, which increases the code complexity.

## Other Repository Folders

When cloning the repository, some other folders will be created. The contents is briefly described next.

### FuncletsInterface

A folder with include files to be shared between ARM firmware and MSP430 *funclets*.

### Hardware

A folder to store all hardware projects. These projects are developed using KiCad 6. Please note the dedicated topic below which describes them with more details.


## Hardware Platform

Two robust hardware alternative were developed to allow one for a professional looking solution and, most important, support for flexible target supply voltages.

Current alternatives are:

- A prototype option using the Blue-pill. This is the best option to those that want to experiment the project. Please note that this option limits the supply voltage to 3.3V, which is good for usual prototype boards, but many other devices will operate in lower levels.  
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

- Generic legacy MSP430 board, for many variants, such as MSP4301611, MSP430F249 and many others. This board has pads for dual pin-outs and instead of a single option with the MCU at the top, an alternative pin-out is also found at the bottom. This optional pins is compatible with the MSP430G2955 and other variants.  
![MSP_Proto.png](Hardware/Target_Proto_Boards/MSP_Proto/images/MSP_Proto.png)  
[Details here](Hardware/Target_Proto_Boards/MSP_Proto/README.md).

- Another option is designed for a newer model using MSP430FR2476.  
![FR2476.png](Hardware/Target_Proto_Boards/FR2476/images/FR2476.png)  
[Details here](Hardware/Target_Proto_Boards/FR2476/README.md).

- The third option is deigned for the MSP430F5418 and its siblings.
![F5418.png](Hardware/Target_Proto_Boards/F5418/images/F5418.png)  
[Details here](Hardware/Target_Proto_Boards/F5418/README.md).

## Secondary Goal ##

New coding is performed in C++ with a self tailored *template library* and it serves to demonstrate how modern C++ can outperform C development with the use of the ``constexpr`` keyword.

The ``constexpr`` keyword sits like a *glove* to embedded development as most peripheral register addresses are pure constants.

So very complex logic decisions are simplified wherever possible during compile time for every ``constexpr`` in the template library and unnecessary code are automatically removed and the final result is just a couple of lines.

`ScrapeDataSheet`
`ImportDB "ExtractChipInfo\MSP430-devices\devices" "ScrapeDataSheet\Results\All Data.csv" "ImportDB\Results\results.db"`
`MkChipInfoDbV2 "ImportDB\Results\results.db" "GDB430\ChipInfoDB.h"`
