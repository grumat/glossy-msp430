# Timer→DMA latency probe (`OPT_TEST_TIM_DMA_TIMING`)

A driver-decoupled, single-shot bench mode that measures the **fixed timer-compare →
DMA-request → register-write latency** of an MCU so the `TimSbw` phase budget
(`kTimerMultiplier_`) and SBW speed grades can be set from data instead of guesswork.

- Firmware: [`Firmware.shared/util/TimDmaTiming.h`](../../../Firmware.shared/util/TimDmaTiming.h)
- Host analysis: [`tools/tim_dma_timing.py`](../../../tools/tim_dma_timing.py)
- Macros: `OPT_TEST_TIM_DMA_TIMING` (0 off / 1 normal / 2 swapped), `OPT_TEST_TIM_DMA_MULT`
  (ticks per wire-cycle, default 8; even, raise to 16/32 for a finer slice) — defaults in
  `Firmware.shared/stdproj.h`, enabled per target in `platform.h`.

It deliberately rebuilds the `TimSbw` timer+DMA machinery (TIM1 advanced timer, PWM clock
channel on `kWaveSbwClk`, two SBW frozen-compare DMA-trigger channels, SBWDIO/PB14,
SBWCLK/PB13) from raw `bmt` primitives with **no include dependency on `TimSbw.h`**, so it
ports cleanly to the STM32G431 (and genuine STM32F103) later.

## What it generates

100 repetitions of one waveform; the SBWCLK falling edge is the timing **anchor**, and
both BSRR DMA requests are triggered at that same timer-compare value:

```
            ____            anchor = SBWCLK falling edge (timer compare @ mult/2)
SBWCLK: ____|   |________
                 _
SBWDIO: ________| |_______   set-DMA (rise) + reset-DMA (fall), concurrently triggered
```

Two measurements per cycle:

| quantity | meaning |
|----------|---------|
| **t_lag** = (1st SBWDIO edge) − (SBWCLK fall) | compare → DMA-request → BSRR-write latency |
| **t_gap** = (2nd SBWDIO edge) − (1st SBWDIO edge) | gap between the two concurrent DMA channels |

`OPT_TEST_TIM_DMA_TIMING`:
- **1 (normal):** DMA channel A → set, B → reset. SBWDIO idles **low**, pulses **high**.
- **2 (swapped):** A → reset, B → set. SBWDIO idles **high**, pulses **low**.

Both DMA channels carry the same software priority, so the DMA controller's **hardware
channel-number arbitration** decides ordering — swapping the set/reset role between the
two channels exposes whether the natural priority changes the lag/gap/jitter. On STLinkV2
the channels are TIM1_CH2→DMA1_CH3 (A) and TIM1_CH4→DMA1_CH4 (B); the lower-numbered CH3
wins arbitration, so mode 1 should show the rise leading cleanly.

## Procedure

1. **Build.** In the target `platform.h` (e.g. `target.stlinv2/platform.h`) uncomment:
   ```c
   #define OPT_TEST_TIM_DMA_TIMING   1     // 1 = normal, 2 = swapped
   //#define OPT_TEST_TIM_DMA_MULT   8     // optional: 16 / 32 for finer slices
   ```
   Rebuild in Visual Studio / VisualGDB and flash. (Per CLAUDE.md the headless `target.*`
   build is unreliable; rebuild in the IDE.) main() prints `TIM->DMA probe: …` over SWO,
   emits the burst, then halts — no GDB host needed.

2. **Capture.** Probe **SBWCLK** (PB13) and **SBWDIO** (PB14) with the logic analyzer at
   the highest practical rate (≥100 MHz; 200 MHz gives ~5 ns resolution — the
   quantization floor of every number). Trigger on the first SBWCLK falling edge; capture
   ≥100 cycles (~100 µs at the 1 MHz wire clock).

3. **Repeat swapped.** Rebuild with `=2`, capture again.

4. **Analyze.** Export each capture as CSV (Saleae Logic 2 "Export Data", per-sample) and:
   ```
   python tools/tim_dma_timing.py mode1.csv
   python tools/tim_dma_timing.py mode2.csv --clk-col SBWCLK --dio-col SBWDIO
   ```
   Columns auto-detect from `clk`/`dio` name substrings, or pass `--time-col/--clk-col/
   --dio-col` (header name or 0-based index). The script is polarity-agnostic (keys off
   the SBWCLK fall + next two SBWDIO transitions), so it handles both modes. It prints
   min / max / mean / std / jitter for `t_lag` and `t_gap`.

5. **Apply.** Convert the measured `t_lag` to timer ticks at the SBW grade in use
   (`tick = 1/(mult·f_wire)`) and check that `TimSbw`'s `kPhaseData_…kPhaseSample_`
   spacing leaves at least `t_lag` of headroom at the top speed grade; adjust
   `kTimerMultiplier_` / the phase constants accordingly.

## Results (fill per MCU)

Wire clock 1 MHz, `mult` = 8 (tick = 125 ns) unless noted. Captured at 200 MHz (5 ns
resolution).

| MCU / board | mode | mult | t_lag min/avg/max (ns) | t_gap min/avg/max (ns) | notes |
|-------------|------|------|------------------------|------------------------|-------|
| Geehy GD32F103 (STLinkV2) | 1 normal  | 8 | 135 / 145 / 180 | 110 / 127 / 155 | 2026-06-12, n=100; jitter ±45 ns each |
| Geehy GD32F103 (STLinkV2) | 2 swapped | 8 | 135 / 148 / 180 | 105 / 122 / 150 | 2026-06-12, n=99; jitter ±45 ns — statistically identical to mode 1 |
| ST STM32F103 (genuine)    | 1 normal  | 8 | _tbd_ | _tbd_ | when available |
| ST STM32G431 (BluePill-G431) | 1 normal  | 8 | _tbd_ | _tbd_ | wired (SBWCLK TIM1_CH2/PA9, SBWDIO PA10); probe MCU pins |

### Interpretation — Geehy GD32F103 (mode 1, mult=8, tick=125 ns)

- **compare→DMA→BSRR lag ≈ 145 ns avg, 180 ns worst** = ~1.16 ticks avg, **1.44 ticks
  worst**. A BSRR write requested at compare tick *N* actually appears on the pin around
  tick *N+1.5* in the worst case. TimSbw sets data up at `kPhaseData_=1` and the SBWCLK
  capture (fall) is at `kPhaseClk_=4` — a 3-tick (375 ns) gap, comfortably > 180 ns, so
  data-before-capture is **safe at mult=8 across all current grades** (even SBW_Speed_5 =
  1.2 MHz → tick 104 ns → 3 ticks = 312 ns > 180 ns).
- **inter-DMA gap ≈ 127 ns avg, 155 ns worst** ≈ **1.0–1.24 ticks**. This is the key
  finding: when two DMA channels are triggered at the *same* compare, the second lands a
  little **more than one mult=8 tick** after the first. So a **1-tick phase separation is
  not enough** to guarantee the earlier-phase DMA has retired before the next phase's
  request at the fast grades — TimSbw's `kPhaseData_(1) → kPhaseDir_(2)` are only 1 tick
  apart. Remedies, in order of preference: (a) widen that separation to ≥2 ticks, or
  (b) raise `OPT_TEST_TIM_DMA_MULT`/`kTimerMultiplier_` so a tick is shorter relative to
  the fixed ~127 ns gap, or (c) rely on the DMA priority hierarchy (Data > Dir > Sample)
  to order the overlap deterministically — which is exactly what TimSbw already does as a
  backstop. The probe confirms that backstop is load-bearing at the top grades, not
  decorative.
- **~45 ns jitter (≈9 LA samples)** on both numbers is the DMA-arbitration/AHB-contention
  spread; budget for it (use the *worst* case, not the mean, when sizing phases).

**Mode 1 vs mode 2 (channel-arbitration experiment) — RESULT:** swapping which physical
DMA channel (DMA1_CH3 vs CH4) plays the set/reset role made **no meaningful difference**:
lag 145→148 ns avg (same 135/180 min/max), gap 127→122 ns avg (105–155 vs 110–155),
**identical 45 ns jitter** on both. The ±2–5 ns shifts are sub-sample noise (< the 5 ns LA
resolution). So the ~125 ns inter-DMA gap is **intrinsic to the DMA controller's
back-to-back servicing latency, not an artifact of channel priority/number** — it cannot
be tuned away by reordering channels. Consequence for TimSbw: the Data>Dir>Sample priority
hierarchy makes the same-compare overlap *deterministic* but does **not** buy time (the
second DMA still lands ~125 ns later regardless); the only real levers are widening the
phase separation to ≥2 ticks or raising the multiplier.

## Porting to a new target

`main.cpp` builds a single generic `TimDmaTimingProbe` alias from a **uniform resource
bundle** that each enabling target's `platform.h` provides under `#if OPT_TEST_TIM_DMA_TIMING`:

| name | meaning |
|------|---------|
| `TimDmaTimingClkPin` | SBWCLK pin in timer-AF mode (e.g. `TIM1_CH1N_PB13<…>`, `TIM1_CH2_PA9_OUT`) |
| `TimDmaTimingDio` | push-pull output pin pulsed via BSRR (idle low) |
| `kTimDmaTimer` | advanced timer unit (TIM1) |
| `kTimDmaClkCh` | SBWCLK PWM channel |
| `kTimDmaTrigACh` / `kTimDmaTrigBCh` | two frozen-compare channels → two distinct DMA channels |
| `kTimDmaClkCmpComplementary` | true → SBWCLK on CHN; false → regular CH |

Pick channels whose DMA mappings are distinct (check the target's `DmaChInfo`/`dma-cfgs`)
and put the *set* role on the lower-numbered DMA channel so the rise leads in mode 1. On
G4 the DMA mapping differs (DMAMUX) but `bmt`'s `DmaChInfo_`/`AnyChannel` hides it — the
probe template is unchanged.

**Wired targets:**
- **STLinkV2 / F103:** reuses the SBW assignments — SBWCLK TIM1_CH1N/PB13, SBWDIO PB14,
  triggers CH2→DMA1_CH3 (A) / CH4→DMA1_CH4 (B).
- **BluePill-G431:** no SBW table, so the probe picks its own — SBWCLK TIM1_CH2/PA9
  (regular CH), SBWDIO PA10, triggers CH1→DMA1_CH2 (A) / CH4→DMA1_CH4 (B), CH3 free.
  The jiga's DIS_* buffers are Hi-Z in this standalone mode, so **probe the MCU pins
  directly** (or pull DIS_JTAG/PB12 low to drive PA9 to the connector). DTRIG timing on
  G431 is itself unvalidated (see the platform.h header), but this probe is independent of
  the JTAG driver.

## Notes / caveats

- TIM1, DMA1 and the GPIO ports are clocked at boot by the platform `PeripheralEnabler`,
  so the probe runs straight out of `main()` with no driver init.
- Software-priority override (forcing one DMA channel above the other instead of relying
  on channel number) is a manual knob only: change the `Prio::kHigh` on `DmaA`/`DmaB` in
  `TimDmaTiming.h` and rebuild. Not wired to the macro.
- This is firmware-side a pure generator; all statistics come from the LA + script.
