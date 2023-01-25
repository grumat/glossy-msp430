# Initial Hardware Development using STM32 BluePill/BlackPill #


This prototype board replaces the 
[STM32 Bluepill proto-board](../BluePill-BMP/README.md) as a result of 
the chip crisis started on early 20's, which caused a overwhelming cases 
of STM32F103 clones and counterfeits.

> This project is moving to more recent releases of the STM32 as there 
> is barely a chance that the STM32F103 will ever get manufactured 
> again. Or it may happen that very limited production batches will be 
> sold at a premium price, for those searching for original parts.

For the development of the project we need access to the hardware pins 
and interfaces that realizes our project. So, one of the easiest way 
to go is the *STM32 Blue and Black Pill* prototyping board. This board 
is based on one of the two existing options: The STM32 BluePill and the STM32 BlackPill boards, easily found on eShops. 

This prototype board offers the following advantages:

- Dual board support (either Blue- or Blackpill)
- Unbeatable cost/performance combo
- Lots of documentation and community support for both options
- Higher clock rates when compared to most *Arduino-like* alternatives, 
with a comparable budget.
- Lots of interface pins
- Possibility to drive JTAG at rates above 4 MHz, supported by the MSP430 
JTAG interface
- Integrated USB port to implement a VCP.
- Programmable target voltage (using a PWM pin).
- Voltage level translator, to drive MSP boards with less than 3.3V.
- A Logic analyzer connection.
- Access to most circuit elements for signal monitoring, even for most 
SMD parts.

![BlackPill-BMP.png](images/BlackPill-BMP-fs8.png)


# Technical Solution Description

The BluePill/BlackPill prototype uses the following special features to 
implement the Glossy MSP430 device:
- SPI1 pins are used as communication port allowing to use SPI for JTAG 
transfers. Bit banging is obviously available as a standard GPIO:
  - MISO for **TDI**
  - MOSI for **TDO**
  - SCK for **TCK**
- TMS is controlled by Timer 1 Channel 3 or bit-banging
- A copy of SCK signal is fed into Timer 1 Channel 1 to allow for **TMS 
automated signal generation**.
- Other JTAG signals such as TEST and RST use regular GPIO pins for bit 
bang.
- The design combines all JTAG interface signals share the same GPIO 
port, allowing for bit-bang using a single port access, either by 
software or timer controlled DMA.
- SPI pins are also used for the **SBW**.
- Timer 1 Channel 2 output is reserved to control the direction of the 
SBW data pin. This allows the use of SPI bus to implement the SBW signal, 
with cooperation of the timer.
- A **GBD port** is provided on USART1, required during the early 
development phase (see note below).
- **TRACESWO** is fixed at PB3 and can be connected to a debug emulator 
that supports it.
- USART2 is connected to the JTAG port, at the moment not used, but 
reserved for the **debug COM port** for the target board. This feature 
will be added when the VCP code is finally developed.  
The pinout of the JTAG connector extends the original TI proposal, like 
many suggestions for BSL programming found on by many other design 
references.
- **PA0** is an ADC input, used to read the target I/O voltage. The 
resistor divisor is adjusted to read 50% of VRef input. 
- Firmware will sample the **PA0** input before setting the output 
voltage using the Timer 4 Channel 3 PWM output on **PB8**. 
- A dedicated jumper set is provided to connect a **Logic Analyzer**, 
which is a very useful tool to check the pulse streams and shapes. For 
bit-banging output, widths are a critical part, as the STM32 sometimes 
generates too fast widths, which causes failures. Pulses cannot be 
shorter than 50 ns. 
- Other jumpers are available for 3.3V and GND power lines and also 
access to other STM32 pins which could help other development tasks 
or, in the worst case, patch the board.

![BlackPill-BMP-B-fs8.png](images/BlackPill-BMP-B-fs8.png)


## Notes Regarding VCP Emulation

To allow for firmware debug capability on the early stage of development, 
a GBD port is provided on USART1.  
This is currently required, because when a device hosts a VCP, it is 
required that the firmware is always online to react to USB host packets. 
USB has a minimal flow of packets required for device management. But, a 
minimal development framework needs the use of breakpoints. A breakpoint 
will halt the CPU and cause a drop in the USB link because USB host will 
stop receiving the minimal USB protocol frames. Therefore, a ordinary 
USART is required for the step by step debugging.

Currently, the firmware found on the repository uses a stand-alone 
COM port hardware, so CPU flow may be stopped for debug inspection, while communication link is still online.

Note that the main impact of this combination is performance. Serial port 
will limit transfers to the serial line speed (bps).  In the future, it 
is planned to implement an own VCP in the firmware, a huge performance 
gain will happen, because serial port packets on the USB side are 
essentially virtual and packets arrive directly on the USB payload 
buffers, no UART serialization is needed anymore. Payloads are handled 
directly in the firmware and **bps** won't cause any effect.

Transfer will follow these steps:
- Windows driver queueing requests into USB host hardware (nearly 
instantaneous);
- Cable transfer at 12 Mb rate (was 115.2 kb in real UART mode)
- STM32 hardware decode and DMA into payload buffers (not measurable yet)
- GDB parsing and JTAG (possibly the single bottleneck on this scenario, 
but driven by a 72 MHz Cortex 3 kernel, far better than any MSP430 based 
debug emulator)

> Quite promising!



# The BluePill Socket

Support for the older Bluepill board is also possible. For this option you should fit the board into the socket, aligned to the **left** row.

The detailed view can be seen on the following picture:

![BlackPill-BMP-INS2-fs8.png](images/BlackPill-BMP-INS2-fs8.png)



## Consolidated Pinout

![BlackPill-BMP-pinout-v2.svg](images/BlackPill-BMP-pinout-v2.svg)


### MCU: Target Board Voltage Control

|   Pin   |   Label   | Description                            |
|:-------:|:---------:|:---------------------------------------|
|   PA0   |  VREF_2   | ADC1 Ch0: Read target voltage samples  |
|   PB8   |  TV_PWM   | TIM4_CH3: PWM for DC voltage generator |


### MCU: JTAG/SBW Bus Interface

|   Pin   |   Label   | Description                            |
|:-------:|:---------:|:---------------------------------------|
|   PA1   |   JRST    | GPIO to control the JTAG RST line      |
|   PA4   |   JTEST   | GPIO to control the JTAG TEST line     |
|   PA5   |   JTCK    | SPI1_SCK to control the JTAG CLK line  |
|   PA6   |   JTDO    | SPI1_MISO to control the JTAG TDO line |
|   PA7   |   JTDI    | SPI1_MOSI to control the JTAG TDI line |
|   PA8   |  JTCK_IN  | TIM1_CH1 to sample JTAG clock          |
|   PA10  |   JTMS    | TIM1_CH3 to generate TMS waveform      |


### MCU: JTAG/SBW Control Lines

|   Pin   |   Label   | Description                            |
|:-------:|:---------:|:---------------------------------------|
|   PA9   |   SBWO    | Controls direction of SBW data line    |
|   PB12  |   ENA1N   | Control lines common to JTAG and SBW   |
|   PB13  |   ENA2N   | Control lines required by JTAG         |
|   PB14  |   ENA3N   | Control lines required by UART         |

JTAG connector lines are negative logic signals and will have the 
following scenarios:

| SBWO  | ENA1N | ENA2N | ENA3N | Function                          |
|:-----:|:-----:|:-----:|:-----:|:----------------------------------|
|   1   |   1   |   1   |   1   | HiZ; bus is disabled              |
|   1   |   1   |   1   |   0   | UART active; Debug inactive       |
|   1   |   0   |   1   |   0   | UART active; bus initialization   |
|   1   |   0   |   0   |   0   | UART active; JTAG active          |
|  0/1  |   1   |   0   |   0   | UART active; SBW active           |
|   1   |   0   |   1   |   1   | UART inactive; bus initialization |
|   1   |   0   |   0   |   1   | UART inactive; JTAG active        |
|  0/1  |   1   |   0   |   1   | UART inactive; SBW active         |


### MCU: Target Board USART Interface

|   Pin   |   Label   | Description                           |
|:-------:|:---------:|:--------------------------------------|
|   PA2   |   JTXD    | USART TXD line for target board       |
|   PA3   |   JRXD    | USART RXD line for target board       |


### MCU: USB Interface

|   Pin   |   Label   | Description                              |
|:-------:|:---------:|:-----------------------------------------|
|   PA11  |  USB_DM   | USBD- signal of the USB device (onboard) |
|   PA12  |  USB_DP   | USBD+ signal of the USB device (onboard) |


### MCU: SWD Interface for the Debug Probe Firmware Development

|   Pin   |   Label   | Description                           |
|:-------:|:---------:|:--------------------------------------|
|   PA13  |   SWDIO   | SWDIO signal in BluePill board        |
|   PA14  |   SWCLK   | SWCLK signal in BluePill board        |
|   PB3   |    SWO    | TRACE SWO signal for debug purpose    |


### MCU: UART for Development Phase

|   Pin   |   Label   | Description                           |
|:-------:|:---------:|:--------------------------------------|
|   PB6   |   GDB_TX  | USART TXD line for GDB communication  |
|   PB7   |   GDB_RX  | USART RXD line for GDB communication  |


### MCU: Other Functions

|   Pin   |   Label   | Description                           |
|:-------:|:---------:|:--------------------------------------|
|   PB2   |   BOOT0   | Bootloader (onboard)                  |
|   PC13  |   LEDS    | Red (active low) and Green LED (active high) |


### MCU: Unassigned Pins

|   Pin   |   Label   | Description                           |
|:-------:|:---------:|:--------------------------------------|
|   PA15  |  SYS_JTDI | Unused                                |
|   PB0   |    PB0    | Unused                                |
|   PB1   |    PB1    | Unused                                |
|   PB4   |    PB4    | Unused                                |
|   PB5   |    PB5    | Unused                                |
|   PB9   |    PB9    | Unused                                |
|   PB10  |    PB10   | Unused                                |
|   PB11  |    PB11   | Unused                                |
|   PB15  |    PB15   | Unused                                |
|   PC14  |    PC14   | Unused                                |
|   PC15  |    PC15   | Unused                                |
|   VBAT  |    VBAT   | Unused                                |
|   NRST  |    NRST   | Unused                                |



# The BlackPill Socket

At the core of this prototype, a BlackPill development board is fitted 
into the main socket. Note that this socket uses dual row sockets, 
because it was designed to accommodate either a BlackPil or a BluePill 
board. The BlackPill should be fitted into the **right** pin row. 

The detailed view can be seen on the picture below:

![BlackPill-BMP-INS-fs8.png](images/BlackPill-BMP-INS-fs8.png)

> Please note that firmware development for the BlackPill is suspended 
> because original STM32F411 have also disappeared since the *chip 
> crisis*.  
> Worse than that, clones like the **AT32F403** implements peripherals of 
> the STM32F103 series, or a combo of both, which is counterproductive.  
> Newer **STM32L4xx** or **STM32G4xx** families are easier to buy and 
> offers a substantial upgrade path.


# The TRACESWO Output

The development of a big firmware is almost impossible without a 
tracing facility. SWO is the standard way to go. So this output has a 
dedicated access on the board, near to the SWD jumpers of the Blackpill 
(or Bluepill) board.

The Black Magic Probe provides an input pin for the SWO. Just connect a 
wire to the pin marked on that device.

Speeds of SWO may be an issue and experimentation proves that values 
listed on specs are far above the practical limits, when debugging and 
VCP is also handled simultaneously.

![BlackPill-BMP-SWO-fs8.png](images/BlackPill-BMP-SWO-fs8.png)



# GBD Serial Port

Note that on the final product, GDB connection is provided using a VCP 
firmware implementation, which couples the tool at the max possible 
transfer speed.  
The advantage of the VCP firmware was covered on a topic above.

Regardless, a simple serial port is used during the core of the firmware 
development.  
Once the firmware is mature, the VCP will be added and this GDB serial 
port will be deactivated.

![BlackPill-BMP-GDB-fs8.png](images/BlackPill-BMP-GDB-fs8.png)

> This connector can only be found on this prototype board. Final designs 
> found on [STLinkForm](../STLinkForm/README.md) or 
> [StickForm](../StickForm/README.md) does not incorporate this interface 
> since they are planed when general firmware development gets more 
> mature.


# Second LED

Two LEDs are provided to improve usability. One Red LED is already 
provided on the Blue/Black Pill board and a second, green colored, was 
added to the board.

Both LEDs share the same pin, like in a regular STLink. Either red or 
green, the color indicates the state of the JTAG bus and toggles during 
packet transfers coming from the host.

A red indicates that the JTAG bus is active (low impedance) and the 
green color indicates standby on a powered device.

![BlackPill-SWO-fs8.png](images/BlackPill-LED-fs8.png)

> On the final design LEDs will se a component with dual LED integration, 
> which is a more consistent solution than the one presented on this 
> prototype board.


# The **Logic Analyzer** Connector

For a bus with a complex logic like the JTAG it is very important to 
check JTAG signals using a Logic Analyzer. So the board offers a 
dedicated connector for this purpose.

![BlackPill-LogicAna-fs8.png](images/BlackPill-LogicAna-fs8.png)

> This connection is provided at the 3.3V realm, before the voltage 
> translation occurs.



# The JTAG Connector

This is the connector used to connect the MSP430 board and follows the 
standard TI JTAG layout with additional pins for a serial VCP connection 
like proposed by Elprotronic.

Pins **TDO**, **TDI**, **TMS**, **TCK**, **RST** and **TEST** follows the 
same convention used by a standard MSP-FET, including both **TDO** and 
**TCK** which are also used for the *Spy-bi-Wire* connection.

![JTAG-fs8.png](images/JTAG-fs8.png)

The [MSP430 JTAG/BSL connectors](https://content.elprotronic.ca/docs/JTAG-BSL-Pinout.pdf) 
from Elprotronic covers the fusion of JTAG and BSL into the same 
connector. At the end BSL uses a serial link, which is exactly 
appropriate for the VCP wiring. Note that like usual on a serial 
connection **TXD** and **RXD** interconnection needs to be crossed. So 
the **pin 12** needs to be connected to a **RXD** pin on the MSP430, 
while the pin 14 will be tied to TXD pin on the target MCU (i.e. signal 
crossing occurs on the target board, not in this debug unit).

So for example, to establish a UART link to the **USART0** of a classic 
**MSP430F149** part, connect **JTAG.12** to the **P3.5/URXD0** and 
**JTAG.14** to the **P3.4/UTXD0** of the MCU.

Now for the **Pin 2 TVCC** line, this prototype board can supply up to 
**100 mA** which is enough for almost every available daughter-board.  
The **VCC Sense** pin is a reference entry for the cases that a board is 
self powered. Then this reference voltage is used to adjust the **TVCC** 
output voltage so that voltage levels are compatible.

Do not tie pins **2** and **4** together. They are mutually exclusive and 
official target boards implements a jumper for this selection (this rule 
is also valid when connecting a MSP430 to the official TI JTAG device). 

![BlackPill-JTAG-fs8.png](images/BlackPill-JTAG-fs8.png)



# Voltage Regulator

Note that the Firmware can source a programmable voltage to the **TVCC** 
line to supply the voltage translator circuit that can also be used to 
attach a MSP430 target board using the **JTAG connector**.  
This behavior is also seen on the official TI MSPFET and the older 
MSPFET430UIF.

The **TVCC** supply voltage is protected by a **100 mA** polyfuse, which 
should be enough for most practical cases.



# Voltage Translator

A voltage translator was added to this test board, walking a step further 
if compared to the first BluePill prototype board, which allows for the 
development of the firmware coding that handles programmable supply 
voltages. 

The output signal of this stage have ESD protection, which ensures that 
connecting cables to that connector is generally safe.

> At the time of this writing, firmware still produces a fixed **3.3 V** 
> output on the **TVCC** line, which is used as reference for the target, 
> even if hardware has support for variable voltage.

![BlackPill-LVC-fs8.png](images/BlackPill-LVC-fs8.png)

The circuit is based on a **74xx2T45** and two **74xx125**. These 
circuits are capable of transferring signals at least at a 100 MHz rate, 
which is far more than the requirements for our application.



# Power Supply Test Points

It is very often required to control supply voltages during firmware 
development, so the board has access to the **GND**, **3.3V** and the 
**TVCC**.

Other interesting signals required for the implementation of the 
**TVCC** supply voltage are **Vref** and **TVpwm**, which are also 
accessible through the jumper pads around the BluePill/BlackPill.

![BlackPill-BMP-VCC-fs8.png](images/BlackPill-BMP-VCC-fs8.png)



# A Typical Use Case

The image below shows a typical use case of a firmware debug session:

![UseCase-fs8.png](images/UseCase-fs8.png)

> USB cable should be connected to the Black Magic Probe, the BluePill 
> and the Logic Analyzer.

Each element of this picture are detailed next.



## Blue/Black Pill Development board

At the center you see the development board described on this topic.

In this setup a BluePill is seated at the dual connector. Connection 
cables are provided for the GDB UART port, the SWD debug port, TRACESWO, 
the Logic Analyzer and the MSP430 target board.


## Debug Unit (Black magic Probe - ARM edition)

At the left you you see a STLink-clone converted to a Black Magic Probe 
(ARM) according to 
[this article](https://github.com/grumat/glossy-msp430/wiki/Convert-stlink-to-bmp) 
in our wiki.

> In this particular conversion, the top connector, originally for the 
> SWIM link, was converted to a 3.3V UART port. An internal hardware 
> modification was required for this functionality and this port is used 
> as the GDB debug port. 
> [Check the article](https://github.com/grumat/glossy-msp430/wiki/Convert-stlink-to-bmp).

Attached to the 20-pin ARM JTAG connector, an adapter board is used to 
facilitate the wiring of the **SWD+SWO** connections. The output of this 
adapter board has five wires running to the debug port of the 
BluePill/BlackPill and the SWO jumper on the Development board. 
Note that connection of the VCC supply **is not advised**, but here in 
this image you will see that it has a wire, but although it cannot be 
verified here, the adapter board has a switch to select the VCC 
function.  
Details to this SWD adapter board can be found [on the hardware projects](../SWD-Adapter/README.md).


## The Logic Analyzer

The logic analyzer in this picture is the **LA2016** which has 16 inputs, 
but for this scenario only 6 inputs and the GND wire are required. All 
other cables are simply left unconnected.

An USB cable needs to be connected between the unit and the PC, so that 
the bundled software is able to capture signals.

The following image is an example of the `0x28` **Shift IR** command, 
while the MSP board returns the `0x91` identification byte:

![shift-ir.png](images/shift-ir.png)

> The SPI channel sends 3 bytes to perform this transmission, while the 
> TMS signal was properly generated using a timer and DMA transactions. 
> Since SPI requires byte aligned transfers we use TMS neutral states so 
> that the some of the clock pulses have no effect on the payload.


## The MSP430 Target Board

On this repository you will find schematics and PCB for some MSP430 
devices.

In this picture a [MSP Proto Board](../Target_Proto_Boards/MSP_Proto/README.md) 
is connected using a standard MSP430 14-pin flat cable. This uses the 
standard pinout for MSP430 JTAG emulators, also compatible with the 
commercial TI MSP-FET.

The particular device used in this case, is the **MSP430F2417** and the 
target board is configured to use the 3.3V power supply provided by the 
BluePill board.



# General Development Environment

A brief documentation of the software development environment is 
described in [here](../../GDB430/docs/DevEnv.md).

