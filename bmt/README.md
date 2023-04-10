# <big>bmp Library</big>


# Introduction

The **bmt** library has a collection of C++ template classes for 
selected STM32 micro-controllers series.


## The C++ `constexpr` modifier

It features modern C++ language features that allows one to produce the 
very compact code. The library benefits from the `constexpr` keyword 
introduced in later C++ compilers, which perfectly fits bare metal 
development, since most of the interface code handles absolute addresses, 
typically hardware registers.

The `constexpr` modifier instructs the compiler that elements are 
constants and expressions and logic decision can be optimized out at 
compile time. Think of it as a complete *C++ interpreter* build into the 
C++ compiler. Everything that can be solved by the *interpreter* happens 
on the compilation phase and the remainder is the code that has to be 
solved at *runtime*.

At the end, most functions looks more complex than the C variant, but 
because logic can be solved as constants it ends up with compact code 
surpassing any of the available C libraries. It ends up with the code as 
if one verbosely writes into hardware registers.

A brilliant example is the `Gpio::AnyPortSetup<>` template class. Using
this template class one is able to provide the full list of pin 
assignments for any available GPIO port in a single element.

The class features two important methods, the `Setup()` and the `Init()`. 
While the first merges GPIO bits the seconds writes registers directly,
which means all pins are setup at once, regardless of pin direction or 
alternate functions. Except if you are using just the raw GPIO elements, 
which tends to be very confusing and a real mess to refactor your 
project, the C API would consist of a lots of `LL_GPIO_Set***()` calls.  
Or at the best use the `HAL_GPIO_Init()` which allows to configure a set 
of pin with identical setup.

## Compatibility

Currently the library supports only STM32F103 chip and an initial 
STM32L432 compatibility is also provided.

On the next topics, a brief usage guide for the library will be 
presented. 


## Dependencies / Compiler Setup

To use the **bmt** library you need to provide paths to the STM32 driver 
files and a define with the MCU model you are using.

The path depends highly on your installation, but for VisualGDB STM32 MCU 
drivers are installed in:

```
%USERPROFILE%\AppData\Local\VisualGDB\EmbeddedBSPs\arm-eabi\com.sysprogs.arm.stm32
```

As already mentioned this is just a base path I mentioned for your 
convenience. For the compiler you will have to provide a complete path 
for your driver files and I will detail this below.

The drivers provided by ST also requires a define with the model of the MCU 
you are using.

Another important setting is a define value for the exact MCU 
model you are willing to use.

So once you have both information you have to configure your build system 
accordingly.

Using the mentioned VisualGDB installation paths as a reference , the 
configuration follows this table:

|     MCU     | Path                                             | Define |
|-------------|--------------------------------------------------|--------|
| STM32F103CB | `.\STM32F1xxxx\CMSIS_HAL\Device\ST\STM32F1xx\Include\stm32f1xx.h` | `STM32F103xB` |
| STM32L432KC | `.\STM32L4xxxx\CMSIS_HAL\Device\ST\STM32L4xx\Include\stm32l4xx.h` | `STM32L432xx` |


For example, the GCC command line could look like this:
```
arm-none-eabi-gcc.exe -DSTM32F103xB -I "%USERPROFILE%\AppData\Local\VisualGDB\EmbeddedBSPs\arm-eabi\com.sysprogs.arm.stm32\STM32F1xxxx\CMSIS_HAL\Device\ST\STM32F1xx\Include\stm32f1xx.h" ...etc...
```


# STM32 Clock Tree

These classes provide access to the clock tree hardware. The clock tree 
differs for each MCU family, so you have to follow the User's Guide for
configurations supported by your hardware.

Each clock related class will contain at least one of the following 
members described next.


## General Rules for the Clock Classes

All classes from this domain are accesses through the `Bmt::Clocks` 
name-space.

In general a MCU will have clock circuits like the HSI, HSE, LSI, LSE and 
probably at least one PLL.

This module will offer at least one template class for each available 
clock circuit. For example on the STM32F103 family, the following classes 
are available for each clock circuit:

| Class    | Description |
|----------|-------------|
| `AnyHsi` | A template class responsible to configure the HSI with a specified trim value. |
| `AnyHse` | A template class responsible to configure the HSE with a set of parameters.|
| `AnyLse` | |
| `Lsi` | |
| `AnyPll` | |


In the next subtopics common members and methods are described.


### The `kClockSource_` Member

This is an ID indicating the clock generator. This value is defined in 
the `enum Id` and varies according to the MCU family. The identifier 
allows other classes to verify if a clock is allowed.  
For example, the PLL can only be sourced by some specific clock circuits 
and the library could have a `static_assert()` to ensure that some 
invalid clock source is chained with the PLL.

> Note that since we are configuring hardware a configuration has only 
> constant values or parameters.  The frequency is obtained from values 
> declared on the data-sheet regardless of component tolerance or 
> trimming values.


### The `kFrequency_` Member

The frequency generated by this clock source. Some clock sources have 
fixed frequency, others depends on configuration parameters and for 
chained clock circuits, like the PLL, from the frequency of this source 
and configuration parameters, computed internally by the library.


### The `kClockInput_` Member

For indepenent clock sources this is exactly the same value as 
`kClockSource_`, meaning its input is generated by an internally 
calibrated circuit. 

But if the clock is connected in chain with another source, this value is 
the `Id` of this source input, so we are able to track how this chain is 
configured.

### The `Init()` Method

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


### The `Enable()` Method

The `Enable()` method is exactly like the `Init()` method with the 
exception that chained clocks are not changed. This means that for most 
clock circuits they are exactly the same.

> As a general rule of thumb you will use the `Init()` during firmware 
> initialization and `Enable()` in normal device use.


### The `Disable()` Method

The `Disable()` method is used to disable the clock circuit, exactly as 
the name suggests.

Please note that it is important to ensure that no peripheral or CPU 
depends on the clock signal you are disabling, or you may stall your 
device.


# SysTick Classes

These classes provides access to the System Tick timer of the MCU.
