# DtrigJtag — Double-Trigger SPI+TIM1 JTAG Driver

## Overview

The `dtrig` JTAG driver is an alternative to the `TIM_DMA_SLOW` and `SPI_DMA` drivers.
It uses SPI1 for JTCK/JTDI/JTDO and TIM1 for per-bit TMS DMA triggering, exploiting a
hardware shortcut present on the ST-Link V2 clone PCB: PA5 (SPI1 SCK) and PB13 (JTCK
GPIO) are physically shorted.  Because SPI1 SCK directly drives JTCK, no separate clock
source is needed.

**Key improvement over `GeneratorSTLinkPWM`:**
- 3 DMA channels per bit → 1 DMA per bit (TMS) + 1 DMA per 8 bits (SPI byte)
- Significantly lower AHB bus pressure → higher sustainable clock rates

---

## Hardware Requirements

### Pin Assignments (STLinkV2 / `target.stlinv2/platform.h`)

| Signal | GPIO | Alt Function | Notes |
|--------|------|--------------|-------|
| JTCK   | PA5  | SPI1_SCK     | Also wired to PB13 on PCB |
| JTDI   | PA7  | SPI1_MOSI    | Also doubles as JTCLK during Run-Test/Idle |
| JTDO   | PA6  | SPI1_MISO    | |
| JTMS   | PB14 | GPIO output  | Driven by TIM1 DMA → BSRR |
| PB13   | —    | Pull-up input | Released to avoid contention with PA5 |

### Why PB13 must be released

When PA5 is in SPI AF mode (push-pull output), PB13 is also electrically connected to the
same trace.  If PB13 were configured as a GPIO push-pull output, both pins would fight
during every SPI SCK low phase.  `JtagSpiOn::Enable()` sets PB13 to pull-up input to
release the line.

### DMA Channel Map (STM32F103)

| Peripheral | DMA Channel | Used for |
|------------|-------------|---------|
| SPI1_RX    | DMA1_CH2    | JTDO receive bytes |
| SPI1_TX    | DMA1_CH3    | JTDI transmit bytes |
| TIM1_CH3   | DMA1_CH6    | TMS BSRR writes (one per JTCK cycle) |
| TIM2_UP    | DMA1_CH2    | JtclkWaveGen (used only during `OnFlashTclk`) |
| TIM3_UP    | DMA1_CH3    | JtclkWaveGen (used only during `OnFlashTclk`) |

TIM1_CH3 (`kWaveJtagTms = Channel::k3`) was chosen for TMS DMA specifically because
DMA1_CH6 has no conflict with either SPI1 DMA channel.  TIM1_CH1 and TIM1_CH2 would
have conflicted.

---

## Architecture

### SPI Configuration

SPI1 is configured in **Mode 3** (CPOL=1, CPHA=1):
- SCK idles **high**
- Data is sampled on the **falling edge** of SCK

Baud rate = APB2 / divisor, one of five grades (0.5625 MHz → 9 MHz).  Each byte
transferred is 8 JTCK cycles.

### TIM1 Configuration

TIM1 is the only advanced timer and is required because the dtrig driver uses its
**repetition counter** to count the exact number of JTCK cycles needed for a scan.

- Input clock: `8 × kFreq` (derived from APB2)
- ARR = 7 → one TIM1 period = one JTCK cycle
- Repetition counter = `kTotalClocks - 1`
- Mode: single-shot (stops automatically after the required number of cycles)
- CH? compare event at count 6 triggers the TMS DMA write

### Synchronisation

Both SPI and TIM1 derive from APB2 at 72 MHz.  SPI baud = APB2 / N, TIM1 period = N
APB2 cycles.  They are started together inside a **critical section** (interrupts
disabled):

```
TIM1->CNT = cnt_offset   // preset phase (tuned per speed grade)
CycleTimer::CounterResume()  // start TIM1 → TMS DMA starts firing at count 6
DmaTms::Enable()             // arm TMS DMA
SpiTxDma::Enable()           // triggers first SPI byte → SPI starts clocking
```

Once running they are phase-locked and never drift.  `cnt_offset` shifts TIM1 forward so
that the first TMS DMA event fires at the correct moment relative to SPI bit edges.

### Signal timing (8-count TIM1 cycle)

```
Count:  0   1   2   3   4   5   6   7   (reload → 0)

JTCK    ─────────────────────────┐          ┌─── …
(SPI SCK)                        │          │
                                 └──────────┘
TMS DMA                                ↑  fires at count 6
                                   (2 counts before SPI rising edge)
MOSI    <stable data bit, set before rising edge>
```

---

## Template: `DtrigJtag<>`

Defined in `Firmware.shared/util/DtrigJtag.h`.

### Template Parameters

| Parameter   | Type                  | Description |
|-------------|-----------------------|-------------|
| `SysClk`    | type                  | System clock (provides APB2 frequency) |
| `kTim`      | `Timer::Unit`         | TIM1 unit (must have repetition counter) |
| `kTms`      | `Timer::Channel`      | TIM1 channel for TMS DMA trigger |
| `SpiDevice` | type                  | SPI peripheral type |
| `kFreq`     | `uint32_t`            | JTAG clock frequency (= SPI baud rate) |
| `kScan`     | `WaveJtag::Scan`      | DR, IR, or GoIdle |
| `kNumBits`  | `WaveJtag::NumBits`   | 8 / 16 / 20 / 32 bits, or GoIdle sentinel |

### Derived Constants

| Constant         | Formula                              | Description |
|------------------|--------------------------------------|-------------|
| `kBitCount`      | `5 + kNumBits + kScan` (or 8)        | JTCK cycles for the selected scan |
| `kSpiBytes`      | `(kBitCount + 7) / 8`               | SPI bytes (whole, may include padding) |
| `kTotalClocks`   | `kSpiBytes × 8`                     | Actual JTCK cycles clocked |

### Compile-time Assertions

- `kTotalClocks ≤ 40`: TMS buffer (`JtagDev::read_buf_`) must fit all words
- `kSpiBytes ≤ 8`: SPI TX/RX byte buffers must fit
- `DmaTms::kChan_ ≠ SpiTxDma::kChan_` and `≠ SpiRxDma::kChan_`: DMA conflicts detected
- `CycleTimer::HasRepetitionCounter()`: ensures advanced timer
- Advanced timer assertion triggered at `Init()` call site for better error locality

### Key Static Methods

| Method | Description |
|--------|-------------|
| `Init()` | One-time hardware init: TIM1 PSC, SPI baud, DMA channels.  Call in `OnOpen()`. |
| `SetupDma()` | Re-arm DMA channels after they were released (e.g. after `JtclkWaveGen`) |
| `ReleaseDma()` | Disable DMA channels so other peripherals can use them |
| `RenderTransaction(tdi_bytes, tms_buf, tclk_high, data_out)` | Build SPI TX and TMS BSRR buffers for one scan |
| `DoGoIdle(tdi_bytes, tms_buf, cnt_offset)` | TAP reset: 6× TMS=1 then TMS=0 |
| `Start(tdi, tdo, tms, cnt_offset)` | Launch the pre-rendered transaction |
| `Wait()` | Wait for TIM1 single-shot to complete, then clean up DMA |
| `GetResult(tdo_bytes)` | Decode received payload from SPI RX buffer |

---

## Implementation: `JtagDev.dtrig.cpp`

Located at `Firmware.shared/drivers/JtagDev.dtrig.cpp`.  Compiled only when
`OPT_INCLUDE_JTAG_DTRIG_` is defined (set automatically in `stdproj.h` when
`OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG`).

### Type Aliases

```cpp
// One SPI template per speed grade
typedef SpiJtagDevType<JTCK_Speed_1> SpiJtagDev_1;   // 0.5625 MHz
typedef SpiJtagDevType<JTCK_Speed_2> SpiJtagDev_2t;  // 1.125 MHz
typedef SpiJtagDevType<JTCK_Speed_3> SpiJtagDev_3t;  // 2.25 MHz
typedef SpiJtagDevType<JTCK_Speed_4> SpiJtagDev_4t;  // 4.5 MHz
typedef SpiJtagDevType<JTCK_Speed_5> SpiJtagDev_5t;  // 9 MHz

// Scan types (all use SpiJtagDev_1 as template param; actual baud set by OnOpen)
using DtrigIr8  = DtrigJtag<..., Scan::kIR, NumBits::k8>;
using DtrigDr8  = DtrigJtag<..., Scan::kDR, NumBits::k8>;
using DtrigDr16 = DtrigJtag<..., Scan::kDR, NumBits::k16>;
using DtrigDr20 = DtrigJtag<..., Scan::kDR, NumBits::k20>;
using DtrigDr32 = DtrigJtag<..., Scan::kDR, NumBits::k32>;

// Per-grade init aliases (only Init() is called on these)
using DtrigInit_1 = DtrigJtag<..., SpiJtagDev_1,  JTCK_Speed_1, ...>;
// ... through DtrigInit_5
```

### Static Storage

| Variable | Type | Purpose |
|----------|------|---------|
| `JtagDev::read_buf_[]` | `uint32_t[kPingPongBufSize_]` | Shared TMS BSRR buffer (one word per JTCK cycle) |
| `s_spi_tx[]` | `uint8_t[DtrigDr32::kSpiBytes]` | SPI transmit buffer (max 5 bytes for DR32) |
| `s_spi_rx[]` | `uint8_t[DtrigDr32::kSpiBytes]` | SPI receive buffer |
| `s_cnt_offset` | `uint16_t` | Active-grade TIM1 CNT preset, updated by each `OnOpen()` |
| `s_bsrr_table[]` | `const uint32_t[2]` | BSRR values for JTCLK GPIO toggle (used by JtclkWaveGen) |

### Speed Grades and `OnOpen()`

Each `JtagDev_N::OnOpen()` sets the SPI baud rate and TIM1 prescaler for its grade
by calling `DtrigInit_N::Init()`, and stores the corresponding `kDtrigCntOffset_N`
into `s_cnt_offset`.

All five `kDtrigCntOffset_N` constants are defined in `target.stlinv2/platform.h` and
default to 0.  They must be calibrated per speed grade with a logic analyzer.

### TCLK Operations

In dtrig mode, SPI SCK (= JTCK) cannot be suppressed independently of the SPI peripheral.
Sending a byte causes 8 JTCK pulses.  This is harmless during TCLK operations because
the JTAG TAP is in **Run-Test/Idle** (TMS=0 is the last state from the previous scan),
and TMS=0 on every JTCK edge simply keeps the TAP in Run-Test/Idle.

| Method | SPI byte sent | MOSI after | Meaning |
|--------|---------------|------------|---------|
| `OnSetTclk()` | `0xFF` | HIGH | JTCLK held high |
| `OnClearTclk()` | `0x00` | LOW | JTCLK held low |
| `OnPulseTclk()` | `0xF0` | HIGH | Low→High rising edge |
| `OnPulseTclkN()` | `0x0F` | HIGH | High→Low→High sequence |

### Flash TCLK (`OnFlashTclk`)

MSP430 flash operations require ~450 kHz JTCLK for thousands of cycles.  SPI is too
fast and the byte-at-a-time overhead is too high.  `JtclkWaveGen` (TIM2 + TIM3 + DMA)
generates the waveform directly.

**Problem**: TIM2_UP → DMA1_CH2 and TIM3_UP → DMA1_CH3 are the same channels used by
SPI1_RX and SPI1_TX respectively.

**Solution**:

```
DtrigDr32::ReleaseDma()   // disable DMA1_CH2, CH3, CH6
JTCLK::SetupPinMode()     // PA7 → GPIO output (not SPI AF)
JtclkWaveGen::RunEx(min_pulses)
JTCLK::SetHigh()           // idle JTCLK high after wave stops
JTCLK_SPI::SetupPinMode() // PA7 → SPI1_MOSI AF
DtrigDr32::SetupDma()     // re-arm all three DMA channels
```

### Scan Operation Pattern

All IR/DR scan methods follow the same four-step pattern:

```cpp
uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
    typedef DtrigIr8 R;
    R::RenderTransaction(s_spi_tx, read_buf_, JTCLK::IsHigh(), instruction);
    R::Start(s_spi_tx, s_spi_rx, read_buf_, s_cnt_offset);
    R::Wait();
    return static_cast<uint8_t>(R::GetResult(s_spi_rx));
}
```

### Buffer Layout (example: 8-bit IR scan)

```
kBitCount  = 5 + 8 + 1 (IR) = 14 bits
kSpiBytes  = ceil(14/8) = 2 bytes
kTotalClocks = 16 bits

Bit stream (MSB first):
  0        : Select-DR (TMS=1, TDI=tclk)
  1        : Select-IR (TMS=1, TDI=tclk)    [IR only]
  2        : Capture   (TMS=0, TDI=tclk)
  3        : Shift 1st (TMS=0, TDI=tclk)    [first of 2 Capture clocks]
  4..10    : Shift bits 7..1 (TMS=0, TDI=data MSB first)
  11       : Exit1 (TMS=1, TDI=data bit 0)
  12       : Update (TMS=1, TDI=tclk)
  13..15   : Run-Test/Idle + padding (TMS=0, TDI=tclk)
```

`tdi_bytes` is pre-filled with `tclk_high ? 0xFF : 0x00` (preserving JTCLK level for
all non-data bits), then individual data bits are overwritten by `SetTdiBit()`.

---

## Configuration

### Enabling the Driver

In `target.stlinv2/platform.h`, change:

```cpp
#define OPT_JTAG_IMPLEMENTATION  OPT_JTAG_IMPL_TIM_DMA_SLOW
```

to:

```cpp
#define OPT_JTAG_IMPLEMENTATION  OPT_JTAG_IMPL_DTRIG
```

### Per-grade CNT Offset Calibration

Defined in `target.stlinv2/platform.h`:

```cpp
static constexpr uint16_t kDtrigCntOffset_1 = 0;  // 0.5625 MHz — tune with LA
static constexpr uint16_t kDtrigCntOffset_2 = 0;  // 1.125 MHz
static constexpr uint16_t kDtrigCntOffset_3 = 0;  // 2.25 MHz
static constexpr uint16_t kDtrigCntOffset_4 = 0;  // 4.5 MHz
static constexpr uint16_t kDtrigCntOffset_5 = 0;  // 9 MHz
```

With a logic analyzer: observe JTMS transitions relative to JTCK rising edges.  TMS must
be stable before each JTCK rising edge.  Increment the offset for the target grade until
TMS setup time is met.  Values are small (0–7 for the 8-count TIM1 period).

---

## Pending Work

- **Add `JtagDev.dtrig.cpp` to Visual Studio project**: the file must be added to
  `target.stlinv2/target.stlinv2.vcxproj` (ClCompile section) and
  `target.stlinv2/target.stlinv2.vcxproj.filters` (alongside other JtagDev driver files).
- **CNT offset calibration**: all five grades default to 0 and need measurement with a
  logic analyzer once hardware is running.
- **Build and integration test**: compile the STLinkV2 target with dtrig enabled and
  verify correct JTAG communication with an MSP430 target.

---

## File Index

| File | Role |
|------|------|
| `Firmware.shared/util/DtrigJtag.h` | Template class: render, start, wait, decode |
| `Firmware.shared/drivers/JtagDev.dtrig.cpp` | `JtagDev` virtual method implementations |
| `target.stlinv2/platform.h` | Pin aliases, DMA channel assignments, speed constants, CNT offsets |
| `Firmware.shared/platform-defs.h` | `OPT_JTAG_IMPL_DTRIG = 5` |
| `Firmware.shared/stdproj.h` | `OPT_INCLUDE_JTAG_DTRIG_` guard, speed-select condition |
| `Firmware.shared/util/WaveJtag.h` | `Scan` and `NumBits` enum classes |
| `Firmware.shared/util/TimDmaWave.h` | `TimDmaWav<>` — JtclkWaveGen for flash TCLK |
