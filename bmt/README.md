# <big>The Bmt (*Bare Metal Templates*) Library</big>


# Introduction

The **bmt** library has a collection of C++ template classes for 
selected STM32 micro-controllers series and was primarily developed for 
the Glossy MSP430 Project.  
In general all templates were designed to be used in other scenarios.


## The C++ `constexpr` modifier

It features modern C++ language features that allows one to produce a 
very compact code. The library benefits from the `constexpr` keyword 
introduced in later C++ compilers, which perfectly fits bare metal 
development, since most of the interface code handles absolute addresses, 
typically hardware registers.

The `constexpr` modifier instructs the compiler that elements are 
constants, also expressions and logic decisions can be optimized out at 
compile time. Think of it as a complete *C++ interpreter* build into the 
C++ compiler. Everything that can be solved by the *interpreter* happens 
on the compilation phase and the remainder is the code that has to be 
solved at *runtime*.

At the end, most functions, even looking more complex than the C variant, 
ends up with compact code, surpassing any of the available C libraries. 
The final result reduces to straight and simple hardware registers 
writes. 

A brilliant example is the `Clocks::AnyPllVco<>` template class (have a 
look into `clocks.h` file). Using this template class one is able to 
use a brute force algorithm to detect the PLL configuration that best 
approximates a desired output frequency.

> This brute-force method seems to be an overkill in a simple PLL like 
> the one featured in the STM32F103, but if you look into the STM32L432 
> data-sheet you will surely need a complex solution to configure it.

The compiler produces a single static record of the `PllFraction` 
structure, which is bound to a *weak symbol* which means all instances 
produced during each single translation unit (i.e. each single `.cpp` 
file is compiled independently), is merged into a single record in the 
linking phase and discarded if not referenced.  
Note that reference to the constants within this structure are solved at 
compile time as *literal constants* and will not necessarily cause a 
reference in the binary code. In other words, most of the time the static 
instances that are created, will probably be discarded by the linker.


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


-------

### Debug Hint: How to inspect the result of PllFraction

Working with those `constexpr` automated algorithms may add complexity 
when developing a new firmware, as it really contains a taste of 
*black magic*. 

So a simple solution is shown on the following code snippet:

```cpp
namespace Bmt
{

extern "C" const Clocks::PllFraction *g_Test;

void Test()
{
    using namespace Bmt::Clocks;
    typedef AnyHse<> HSE;
    typedef PllVcoAuto<> Calculator;	// STM32L432 only
    typedef AnyPll<HSE, 11500000UL, Calculator> PLL;
    PLL::Init();
    // This adds a reference that linker cannot discard the 
    // resulting PllFraction record
    g_Test = &PLL::kPllFraction_;
}

} // namespace Bmt
```

When executing the application call the function and follow the contents 
of `g_Test` variable using the debugger. 

Alternatively, compiling the code with assembler output activated, open 
the Assembler output search for the `PllFraction` substring (compiler 
the structure a very long mangled symbol name having the referred 
structure as part of this label).

In my example case, this is what I found on my assembly file:
```
    .weak   _ZN3Bmt6Clocks6AnyPllINS0_6AnyHseILm8000000ELb0ELb1EEELm11500000ENS0_10PllVcoAutoILNS_5Power4ModeE0EEELb1ELm0ELm0ELm5EE13kPllFraction_E
    .section .rodata._ZN3Bmt6Clocks6AnyPllINS0_6AnyHseILm8000000ELb0ELb1EEELm11500000ENS0_10PllVcoAutoILNS_5Power4ModeE0EEELb1ELm0ELm0ELm5EE13kPllFraction_E,"aG",%progbits,_ZN3Bmt6Clocks6AnyPllINS0_6AnyHseILm8000000ELb0ELb1EEELm11500000ENS0_10PllVcoAutoILNS_5Power4ModeE0EEELb1ELm0ELm0ELm5EE13kPllFraction_E,comdat
    .align 2
    .type   _ZN3Bmt6Clocks6AnyPllINS0_6AnyHseILm8000000ELb0ELb1EEELm11500000ENS0_10PllVcoAutoILNS_5Power4ModeE0EEELb1ELm0ELm0ELm5EE13kPllFraction_E, %object
    .size   _ZN3Bmt6Clocks6AnyPllINS0_6AnyHseILm8000000ELb0ELb1EEELm11500000ENS0_10PllVcoAutoILNS_5Power4ModeE0EEELb1ELm0ELm0ELm5EE13kPllFraction_E, 32
_ZN3Bmt6Clocks6AnyPllINS0_6AnyHseILm8000000ELb0ELb1EEELm11500000ENS0_10PllVcoAutoILNS_5Power4ModeE0EEELb1ELm0ELm0ELm5EE13kPllFraction_E:
    .word   8000000
    .word   92000000
    .word   8000000
    .word   96000000
    .word   12
    .word   1
    .word   8
    .word   4
```

Putting data into a nice table (see data-sheet for terms):

| Field  | Value    | Description                                        |
|--------|----------|----------------------------------------------------|
| `clk`  | 8000000  | Clock value sourcing the PLL (obtained from `HSE`) |
| `fq`   | 92000000 | Desired PLL frequency (`92 MHz / 8 = 11.5 MHz`)    |
| `fin`  | 8000000  | PLL input frequency (after applying '/M' divisor)  |
| `fout` | 96000000 | Effective PLL frequency (HW limits applied)        |
| `n`    | 12       | Selected multiplier 'xN'                           |
| `m`    | 1        | Selected input divider '/M'                        |
| `r`    | 8        | Selected output divider '/R' for SYSCLK            |
| `err`  | 4        | Approximate frequency error (4%)                   |


-------

# Documentation for Library Name-spaces

## Namespace `clocks`

This namespace is responsible for configuring the clock tree of the STM32 
micro-controller.

[See details here](colocks.md).


# SysTick Classes

These classes provides access to the System Tick timer of the MCU.
