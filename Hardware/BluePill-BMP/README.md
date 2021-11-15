# Initial Hardware Development using STM32 BluePill #


For the development of the project we need access to the hardware pins and components. So, one of the easiest way to go is the *STM32F103 Blue Pill*.


This is a really cheap and powerful solution. The chip offers following advantages if compared to other 8 or 16-bit cheap modules:

- Unbeatable price/performance
- Lots of documentation and community support
- Higher clock rates when compared to MSP430 alternatives
- Lots of interface pins
- Possibility to *bit-bang* pin at a rate near 10 MHz, which is the upper limit of the MSP430 JTAG interface
- A SPI peripheral, also able to run on those high clock speeds, which is a real alternative to the *bit bang* option
- Possibility to timer a DMA to bit bang GPIO port at these high rates (clock generation for the Flash erase command).
- Integrated USB port to implement a VCP.

![BluePill-BMP.png](images/BluePill-BMP.png)

> Note that using the *Blue Pill* you are at a fixed supply
> rate of 3.3V, which may not be compatible to all MSP430
> designs out there.
