# Initial Hardware Development using Nucleo-32 STM32L432KC or STM32G431KB


Since the chip crisis of 2020 an alternative was considered to replace 
the STM32F103 series since official stores have no stock and chinese 
sources are unreliable, providing mostly clones or counterfeits. Still 
accessible are newer releases of STM32 like the **STM32L432KC** or 
**STM32G431KB**, through the Nucleo-32 development boards.

This is a still cheap and powerful solution and has the advantage that 
newer chips offers more efficient design than the older Cortex M3:

- Lots of documentation and community support
- Higher clock rates when compared to STM32F103 alternatives
- Enhanced peripherals
- Possibility to *bit-bang* pin at a rate between 4 MHz and 10 Mhz, which 
is the upper limit of the MSP430 JTAG interface
- A SPI peripheral, also able to run on those high clock speeds, which is 
a real alternative to the *bit bang* option.
- Possibility to DMA timer transitions and emulate the TMS signal at high 
speeds and take advantage of the SPI interface near 10MHz bit rate.
- Possibility to timer a DMA to bit bang GPIO port at high rates to 
provide flash erase clock.
- Integrated USB port to implement a fast VCP.

![BSTM32L432KC.png](images/L432KC-fs8.png)

> Note that this new design also includes voltage regulators and bus 
> voltage translators to allow for JTAG interfaces at lower voltages.

# Feature Description

The **STM32L432KC/STM32G431KB** prototype has the following features:
- SPI pins are used as communication port allowing to use SPI for JTAG 
transfers. Bit banging is obviously available as a standard GPIO:
  - MISO for **TDI**
  - MOSI for **TDO**
  - SCK for **TCK**
- **TMS** is controlled by Timer 1 Channel 2 or bit-banging
- A copy of SCK signal is fed into Timer 1 Channel 1 (aka **TI1FP1**) 
to allow for **TMS automated signal generation**.
- Other JTAG signals such as **TEST** and **RST** use regular GPIO pins 
for bit bang. In the design all signals share the same GPIO port, 
allowing for bit-bang using a single access to BSRR register, either by 
software or timer controlled DMA.
- The Nucleo-32 board has an VCOM already implemented in the embedded 
ST-Link emulator. This port is connected to **USART2**. As mentioned, 
this VCOM is used in the early stage of the development, because we 
frequently need to stop CPU for debugging purpose and a VCOM 
implementation on our firmware would drop USB link.  
An independent COM port hardware allows us to set breakpoints wherever 
needed to inspect variables and hardware registers while communication 
resources are indefinitely online.
- **TRACESWO** is fixed at PB3 and already connected to the ST-Link 
emulator inside the Nucleo-32 board.
- **USART1** is connected to the JTAG port, at the moment not used, but 
reserved for the debug COM port for the target board. This feature comes 
when the USB firmware part is finally developed.
Note that this pin-out is suggested for BSL programming by many 
references on the internet (more details below).
- **USB port connector** that will be used on the mature releases of the 
firmware, implementing two VCOMs: one for GDB and the other for the debug 
UART.
- PB0 is configured as an ADC input and used to read the 
**target reference voltage**, which in this case will always be 50% of 
the pin 4 of the JTAG connector. This allows for variable JTAG interface 
voltages.
- A dedicated jumper set is provided to connect a **Logic Analyzer**, 
which is a very useful tool to check the signal shape and sequences. For 
a bit-banging control, widths are a critical point, as the STM32 can 
generate pulses shorter than the minimum allowed pulse width for a 
MSP430, which causes failures. Pulses shall not be shorter than 50 ns.
- Outputs for **red** and **green** LED. Green LED is turned on as device 
is powered and blinks when commands are received from the GDB. The red 
LED indicates that an MSP430 is attached and controlled by JTAG. Boards 
should not be connected or disconnected while this LED is on.
- Other jumpers are available for **3.3V**, **TVCC** and **GND** power 
lines and also access to other STM32 pins which could help other 
development tests.

This is the board without the Nucleo-32 board:

![L432KC-B-fs8.png](images/L432KC-B-fs8.png)



# The Nucleo-32 Socket

At the core of these prototype, a Nucleo-32 development board is fitted. 
It is a very common and affordable development platform. An official 
board which includes an ST-Link emulator costs less than US$20,00. 
While options like the Blue and Black Pill may cost less, they require 
a separate emulator and are usually populated with clones or 
counterfeits, specially for the BluePill.

![L432KC-Conn-fs8.png](images/L432KC-Conn-fs8.png)


## Configuring STM32L432KC or STM32G431KB

Please note that both Nucleo-32 options have a minor pinout difference, 
so you are required to compensate it by adjusting two jumper setting, as 
shown below:

![L432KC-MCU-Sel-fs8.png](images/L432KC-MCU-Sel-fs8.png)

This jumper selects the LED and the TXD function pins. For a Nucleo-32 
**L432KC** you should short the two lower pins. For the **G431KB** model 
short the upper pins, like shown on the silk-screen.



# USB Connector

During the development phase, power supply is taken from the embedded 
ST-Link board. On a later firmware release a VCOM interface will be 
implemented for fast GDB transfers and the ST-Link will not be needed 
anymore.

The installed Micro USB connector allows you to supply the Nucleo board 
with power and establish an USB connection.

![L432KC-USB-fs8.png](images/L432KC-USB-fs8.png)



# GBD Serial Port

For the first stage of the firmware development we will use an external 
VCP implementation to create the GDB link. For this situation the VCP 
port provided by the ST-link emulator will be used. This interface 
connects to the USART2 peripheral of the MCU.

The port is used directly by GDB to control a debugging session.
Supposing your VCP port is installed on COM3, a typical GDB link is 
obtained by commanding:

```
target extended-remote COM3
```

on the GDB terminal.

It is planned on a later stage to add a VCP implementation into the 
firmware, when we finally can get rid of the latencies caused by 
serializing the VCOM request to the USART2 bus at the specified BAUD rate.  
A VCP implementation at the firmware level has the advantage that data 
payloads are promptly on the USB buffers and can be handled immediately, 
regardless of the programmed BAUD rate.

But since a complex project requires the possibility to debug if line by 
line, an internal VCOM port would stall each time a breakpoint hits, 
because the processor is in a frozen state and the PC will end up 
dropping the peripheral.

The external VCOM is slow but it is the best way to decouple 
communication from an ongoing debug session. 



# Indicator LEDs

Two LEDs are provided to improve usability. A Green LED that indicates 
power is on and incoming data packets by short flashes. The Red LED is 
used to indicate an ongoing debug session, which means, the target CPU is 
under JTAG control.

Like in any other JTAG tool, it is not recommended to disconnect the 
emulator from the target CPU while a debug session is ongoing.

![L432KC-LEDs-fs8.png](images/L432KC-LEDs-fs8.png)



# The LogicAna Connector

For a bus with a complex logic like the JTAG it is very important to 
check JTAG signals using a Logic Analyzer. So the board offers a 
dedicated connector for this purpose.

![L432KC-LogicAna-fs8.png](images/L432KC-LogicAna-fs8.png)

> This connection is provided at the 3.3V realm, before the voltage 
> translation occurs.


# The JTAG Connector

This is the connector used to connect the MSP430 board and follows the 
standard TI JTAG layout with additional pins for a serial VCP connection 
like proposed by Elprotronic.

Pins **TDO**, **TDI**, **TMS**, **TCK**, **RST** and **TEST** follows the 
same convention used by a standard MSP-FET, including both **TDO** and 
**TCK** which are also used for the *Spy-bi-Wire* connection.

![JTAG-fs8.png](../BlackPill-BMP/images/JTAG-fs8.png)

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

![L432KC-JTAG-fs8.png](images/L432KC-JTAG-fs8.png)



# Voltage Regulator

Note that the Firmware can source a programmable voltage to the **TVCC** 
line to supply the voltage translator circuit that can also be used to 
attach a MSP430 target board using the **JTAG connector**.  
This behavior is also seen on the official TI MSPFET and the older 
MSPFET430UIF.

The **TVCC** supply voltage is protected by a **100 mA** polyfuse, which 
should be enough for most practical cases.



# Voltage Translator

A voltage translator was added to this test board, which allows for the 
development of the firmware coding that handles programmable supply 
voltages. 

The output signal of this stage have ESD protection, which ensures that 
connecting cables to that connector is generally safe.

> At the time of this writing, firmware still produces a fixed **3.3 V** 
> output on the **TVCC** line, which is used as reference for the target, 
> even if hardware has support for variable voltage.

![L432KC-TXS-fs8.png](images/L432KC-TXS-fs8.png)

The circuit is based on a **74xx2T45** and two **74xx125**. These 
circuits are capable of transferring signals at least at a 100 MHz rate, 
which is far more than the requirements for our application.



# Power Supplies Test Points

It is very often required to control supply voltages during firmware 
development, so the board has access to the **GND**, **3.3V** and the 
**TVCC**.

Another interesting signal required for the implementation of the 
**TVCC** supply voltage is **Vref**.  
**Vref** has to be sampled by the ADC and a DAC is be used to 
generate a voltage proportional to the **Vref** input to generate the 
**TVCC**.

![L432KC-VCC-fs8.png](images/L432KC-VCC-fs8.png)



# A Typical Use Case

The image below shows a typical use case of a firmware debug session:

![UseCase-fs8.png](images/UseCase-fs8.png)

> USB cables should be connected to the ST-Link port, and the Logic 
> Analyzer.

Each element of this picture are detailed next.



## Nucleo-32 and Development Board

At the center you see the development board described on this topic.

In this setup a Nucleo-32 is seated at the provided connector. 
Connections cables are provided for the Logic Analyzer and the MSP430 
target board which drains power from the Nucleo-32 board.

The Nucleo-32 contains a ST-Link emulator providing SWD, SWO and GDB 
interfaces. The SWD allows one to install a new firmware to the core 
controller and perform debug sessions. The SWO is able to read the 
trace messages that helps debugging the firmware. And the GDB UART port 
was already described before.

This setup provides an lighter setup when compared to the Blue/Back Pill 
development variant.


## The Logic Analyzer

The logic analyzer **LA2016** has 16 inputs, but we need just 6 inputs and 
a GND wire. All other cables are simply left unconnected.

An USB cable needs to be connected between the unit and the PC, so the 
bundled software is able to capture the JTAG pulses.

This is an example of the `0x28` **Shift IR** command, while the MSP board 
returns the `0x91` identification byte:

![shift-ir.png](images/shift-ir.png)

> The SPI chanel sends 3 bytes to perform this transmission, while the 
> TMS signal was properly generated using a timer and DMA transactions. 
> Since SPI requires byte aligned transfers we use TMS Run/Idle states so 
> that the clock pulses without data have no effect on the payload.


## The MSP430 Target Board

On this repository you will find schematics and PCB for some MSP430 
devices.

In this picture a [MSP Proto Board](../Target_Proto_Boards/MSP_Proto/README.md) 
is connected using a standard MSP430 14-pin flat cable, and the pin-out 
is compatible with other existing JTAG emulators, such as the TI MSP-FET.

The particular device used in this case, is the **MSP430F1611** and the 
target board uses the 3.3V power supply provided by the Nucleo-32 board.



# General Development Environment

A brief documentation of the software development environment is 
described in [here](../../GDB430/docs/DevEnv.md).

