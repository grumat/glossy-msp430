---
name: analyze-jtag-la
description: >-
  Evaluate a logic-analyzer (LA) capture of the 4-wire JTAG bus (TMS/TCK/TDI/
  TDO/TRST) exported as a Saleae-style transition CSV. Use whenever asked to
  look at, decode, or check timing/alignment in a JTAG LA capture — e.g.
  "check the LA capture", "is TMS centered on the clock", "decode this
  jtag_scan capture", "what does the logic analyzer show for frame N",
  cnt_offset/TMS-phase tuning work, or verifying a fix against a fresh
  capture. Covers the fixed CSV column mapping, the existing TAP-FSM decoder
  script, how to segment individual JTAG frames out of a transition export,
  and the recipe for measuring TMS-to-TCK phase/setup-margin used for
  DtrigJtag `cnt_offset` calibration. Does NOT apply to Spy-Bi-Wire (SBW)
  captures — see the `decode_sbw*.py` scripts for that instead.
---

# Analyzing JTAG logic-analyzer captures

## Where captures live

- Default folder: `P:\github.com\glossy-msp430\supp\docs-ai`
- Files are `.csv`, exported by the LA software as a **transition export**:
  each row is a signal-change event, not a fixed-rate sample. Consecutive
  rows can be nanoseconds or milliseconds apart — don't assume a uniform
  sample interval.
- Unless the user names a different file, the most recent/relevant capture
  is `test-logic-analyzer.csv` in that folder.
- This is under `supp/`, which CLAUDE.md normally says not to scan by
  default — that policy doesn't apply here since the user is explicitly
  pointing at this folder for LA work.

## CSV column mapping (fixed — this is how the LA software always exports it)

| Column | Signal | Notes |
|---|---|---|
| `Time[s]` | — | Always seconds, regardless of what unit (µs/ms/s) the LA GUI displays at the current zoom level. |
| `Channel 0` | **TMS** (JTMS) | Usually timer output (PWM) or DMA-driven bit-banging. |
| `Channel 1` | **TCK** (JTCK, "CLK") | Usually the SPI1 clock signal. **Never** JTCLK/TCLK — that's a different, commonly-confused signal name (TCLK is the MSP430 target clock, not this). |
| `Channel 2` | **TDI** (JTDI) | Output from the probe, input to the target. Usually SPI1 MOSI. |
| `Channel 3` | **TDO** (JTDO) | Input to the probe. Usually SPI1 MISO. |
| `Channel 4` | **RST** (TRST) | Bit-banged output, tied to the target MCU's reset pin. Present on all targets. |
| `TEST` | **TEST** (TTEST) | Bit-banged output. Controls JTAG bus enable on the target; floating/"open" on older target MCUs. |

**Warning: this mapping and the JTAG decoder do not apply to SBW captures.**
SBW uses a single bidirectional SBWTDIO pin plus SBWTCK — there's no
separate TDI/TDO/TCK to map. SBW captures have their own decoders
(`decode_sbw.py`, `decode_sbw2.py`, `decode_sbw_fsm.py` in the same folder).
Don't reach for this skill's channel table on an SBW capture.

## Existing tooling

`supp/docs-ai/decode_jtag_fsm.py` is a working IEEE-1149.1 TAP-FSM decoder:
walks the standard TAP state machine off TCK rising edges (TMS/TDI sampled
on rising, per spec), reconstructs every IR/DR shift with its captured value
(MSP430 IR length = 8 bits), and prints `(t, kind, nbits, value, ir_ctx)` for
each shift it found.

```bash
python decode_jtag_fsm.py [path-to-csv]   # defaults to test-logic-analyzer.csv in cwd
```

Use this first for "what did the TAP actually do / what value came back"
questions — it's already correct and tested, don't re-derive the state
table from scratch.

For anything the FSM decoder doesn't answer — timing margins, phase
alignment, frame segmentation, per-toggle-event latency — write a small
one-off Python script against the CSV directly (a system Python is on PATH,
e.g. `C:\Python314\python.exe`). Load rows with `csv.reader`, skip the
header, cast columns to `float`/`int`. Because it's a transition export,
detecting an edge is just "does this column differ from the previous row" —
there is no fixed sample rate to reason about.

## Segmenting individual JTAG frames

Each `DtrigJtag`/`JtagDev` transaction clocks TCK continuously for the whole
frame, then TCK goes idle (no toggling) until the next frame starts. So:

1. Collect all TCK rising-edge timestamps.
2. Group them where the gap to the next rising edge exceeds some threshold
   comfortably larger than one JTCK period at the speed grade under test
   (e.g. `>2µs` works for the slower grades; tighten it for `kFast`/
   `kFastest` captures where a real inter-edge gap is much shorter).
3. Each group's edge count tells you the scan type: `DtrigJtag.h`'s
   `kTotalClocks = kSpiBytes * 8` where `kSpiBytes = ceil(kBitCount/8)` and
   `kBitCount = 5 + kNumBits + (uint8_t)kScan` for DR/IR (or a fixed 8 for
   GoIdle). E.g. 24 rising edges → 3 SPI bytes → `kBitCount` in 17..24 →
   a 16-bit DR shift (`5+16+0=21`) is the common case that lands there.

This lets you find "the Nth frame" or "the 16-bit DR frame" without decoding
the full FSM, just by counting edges per group.

## Recipe: measuring TMS-to-TCK phase / setup margin (cnt_offset tuning)

This is the recipe used for `DtrigJtag`'s per-speed-grade `cnt_offset`
calibration (see `Firmware.shared/util/DtrigJtag.h` and
`Firmware.shared/drivers/JtagDev.dtrig.cpp`'s `kDtrigCntOffset_*` table).

TMS only needs to be stable around the TCK **rising** edge (that's the only
edge the TAP samples on); the falling edge doesn't matter for TMS. Since the
SPI-generated JTCK is ~50% duty, "centered" for a TMS transition means
**landing on/near the TCK falling edge** — that's the point equidistant
between the preceding and following rising edges, giving maximum symmetric
setup margin into the next sample.

For each TMS transition of interest:
1. Find the nearest preceding and following TCK rising edges.
2. `frac = (t_tms - t_prior_rise) / (t_next_rise - t_prior_rise) * 100` — a
   result near 50% means well-centered; > 50% means it landed after the
   ideal falling-edge point (still safe, since it biases margin *toward*
   the actual sampling edge); < 50% would mean it's encroaching on the
   *previous* cycle's sample, which is the direction that actually matters.
3. Do this separately for **every** toggle event in the frame — entry pulse
   (Sel-DR/Sel-IR, driven directly off the critical-section-synchronized
   timer start) and exit pulse (Exit1+Update, driven by a mid-frame
   CC3-triggered DMA reload of CCR2) can land at meaningfully different
   phases, because they go through different latency chains. A `cnt_offset`
   fix derived only from the entry pulse says nothing about the exit
   pulse's margin — check both.
4. A fixed real-time latency (CPU cycles, DMA arbitration) shows up as a
   roughly-constant nanosecond skew across speed grades, but that same skew
   is a shrinking fraction of the period at slow grades and a growing one at
   fast grades — a comfortable margin at `kSlowest` can evaporate entirely
   by `kFastest`. Don't assume a good result at one grade generalizes; check
   each grade's capture independently, especially the exit-pulse timing.
