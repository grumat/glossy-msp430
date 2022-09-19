# MSP430i2041 Proto Board (SLAU335)

> For testing **SLAU335** devices family.

The proto-board for the **MSP430i20xx** family is target for the 
application specific devices with **TSSOP-28** package. This is a special 
family designed for metering and are equipped with extraordinary 
**24-bit D/A**, so there are not so many options available. 

This board offers a set of configurable options that will be covered by 
this documentation.

The pictures shows a 3D model of this prototype board:

![SLAU335-fs8.png](images/SLAU335-fs8.png)

These are the features:
- Support for the single series of chips having a **TSSOP-28** pin-out: 
the MSP430i2020, MSP430i2021, MSP430i2030, MSP430i2031, MSP430i2040, 
and the MSP430i2041. 
- Standard 14-pin JTAG connector.
- JTAG bus access for a logic analyzer.
- Configurable support for JTAG and Spi By Wire.
- Support for power supply from JTAG connector or internal 3.3V regulator 
using a Micro USB cable.
- Reset button.
- A very basic analog voltage support is provided for ADC use.
- Test led on **P2.2** configurable by jumper.
- All IO ports wired to accessible header pins.
- Header pins for **VCC**, **USB +5V** and **GND**.


# Users Guide

The following points describes general use of this board.


## Configuring the Board According to MCU model

This board supports all distinct **MSP430i20xx** parts of this family 
having **TSSOP-28** package. Although they have almost identical pin-out, 
note that the analog frontend is different and the part model determines 
the number of existing analog channels.

Note that there is a dedicated jumper set for the analog inputs which are 
differential. So for example, if you solder a MSP430i2031, only 3 analog 
inputs are available, so two inputs are not available and pins have no other usable function.

<img src="images/3-inputs-fs8.png" alt="3-inputs-fs8.png" width="320">

> Like the picture above, you are allowed to solder a shorter jumper to 
> avoid improper use of channels that are not available.


## External USB power supply

To use the external power supply connect a powered µUSB cable into 
**J1**. For this case the switch **SW1** will control the power supply. 

<img src="images/usb-fs8.png" alt="usb-fs8.png" width="240">

Before connecting a JTAG cable into **J7**, ensure that the **VSEL** 
jumper shorts the **Vref** position.

<img src="images/Vref-fs8.png" alt="Vref-fs8.png" width="220">


## Powering from the JTAG tool

If you are just connecting the proto-board without additional hardware, then the power supply of the emulator will be enough to supply the 
installed chip and it is easier to use the emulator as source.

For this option just move the jumper on **VSEL** to the **Vtool** 
position and connect the JTAG emulator to the **J7** JTAG connector. When 
the JTAG is connected and running it will supply the board.

<img src="images/Vtool-fs8.png" alt="Vtool-fs8.png" width="220">

Note that the switch **SW1** and the **+5V** jumper has no effect on this 
configuration and should be left disconnected or turned off.


## Selecting JTAG or SBW Modes

Since pin-outs may differ for different debug emulators the board offers 
two different options in the **J3** jumper set.

The silk screen indicates four jumpers that needs to be shorted for the 
standard JTAG interface.

If you choose Spy-Bi-Wire, only two jumpers are necessary.

> These options are mutually exclusive: **although possible, it is not 
> recommended to enable multiple options at the same time**.


### JTAG Configuration

For the JTAG mode the four jumpers connects the following pins:
- TDO &rarr; TDO (P1.3)
- TCK &rarr; TCK (P1.0)
- RST &rarr; RESET
- TEST &rarr; TEST

<img src="images/JTAG-fs8.png" alt="JTAG-fs8.png" width="220">

> Please make sure that no other connection is made on **P1.0**, 
> **P1.1**, **P1.2** and **P1.3** while using the JTAG bus as this could 
> cause a conflict with the emulator.

### Spy-By-Wire Configuration - TI pin-out

TI SBW uses the following connections:
- TDO &rarr; RESET
- TCK &rarr; TEST

<img src="images/SBW-TI-fs8.png" alt="SBW-TI-fs8.png" width="210">


### Connection for the Logic Analyzer

During firmware development it is very important to have the ability to 
read out the digital waves for the **JTAG** bus, since timing is a very 
critical factor.

This board offers an access to all signal required for debug:

<img src="images/LogicAna-fs8.png" alt="LogicAna-fs8.png" width="180">


## Reset Button

The Reset button can be used to restart the device. It is not advised to 
interrupt a running JTAG connection by pressing this button. Some 
references states that attached MCU may enter an undefined state.

<img src="images/reset-fs8.png" alt="reset-fs8.png" width="180">


## Other Voltage Supplies

The board exposes all power supplies through jumpers. It is advised to 
follow good practice rules, as these are unprotected. The **+5V** pins 
are connected to a USB bus or power adapter and is only available if 
supplied by an USB cable. Low quality power sources may be a cause of 
issues.

<img src="images/power-fs8.png" alt="power-fs8.png" width="150">


## Using the LED function

To use the LED, just short the **LED_P1.2** jumper.

<img src="images/LED-fs8.png" alt="LED-fs8.png" width="240">

A test program for the LED test could be:

```cpp
#include <msp430.h>

void Delay()
{
	long counter = 0;
	while (counter++ < 5000)
		asm("nop");
}

void MainLoop()
{
	WDTCTL = WDTPW | WDTHOLD;
	
	P2DIR |= (1 << 2);
	
	for (;;)
	{
		P2OUT |= (1 << 2);
		Delay();
		P2OUT &= ~(1 << 2);
		Delay();
	}
}

int main()
{
	MainLoop();
}
```


