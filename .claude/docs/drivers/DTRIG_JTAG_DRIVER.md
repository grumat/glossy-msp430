# DtrigJtag — Double-Trigger SPI+TIM1 JTAG Driver

## Overview

`dtrig` is the **only supported** JTAG transport (`OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG`
— the legacy `SPI`/`TIM_DMA` variants were removed, see `.claude/docs/drivers/SPI_VARIANT_REMOVED.md`
and `TIM_VARIANT_REMOVED.md`). It uses SPI1 to generate JTCK/JTDI/JTDO and a single
advanced timer (TIM1) in **PWM mode** to generate JTMS entirely in hardware — no
per-bit DMA is involved for TMS at all. SPI and TIM1 are launched together inside a
critical section so the two peripherals stay phase-locked for the whole frame.

Two toggle events per frame (entry pulse, exit pulse) are enough to reproduce every
JTAG scan the TAP needs; there's no TMS buffer, no per-bit DMA descriptor, and no
GPIO bit-banging once a frame is running.

---

## Hardware Requirements

### Pin Assignments

JTCK/JTDI/JTDO always ride on SPI1 (SCK/MOSI/MISO); only the JTMS timer channel and
its complementary-output polarity differ per board:

| Signal | STLinkV2 (`target.stlinv2`) | BluePill (`target.bluepill`) |
|--------|------|------|
| JTCK | PA5 = SPI1_SCK (also shorted to PB13 on the STLinkV2 PCB) | PA5 = SPI1_SCK |
| JTDI | PA7 = SPI1_MOSI (doubles as JTCLK during Run-Test/Idle) | PA7 = SPI1_MOSI (same) |
| JTDO | PA6 = SPI1_MISO | PA6 = SPI1_MISO |
| JTMS | PB14 = **TIM1_CH2N** (complementary output) | PA9 = **TIM1_CH2** (regular output) |

`kWaveJtagTmsCmpComplementary` (in each `platform.h`) tells `DtrigJtag<>` which of
the two cases it's in: `true` (STLinkV2, CH2N) or `false` (BluePill, CH2, matching
the G431 target too). The two paths produce the same TMS waveform through different
PWM-mode/polarity combinations — see the `DtrigJtag.h` header comment for the
per-path OCREF derivation; on the BluePill's F103 the required `CCxP=1` polarity is
empirically necessary and not fully explained by the reference manual's own
description of CH-vs-CHN inversion.

Dead reference: earlier revisions of this file (and a stale inline example still in
`DtrigJtag.h`'s top comment) mention BluePill JTMS on PA10/TIM1_CH3. That was an
obsolete hardware prototype from before the BluePill→Jiga migration and is never
being revisited — `target.bluepill/platform.h` (PA9/TIM1_CH2, above) is the only
current BluePill pinout and the only one worth reading.

### Why PB13 must be released (STLinkV2 only)

When PA5 is in SPI AF mode (push-pull output), PB13 is also electrically connected
to the same trace. If PB13 were configured as a GPIO push-pull output, both pins
would fight during every SPI SCK low phase. `JtagSpiOn::Enable()` sets PB13 to
pull-up input to release the line.

### DMA Channel Map (STM32F1, fixed silicon request table)

| Peripheral | DMA Channel | Used for |
|------------|-------------|---------|
| SPI1_RX | DMA1_CH2 | JTDO receive bytes |
| SPI1_TX | DMA1_CH3 | JTDI transmit bytes (bytes 1..N−1; byte 0 is preloaded directly in the critical section) |
| TIM1_CH3 (STLinkV2's `kWaveJtagTmsRld1`) | DMA1_CH6 | Single-element write that reloads CCR2 with the exit-pulse compare value |
| TIM1_CH4 (BluePill's `kWaveJtagTmsRld1`) | DMA1_CH4 | Same, chosen instead of CH3 only because BluePill's SPI RX/TX already occupy DMA1_CH2/CH3 |
| TIM1_CH2 / CH2N (`kWaveJtagTms`) | — (no DMA) | JTMS pin, driven purely by the timer's own PWM output — this is the whole point of the design: **TMS costs zero DMA bandwidth** |

There is only **one** reload channel (`kTmsRld1`), not two — it's reused: for a
`kDR` scan its DMA fires once, at the entry→shift boundary, to rewrite CCR2 with
the exit-pulse value ahead of time. (An older draft of this document, and a stray
comment in `JtagDev.dtrig.cpp`, mention a second `kTmsRld2`/CH4 channel for the
exit; that no longer exists in the current `DtrigJtag<>` template signature.)

---

## Architecture

### SPI Configuration

SPI1 runs in **Mode 3** (CPOL=1, CPHA=1): SCK idles high, data is sampled on the
falling edge. Baud rate is one of five grades (0.5625 MHz → 9 MHz, `JTCK_Speed_1..5`).
Each byte transferred is 8 JTCK cycles.

### TIM1: PWM-mode TMS generation

TIM1 runs as a **single-shot**, `kTimerMultiplier_ = 8` ticks per JTCK cycle (so a
timer tick is 1/8 of a bit cell, giving 8 positions of trim resolution per cycle). Two auxiliary constructs make one CCR2 register describe an entire
frame's TMS waveform with no further intervention:

**1. The counter starts near the top of its own period, not at zero.**
`kCntStart_` presets `CNT` to `kTimerPeriod_ − ~2.5 JTCK-cycles-worth-of-ticks`
(`kCntStart_ - cnt_offset`, `cnt_offset` trims this per speed grade). Because CNT is
already close to `ARR`, the timer's *first* lap is a short sliver — that sliver
*is* the entry pulse. When CNT wraps (1st overflow), TIM1 runs one full period
(`kTimerPeriod_` ticks = `3+kNumBits` JTCK cycles) covering Capture+Shift+Exit+Update,
then the repetition counter (`SetupRepetition(kTimerPeriod_, 2)`) stops the timer
after this 2nd overflow. TMS's PWM comparator (`CCR2`) is evaluated fresh every lap,
so the *same* CCR2 value that produced the entry pulse in the short first lap will,
unless changed, reproduce an identically-shaped pulse near the end of the second
(full) lap too.

**2. Entry and exit pulse widths, and why only DR needs a mid-frame CCR2 reload:**

| Scan | Entry pulse (Sel-DR[, Sel-IR]) | Exit pulse (Exit1+Update) | CCR2 reload needed? |
|------|-------------------------------|----------------------------|----|
| DR | **1** JTCK cycle | **2** JTCK cycles | **Yes** — entry width (1) ≠ exit width (2), so the value that made the short first lap 1-cycle-wide must be replaced before the second lap, or the exit pulse would come out 1 cycle wide too. |
| IR | **2** JTCK cycles (Sel-DR + Sel-IR) | **2** JTCK cycles | **No** — entry and exit are already the same width, so the *same* CCR2 value naturally reproduces the correct pulse on the second lap. `Start()` skips the reload DMA entirely when `kScan == kIR` — this is intentional, not a missing feature. |
| GoIdle | 6 JTCK cycles (TAP reset prelude) | — (no exit; single lap only, `SetupRepetition(kTimerPeriod_, 1)`) | N/A |

`kTmsHigh1` (the CCR2 value used for the entry lap) and `tmsHigh2` (the value the
CC3-triggered DMA writes in for `kDR`'s second lap) are computed in `DtrigJtag.h` as
`kTimerPeriod_ − (entry-or-exit-width × kTimerMultiplier_) + 1` — same formula,
different width operand. For IR, `kTmsHigh1` already equals what `tmsHigh2` would
be, which is *why* the reload is a no-op and gets skipped.

### Synchronisation — critical section

```cpp
CriticalSection lock;
CycleTimer::CounterResumeFast();          // TIM1 starts; TMS goes HIGH per its PWM state
SpiDevice::WriteCharSloppy(kFirstTxByte); // preloaded byte starts clocking immediately
if constexpr (kSpiBytes > 1)
    SpiTxDma::EnableFast();               // DMA feeds the remaining bytes on TXE
```

TIM1 is resumed *before* the SPI DR write, so OCREF has at least one `CK_INT` tick to
settle before the first SCK edge — required at high JTAG speeds where the settling
window is shorter than one TCK cycle. `WriteCharSloppy()` (vs. the normal `WriteChar()`)
skips the `TXE` poll — always safe here since `Wait()` guarantees DR is empty — which
removes one conditional branch from this latency-critical path, making the
SPI-vs-timer phase offset a fixed instruction count instead of one with a
data-dependent branch in it.

`cnt_offset` (`kCntStart_ − cnt_offset`) is the only per-grade trim knob, and it only
shifts the **entry** pulse's phase (it moves where counting starts). It does **not**
correct the exit pulse's phase on `kDR` scans — that pulse's timing includes the
CC3→DMA→CCR2-write latency chain, which is a separate, DMA-arbitration-bound delay.
Increasing `cnt_offset` shifts the whole TMS waveform later (rightward) on a logic
analyzer capture. See the `analyze-jtag-la` skill for the concrete recipe (measure
`t_tms` as a fraction of the enclosing TCK rise-to-rise interval; ~50% = ideal,
since TMS only needs setup/hold margin around the TCK **rising** edge).

Current calibration (`Firmware.shared/drivers/JtagDev.dtrig.cpp`):

```cpp
static constexpr uint16_t kDtrigCntOffset_R = 10; ///< GoIdle
static constexpr uint16_t kDtrigCntOffset_1 = 0;  ///< 0.5625 MHz — LA-confirmed: entry pulse ~55% through
                                                   ///< the rise-to-rise window (near-ideal); exit pulse of
                                                   ///< the same frame measured ~67% (extra ~230 ns from the
                                                   ///< CC3→DMA→CCR2 latency chain — still safe at this grade,
                                                   ///< not yet re-checked at faster grades).
static constexpr uint16_t kDtrigCntOffset_2 = 1;  ///< 1.125 MHz
static constexpr uint16_t kDtrigCntOffset_3 = 3;  ///< 2.25 MHz
static constexpr uint16_t kDtrigCntOffset_4 = 8;  ///< 4.5 MHz
static constexpr uint16_t kDtrigCntOffset_5 = 17; ///< 9 MHz
```

A fixed real-time latency (CPU cycles, DMA arbitration) stays roughly constant in
absolute nanoseconds across grades, but the JTCK period itself shrinks — so a
comfortable margin at `kSlowest` can be marginal or unsafe by `kFastest`. Each
grade's entry *and* exit pulse timing should be checked independently with a fresh
LA capture, not assumed from a slower grade's result.

---

## Template: `DtrigJtag<>`

Defined in `Firmware.shared/util/DtrigJtag.h`.

### Template Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `SysClk` | type | System clock (provides APB2 frequency) |
| `kTim` | `Timer::Unit` | TIM1 (must have a repetition counter) |
| `kTms` | `Timer::Channel` | Channel driving JTMS (CH or CHN, per `kCmpComplementary`) |
| `kTmsRld1` | `Timer::Channel` | Compare-only channel whose DMA request reloads CCR2 at the entry→shift boundary |
| `SpiDevice` | type | SPI peripheral type (provides `DmaChInfoTx_`/`DmaChInfoRx_`) |
| `kFreq` | `uint32_t` | JTAG clock frequency (= SPI baud rate) |
| `kScan` | `JtagFrame::Scan` | `kDR`, `kIR`, or `kGoIdle` |
| `kNumBits` | `JtagFrame::NumBits` | 8 / 16 / 20 / 32, or the GoIdle sentinel |
| `kCmpComplementary` | `bool` | `true`: JTMS on CHN (STLinkV2). `false`: JTMS on regular CH (BluePill/G431). |

There is **no TMS buffer parameter** — TMS needs no per-bit data since it's fully
described by two CCR2 compare values per frame.

### Derived Constants

| Constant | Formula | Description |
|----------|---------|--------------|
| `kBitCount` | `5 + kNumBits + (uint8_t)kScan` (or 8 for GoIdle) | JTCK cycles for the selected scan (the `+(uint8_t)kScan` term is why IR needs one more cycle than DR at the same `kNumBits`) |
| `kSpiBytes` | `ceil(kBitCount / 8)` | SPI bytes (whole; may include padding clocks) |
| `kTotalClocks` | `kSpiBytes × 8` | Actual JTCK cycles clocked, including padding |

### Compile-time Assertions

- `kTotalClocks ≤ 40`: must fit `JtagDev::read_buf_`'s ping-pong slot size
- `kSpiBytes ≤ 8`: must fit the SPI TX/RX byte buffers
- `DmaCcr2Rld1::kChan_ ≠ SpiTxDma::kChan_` and `≠ SpiRxDma::kChan_`: DMA conflicts caught at compile time
- `CycleTimer::HasRepetitionCounter()`: TIM1 (or equivalent) is required

### Key Static Methods

| Method | Description |
|--------|-------------|
| `Init()` | One-time hardware init: TIM1 PSC, TMS output config, DMA channel setup, SPI setup. Call once (e.g. `OnOpen()`). |
| `SetupDma()` / `ReleaseDma()` | Re-arm / release the three DMA channels (e.g. to let `JtclkWaveGen`-style code borrow them) |
| `ApplySpeed()` | Speed-grade change only: TIM1 PSC + SPI `CR1[BR]`, applied together (see the DoGoIdle note below on why this matters) |
| `RenderTransaction(tdi_bytes, tclk_high, data_out)` | Fill the SPI TX byte buffer for one IR/DR scan (TDI stream only — no TMS buffer) |
| `DoGoIdle(tdi_bytes, cnt_offset)` | TAP reset: forces grade-1 via `ApplySpeed()`, then 6× TMS=1 + 1× TMS=0 |
| `Start(tdi_bytes, tdo_bytes, cnt_offset)` | Launch the pre-rendered transaction (critical-section start) |
| `Wait()` | Block until TIM1's single-shot auto-stops and SPI finishes, then clean up DMA |
| `GetResult(tdo_bytes)` | Decode the received payload from the SPI RX buffer |

`DoGoIdle()` calling `ApplySpeed()` (not a bare `SetPrescaler()`) matters: a bare
prescaler reset would slow only the timer while SPI baud stayed at whatever the
previous scan's grade left it at, splitting TMS and JTCK onto two independent
timebases — this was a real, fixed bug (commit reference: the "DoGoIdle timer/SPI
split-timebase" fix).

---

## Implementation: `JtagDev.dtrig.cpp`

Located at `Firmware.shared/drivers/JtagDev.dtrig.cpp`. Compiled only when
`OPT_INCLUDE_JTAG_DTRIG_` is defined (set automatically in `stdproj.h` when
`OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG`, which is the only value that
exists now).

### Type Aliases

```cpp
// One SPI template per speed grade
using SpiJtagDevType<JTCK_Speed_1..5>;   // 0.5625 / 1.125 / 2.25 / 4.5 / 9 MHz

// Platform-bound alias — bakes in timer/channel/complementary-flag per board
template <typename SpiDev, uint32_t Freq, Scan S, NumBits N>
using DtrigImpl = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms, kWaveJtagTmsRld1,
                            SpiDev, Freq, S, N, kWaveJtagTmsCmpComplementary>;

// Scan-type aliases (all use grade-1 SPI type; actual baud is set at OnOpen/SetSpeed time)
using DtrigGoIdle = DtrigImpl<SpiJtagDev, JTCK_Speed_1, Scan::kGoIdle, NumBits::kGoIdle>;
using DtrigIr8    = DtrigImpl<SpiJtagDev, JTCK_Speed_1, Scan::kIR, NumBits::k8>;
using DtrigDr8/16/20/32 = DtrigImpl<SpiJtagDev, JTCK_Speed_1, Scan::kDR, NumBits::k8/16/20/32>;

// Per-grade Init-only instantiations (only Init()/ApplySpeed() called on these)
using DtrigInit_1..5 = DtrigImpl<SpiJtagDevType<JTCK_Speed_N>, JTCK_Speed_N, Scan::kDR, NumBits::k8>;
```

### Async shift pattern

Every `OnXxxShift()` overlaps *rendering* the new frame with the *DMA* of the
previous one: `AcquireSpiMode()` → `RenderTransaction()` into the ping-pong buffer's
next slot (pure CPU, runs while the previous frame's DMA is still in flight) →
`JtagWaitTransfer()` (blocks on the previous frame) → `buf.Step()` → `R::Start()`
(kicks the new DMA, returns immediately) → return a `JtagPending<T>` wrapping the
new frame's RX slot and a decoder function pointer.

### TCLK Operations (Run-Test/Idle only)

SPI SCK (=JTCK) can't be suppressed independently of the SPI peripheral — sending a
byte always produces 8 JTCK pulses. This is harmless during these helpers because
the TAP is in Run-Test/Idle and TMS=0 on every edge just keeps it there.

| Method | SPI byte sent | JTDI/JTCLK after | Meaning |
|--------|---------------|-------------------|---------|
| `OnSetTclk()` | `0xFF` (via `AcquireSpiClkMuted()`, PA5 parked GPIO-high so JTCK doesn't pulse) | HIGH | JTCLK held high |
| `OnClearTclk()` | `0x00` | LOW | JTCLK held low |
| `OnPulseTclk()` | `0xF0` | HIGH | Low→High rising edge |
| `OnPulseTclkN()` | `0x0F` | HIGH | High→Low→High sequence |

### Flash TCLK (`OnFlashTclk`) — SPI-waveform variant

MSP430 flash operations need a sustained ~450 kHz JTCLK for thousands of cycles.
This build uses `OPT_JTCLK_IMPL_SPI` (see `Firmware.shared/util/WaveSet.h`): JTCK
(PA5) is parked GPIO-high (`AcquireSpiClkMuted()`), and a *different* SPI baud
(`SpiJtmsWave`, `kJtclkSpiClock`) streams a precomputed bit pattern (`gJtmsWave`)
out of MOSI (PA7 = JTDI, which doubles as JTCLK in Run-Test/Idle) — each JTCLK cycle
costs 2 SPI bits (one full SCK toggle), since the waveform is literally the SPI
clock's own toggling reinterpreted as data. This entirely avoids touching TIM2/TIM3
or `ReleaseDma()`/`SetupDma()` — the older `JtclkWaveGen` (TIM2+TIM3+DMA) approach
this document previously described is the *other* selectable variant,
`OPT_JTCLK_IMPL_TIM_DMA` (see `Firmware.shared/platform-defs.h` /
`OPT_JTAG_TCLK_IMPLEMENTATION` in `CLAUDE.md`), not what's active in the code above.

### TDI/TMS Buffer Layout (example: 16-bit DR scan)

`RenderTransaction()` fills **only the TDI stream** — TMS is generated entirely by
the timer, decoupled from the SPI byte content:

```
kBitCount    = 5 + 16 + 0 (DR) = 21 bits
kSpiBytes    = ceil(21/8) = 3 bytes
kTotalClocks = 24 bits

TDI stream (MSB-first, packed via one __REV + store, see RenderTransaction comment):
  bits [0..3]              : head fill   (TDI = tclk_high) — TAP is in/entering Select-DR here
  bits [4..(4+kNumBits-1)]  : payload, MSB first
  bits [(4+kNumBits)..23]   : tail fill   (TDI = tclk_high) — TAP is in Exit/Update/RTI here
```

The fill bits intentionally straddle the portions of the frame where the TAP state
machine is in transition/idle states, not shifting real payload — so their content
doesn't matter functionally, only `tclk_high` needs to be preserved through them.

---

## Configuration

### Per-grade CNT Offset Calibration

See the "TIM1: PWM-mode TMS generation" section above for the current values and
methodology. Use the `analyze-jtag-la` skill against a fresh capture
(`supp/docs-ai/test-logic-analyzer.csv`) to re-derive or verify a grade's offset:
measure each toggle event (entry **and** exit) as a fraction of its enclosing
TCK rise-to-rise window; ~50% is the safe target.

---

## Open Items

- **Exit-pulse margin at faster grades unverified.** Grade 1's exit pulse measured
  ~230 ns later than its entry pulse (same frame) — comfortable at 1.78 µs period,
  but that's a DMA-latency-bound skew that doesn't shrink with the period. Grades
  3–5 need their own LA pass checking *both* pulses, not just the entry pulse.
  Confirmed **not** a code-quality/optimization artifact: a Release-build capture
  at grade 1 (`test-logic-analyzer-release.csv`) reproduced the same ~55%/~67%
  entry/exit phase split (within ~10–30 ns, i.e. LA sample-grid noise) as the
  Debug-build capture — the skew is hardware/DMA-latency-bound, not CPU-cycle- or
  compiler-dependent, which fits the CC3→DMA→CCR2-write chain explanation.
- **`DtrigJtag.h`'s header-comment example pin (BluePill PA10/TIM1_CH3)** refers to
  an obsolete, never-to-be-revisited prototype — harmless as a comment, not worth
  editing, just don't trust it over `target.bluepill/platform.h`.
- **Stray `kTmsRld2` mention in `JtagDev.dtrig.cpp`'s DMA-map comment block** no
  longer matches the single-reload-channel template — cosmetic, but worth cleaning
  up next time that file is touched.

---

## File Index

| File | Role |
|------|------|
| `Firmware.shared/util/DtrigJtag.h` | Template class: render, start, wait, decode. Its own top-of-file comment is the most detailed/authoritative source on the PWM mechanism. |
| `Firmware.shared/drivers/JtagDev.dtrig.cpp` | `JtagDev` virtual method implementations, per-grade `cnt_offset` table |
| `Firmware.shared/util/WaveSet.h` | `SpiJtmsWave`/`gJtmsWave` — the SPI-waveform flash-TCLK generator (`OPT_JTCLK_IMPL_SPI`) |
| `target.stlinv2/platform.h` | STLinkV2 pin aliases, `kWaveJtagTms*` constants (CH2N/PB14) |
| `target.bluepill/platform.h` | BluePill pin aliases, `kWaveJtagTms*` constants (CH2/PA9) |
| `Firmware.shared/platform-defs.h` | `OPT_JTAG_IMPL_DTRIG`, `OPT_JTCLK_IMPL_SPI`/`OPT_JTCLK_IMPL_TIM_DMA` |
| `Firmware.shared/stdproj.h` | `OPT_INCLUDE_JTAG_DTRIG_` guard |
| `Firmware.shared/util/JtagFrame.h` | `Scan` and `NumBits` enum classes |
| `.claude/skills/analyze-jtag-la/SKILL.md` | Recipe for LA-based `cnt_offset` calibration and frame decoding |
