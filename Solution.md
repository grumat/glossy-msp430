# List of Projects in the VS2022 Solution

This repository has a VS2022 solution which contains various projects 
described below:


## bmt

A template library for the STM32 MCU that uses advanced C++ techniques to 
develop an highly optimized code having the flexibility of a "HAL" 
environment, but the same performance of a bare metal firmware that has 
hard-coded direct register access.


## ChipInfo/ExtractChipInfo

A very simple C project to extract the TI database from the 
**MSP Debug Stack 3.15.1.1** source code. This is actually a trick where 
a zip file is encoded into a C array definition -- typical result of a 
Bin2C tool. This zip contains a set of XML files that forms a MSP430 
device information database.

The result can be extracted using a conventional ZIP tool. The result was 
expanded into the **MSP430-devices** folder.

> Note that there are errors in the **xsd** XML definition and this 
> extracted instance has these corrections.


## ChipInfo/ImportDB

This is a C# project that decodes the XML files and produces a SQLite 
database with the contents of the XML. SQL is far more practical for the 
data-mining work that was made to create the chip database.

Not all information was imported from the XML. The work was focused on 
the requirements for the JTAG debugging interface.

Another source of data is the data extracted with the **ScrapeDatasheet** 
C# project. There we find flash timing information required for the Flash 
routines, not provided by the usual TI database.


## ChipInfo/MkChipInfoDbV2

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


## ChipInfo/ScrapeDataSheet

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


## Funclets/EraseXv2

This is a MSP430 *funclet* responsible for erasing flash on MSP430 
CPUXv2 parts, which integrates dedicated timing circuits for the Flash 
memory.


## Funclets/Interface

A folder with include files to be shared between ARM firmware and MSP430 
*funclets*.


## Funclets/WriteFlashXv2

This is a MSP430 *funclet* responsible for writing flash on MSP430 
CPUXv2 parts, which integrates dedicated timing circuits for the Flash 
memory.


## GDB430

This is the firmware, based on the VisualGDB plugin using at it's core a 
gcc ARM compiler. It is planned to develop make-files, so one have the 
possibility to compile it in a Unix system.


## Hardware

A folder to store all hardware projects. These projects are developed 
using KiCad 6. Please note the dedicated topic below, which describes 
them with more details.

[See details here](Hardware/README.md).


## TestFunclets

This folder contains little MSP430 programs used by the unit tests.


## UnitTest

This folder contains a C# program used to perform unit tests. These 
tests can also be performed using regular debug probes, like the 
**MSP-FET**, **MSP-FET430UIF** or **Olimex MSP430-JTAG-TINY-V2**.
This test is crucial to maintain consistency between the large MSP430 
family devices.

