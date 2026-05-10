# Why `JtagDev.spi.cpp` was removed in favour of DTRIG

Status: rationale captured 2026-05-08; variant removed 2026-05-10 alongside the
async `JtagPending<T>` shift refactor. Companion to
[DTRIG_JTAG_DRIVER.md](DTRIG_JTAG_DRIVER.md).

## Last-working git references

If you need to revive any of this code, check out:

| What | Last-working commit | Notes |
|------|---------------------|-------|
| Tip of `JtagDev.spi.cpp` history | `af121e4` (`JtagDev: hoist common OnEnterTap into shared TU`) | Compiles cleanly under the pre-async ITapInterface (`uint8_t OnIrShift(uint8_t)` etc.) |
| `434643d` "SPI_DMA: split SendStream into Start/Wait" | The architectural touchpoint where the SPI variant's render/wait split happened — useful if you want to see the synchronous DMA pipeline DTRIG inherited from |
| `Firmware.shared/util/SpiJtagDataShift.h` final state | `f60a2bf` ("SpiJtagDataShift: fix sizeof check that masked the uint64_t branch") | The deleted SPI helper |
| `Firmware.shared/util/TmsAutoShaper.h` final state | `6d3742c` ("TimDmaWave / TmsAutoShaper: explicit master/slave clock binding") | The deleted external-clock TMS shaper |

## What was deleted

- `Firmware.shared/drivers/JtagDev.spi.cpp`
- `Firmware.shared/util/SpiJtagDataShift.h`
- `Firmware.shared/util/TmsAutoShaper.h`
- `OPT_JTAG_IMPL_SPI` and `OPT_JTAG_IMPL_SPI_DMA` selectors in `platform-defs.h`
- All `#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI*` branches in every `target.*/platform.h`

## TL;DR

In the SPI variant, TMS pulse edges are produced by TIM1 PWM with TIM1
clocked **externally from SCK** (`TI1F_ED`, both edges). That pegs the
TMS edge resolution to **half a SCK period** — every CCR value lands on
either a SCK rising edge or a SCK falling edge, never between them.

A DR/IR scan needs an odd-spaced pulse pair (e.g. `kStart1stPulse_=3,
kEnd1stPulse_=4`), so by parity at least **one TMS edge always lands on
a TCK rising edge** — exactly the edge the JTAG target uses to sample
TMS. The MSP430 sees a setup/hold violation; a 100 MS/s logic analyzer
sees the TMS transition as simultaneous with the clock edge and fails
to decode. `anticipate_clock=true` only swaps which of the two edges
collides; it cannot place either edge mid-period.

DTRIG sidesteps this by clocking TIM1 from **SYSCLK (72 MHz)** with the
slave-mode trigger reset by SCK. Resolution drops from ~890 ns (half a
562 kHz SCK period) to ~14 ns, so TMS edges can be placed deep inside
the SCK low-half with proper margin.

## Geometric proof of the SPI variant's limit

`TmsAutoShaper.h:24` configures TIM1 with `Timer::ExtClk::kTI1F_ED`.
TIM1 ticks twice per SCK period. `SpiJtagDataShift.h:62` matches with
`kClocksPerBit_ = 2`.

For a DR-scan with `kSelectDR_Scan=1, kCaptureDr_=2`:

```
kHeadClocks_       = kDummyBits_ + kSelectScan + kCapture_ = 1 + 1 + 2 = 4
kStart1stPulse_    = kDummyBits_ * kClocksPerBit_ + kCounterStart_ = 1*2 + 1 = 3
kEnd1stPulse_      = (kDummyBits_ + kSelectScan) * kClocksPerBit_ + kCounterStart_ - kPeriodOffset_
                   = (1+1)*2 + 1 - 1 = 4
```

So the first TMS pulse spans CCR=3 → CCR=4. CCR transitions land on
SCK edges (one tick = one SCK edge):

| CCR | SCK edge type at that tick |
| ---:| ---                       |
|  1  | rising  (sample edge)     |
|  2  | falling (mid-period)      |
|  3  | rising  (sample edge)     |
|  4  | falling (mid-period)      |

(The exact parity flips depending on which edge the burst started on,
but the **spacing of 1** is invariant.)

Whatever edge type CCR=3 lands on, CCR=4 lands on the opposite. There
is **no integer assignment** that puts both edges of a 1-tick-wide
pulse mid-period. `anticipate_clock=true` shifts both CCRs by −1,
flipping which one collides.

### Confirmed against silicon

`supp/docs-ai/Failed.csv` (MSP430F1611, slowest grade, 562 kHz):

- TCK rises at 0.07705661 s (12th SCK edge of the burst)
- TMS falls at 0.07705669 s — **8 ns later**

The 8 ns is just TIM1 → PA10 combinational delay; the timer fires CCR
on the same SCK edge that samples TMS. From the analyzer's 10 ns
sampling window the two events are simultaneous, and from the MSP430's
setup/hold viewpoint they are at the edge of validity.

## Why DTRIG escapes the parity trap

In the dtrig variant (`target.stlinv2/platform.h` and the dtrig
branches already baked into `target.bluepill/platform.h`):

- TIM1 runs from **SYSCLK** (72 MHz) — internal clock, not external
- Slave mode = **Reset on TI1FP1** — every SCK rising edge resets CNT
  to 0
- TIM1 counts up at 72 MHz between SCK edges; CCR3 fires the TMS edge
  at any sub-period offset

For 562 kHz SCK on a 72 MHz timer: SCK period = 128 ticks, half-period
= 64 ticks. A TMS edge at CCR=32 lands a quarter-period into the SCK
low-half — full setup/hold margin in both directions.

For 9 MHz SCK (fastest grade): SCK period = 8 ticks. The "8× faster"
the user tuned matches exactly this 72 MHz / 9 MHz ratio; at slower
grades the oversampling ratio is even larger, so the dtrig design
auto-scales correctly.

## What's already prepared in `target.bluepill/platform.h`

The bluepill platform.h already has DTRIG branches alongside the SPI
ones — code review of lines 121–131, 140–143, 293–305, 341–355,
364–378, 469–… shows:

- `TmsShapeGpioIn = Unchanged<8>` in DTRIG mode (PA8 freed; TIM1 uses
  internal clock instead of SCK input)
- `JTMS_PWM = TIM1_CH3_PA10_OUT` for the slave-mode PWM output
- `JtagOn` AnyPinGroup includes `JTMS_PWM` and leaves PA8 untouched
- `JtagSpiOn` / `JtagGpioOn` split the SPI vs bit-bang windows
- `kWaveJtagTimer = TIM1`, `kWaveJtagTms = Channel::k3`,
  `kWaveJtagTmsRld1 = Channel::k4` (CC4 DMA reload of CCR3)
- `PeripheralEnabler` block selected by
  `OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG`

The previous port-blocker — `DtrigJtag` only supporting CHN as the
TMS output (STLinkV2 PB14 = TIM1_CH2N) while bluepill needs a regular
CH (PA10 = TIM1_CH3) — has been resolved. The template now takes a
`kCmpComplementary` boolean parameter (default `true` for backward
compat with STLinkV2): when `false` it enables the main CH instead of
CHN and runs OCREF in PWM1 instead of PWM2 so TMS still starts HIGH
during the entry pulse. Each platform.h's DTRIG block exports a
matching `static constexpr bool kWaveJtagTmsCmpComplementary` that
`JtagDev.dtrig.cpp` threads into the per-frame aliases through a
single `DtrigImpl<>` helper.

## Migration history

1. ~~Generalise the DtrigJtag template so the TMS output channel can
   be either CH or CHN.~~ Done — `kCmpComplementary` parameter +
   `kWaveJtagTmsCmpComplementary` platform constant.
2. ~~Switch bluepill from `OPT_JTAG_IMPL_SPI` to `OPT_JTAG_IMPL_DTRIG`.~~ Done.
3. ~~Mark `JtagDev.spi.cpp` as deprecated in source comments.~~ Done at `434643d`.
4. ~~Retire `JtagDev.spi.cpp` and the `OPT_JTAG_IMPL_SPI` / `OPT_JTAG_IMPL_SPI_DMA` paths.~~ Done 2026-05-10 (`af121e4` is the last commit where the SPI variant compiled).

## What this rationale does NOT argue for

- It does not argue against using SPI1 hardware to shift the data
  bytes — DTRIG also uses SPI1 for JTCK/JTDI/JTDO. The deprecation is
  specifically about the **TIM1-clocked-from-SCK** TMS shaping
  approach, which is what `JtagDev.spi.cpp` and the `SPI_DMA` variant
  both use today.
- It does not require any pin remap on bluepill: PA10 stays the TMS
  pin in both modes, PA8 just becomes free in DTRIG mode instead of
  being TIM1's clock input.

## Pointers

- `Firmware.shared/util/TmsAutoShaper.h` — current SPI TMS shaper (external clock, the one being deprecated)
- `Firmware.shared/util/SpiJtagDataShift.h` — CCR derivation that exposes the parity trap
- `Firmware.shared/util/DtrigJtag.h` — DTRIG-side TMS shaper (target architecture)
- `target.bluepill/platform.h` — both variants live here side-by-side; DTRIG branches are the migration target
- `target.stlinv2/platform.h` — reference DTRIG configuration
- `supp/docs-ai/Failed.csv` — capture from MSP430F1611 demonstrating the edge-alignment failure
