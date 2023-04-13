# <big>STM32 Clock Tree</big>

These template classes provide access to the clock tree of selected STM32 
MCUs. The clock tree differs for each MCU family, so you have to follow 
the User's Guide for configurations supported by your hardware.

> These template classes are only type definitions. No instance are 
> required. Methods are all static.


# General Rules for the Clock Classes

All classes from this domain are accesses through the `Bmt::Clocks` 
name-space.

In general a MCU will have clock circuits like the HSI, HSE, LSI, LSE and 
probably at least one PLL.

This name-space will offer at least one template class for each available 
clock circuit. For example, on the STM32F103 family, the following 
classes are available for each clock circuit:

| Class    | Description                                                | 
|----------|------------------------------------------------------------| 
| `AnyHsi` | A template class responsible to configure the HSI with a specified trim value. |
| `AnyHse` | A template class responsible to configure the HSE with a set of parameters.|
| `AnyLse` | A template class responsible to configure the LSE with a set of parameters.|
| `AnyLsi` | A template class that represents the LSI clock instance.   | 
| `PllVco` | A template class with a PLL ratio calculator.              |
| `AnyPll` | A template class to configure the PLL with given parameters. |
| `AnySycClk` | A template class to configure the clock tree and buses. |
| `AnyUsbSycClk` | A derivative of `AnySycClk` tailored for USB applications. |

Apart from the last two classes, which binds clocks sources and 
establishes a setup for all clock buses, all other clock classes will 
contain at least one of the following members described in the next 
subtopics. 


## The `kClockSource_` Member

This is an ID indicating the clock generator. This value is defined in 
the `enum Id` and varies according to the MCU family. The identifier 
allows other classes to verify if this clock source is supported.  
For example, the PLL can only be sourced by the HSI or HSE clock. Other 
combinations will stop compilation because a `static_assert()` will 
ensure that an invalid clock source cannot be chained with the PLL.


## The `kFrequency_` Member

The frequency generated by this clock source. Some clock sources have 
fixed frequency, others depends on configuration parameters and for 
chained clock circuits, like the PLL, from the frequency of this source 
and configuration parameters, computed internally by the library.

> Note that since we are configuring hardware, a configuration has only 
> constant values or parameters.  The frequencies are obtained from 
> values declared on the data-sheet regardless of component tolerance or 
> trimming values. 


## The `kClockInput_` Member

For independent clock sources this is exactly the same value as 
`kClockSource_`, meaning its input is generated by an internal 
dedicated circuit. 

But if the clock is connected in chain with another source, this value is 
the `Id` of this source input, so we are able to track how this chain is 
configured.


## The `Init()` Method

This method is provided by all clock sources and is used to initialize a 
clock circuit for the first time, typically when a firmware boots.

For the most cases this method just calls the `Enable()` method. An 
exception to this rule is when a clock is chained like the PLL, the proper 
`Init()` from the attached source is called implicitly, so you are not 
required to write multiple `Init()` lines.

Example:
```cpp
// The HSE clock has a 8MHz crystal soldered on the board
typedef Clocks::AnyHse<8000000UL> HSE;

void main()
{
    // ... Init other stuff ...

    // Starts the crystal
    HSE::Init();

    // ... etc ...
}
```


## The `Enable()` Method

The `Enable()` method is exactly like the `Init()` method with the 
exception that chained clocks are not changed. This means that for most 
clock circuits they are exactly the same.

> As a general rule of thumb you will use the `Init()` during firmware 
> initialization and `Enable()` in normal device use.


## The `Disable()` Method

The `Disable()` method is used to disable the clock circuit, exactly as 
the name suggests.

Please note that it is important to ensure that no peripheral or CPU 
depends on the clock signal you are disabling, or you may stall your 
device.


# The `AnyHsi<>`/`AnyHsi16<>` template class

The HSI is an internal clock, usually used as primary clock when a device 
boots. It is typically imprecise, because no stable timing element is 
used for frequency generation, so it is not advised to be used as source 
of a peripherals that requires clock accuracy, like USB.

On devices that features an MSI clock, this clock is not used for the 
boot process. Details on device-specific data-sheet.

This circuit offers a trimming adjustment which can be defined as a 
constant (default) or variable using any means a user wants for 
calibration.

When instantiating the template without an argument the factory default 
trimming value is used.

The example below assumes that the HSI is already running (as part of the 
boot process), but lets one change the trimming value:
```cpp
void TrimHsi(uint8_t val)
{
    // Changes the trim value
    AnyHsi<>::Trim(val);
}
```

Other methods like `AnyHsi<>::Init()`, `AnyHsi<>::Enable()` and 
`AnyHsi<>::Disable()` are available. The last one is probably the most 
important one, when one drives the SYSCLK with an external crystal
(i.e. `AnyHse<>` template) and wants to disable the HSI to save energy. 


# The `AnyMsi<>` template class

MSI stands for *Multi Speed Internal* clock.  
This class happens only on some family members, when the MSI clock is 
available. This clock source offers additional features when compared to 
the HSI clock.

> When the MCU features the MSI clock it is also the default clock source 
> used after reset and starts typically at 4 MHz.

This template has the following parameters:
- `kFreqCR`: This enumeration allows one to select one of the predefined 
clock frequencies in the range of 100 kHz up to 48 MHz.
- `kPllMode`: This flag activates a very useful feature of this clock 
circuit, which is a PLL controlled by the LSE clock. This configuration 
enhances the stability and accuracy of this clock source, enabling 
support for USB communication.

Example:

```cpp
using namespace Bmt;

// LSE using standard 32768 XTAL
typedef Clocks::AnyLSE<> LSE;
// MSI 48 MHz using PLL feature
typedef Clocks::AnyMSI<Clocks::MsiFreq::k48_MHz, true> MSI;
// System clock driven by MSI
typedef Clocks::AnySycClk<MSI> SYSCLK;

void main()
{
    // ...

    // Initializes the 32768 XTAL
    LSE::Init();
    // Now that LSE is running, initializes and run system 
    // with MSI at 48 MHz clock
    SYSCLK::Init();

    // ...
}
```

Note that the configuration show above assumes that hardware contains a
32.768 kHz crystal soldered to the LSE clock pins.  
The example initializes the LSE first before initializing the clock tree. 
The clock tree is represented by `AnySysClk<>`, which is documented 
later on this document. The clock tree internally calls `MSI::Init()` 
to initialize the MSI; this is possible since it is part of `AnySysClk<>` 
declaration.


# The `AnyHse<>` template class

This class is responsible for configuring the HSE clock. The HSE clock
is the *High Speed External Clock*, which contains an external crystal as 
timing reference and is generally recommended when time base must be 
exact.

The template class accepts 3 arguments:
- `kFrequency`: Is the frequency of the crystal and is defined by your 
hardware design. Typical values are 8 or 16 MHz, which should be typed 
in Hz literally.
- `kBypass`: Is a flag that indicates if the circuit uses bypass mode. 
This is also defined by your hardware design and indicates that other 
device is providing the clock in a dedicated input pin. It also means 
that the active part of the clock circuit is turned off.
- `kCssEnabled`: Enables the *Clock Security Feature*. Check data-sheet 
for usage details.

Example:

```cpp
using namespace Bmt;

// HSE clock generator has as 12 MHz XTAL
typedef Clocks::AnyHSE<12000000UL> HSE;
typedef Clocks::AnySycClk<HSE  // Turns HSE into main clock
    , Power::Mode::kRange1     // remove this line for STMF1xx
    > SYSCLK;
void main()
{
    // ...

    // Initializes and run system with HSE 12 MHz clock
    SYSCLK::Init();
}
```


# The `PllVco<>` template class

In this name-space this is called a *calculator class*. It uses an 
interesting `constexpr` method to figure out the best values for a PLL 
fraction to generate a specific frequency.

