# Glossy-MSP430 Development Hardware Jiga based On BluePill+G431

This prototype board replaces the [STM32 Bluepill/BlackPill](../BlackPill-BMP/README.md) proto-board since this design had issues related to SBW protocol.

This is a non-scale prototype of the expected hardware design, which provides generous access to signal pins, enabling a better debug experience.

Meanwhile STM32G431 in BluePill-like packaging is now available, making the STM32F411 an obsolete platform.

This prototype board offers the following advantages:
- Dual board support (either BluePill or G431-BluePill-style board).
- Unbeatable cost/performance combo.
- Lots of documentation and community support for STM32F103 and a growing support for STM32G431.
- Higher clock rates when compared to most *Arduino-like* alternatives, with a comparable budget.
- Lots of interface pins.
- Possibility to drive JTAG at rates above 4 MHz, supported by the MSP430 JTAG interface.
- Integrated USB port to implement a VCP for the GDB connection.
- A legacy serial port for GDB connection during debug session, avoiding "always ready" requirement of a running USB connection (breakpoint pause for inspection would cause an USB connection loss).
- Programmable target voltage (using a PWM pin).
- Voltage level translator, to drive MSP boards with less than 3.3V.
- A Logic analyzer connection.
- Access to most circuit elements for signal monitoring, even for most SMD parts.


# Naming Conventions

In this design we introduced another pin naming convention for the schematics, centered on the JTAG bus, which allows us to keep the usual signal's JTAG name in evidence, regardless the circuit stage, between host MCU and connector port. 

Here we summarize a typical signal path, which repeat eight times along the circuit:
- Path labeled with `h.XXX`:
  - In this domain the voltages are 3.3V fix.
  - Host MCU is the controller of writes or reads for any signal.
  - This host signal binds to a voltage level converter, so lower voltages are supported.
- Path labeled with `r.XXX`:
  - The other side of a voltage level converter works on the target voltage domain (a reference received by the JTAG connector or specified by software).
  - This lines runs to the pull-up or pull-down resistor (depends on the pin) and to the damping resistors.
- Path labeled with `t.XXX`:
  - This path still belongs to the voltage domain of the target
  - This path binds the damping resistor to the Target interface connector (JTAG or Serial connector)

  ## Table of the Remarkable Signal Paths

| Signal Name |    A.K.A.   | A.K.A.(ARM) |   Host MCU  |  Damping R  |  Connector  |
|:-----------:|:-----------:|:-----------:|:-----------:|:-----------:|:-----------:|
|     TDO     |    JTDO     |    TTDO     |    h.TDO    |    r.TDO    |    t.TDO    |
|     TDI     |    JTDI     |    TTDI     |    h.TDI    |    r.TDI    |    t.TDI    |
|     TMS     |    JTMS     |    TTMS     |    h.TMS    |    r.TMS    |    t.TMS    |
|     TCK     |    JTCK     |    TTCK     |    h.TCK    |    r.TCK    |    t.TCK    |
|     RST     |    JRST     |    TRST     |    h.RST    |    r.RST    |    t.RST    |
|     TEST    |    JTEST    |    TTEST    |    h.TEST   |    r.TEST   |    t.TEST   |
|    TXD (*)  |    JTXD     |    TTXD     |    h.TXD    |    r.TXD    |    t.TXD    |
|    RXD (*)  |    JRXD     |    TRXD     |    h.RXD    |    r.RXD    |    t.RXD    |
|   SBWCLK    |   SBWTCK    |             |  h.SBWCLK   |  r.SBWCLK   |  t.SBWCLK   |
|   SBWDIO    |   SBWTDIO   |             |  h.SBWDI/DO |  r.SBWDIO   |  t.SBWDIO   |

(*) Regular low voltage USART signals.

> The SBW (Spy-Bi-Wire) paths reuse the JTAG host pins (SBWCLK on TCK, SBWDIO on the TDO/TDI pair) but carry their own schematic labels through the level-converter stage. `h.SBWDI`/`h.SBWDO` are the separate host-side input/output legs that merge into the single bidirectional `r./t.SBWDIO` line on the target side.

> MSP430 Literature also refers the **TCLK** (Target Clock) signal (AKA **JTCLK**), but this signal shares the **TDI** line, when the JTAG state machine is on a known steady state, the clock signal can *bypass* the normal TDI path.


## Configuration summary

| Function | Peripheral | Pins |
|----------|------------|------|
| BUS CTRL | GPIO | PB10 (DIS_RST), PB11 (DIS_TCK), PB12 (DIS_JTAG), PB13 (DIS_COM), PA10 (SBW_RD) |
| GDB serial (host) | USART2 | PA2 (TX), PA3 (RX) / Disabled in VCP builds |
| Target UART | USART1 | PB6 (TX→JRXD), PB7 (RX→TXD) / Only in VCP builds |
| JTAG data/clock | SPI1 | PA5 (TCK), PA6 (TDO), PA7 (TDI) |
| JTAG TMS | TIM1_CH2 | PA9 |
| JTAG CTRL | GPIO | PA0 (TEST), PA1 (RST) |
| TCLK generation | SPI (`OPT_JTCLK_IMPL_SPI`) | on SPI1 pins |
| SBW data/clock | t.b.d. | PA5 (TCK), PA6 (TDO), PA7 (TDI) |
| Target voltage (DAC) | DAC1_OUT1 | PA4 |
| Target voltage (PWM) | TIM4_CH4 | PB9 |
| Target voltage read (ADC) | ADC (PB0) | F103: ADC12_IN8 / G431: ADC1_IN15
| STM32 debug | SWD | PA13 (SWDIO), PA14 (SWCLK) |
| ARM trace | TRACESWO | PB3 |
| VCP | USB | PA11 (USBM), PA12 (USBP) |

## PORTA

| Pin  | Sch Label | Function     | Notes |
|------|-----------|--------------|-------|
| PA0  | h.TEST | TEST            | bit-bang; MSP430 TEST (JTAG-14 pin 8) |
| PA1  | h.RST  | RST             | bit-bang; MSP430 RST/NMI (JTAG-14 pin 11) |
| PA2  | GDB_TX | USART2_TX       | GDB serial TX (disabled in VCP mode) |
| PA3  | GDB_RX | USART2_RX       | GDB serial RX (disabled in VCP mode) |
| PA4  | GEN_VT | DAC1_OUT1 (DAC_VT) | target voltage (JVCC), DAC-generated |
| PA5  | h.TCK  | TCK / SPI1_SCK  | JTAG clock  (JTAG-14 pin 7) |
| PA6  | h.TDO  | TDO / SPI1_MISO | JTAG data out (from target) (JTAG-14 pin 1) |
| PA7  | h.TDI  | TDI / SPI1_MOSI | JTAG data in (to target) (JTAG-14 pin 3) |
| PA8  | h.TCK  | TCK (bridged)   | hard-wired to the h.TCK net (shared with PA5) on the PCB; serves as TIM1_CH1 external-clock input for the legacy SPI path, idle in DTRIG mode (TIM1 clocks internally) |
| PA9  | h.TMS  | TMS / TIM1_CH2  | timer-driven during JTAG frames |
| PA10 | SBW_RD | GPIO / TIM1_CH3 | SBW direction control; currently GPIO bit-bang. Pin is TIM1_CH3-capable (also TIM2_CH4) — reserved for hardware-timed SBWO direction toggle in the planned SPI-stream SBW driver |
| PA11 |       | USB DM           | internally wired in BluePill; USB VCP (t.b.d.) |
| PA12 |       | USB DP           | internally wired in BluePill; USB VCP (t.b.d.) |
| PA13 |       | SWDIO            | internally wired in BluePill; STM32 debug — left unchanged |
| PA14 |       | SWCLK            | internally wired in BluePill; STM32 debug — left unchanged |
| PA15 | LEDS  | LED control      | Dual LED: tri-state for OFF; high: Red, low: Green  |

## PORTB

| Pin  | Sch Label | Function       | Notes |
|------|----------|-----------------|-------|
| PB0  |  V2REF   | Vref (pending)  | 0.5 x DC(JTAG-14 pin 4) |
| PB1  |   PB1    | — (unused)      | breakout pin |
| PB2  |          | BOOT1           | internally wired in BluePill; STM32 boot select |
| PB3  | TRACESWO | TRACESWO        | ARM SWO trace |
| PB4  |   PB4    | — (unused)      | breakout pin |
| PB5  |   PB5    | — (unused)      | breakout pin |
| PB6  |  h.TXD   | USART1_TX       | target UART → JRXD |
| PB7  |  h.RXD   | USART1_RX       | target UART → JTXD |
| PB8  |   PB8    | BOOT0           | **keep floating — do NOT add a pull-up** (would force DFU at reset) / breakout pin |
| PB9  | GEN_VT   | TIM4_CH4 | target-voltage PWM regulator |
| PB10 | DIS_RST  | GPIO | buffer/output enable (active-low), init Hi-Z |
| PB11 | DIS_TCK  | GPIO | buffer/output enable (active-low), init Hi-Z |
| PB12 | DIS_JTAG | GPIO | buffer/output enable (active-low), init Hi-Z |
| PB13 | DIS_COM  | GPIO | buffer/output enable (active-low), init Hi-Z |
| PB14 |  PB14    | — (unused) | breakout pin |
| PB15 |  PB15    | — (unused) | breakout pin |

## Notes & cautions & porting issues

- **ENAxxx** pins were renamed to **DISxxx**, to match the negative logic of the buffer.
- They were also resorted and another grouping logic was established to keep unused lines in tri-state.
- **PB8 is BOOT0** on the G431. The target-voltage PWM was deliberately placed on **PB9/TIM4_CH4** (not PB8/TIM4_CH3) so a regulator-input pull-up cannot latch the chip into the system bootloader at reset. Same pin on both BluePill sockets for clean porting.
- **Two target-voltage paths share one net.** `DAC_VT` (PA4, DAC1_OUT1) and the PWM regulator (PB9, TIM4_CH4) are both wired to the **same `GEN_VT` node** on the PCB. Firmware must drive **only one at a time** — driving both simultaneously causes output contention. On G431 the DAC is the more stable choice.
- GDB host serial is on **USART2** (PA2/PA3); the **target** UART passthrough is on **USART1** (PB6/PB7). They are different ports — don't confuse them. The later can only be used when a VCP build is active.
- JTAG/SBW are mutually exclusive at runtime and share **TIM1**; JTCLK uses **SPI**, so TIM3/TIM4 are free (TIM4 hosts the voltage PWM).
- **PWM_VT** renamed to **GEN_VT**, since a PWM is not requirement for voltage generator
- LEDS control a dual LED, similar to STLinkV2: Keep it tri-state to disable both LEDs; 1 for Red color and 0 for green color
- Unused pins are fed to breakout connectors. This could help any debug function or design fixes.

## Bus buffer control

These are the line in contact with the target board:

| Signal | Control Line |
|:------:|:------------:|
|  TDI   |   DIS_JTAG   |
|  TDO   |   *input*    |
|  TMS   |   DIS_JTAG   |
|  TCK   |   DIS_TCK    |
|  RST   |   DIS_RST    |
|  TEST  |   DIS_RST    |
|  TXD   |   DIS_COM    |
|  RXD   |   *input*    |

### States

| Control Line | Standby Mode | COM Open | Acquiring JTAG | Acquiring SBW | JTAG | SBW |
|:------------:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|
|   DIS_RST    |   1   |   X   |   0   |   0   |   0   |   1   |
|   DIS_TCK    |   1   |   X   |   1   |   1   |   0   |   0   |
|   DIS_JTAG   |   1   |   X   |   1   |   1   |   0   |   1   |
|    SBW_RD    |   1   |   X   |   1   |   0   |   1   |  R/W  |
|   DIS_COM    |   1   |   0   |   X   |   X   |   X   |   X   |

> `BusState` enumeration should reflect the states related to JTAB/SBW and code should set them properly depending on the activity it is doing.

About the states:
- **Standby Mode**: Dongle is in high impedance state, usually after a cold boot, or because all offered interfaces are closed
- **COM Open**: USB implemetaton will offer two COM ports: The first is for GDB remote protocol and the second a COM port for the target board. This signal control that bus and is inverse of the "COM port open state". This is independent of the GDB remove protocol.
- **JTAG Bus Acquisition**: The phase where the JTAG entry sequence is done so that JTAG is activated.
- **SBW Bus Acquisition**: The phase where the SBW entry sequence is done so that SBW is activated. Note that this hardware follows TI wiring and although pin names on the target are TEST and TCK, they are driven by another pinout of the JTAG14 connector.
- **JTAG bus active**: All required lines to send JTAG frames.
- **SBW bus active**: All required lines to send SBW frames.

