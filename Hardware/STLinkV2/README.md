# STLink V2

This is a picture of a typical STLinkV2:

[<img src="https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTokTmzBSaADidUkIWzn1jOEZ21H9Y7c91xpA&usqp=CAU">](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTokTmzBSaADidUkIWzn1jOEZ21H9Y7c91xpA&usqp=CAU)


## Introduction 

This project is a redesign of the original STLinkV2 project, using the 
same components of the **Glossy MSP430**. The reasons for this redesign 
are:
- The original STLinkV2 cannot be found anymore, as it was replaced by 
a **Version 3** using a much faster controller
- Clones have no support for variable target voltages.
- Also no support for a UART connection, which in this version, borrows 
the **SWIM** connector for that purpose.

> **A note on the MCU.** The original **STM32F103CBT6** is the intended
> part. When this board was laid out in 2022 the genuine STM32 was
> impossible to source, so the first batch (5 PCBs) was populated with the
> **Geehy APM32F103CB** instead. The STM32 build is still on the to-do
> list. The Geehy substitute is *not* a requirement — but it has turned
> out to be useful: it surfaces the kinds of problems people will hit when
> running this firmware on a random clone. The clone gamut is essentially
> unpredictable, so consider any clone-specific workaround (see the SWO
> note in the GPIO table) a hint, not an exhaustive list.


## Use Cases 

Two specific use cases are planned: 
- Install the [Black Magic Probe firmware](https://github.com/blackmagic-debug/blackmagic) 
on them
- Develop the port of **Glossy MSP430** firmware to these devices.


## Important Note About Variable Target Voltage on a STLinkV2

Although support for variable voltage is present on an Original STLinkV2 
it is not possible to use this feature outside the STM32 world. The 
reason is that STM32 chips have 5V-tolerant pins on the JTAG bus and the 
original STM design considers this as true. All outputs are on a 3.3V realm, 
regardless of the voltage applied on pin 1/2:

![20-PIN-con.jpg](https://i0.wp.com/www.playembedded.org/blog/wp-content/uploads/2016/04/20-PIN-con.jpg)

Voltage translators installed on this design are only used to translate 
input signals, since it is not possible to identify logic true value on a 
1.8V link.

So even if I code a **Glossy MSP430** firmware port for this platform, it 
is not possible to use a STLinkV2 with a MSP430 using 3V or lower supply 
voltage, because the MSP430 MCU will suffer over-voltage on the debug 
pins. 


## A Brief Project Presentation

This is the 3D model:

![STLinkV2-fs8.png](images/STLinkV2-fs8.png)

The board can directly replace clones you've bought in AliExpress or 
Amazon. 

For the development of the **Glossy MSP430** firmware a standard ARM 
10-pin connector was added:

![STLinkV2-JTAG-fs8.png](images/STLinkV2-JTAG-fs8.png)

But for a regular SWD access you can also have access using regular 
jumper cables:

![STLinkV2-SWD-fs8.png](images/STLinkV2-SWD-fs8.png)

And the SWIM connector was modified to implement a UART port:

![STLinkV2-UART-fs8.png](images/STLinkV2-UART-fs8.png)

Note that the VCC pin is a power input pin, which is also connected to 
pins 1 and 2 of the JTAG connector. Keep in mind that they should belong 
to the same power supply realm.


## Signal / GPIO Mapping

The firmware port lives in [`target.stlinv2/platform.h`](../../target.stlinv2/platform.h).
The table below is the authoritative mapping of MCU pins to probe signals
and the peripheral that drives each one. Pin assignments are dictated by
the original STLinkV2 routing — the firmware adapts to the board, not the
other way around.

### Target debug bus (MSP430 side)

| MCU pin | Signal | Dir | Peripheral / mode | Notes |
|---------|--------|:---:|-------------------|-------|
| PA0  | VREF/2        | in  | ADC (pending)            | Target voltage sense (half of VCC). |
| PA5  | JTCK / SBWCLK | out | SPI1_SCK / GPIO          | DTRIG SPI clock; **shorted to PB13** on the PCB. Also the SBW clock. |
| PA6  | JTDO          | in  | SPI1_MISO / GPIO         | The only signal behind a level converter (input translation only). |
| PA7  | JTDI / JTCLK  | out | SPI1_MOSI / GPIO / TIM3_CH2 | MOSI carries JTDI; doubles as JTCLK during Run-Test/Idle. |
| PB0  | /RST          | out | GPIO                     | MSP430 reset. |
| PB1  | TRST / TEST   | out | GPIO                     | Retargets to TEST on MSP430; adapter board needs a strong pull-down. |
| PB14 | JTMS / SBWDIO | out | TIM1_CH2N / GPIO         | TMS via complementary timer output; also the bidirectional SBW data line. |
| PB12 | SWD_IN        | in  | GPIO                     | Passive level-translated echo of PB14 (= JTAG-20 pin 7). Reads the *actual* bus level whether the probe drives or releases PB14 — the SBW read-back path (see below). |

### UART (borrowed SWIM connector)

| MCU pin | Signal | Dir | Peripheral | Notes |
|---------|--------|:---:|------------|-------|
| PA2 | TXD | out | USART2_TX | GDB serial until USB CDC lands. |
| PA3 | RXD | in  | USART2_RX | GDB serial until USB CDC lands. |

### Probe-internal / host side

| MCU pin | Signal | Notes |
|---------|--------|-------|
| PA9  | LED      | High = red, Low = green, floating = off. |
| PA11 | USB−     | |
| PA12 | USB+     | |
| PA13 | TMS/SWDIO | Host SWD — for debugging the probe's own STM32. |
| PA14 | TCK/SWCLK | Host SWD. |
| PA15 | TDI       | Host JTAG. |
| PB3  | TRACESWO | **Firmware self-trace (SWO)** — the probe's own debug trace output, *not* an MSP430 signal. On clone MCUs such as the Geehy APM32F103CB this pin must be driven as a push-pull output (`OPT_GEEGY_APM32F103CB`) for trace to work reliably; a genuine STM32 takes over the pin automatically. |

## Spy-Bi-Wire wiring

SBW is currently compiled out on this target (`OPT_SBW_IMPLEMENTATION =
OPT_SBW_IMPL_OFF`), but the pin choice is already fixed and **must follow
the [STLink-Adapter](../STLink-Adapter/README.md) wiring**, which maps the
SBW lines onto the ARM SWD pinout rather than the TI MSP-FET layout:

| SBW line | This probe | MCU pin |
|----------|:----------:|:-------:|
| DIO | TMS (JTMS) | PB14 |
| CLK | TCK (JTCK) | PA5  |

⚠ Because of this remapping, **the JTAG-14 connector cannot be used for
SBW** — always use the dedicated SBW connector on the adapter. See the
debugger comparison table in the adapter README.

### Bus direction (read-back) on this board

SBW is half-duplex: the single `SBWDIO` line carries the probe's drive bits
and then must be released so the target can answer in the same frame. On
the other (newer) targets a dedicated **PA9 hardware mux** is planned, so
the direction bit can be folded into the DMA stream and the turnaround is
fully autonomous. **That mux does not exist on the STLinkV2 layout** — here
direction has to be handled the hard way, in software.

What the original STM design gives us instead is a passive read-back path:

- **PB14 (TMS_SWDO)** is echoed to **PB12 (SWD_IN)** through the voltage
  level converter. PB12 is a passive copy of the TMS line on **JTAG-20 pin 7**.
- While PB14 actively drives, the echo reflects the driven **3.3 V** level.
- When PB14 goes tri-state, the line floats into the **Target-VCC** domain
  (held by the SBW pull-up); the translator brings that back to a clean
  3.3 V logic level. So **PB12 is always an accurate read of the real bus
  level**, regardless of who is currently driving.

That leaves two practical strategies for the turnaround, both software-paced
(no DMA-atomic direction switch like the PA9 targets):

1. **Tri-state PB14 and sample PB12** during the target's turn.
2. **Keep everything on PB14**, flipping its GPIO direction in place.

Which is faster on this board is still open. Either way the over-voltage
caveat stands: the read-back path only helps *inputs* — PB14 still drives a
3.3 V output, so a sub-3 V MSP430 is over-driven regardless. This is why the
variable-voltage feature cannot be used with MSP430 targets below 3 V.

## Other features

This design also features some nice adds that are typically not present 
on a clone:
- ESD protection for the USB bus (**U2**)
- EMI protection for the USB bus (**C4** and **C5**)
- ESD protection for the JTAG bus (**U4** and **U5**)
- Voltage translation chips (**U6** and **U7**)
- 200 mA Fuse protection for the 3.3V output, when using the emulator to 
supply voltage to a target board (JTAG pin 19).
- A Micro-USB connector instead of Mini-USB.

