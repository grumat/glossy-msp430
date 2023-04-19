# <big>The `Bmt::Gpio` Name-Space</big>

The template classes exposed by this namespace shows the real advantage 
of combining templates with the `constexpr` C++ keyword.

A good example of an blue-pill application using USART1 and a LED:
```cpp
using namespace Bmt;

typedef Gpio::AnyPortSetup<
    Gpio::Port::PA,
    Gpio::Unused<0>,        // unused pin (input + pull-down)
    Gpio::Unused<1>,        // unused pin (input + pull-down)
    Gpio::Unused<2>,        // unused pin (input + pull-down)
    Gpio::Unused<3>,        // unused pin (input + pull-down)
    Gpio::Unused<4>,        // unused pin (input + pull-down)
    Gpio::Unused<5>,        // unused pin (input + pull-down)
    Gpio::Unused<6>,        // unused pin (input + pull-down)
    Gpio::Unused<7>,        // unused pin (input + pull-down)
    Gpio::Unused<8>,        // unused pin (input + pull-down)
    Gpio::USART1_TX_PA9,    // USART1 transmit
    Gpio::USART1_RX_PA10,   // USART1 rreceive
    Gpio::Unused<11>,       // unused pin (input + pull-down)
    Gpio::Unused<12>,       // unused pin (input + pull-down)
    Gpio::Unused<13>,       // unused pin (input + pull-down)
    Gpio::Unused<14>,       // unused pin (input + pull-down)
    Gpio::Unused<15>,       // unused pin (input + pull-down)
    > InitPA;

typedef Gpio::AnyPortSetup<
    Gpio::Unused<0>,        // unused pin (input + pull-down)
    Gpio::Unused<1>,        // unused pin (input + pull-down)
    Gpio::Unused<2>,        // unused pin (input + pull-down)
    Gpio::Unused<3>,        // unused pin (input + pull-down)
    Gpio::Unused<4>,        // unused pin (input + pull-down)
    Gpio::Unused<5>,        // unused pin (input + pull-down)
    Gpio::Unused<6>,        // unused pin (input + pull-down)
    Gpio::Unused<7>,        // unused pin (input + pull-down)
    Gpio::Unused<8>,        // unused pin (input + pull-down)
    Gpio::USART1_TX_PA9,    // USART1 transmit
    Gpio::USART1_RX_PA10,   // USART1 rreceive
    Gpio::Unused<11>,       // unused pin (input + pull-down)
    Gpio::Unused<12>,       // unused pin (input + pull-down)
    Gpio::Unused<13>,       // unused pin (input + pull-down)
    Gpio::Unused<14>,       // unused pin (input + pull-down)
    Gpio::Unused<15>,       // unused pin (input + pull-down)
    > InitPA;
```

At the center of this design is the `AnyPortSetup<>` template. You can 
see a definition of this data-type as a working state of your GPIO in 
when your design enters some specific mode.

For example, on the **glossy-msp430** project, you have an initial state 
where firmware assigns default functions for each GPIO pin. Later, 
prompting for a command received by the PC, it establishes a JTAG 
connection to a target MSP430 chip using many MCU resources, such as SPI, 
Timers, DMA and the classic bit-banging on some pins.

Because of the nature of a JTAG connection, the firmare starts this bus 
using Hi-Z. During the target reset, some pins are are bit-banged while 
others are kept in Hi-Z. Finally, when the JTAG target is captured, all 
bus pins enters low impedance.

Imagine now that through the `AnyPortSetup<>` template you define each of 
these states and through a simple `MyType::Enable()` call alls GPIO pins 
are converted using the least possible number of instructions, almost 
exactly if you code everything in assembly and lots of manual calculation 
for the constants involved for each case.

In C++, you work with many `typedef`, combining them as much as possible, 
which are *abstractions* that compute in *compile-time* each of the 
constant involved on a specific operation.

