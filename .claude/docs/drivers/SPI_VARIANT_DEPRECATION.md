# Why `JtagDev.spi.cpp` is being deprecated in favour of DTRIG

Status: rationale captured 2026-05-08. Companion to
[DTRIG_JTAG_DRIVER.md](DTRIG_JTAG_DRIVER.md).

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

A platform.h comment near the JTAG transport selector (lines 50–54)
flags one outstanding port-blocker: the existing `DtrigJtag` template
drives TMS via TIM1_CH2N (the STLinkV2 path through PB14). On bluepill
TMS is on PA10 = TIM1_CH3 (regular CH, not CHN). The template needs a
small generalisation to accept either CH or CHN as the TMS output
before DTRIG can actually drive bytes on bluepill.

## Migration plan (high level)

1. Generalise the DtrigJtag template so the TMS output channel can be
   either CH or CHN — selected by a template parameter and matching
   the channel constant from `platform.h`.
2. Once that's in, switch the bluepill default in `platform.h` from
   `OPT_JTAG_IMPL_SPI` to `OPT_JTAG_IMPL_DTRIG` and verify on the
   bench.
3. Mark `JtagDev.spi.cpp` as deprecated in source comments (already
   done — points back here).
4. Eventually retire `JtagDev.spi.cpp` and the `OPT_JTAG_IMPL_SPI`
   path entirely. Until step 2 succeeds the SPI path stays as a
   fallback so no target loses functionality.

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
