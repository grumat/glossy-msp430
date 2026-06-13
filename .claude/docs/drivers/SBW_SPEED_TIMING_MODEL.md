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
              rising edge = slot start    falling edge = capture       next rising = slot end
              (host sets up SBWDIO here)  (target latches TMS/TDI;     (target releases TDO;
                    │                     target begins driving TDO)   host re-acquires bus)
                    ▼                             ▼                              ▼
SBWTCK              ┌─────────────────────────────┐                              ┌──────────
          ──────────┘                             └──────────────────────────────┘          
                                                  🢔─────────── Max 7µs ──────────🢖
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

### The read-side term `T_settle` — MEASURED (negligible)

The model's read side also depends on `T_settle` — how long the target's TDO drive takes to
settle on the read slot. **Bench result (2026-06-13, MSP-EXP430G2 LaunchPad Rev 1.4 — the
canonical SBW reference target, so this is the *expected use case*):** `T_settle ≈ 20–25 ns`.

Measured by `tools/sbw_tdo_settle.py` on a 200 MHz LA capture of the settle sweep (281 kHz =
PSC-rounded 300 kHz, mult=64, ~1110 IR-ID frames). The decisive observation is an **asymmetry**:
SBWDIO idles HIGH (it is the RST/NMI line with a 47 k pull-up), so the target only ever **pulls
it LOW** for 0-bits — actively, in ~20 ns — while 1-bits need no transition at all. The RC-slow
*rising* edge (one 355 ns event in 0.27 s) is **never on the read path**. So the read is
**immune to the reset-line RC** — even on the G2 LaunchPad's deliberately heavy ≈47 k/1 nF
reset network (τ≈47 µs on the pull-up), the read settle is 20 ns.

**Read-path note (STLinkV2):** the ≈20 ns is the *bus* (PB14) settle from the LA. The firmware
actually samples **PB12**, the bus echoed through a **74AVCH2T45** level converter (4 ns prop,
asymmetric `V_IH=2 V / V_IL=0.8 V` @3.3 V). The asymmetry is structurally harmless: the bus swings
**rail-to-rail actively** (PB14 drives it directly — the converter is read-path only — and the host
pre-drives the line to 3.3 V in every TMS/TDI slot, so a TDO=1 is solidly >2 V before the read;
TDO=0 is pulled to ~mV, far below 0.8 V). The converter's high `V_IH=2 V` would only bite a *passive*
RC recovery, which the active pre-drive eliminates. End-to-end (target → bus RC → converter → pin)
the effective read settle is ≈45 ns worst case (falling: bus to 0.8 V + 4 ns prop).

Therefore `T_settle (≈20–45 ns) << tick << L (135–180 ns) + jitter (45 ns)`: **the read-side wall
is firmware DMA latency, not the target/board RC or the level converter.** This settles the open question — the
~1.3–1.4 MHz ceiling is `jitter + L` (firmware), a better-isolated board cannot move it, and the
v2 late-write scheme is the lever. The RC *does* bite the **write-side turnaround** (driving the
line HIGH against the cap), which is exactly what v2's dir-first + late-write addresses — not
the read.

## Next steps

1. ✅ **Compensated phases + v2 late write** encoded in `TimSbwSTLink` (2026-06-13) — see the
   v2 status box below. The TDO sample is computed per grade by the `LatTicks` `constexpr`
   and re-placed in `ApplySpeed()`; the write is the grade-independent late write.
2. ✅ **`T_settle` measured** — negligible (≈20–45 ns), read is firmware-bound (see above).
3. **Bench-validate v2** on the STLinkV2 + EXP430G2: confirm clean reads at 0.3 → 1.3 MHz,
   LA-check the dir-first turnaround (no glitch on the TDO→TMS boundary) and the Hi-Z scan
   end (bus not driven low between frames). The 300 kHz floor has only ~1.7 ns of
   `L_min − tick` margin at `mult=25`; if a genuine STM32F103 measures `L_min < 135 ns`,
   raise `kMult` (or `OPT_SBW_DMA_LAT_MIN_NS`) so the no-contention assert holds.
4. **Re-grade `SBW_Speed_*`** around a **1.5 MHz** top rather than 2 MHz once v2 is proven;
   honour the flash-clock floor already documented (see `project_sbw_speed_limit`).

## v2 scheme — latency-aligned late write (IMPLEMENTED 2026-06-13 in `TimSbwSTLink`)

> **Status:** landed in `Firmware.shared/util/TimSbw.h` (`TimSbwSTLink`), bench-validation
> pending. Default `kMult` raised 8 → **25** (covers 0.3–2 MHz, `mult ≥ T/L_min`,
> static_assert'd). The write triggers (data BSRR + dir CRH) fire at `kPhaseWrite_ =
> kCycleTicks_-1`; the buffers are shifted by one with slot 0 primed in `Start()`; both
> the data and direction DMAs run `N-1` transfers (non-circular dir) so the bus ends Hi-Z;
> `DirDma` is raised to top priority for the dir-first turnaround; the TDO slot pre-stages
> the next TMS into the data ODR; and the TDO **sample** is the only per-grade phase,
> recomputed by a `constexpr` (`LatTicks`) and re-placed in `ApplySpeed()`.

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

## Measuring T_settle (`OPT_SBW_TDO_SETTLE_SWEEP`)

A default-off bench probe (`stdproj.h` → `SbwDev::DoTdoSettleSweep`, gated by
`OPT_SBW_TDO_SETTLE_SWEEP`) that measures the read-settle on the attached target with
**no external probe**, reusing the SBW read path itself.

**Principle.** The IDR sample DMA latches `~L` (135–180 ns, from the timer→DMA probe)
*after* its compare. So sweeping the **sample compare** across the low phase — and a few
ticks *before* the SBWCLK fall — moves the *effective* sample from the fall outward. Read a
known stimulus (the JTAG ID, auto-presented on every IR scan) `N` times per phase; the
earliest phase that reads reliably marks where the effective sample first clears settle:

```
eff(P)   = (P − kPhaseClk_)·tick + L          (effective sample offset from the fall)
T_settle ≈ eff_lo at the first phase with ok = N/N      (using L_min)
         ∈ ( eff_hi of the last ok=0 , eff_lo of the first ok=N )   — band ≈ jitter
```

Because the effective sample can be pushed back **to** the fall (compare placed `L/tick`
before it), even a sub-`L` settle on a fast chip is resolvable — down to the ~45 ns latency
jitter floor. If the read is already reliable at the earliest swept phase, `T_settle` is
below that floor and the read-side wall is **jitter + L**, not settle (then the v2
late-write scheme is the lever, not the board).

**Resolution** comes from a high multiplier at a low wire frequency: the probe instantiates
a dedicated `TimSbwSweep` (`TimSbwSTLink` with `kMult = OPT_SBW_TDO_SETTLE_MULT`, default 64,
at `OPT_SBW_TDO_SETTLE_FREQ`, default 300 kHz → tick ≈ 52 ns, low phase ≈ 32 steps). Timer
clock = `mult·f` (≈ 19 MHz), well under the 72 MHz ceiling.

**Procedure.** Set `OPT_SBW_TDO_SETTLE_SWEEP=1` **and** `OPT_BARE_RUN=OPT_BARE_RUN_SBW` in
`target.stlinv2/platform.h`. The latter is the "scan ASAP" backdoor: `main()` does
`SetTransport(kSbw)` + `Open()` autonomously at power-up, so `OnConnectJtag` (and the probe)
fires with **no GDB host**. (In the default GDB mode `Open()`/`OnConnectJtag` only run when a
monitor `sbw_scan`/connect command asks — `OPT_HARD_SELECT_SBW_TMP` now only picks the *default*
transport for that path, it no longer auto-scans.) Rebuild in VS, attach the target + TRACESWO,
power up. The probe enters the TAP, resets, then traces one line per sample phase:

```
P<n>  d<ticks-from-fall>  t<ns>  eff<lo..hi ns>  ok<k/N>  id0x<val>
```

Read down to the first `ok = N/N`; its `eff_lo` is `T_settle`. Verify `golden` equals the
expected JTAG ID first (a `0xFF` golden = no target / not acquired). Update the `L` band
(`OPT_SBW_DMA_LAT_MIN/MAX_NS`) per MCU from `OPT_TEST_TIM_DMA_TIMING`. The probe halts after
the sweep.

**Host-side alternative — `tools/sbw_tdo_settle.py`.** If you put an LA on SBWDIO+SBWCLK while
the sweep runs, this stdlib script reads the 2-channel transition CSV and measures `T_settle`
straight off the bus: it isolates target TDO-drive edges as SBWDIO transitions that land in the
**low phase** (after a CLK fall — host TMS/TDI is set up *before* the fall), splits them into
active-LOW vs RC-HIGH, and histograms the fall→edge delay. This is the measurement that produced
the ≈20 ns G2-LaunchPad result above, and it needs no SWO decode.

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
