# SBW Driver — GitHub Issue Drafts

Drafts to paste into `grumat/glossy-msp430` issues. Each `## Issue N` block is one
issue. Title goes on the first line after the heading; everything below the title
up to the next `## Issue` is the body.

Recommended labels: `enhancement`, `driver`, `sbw`. Suggested milestone: *SBW MVP*.

Suggested order: epic first (Issue 0), then 1 → 5 as dependencies for it.

> **Status (2026-05-25):** #1, #2, #3 are **done** (implemented, compiles clean on
> STLinkV2 via MSBuild; commits `51789d6`, `7dc6730`). #4 is **partially done**
> (dispatch + sovereign Init in place; final runtime mode selector still a
> temporary compile-time flag). #5 (bench validation) is **open** — needs
> hardware. Note the original drafts below predate the implementation: several
> "TODO/stub/Option 1 only/critical section" notes are now superseded; the
> per-issue **DONE** banners record what actually shipped.

---

## Issue 0 — Epic: Spy-Bi-Wire protocol support

**Title:** Epic — Spy-Bi-Wire (SBW) protocol support

**Body:**

Track the work to bring up an independent Spy-Bi-Wire transport layer alongside
the existing 4-wire JTAG driver. SBW is required for two-wire MSP430 parts
(F5xx/F6xx in SBW-only packages, FR series) and for board layouts where JTAG
pins are not all broken out.

### Design summary

- Mirror the `DtrigJtag` template pattern with a new `TimSbw` (already
  drafted as a skeleton at `Firmware.shared/util/TimSbw.h`).
- Same async contract: `OnXxxShift()` returns `JtagPending<T>`; render(N+1)
  overlaps DMA(N).
- 5 MHz SBWCLK ceiling (MSP430 spec), TIM1_CH1 PWM drives SBWCLK, BSRR DMA
  script drives SBWTDIO and direction.
- Each logical JTAG bit → 3 SBWCLK cycles (TMS, TDI, TDO-sample).

### Resource model

JTAG and SBW share TIM1 + GPIO + DMA channels and cannot run concurrently.
`TapMcu::Open()` picks one protocol and calls **exactly one** `Init()` for the
session. Switching protocols requires close → reopen.

Adopted **"Init() is sovereign"**: every driver's `Init()` unconditionally
reconfigures GPIO + RCC + DMA regardless of who owned the resources before.
No claim/release dance.

### Sub-tasks

- [x] #1 — `SbwDev` driver class + skeleton TUs
- [x] #2 — `TimSbw` encoder: BSRR script + IDR sample + direction script
- [x] #3 — Mid-frame SBWTDIO direction turnaround (full-CRH single-pin + PA9 mux)
- [ ] #4 — `TapMcu` mode selection and Init() arbitration *(dispatch done; runtime selector pending)*
- [ ] #5 — Bench validation against MSP430 reference targets *(needs hardware)*

### References

- Design doc: `.claude/docs/drivers/TIM_SBW_DRIVER.md`
- JTAG counterpart for comparison: `.claude/docs/drivers/DTRIG_JTAG_DRIVER.md`
- TI slau320 (SBW timing section)

---

## Issue 1 — SbwDev driver class + skeleton TUs

> **DONE (2026-05-25).** Build options defined; `OnEnterTap()` implemented (real
> slau320 entry sequence, no longer a stub); `JtagPending<T>` drain-previous-frame
> plumbing in place; compiles against `ITapInterface` (verified on STLinkV2,
> contract present on Bluepill). The "stub/placeholder" notes below are stale.

**Title:** SBW: add `SbwDev` driver class mirroring `JtagDev`

**Body:**

Create the SBW counterpart to `JtagDev` so `TapDev430*` protocol layers can sit
on either transport without rewriting protocol code.

### Files (already added)

- `Firmware.shared/drivers/SbwDev.h` — class declaration, `ITapInterface`
  overrides, ping-pong buffer of `uint32_t` (BSRR scripts + IDR samples).
- `Firmware.shared/drivers/SbwDev.cpp` — backend-independent common methods
  (`OnEnterTap`, `OnInstrLoad`, `OnTclk`, `OnData16`, `OnReadJmbOut`,
  `OnWriteJmbIn16`). Currently `OnEnterTap()` is a TODO stub.
- `Firmware.shared/drivers/SbwDev.tim.cpp` — backend-specific stubs guarded
  by `OPT_INCLUDE_SBW_TIM_`. All `OnXxx` methods are empty placeholders.
- Project files updated: `Firmware.shared.vcxitems(.filters)` and all four
  `target.*/target.*.vcxproj(.filters)` reference the new TUs.

### Scope of this issue

- [x] Define `OPT_INCLUDE_SBW_TIM_`, `OPT_SBW_IMPLEMENTATION`,
      `OPT_SBW_BUFFER_CNT_` in `platform-defs.h` and per-target `platform.h`.
- [x] Implement `SbwDev::OnEnterTap()` per slau320 SBW entry sequence.
- [x] Wire `JtagPending<T>` plumbing (drain-previous-frame on each new shift).
- [x] Verify the class compiles against `ITapInterface` (STLinkV2 build clean;
      Bluepill contract present; G431/Nucleo carry `OPT_SBW_IMPL_OFF`).

### Out of scope

- Actual SBW frame encoding → #2
- Mid-frame turnaround timing → #3
- Mode arbitration in `TapMcu` → #4

---

## Issue 2 — TimSbw frame encoder

> **DONE (2026-05-25).** All methods implemented and build clean. Deviations from
> the draft: (a) a straightforward per-bit loop is used, not the pack-and-`__REV`
> LUT — functionally complete, optimization is an optional follow-up; (b) `Start()`
> has **no critical section** — that was a dtrig-model assumption; the timdma model
> has no competing peripheral to phase-lock. Remaining: `cnt_offset` calibration
> (all 0) → tracked under #5 (bench).

**Title:** SBW: implement `TimSbw` BSRR/IDR/direction encoder

**Body:**

Populate the method bodies in `Firmware.shared/util/TimSbw.h`. The template
surface and per-method TODOs are already drafted.

### Encoding model

Each JTAG bit `i` expands to 3 SBWCLK cycles:

- `3i+0` — drive SBWTDIO with TMS_i (output)
- `3i+1` — drive SBWTDIO with TDI_i (output)
- `3i+2` — release SBWTDIO, sample TDO_i (input)

So `kJtagBits` → `kSbwCycles = 3 × kJtagBits` cycles, and one TIM1 single-shot
covers the full scan via RCR.

### Tasks

- [x] `RenderTransaction()` — per-bit loop expands into 3 BSRR words/bit. (Used a
      plain loop, not the pack-and-`__REV` LUT — optional optimization, see banner.)
- [x] `DoGoIdle()` — render 6× TMS=1, 1× TMS=0 reset frame.
- [x] `Start()` — arm the DMAs (data BSRR, direction full-CRH, IDR sample), preset
      `CNT = kTimerPeriod - cnt_offset`, release the timer. **No critical section**
      (timdma model — nothing competes for the bus). On the single-pin board the
      direction DMA is a separate full-CRH write; on buffered boards the dir bit is
      folded into the data BSRR (no third DMA).
- [x] `Wait()` — drain auto-stop + final IDR DMA, then disable all (incl. dir DMA).
- [x] `GetResult()` — pick every 3rd IDR sample (the `3i+2` slot), pack
      MSB-first into the result word, apply the payload-window mask.
- [x] Compile-time checks (DMA channels distinct, `kSbwCycles` ≤ buffer size,
      dir/data/sample channels distinct on the single-pin path).

### Calibration

LA-validated `cnt_offset` per speed grade goes into `SbwDev.tim.cpp` like
the `kDtrigCntOffset_N` constants in JTAG.

Depends on: #1, #3 (direction strategy).

---

## Issue 3 — Mid-frame SBWTDIO direction turnaround

> **DONE (2026-05-25), implementation.** Outcome differs from the single-option
> draft below: **two** `DirPolicy` strategies ship, picked per board class:
> - **Buffered boards** (Bluepill/G431): PA9/PA10 BSRR mux — the dir bit folds
>   into the data BSRR DMA (`DirPolicy_BsrrMux_GPIOA`). ≈ draft Option 1.
> - **Single-pin boards** (STLinkV2, PB14): a separate **full-CRH DMA** writes the
>   whole `GPIOB->CRH` once per cycle from a data-independent dir-script
>   (`DirPolicy_FullCrh<DriveGroup,ReleaseGroup>`, CRH words derived from
>   `AnyPinGroup`). This is the refined Option 3 — bit-band was rejected because
>   linear DMA can't rewind to the same alias words for per-bit turnaround.
>
> Remaining (→ #5, bench): validate turnaround latency on real silicon (LA) and
> record the calibrated timing window.

**Title:** SBW: choose and implement mid-frame SBWTDIO direction strategy

**Body:**

SBW's single bidirectional SBWTDIO line is the novel piece vs JTAG — the
shift engine must release its driver and sample open-drain during the slave's
turn. JTAG has separate TDI/TDO lines and never flips GPIO direction
mid-frame, so this is unique to SBW.

### Strategies under consideration

**Option 1 — PA9 hardware mux (preferred):**

Direction encoded as a single BSRR bit folded into the same DMA word as
SBWTDI. Timer phases shift-window and mux trigger to PA9, allowing atomic
direction switching without software intervention.

- ✅ Fully autonomous, zero latency
- ✅ Matches DtrigJtag encoding style exactly
- ❌ Requires dedicated hardware mux pin (PA9 — already in pin plan)

**Option 2 — GPIO timing:**

Software flips CRL/CRH directly with an inline delay tuned for the 5 MHz
phase.

- ✅ Simpler, no mux dependency
- ❌ ISR jitter; couples direction timing to CPU clock vs SBWCLK phase
- ❌ Not async — blocks the shift overlap pattern

**Option 3 — Pure DMA, staggered BSRR:**

A second DMA descriptor toggles the direction bit between shift phases,
fully autonomous but with no hardware mux.

- ✅ No GPIO write in CPU
- ❌ Tricky descriptor chaining; edge cases (slave violates timing, short
      turnaround) not handled cleanly

### Tasks

- [x] Pick the strategy — per-board: BSRR mux (buffered) + full-CRH DMA (single-pin).
- [x] Implement `DirPolicy` strategy classes (`DirPolicy_BsrrMux_GPIOA`,
      `DirPolicy_FullCrh`) as documented in `TimSbw.h` template parameters.
- [ ] Validate turnaround latency against MSP430 protocol margins on a
      reference target (FR2xx or F5xx in SBW mode) with a logic analyzer. *(→ #5)*
- [ ] Document the calibrated timing window in `TIM_SBW_DRIVER.md`. *(→ #5)*

### References

- slau320: "Master drives first 8 bits (TDI), then turnaround, then reads 8
  bits (TDO) in single continuous frame at 5 MHz"
- Pin plan in `.claude/docs/drivers/TIM_SBW_DRIVER.md`

Blocks: #2

---

## Issue 4 — TapMcu mode selection and Init() arbitration

> **PARTIAL (2026-05-25) — keep open.** `TapMcu::Open()` already dispatches to one
> of `jtag_device`/`sbw_device` and calls exactly one `OnOpen()`; both `Init()`s
> are sovereign. But selection is the **temporary compile-time flag**
> `OPT_HARD_SELECT_SBW_TMP` — the intended runtime mechanism (GDB `qRcmd`/monitor)
> is not yet decided/implemented. That decision is what remains.

**Title:** SBW: wire mode selection into `TapMcu::Open()`

**Body:**

`TapMcu::Open()` currently always brings up `JtagDev`. Extend it to pick one
of `JtagDev` / `SbwDev` based on a runtime or compile-time mode flag, calling
**exactly one** `Init()` per session.

### Init() is sovereign — design rule

Each driver's `Init()` unconditionally reconfigures every resource it owns
(GPIO modes, RCC enables, DMA channel setup, TIM1 channels), regardless of
the prior owner's state. **No claim/release dance** between protocols.

The contract is:

```
TapMcu::Open(mode):
    if (mode == kJtag) JtagDev::Init()  // wipes/reclaims everything
    else               SbwDev::Init()   // wipes/reclaims everything

TapMcu::Close():
    drain any in-flight DMA, tri-state pins.
    Switching protocols later = Close() + Open(other_mode).
```

This eliminates half-configured-channel hazards and matches the CLAUDE.md
"Allocate MCU resources wisely" rule (exclusive ownership > runtime
hand-off).

### Tasks

- [ ] Decide mode-selection mechanism (GDB command? probe-side autodetect?
      compile-time per target?). **← the remaining work** (temp flag
      `OPT_HARD_SELECT_SBW_TMP` is in place as a stopgap).
- [x] Add the dispatch in `TapMcu::Open()` (selects one ITapInterface, one OnOpen).
- [x] Audit `JtagDev::Init()` and `SbwDev::Init()` for sovereignty.
- [ ] Document the close/reopen requirement for protocol switching in
      `TIM_SBW_DRIVER.md`.

Depends on: #1, #2.

---

## Issue 5 — Bench validation

**Title:** SBW: bench-validate against MSP430 reference targets

**Body:**

Once the driver compiles and runs the encoder, validate end-to-end against
real MSP430 silicon in SBW mode.

### Targets

- [ ] MSP430FR2433 (SBW-only) — primary FR target
- [ ] MSP430F5418A (SBW capable) — cross-check vs known-good JTAG path
- [ ] MSP430G2553 (LaunchPad onboard) — convenient for desk validation

### Checks

- [ ] JTAG ID readout matches expected fuse-blown vs fuse-intact behaviour.
- [ ] Full flash erase + write + verify cycle using existing funclets.
- [ ] Read/write memory at various widths (8/16/20/32 bit shifts).
- [ ] Async overlap actually engages — LA capture should show DMA(N) and
      render(N+1) overlapping.
- [ ] Speed grade ramp: confirm 5 MHz upper bound holds, lower grades work
      from `BusSpeed::kSlowest`.
- [ ] Open/close/reopen with protocol switch (JTAG → SBW → JTAG) on the
      same hardware.

### Artifacts

- LA captures saved under `Firmware.shared/docs/sbw_captures/` (CSV per
  `reference_logic_analyzer_csv` channel mapping).
- Calibrated `cnt_offset` constants committed to `SbwDev.tim.cpp`.

Depends on: #1, #2, #3, #4.
