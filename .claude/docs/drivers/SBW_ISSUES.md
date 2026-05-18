# SBW Driver — GitHub Issue Drafts

Drafts to paste into `grumat/glossy-msp430` issues. Each `## Issue N` block is one
issue. Title goes on the first line after the heading; everything below the title
up to the next `## Issue` is the body.

Recommended labels: `enhancement`, `driver`, `sbw`. Suggested milestone: *SBW MVP*.

Suggested order: epic first (Issue 0), then 1 → 5 as dependencies for it.

---

## Issue 0 — Epic: Spy-Bi-Wire protocol support

**Title:** Epic — Spy-Bi-Wire (SBW) protocol support

**Body:**

Track the work to bring up an independent Spy-Bi-Wire transport layer alongside
the existing 4-wire JTAG driver. SBW is required for two-wire MSP430 parts
(F5xx/F6xx in SBW-only packages, FR series) and for board layouts where JTAG
pins are not all broken out.

### Design summary

- Mirror the `DtrigJtag` template pattern with a new `DtrigSbw` (already
  drafted as a skeleton at `Firmware.shared/util/DtrigSbw.h`).
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

- [ ] #1 — `SbwDev` driver class + skeleton TUs
- [ ] #2 — `DtrigSbw` encoder: BSRR script + IDR sample + direction script
- [ ] #3 — Mid-frame SBWTDIO direction turnaround (3 options to evaluate)
- [ ] #4 — `TapMcu` mode selection and Init() arbitration
- [ ] #5 — Bench validation against MSP430 reference targets

### References

- Design doc: `.claude/docs/drivers/DTRIG_SBW_DRIVER.md`
- JTAG counterpart for comparison: `.claude/docs/drivers/DTRIG_JTAG_DRIVER.md`
- TI slau320 (SBW timing section)

---

## Issue 1 — SbwDev driver class + skeleton TUs

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
- `Firmware.shared/drivers/SbwDev.dtrig.cpp` — backend-specific stubs guarded
  by `OPT_INCLUDE_SBW_DTRIG_`. All `OnXxx` methods are empty placeholders.
- Project files updated: `Firmware.shared.vcxitems(.filters)` and all four
  `target.*/target.*.vcxproj(.filters)` reference the new TUs.

### Scope of this issue

- [ ] Define `OPT_INCLUDE_SBW_DTRIG_`, `OPT_SBW_IMPLEMENTATION`,
      `OPT_SBW_BUFFER_CNT_` in `platform-defs.h` and per-target `platform.h`.
- [ ] Implement `SbwDev::OnEnterTap()` per slau320 SBW entry sequence.
- [ ] Wire `JtagPending<T>` plumbing (drain-previous-frame on each new shift).
- [ ] Verify the class compiles against `ITapInterface` for all four targets.

### Out of scope

- Actual SBW frame encoding → #2
- Mid-frame turnaround timing → #3
- Mode arbitration in `TapMcu` → #4

---

## Issue 2 — DtrigSbw frame encoder

**Title:** SBW: implement `DtrigSbw` BSRR/IDR/direction encoder

**Body:**

Populate the method bodies in `Firmware.shared/util/DtrigSbw.h`. The template
surface and per-method TODOs are already drafted.

### Encoding model

Each JTAG bit `i` expands to 3 SBWCLK cycles:

- `3i+0` — drive SBWTDIO with TMS_i (output)
- `3i+1` — drive SBWTDIO with TDI_i (output)
- `3i+2` — release SBWTDIO, sample TDO_i (input)

So `kJtagBits` → `kSbwCycles = 3 × kJtagBits` cycles, and one TIM1 single-shot
covers the full scan via RCR.

### Tasks

- [ ] `RenderTransaction()` — build packed (TMS, TDI) bit-planes using the
      pack-and-`__REV` trick from `DtrigJtag`, expand into 3 BSRR words per
      bit (small 8-entry LUT or inline expansion).
- [ ] `DoGoIdle()` — render 6× TMS=1, 1× TMS=0 reset frame.
- [ ] `Start()` — arm the three DMAs (data BSRR, direction CRx, IDR sample),
      preset `TIM1->CNT = kCntStart - cnt_offset`, enable in a critical
      section so all phase-lock to the first SBWCLK edge.
- [ ] `Wait()` — drain auto-stop + final IDR DMA, then disable all three.
- [ ] `GetResult()` — pick every 3rd IDR sample (the `3i+2` slot), pack
      MSB-first into the result word, apply the payload-window mask.
- [ ] Compile-time checks (DMA channels distinct, `kSbwCycles` ≤ buffer size).

### Calibration

LA-validated `cnt_offset` per speed grade goes into `SbwDev.dtrig.cpp` like
the `kDtrigCntOffset_N` constants in JTAG.

Depends on: #1, #3 (direction strategy).

---

## Issue 3 — Mid-frame SBWTDIO direction turnaround

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

- [ ] Pick the strategy (current plan: **Option 1**; pin plan already
      includes PA9).
- [ ] Implement `DirPolicy` strategy class as documented in `DtrigSbw.h`
      template parameters.
- [ ] Validate turnaround latency against MSP430 protocol margins on a
      reference target (FR2xx or F5xx in SBW mode) with a logic analyzer.
- [ ] Document the calibrated timing window in `DTRIG_SBW_DRIVER.md`.

### References

- slau320: "Master drives first 8 bits (TDI), then turnaround, then reads 8
  bits (TDO) in single continuous frame at 5 MHz"
- Pin plan in `.claude/docs/drivers/DTRIG_SBW_DRIVER.md`

Blocks: #2

---

## Issue 4 — TapMcu mode selection and Init() arbitration

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
      compile-time per target?).
- [ ] Add the dispatch in `TapMcu::Open()`.
- [ ] Audit `JtagDev::Init()` and `SbwDev::Init()` for sovereignty — confirm
      both force-disable all shared DMA channels before re-Setup().
- [ ] Document the close/reopen requirement for protocol switching in
      `DTRIG_SBW_DRIVER.md`.

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
- Calibrated `cnt_offset` constants committed to `SbwDev.dtrig.cpp`.

Depends on: #1, #2, #3, #4.
