# MSP430FR2476 Proto Board

The proto-board for the MSP430FR2476 family is target for the newer FRAM devices with 
LQFP48. There are not so many options with this pin-out, but FRAM changes JTAG 
operations quite a lot, so it is worth the investment for the development of a embracing
MSPBMP firmware.

This board offers many configuration options as these chips support JTAG and SBW and
many reserved pins for legacy parts can now operate as GPIO. Then you have more jumpers 
and solder jumpers than on other prototype boards.

The pictures shows a 3D model of this prototype board:

![FR2476.png](images/FR2476.png)

These are the features:
- Support for a couple of LQFP48 FRAM parts: the MSP430FR2475, MSP430FR2476, MSP430FR2675 
and MSP430FR2676.
- Standard 14-pin JTAG connector
- Configurable support for JTAG and Spi By Wire. SBW can be configured for standard TI 
emulators and Olimex MSP430-JTAG-Tiny-V2 emulators.
- Serial port on the JTAG connector.
- Support for power supply from JTAG connector or internal 3.3V regulator using a Micro 
USB cable.
- Configurable use of a 16 MHz crystal.
- Reset button.
- Configurable use of analog voltage (ADC support).
- Test led on P4.1 configurable by jumper.
- Al IO ports wired to accessible header pins.
- Header pins for VCC, USB +5V and GND.

## Users Guide

The following points describes general use of these boards.

### External USB power supply

To use the external power supply connect a powered µUSB cable into **J4**. For this 
case the switch **SW1** will control the power.

Before connecting a JTAG cable into **J12**, ensure that the **VSEL** jumper shorts 
the **Vref** position.

### Powering from the JTAG tool

During most simple tests with the board the best option is to power it from the
JTAG emulator.

For this option just connect the JTAG tool to the **J12** JTAG connector and move 
the jumper on **VSEL** to the **Vtool** position. When the JTAG is connected and
running it will supply the board with its internal power supply.

Note that the switch **SW1** and the **+5V** jumper has no effect on this 
configuration.

### Reset Button

The Reset button can be used at any time to restart the device. It is not advised
too interrupt a JTAG connection by pressing this button. Some references states
that MCU may enter an undefined state.

### Attaching the Crystal

The crystal is by default not connected, since pins may be configured as a general 
GPIO port. To enable this function, short both solder bridges marked with the 
**XTAL** label.

### Analog Power Supply

The Analog supply pins are configurable as GPIO pins. To enable them as power 
source short both solder jumpers **AVCC** and **AGND**. In this case ensure
not to set the pins with the GPIO function.

### Other Voltage Supplies

The board exposes all power supplies through jumpers. It is advised to follow
good practice rules, as there are no kind of protection. The +5V pins are 
connected to a USB bus or power adapter. Low quality power sources may be 
a cause of issues.

### Using the LED function

To use the LED, just connect the **LED_P4.1** jumper.

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
	
	P4DIR |= (1 << 1);
	
	for (;;)
	{
		P4OUT |= (1 << 1);
		Delay();
		P4OUT &= ~(1 << 1);
		Delay();
	}
}

int main()
{
	MainLoop();
}
```


