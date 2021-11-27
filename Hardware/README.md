# Hardware Platform

At the current point there are many alternatives to be used as a platform
to run the firmware developed here.

The first is an accessible option that can be developed using simple prototype
boards and its primary goal is to serve as a firmware development platform.

Two other robust hardware designs were also developed to allow the use of the 
tool with a professional looking solution and, most important, support for 
flexible target supply voltages, as it incorporates a level shifter.

Note that for all options you will find a KiCad project on the repo.

Last but not least, it is planed to support ST-Link V2 clones using the clone 
hardware provided with them, but at the cost of some important features:
- no support to power supply voltages other than 3.3V
- no standard MSP JTAG connector
- no UART function.

So, a low cost option is possible to make tests before going to a more featured 
but costly option.

## Firmware Development Platform

A design was made for the firmware development, based on the STM32 Blue-Pill. 
This design used standard through hole resistors and the typical 2.54 mm pin 
distances.

The design has all necessary contents to be sent to a PCB manufacturer but it
is also perfect for ones that have standard prototyping boards.

The design contains pins for the connection of a Logic Analyzer, which is very
important in the development stage. Surely if one wants just test the firmware
this is also the best option.

As this platform has a fixed 3.3V regulator this is the only supply option and
MSP prototype boards are best connected to the JTAG connector, drawing current
from the standard pin 2.

![BluePill-BMP,png](BluePill-BMP/images/BluePill-BMP.png)  
[Details for this option can be found here](BluePill-BMP/README.md).


## Workbench quality designs

In general, I searched for options based on the ARM platform as they are very 
affordable. and maybe it is possible to desolder and reuse components now that
those chips are impossible to find (chip crisis!).

My interest was to reuse the plastic case and have a professional look. This 
designs have special board shapes that will fit into those chinese JTAG clones.

The first option is to produce the full featured and complete version of the
MSP430 BMP and is designed to fit into those ST-LINK v2 clones, which costs 
around 20€ in amazon.de. If you buy them in aliexpress, then prices are better.

This is the design that fits this enclosures:

![MSPBMP.png](MSPBMP/images/MSPBMP.png)  
[Details here](MSPBMP/README.md).


The second design uses a more compact option but for this looses the additional
serial port connector.

It uses a more rare alternative to the ST-Link called "baite" variant, which are 
sold in a green transparent plastic case.

As TI JTAG connector uses a 14-pin connector a change on the original design was 
done: The connector is extern to the plastic case, which at the end produces a 
perfect fit.

The BMPMSP2-stick uses a more recent version of the STM32 MCU and is illustrated 
below:

![MSPBMP2-stick.png](MSPBMP2-stick/images/MSPBMP2-stick.png)  
[Details here](MSPBMP2-stick/README.md).


## Target Prototype Boards ##

Besides the series of affordable targets of the *LaunchPad series*, some larger 
targets requires prototype boards for the development of the firmware.

On the repo you will find KiCad prototypes for:

- Generic legacy MSP430 board, for many variants, such as MSP4301611, MSP430F249 
and many others. This board has pads for dual pin-outs and instead of just the
option of a MCU at the top, an alternative pin-out was added at the bottom that 
is compatible with the MSP430G2955 and similar variants.   
So a single PCB manufacturing provides really lots of alternatives:  
![MSP_Proto.png](Target_Proto_Boards/MSP_Proto/images/MSP_Proto.png)  
[Details here](Target_Proto_Boards/MSP_Proto/README.md).

- An option was designed for a newer model using the FRAM based MSP430FR2476.  
![FR2476.png](Target_Proto_Boards/FR2476/images/FR2476.png)  
[Details here](Target_Proto_Boards/FR2476/README.md).

- As additional option was designed for the MSP430F5418A family, which is still
based on flash memory, but uses the CPUXv2 of the MSP430.
![F5418.png](Target_Proto_Boards/F5418/images/F5418.png)  
[Details here](Target_Proto_Boards/F5418/README.md).

This diversity is required as JTAG protocols for each family have minor 
differences and we aim more compatibility.
