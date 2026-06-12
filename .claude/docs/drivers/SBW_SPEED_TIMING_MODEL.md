# SBW speed ceiling & frequency-compensated phase model

Why SBW tops out at ~1.3–1.4 MHz on every board tried, and what the measured
DMA latency says about pushing it higher (and how to place the per-cycle
compares so the limit is data-driven, not folklore).

Companion docs:
- [`TIM_DMA_TIMING_PROBE.md`](TIM_DMA_TIMING_PROBE.md) — the bench mode that **measures** the
  compare→DMA→BSRR latency (`L`) this analysis consumes.
- [`TIM_SBW_DRIVER.md`](TIM_SBW_DRIVER.md) — the driver itself.
- Source: [`Firmware.shared/util/TimSbw.h`](../../../Firmware.shared/util/TimSbw.h)
  (`kPhaseData_/kPhaseDir_/kPhaseClk_/kPhaseSample_`, lines ~250-296).
- Reference: TI **SLAU320 Figure 2-8** (Spy-Bi-Wire timing) — bitmap in
  `supp/docs-ai/SBW-Timing-Diagram.jpg`.

## The diagram (SLAU320 Fig 2-8), restated

SBWTCK idles **high**; each slot is one low-going clock pulse. Per slot:

```
        rising edge = slot start          falling edge = capture        next rising = slot end
        (host sets up SBWDIO here)        (target latches TMS/TDI;       (target releases TDO;
              │                            target begins driving TDO)     host re-acquires bus)
              ▼                                    ▼                              ▼
SBWTCK  ──────┐                          ┌─────────┐                    ┌────────
              └──────────────────────────┘         └────────────────────┘
                         HIGH phase                      LOW phase
                    (data setup window)              (TDO valid window)
```

- **host→target** (TMS/TDI slots): SBWDIO must be valid **before** the falling edge.
- **target→host** (TDO slot): the target starts driving on the **falling** edge and is
  valid through the **low** phase; the line is an RC node (the diagram draws the recovery
  curve), so the host must sample **late** in the low phase, after settle, before the rise.

This matches the model already in `TimSbw.h:200-214` — no correction needed there.

## The one asymmetry that sets the ceiling

SBWCLK is a **PWM output** (`SbwClkOut`, `AnyOutputChannel<…kPWM1…>`): timer compare
straight to the pin — **~0 ns lag, ~0 ns jitter**. Everything else is **DMA**:

| signal | mechanism | latency |
|--------|-----------|---------|
| SBWCLK | PWM (timer→pin) | ~0 ns |
| SBWDIO data (BSRR) | DMA | `L` |
| direction (CRH, single-pin path only) | DMA | `L` |
| TDO sample (IDR→mem) | DMA | `L` |

**Measured `L` (Geehy GD32F103, STLinkV2, mult=8, `OPT_TEST_TIM_DMA_TIMING`):**
`L = 145 ns avg / 180 ns worst / 135 ns min → 45 ns jitter`. Inter-DMA back-to-back
gap (two DMAs at the same compare) ≈ **127 ns avg / 155 ns worst**, intrinsic and
**not** reorderable by channel priority (see the probe doc's mode-1-vs-mode-2 result).

The clock is exact; the data/sample slip late by `L`. The current phase constants
(`kPhaseData_=1, kPhaseClk_=4, kPhaseSample_=6`) are **fixed fractions of the cycle**.
As `f` rises the cycle shrinks but `L` does not, so `L` eats a growing fraction of every
margin. **That is the speed ceiling — DMA latency inside TimSbw, not the board RC on the
strong output edges.** A better-isolated probe (e.g. 3K3 ahead of the 47K/2.2nF reset
network) does not move the wall because the wall is in firmware.

## Timing model (one cycle; rising edge at t=0, fall at `t_fall = C·tick`)

| event | instant | constraint |
|-------|---------|------------|
| data set up (BSRR DMA @ `D`) | `D·tick + L` | ≤ `t_fall` (valid before capture) |
| dir switch (CRH DMA @ `Dir`, single-pin only) | `Dir·tick + L` | ≤ `t_fall`; **≥127 ns after data** (inter-DMA gap) |
| **clock fall = capture** (PWM @ `C`) | `C·tick` (exact) | the anchor |
| TDO sample (IDR DMA @ `S`) | `S·tick + L` | ≥ `t_fall + T_settle` **and** ≤ `t_rise` |

Two design rules, both **anchored to absolute `L`, not a cycle fraction** — this *is*
frequency compensation:

- **Data:** worst-case-late write still clears the fall → `D·tick ≤ C·tick − (L_max + guard)`.
- **Sample (the late-capture priority):** worst-case-late sample still clears the rise →
  `S·tick ≤ t_rise − (L_max + guard)`. This samples **as late as possible at every grade**
  — exactly the "late capture is positive" behaviour wanted for low-bandwidth / heavily
  damped links — from the same `L_max`-anchored formula, so it self-scales.

Note the DMA latency *helps* the read (it pushes the sample later) and *hurts* the write
(it pushes data later, toward the fall). Same `L`, opposite sign.

## Recommended CCR (compare) values

Buffered/folded path (one write DMA carrying data+dir), 50 % duty, **mult = 16** held
constant so all the compensation lives in the compares; `L_max = 180 ns`, guard ≈ 40 ns:

| f_wire | T | tick | kPhaseClk_ (fall) | kPhaseData_ | kPhaseSample_ | write margin | read guard (to rise) | settle room |
|--------|-----|------|-------------------|-------------|---------------|--------------|----------------------|-------------|
| **2.0 MHz** | 500 ns | 31.25 ns | 8 | **0** | **9** | 70 ns | **≈39 ns** ⚠ | ~166 ns |
| **1.0 MHz** | 1000 ns | 62.5 ns | 8 | **4** | **12** | 70 ns | 70 ns | ~385 ns |
| **0.5 MHz** | 2000 ns | 125 ns | 8 | **6** | **14** | 70 ns | 70 ns | ~885 ns |
| **0.3 MHz** | 3333 ns | 208 ns | 8 | **6** | **14** | 238 ns | 241 ns | ~1.38 µs |

The compares converge to a fixed-**time** meaning regardless of grade: *data ≈220 ns before
the fall, sample ≈220 ns before the rise* (= `L_max + guard`). That invariant is what a
`constexpr` phase function should encode (parameterised on `kFreq`, `kCycleTicks_`, and a
`kDmaLatencyNs ≈ 180`), replacing the three hand-tuned constants.

## Verdict

**Frequency compensation makes clear sense — implement phases as absolute-time placement.**
Two hard caveats on 2 MHz specifically:

1. **2 MHz read margin ≈ 39 ns < the 45 ns jitter.** At 2 MHz the half-period (250 ns)
   barely exceeds `L_max` (180 ns), so a worst-case jitter excursion can violate the sample
   window. **~1.5 MHz is the defensible ceiling** (margin ≥ jitter); 2 MHz is reachable but
   on the edge. Higher `mult` improves *placement resolution*, not jitter.

2. **The single-pin STLinkV2 path (`kSeparateDirDma=true`) is the worse offender and likely
   cannot reach 2 MHz at all.** Its write slot must retire **two serialized DMAs** — data
   BSRR *then* dir CRH, ~127 ns apart — before the fall: ≈180 + 127 ≈ **307 ns needed vs a
   250 ns half-period.** A **buffered board that folds data+dir into one BSRR word** needs
   only one 180 ns transfer, which 250 ns allows. **The road to 2 MHz is the folded/buffered
   architecture, not the single-pin one.** If the "regardless of board" tests were all on the
   single-pin STLinkV2 path, that two-DMA write serialization is the firmware wall being felt.

### The unmeasured term: `T_settle`

The model's read side also depends on `T_settle` — how long the target's TDO drive takes to
settle through the RC on the read slot. The non-empty-window condition is roughly
`T/2 > jitter + T_settle`. A real ceiling at **1.4 MHz ⇒ T_settle ≈ 300 ns**. If that's the
binding term, **no phase tuning fixes it** — only sampling as late as possible helps, which
the rule above already does. `T_settle` has **not** been measured yet; extend the probe (or
use the `OPT_SBWDEV_DUMP_READ_PHASE` IDR capture) to time the TDO low→valid edge and decide
whether the wall is write-DMA-serialization or read-settle.

## Next steps

1. **Encode compensated phases** in `TimSbw.h`: a `constexpr` computing
   `kPhaseData_/kPhaseSample_` from `kFreq`, `kCycleTicks_`, `kDmaLatencyNs (=180)`. Keep the
   order-invariant `static_assert`s; make every grade optimal from one formula. Keep it
   default-equivalent at today's grades; add a higher grade behind bench validation.
2. **Measure `T_settle`** on the bench (probe extension / read-phase dump).
3. **Re-grade `SBW_Speed_*`** around a **1.5 MHz** top rather than 2 MHz; honour the
   flash-clock floor already documented (see `project_sbw_speed_limit`).

## v2 scheme — latency-aligned late write (proposed, not yet implemented)

Instead of fighting the DMA lag (place data *early*, hope it clears the fall), **use it**:
trigger the direction + data writes at the **last CCR of the cycle** and let the ~180 ns
latency carry the effect to the **rising edge** of the next slot — i.e. the slot boundary,
where the new bit belongs. This gives nearly a full slot of setup time.

### The linchpin condition

Write effect lands at `(mult-1)·tick + L`; the next rising edge is at `T = mult·tick`, so:

```
landing − rising = L − tick
```

- `L > tick` → lands **after** the rising edge (target already released TDO) → clean.
- `L < tick` → lands **before** the rising edge, inside the **TDO low phase while the target
  still drives** → **input→output contention** on the TDO→TMS turnaround.

To stay clean even on the shortest-latency cycles (`L_min = 135 ns`):

```
mult ≥ T / L_min = 1 / (f · 135 ns)
```

| f | T | min mult (no contention) | tick | setup margin to next fall |
|---|---|--------------------------|------|---------------------------|
| 2.0 MHz | 500 ns | 4 | 125 ns | ~195 ns |
| 1.0 MHz | 1000 ns | **8** | 125 ns | ~445 ns |
| 0.5 MHz | 2000 ns | 15 | 133 ns | ~950 ns |
| 0.3 MHz | 3333 ns | 25 | 133 ns | ~1.6 µs |

Consequences:
- **`mult` must grow at slow grades** (or pick one fixed `mult ≈ 24–25` covering 0.3–2 MHz;
  timer clock stays ≤ 7.5 MHz slow / ≤ 50 MHz at 2 MHz — both fine). This is why a *fixed*
  `mult=8` self-aligns at 1 MHz (`tick=125 ≤ 135`) but would reintroduce turnaround
  contention below 1 MHz under the late-write scheme.
- **With the rule satisfied, "only the sample needs frequency compensation" is TRUE:** dir+data
  self-align to the rising edge (a fixed reference); only the TDO sample CCR needs the
  `S·tick ≤ T − (L_max+guard)` late placement.

### "Future bit" shift + priming

Because the end-of-cycle write configures the *next* slot, the dir/data buffers are the
**effect stream shifted left by one**:
- **slot 0 is primed** by a direct register write (`ODR = tms₀`, dir = OUTPUT) *before*
  `CounterResume()`;
- the DMA streams effect-slots 1…N-1 (transfer count `N-1`); the last cycle issues no write;
- the **sample (read) stream is NOT shifted** — it reads slot *c* during cycle *c*.

### Dir-first + TMS-staging = glitch-free turnaround

Copying the upcoming **TMS value into the TDO slot's ODR** makes the effect-ODR stream
`tms₀, tdi₀, tms₁, tms₁, tdi₁, tms₂, …` — the TDO slot pre-stages the next TMS level. So at
the TDO→TMS boundary the **data line never moves**, only direction flips IN→OUT. With
direction on the **higher-priority** DMA, the flip finds ODR already correct → no spurious
spike. On TDI→TDO, dir-first releases the bus promptly (data write lands into Hi-Z, harmless).

## Refactor — split `TimSbw` / `TimSbwSTLink`

Following the `WaveJtag.h` precedent (two sibling classes `Generator` / `GeneratorSTLinkPWM`,
no shared `bool` switch), the `kSeparateDirDma` template parameter is replaced by two classes:

- **`TimSbwSTLink`** (active) — single-pin PB14 path: separate full-CRH direction DMA, the
  read-back on PB12. The current implementation, with `kSeparateDirDma` hardwired true. The
  **only** live SBW instantiation (`SbwDev.tim.cpp`, STLinkV2; bluepill is `OPT_SBW_IMPL_OFF`,
  g431 builds no SBW).
- **`TimSbw`** (placeholder) — buffered/mux ("bluepill direction-switch") path. Reduced to a
  documented stub (`static_assert` fires only on instantiation) pending a fresh redesign around
  the v2 scheme; the legacy folded-BSRR code is recoverable from git history.

Step 1 (done) is the **structural split preserving today's early-write behavior** — a pure
refactor, compile-checked in VS, no v2 timing yet. The v2 late-write scheme above lands in
`TimSbwSTLink` only after the `T_settle` bench measurement and LA validation.

## Quick reference — constraints

```
write ok :  (C − D)·tick            ≥ L_max            (single BSRR / folded)
write ok :  (C − Dir)·tick          ≥ L_max  AND  Dir−D ≥ 127ns/tick   (single-pin, 2 DMAs)
read  ok :  S·tick + L_max          ≤ T − guard        (clears the rising edge)
read  ok :  S·tick + L_min          ≥ C·tick + T_settle (after target drives + settle)
exists   :  T/2                     >  jitter + T_settle (read window non-empty)
```

`L_max = 180 ns`, `L_min = 135 ns`, `jitter = 45 ns` (GD32F103, mult=8 — re-measure per MCU
with `OPT_TEST_TIM_DMA_TIMING`).
