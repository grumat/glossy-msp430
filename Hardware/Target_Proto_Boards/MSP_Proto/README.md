# MSP430 Generic Proto Board #

The generic Proto-board for the MSP430 family is target to the older models with LQFP64
case, which was a very consistent pin-out, meaning that it is possible to solder many
MSP430F1xx or MSP340F2xx parts without special changes.

At the bottom an extra pin-out was added for the MSP430G2955 part or similar. By 
manufacturing this board on China, it is common to order a 5 pieces batch. You can
surely use all of them for many MSP430 variants.

The pictures shows a 3D model of this prototype board:

![MSP_Proto.png](images/MSP_Proto.png)

These are the features:
- Support for common LQFP64 parts, such as MSP430F23x, MSP430F24x(1), MSP430F2410, 
MSP430F2619, MSP430F2618, MSP430F2617, MSP430F2616, MSP430F2419, MSP430F2418, 
MSP430F2417, MSP430F2416, MSP430F15x, MSP430F16x, MSP430F161x, MSP430F147, MSP430F148, 
MSP430F149, MSP430F1471, MSP430F1481, MSP430F1491, MSP430F133, MSP430F135 and possibly
others.
- Support for MSP430G2x44, MSP430G2x55, MSP430G2x44, MSP430F22x2 and MSP430F22x4
- Standard 14-pin JTAG connector
- Support for Spi By Wire (for the 38-pin parts)
- Serial port on the JTAG connector.
- Support for power supply from JTAG connector or internal 3.3V regulator using a Micro 
USB cable.
- 32768 Hz crystal on XT1.
- 8 or 16 MHz crystal on XT2.
- Basic analog voltage filtering for ADC usage.
- Test led on P4.1 configurable by jumper.
- Al IO ports wired to accessible header pins.
- Header pins for VCC, USB +5V, GND, AVCC and AGND.

## Choosing the MCU model

Please note that the board supports two different MCU pin-outs (mutually exclusive).
Follow the instructions depending on the MCU model you want to mount.

### Mounting a 64 pin LCQFP MCU

These are the steps to follow:
- Solder the LQFP64 part into **U1** aligning the pin 1 to the mark on the silk-screen.
- Choose a crystal **Y2** for 8 or 16 MHz according to the MCU model you are soldering 
(see the datasheet for the allowed frequency). The board is configured for a 12pF 
crystal model.
- Remove resistor **R1** and **R6** for optimal JTAG performance
- Remove capacitor **C3** for optimal JTAG performance

### Mounting a 38 pin MCU

These are the steps to follow:
- Solder the TSSOP 38 part in the **U2** position (bottom) aligning the pin1 to the 
tiny copper dot.
- Choose a **Y2** crystal compatible to the MCU model
- Following components aren't used and can optionally be removed (or not mounted):
**R2**, **C7** and **Y1**.

## Users Guide

The following points describes general use of these boards.

### External USB power supply

To use the external power supply connect a powered µUSB cable into **J4**. For this 
case the switch **SW1** will control the power.

Before connecting a JTAG cable into **J1**, ensure that the **VSEL** jumper shorts 
the **Vref** position.

### Powering from the JTAG tool

During most simple tests with the board the best option is to power it from the
JTAG emulator.

For this option just connect the JTAG tool to the **J1** JTAG connector and move 
the jumper on **VSEL** to the **Vtool** position. When the JTAG is connected and
running it will supply the board with its internal power supply.

Note that the switch **SW1** has no effect on this configuration, also the 
**+5V** jumper has no effect.

### Header for Supplies Voltages

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

