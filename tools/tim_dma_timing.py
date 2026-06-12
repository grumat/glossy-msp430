#!/usr/bin/env python3
"""Analyze a logic-analyzer capture from the OPT_TEST_TIM_DMA_TIMING firmware probe.

The probe (Firmware.shared/util/TimDmaTiming.h) emits 100 repetitions of:

        ____            anchor = SBWCLK falling edge (timer compare)
    ___|    |_______    SBWCLK
              _
    _________| |_____   SBWDIO   (pulse straddling the SBWCLK falling edge)

Two quantities are extracted per repetition, then summarized statistically:

  * t_lag  = (first SBWDIO edge after the SBWCLK fall) - (SBWCLK fall)
             -> the timer compare -> DMA-request -> BSRR-write latency ("anchor lag").
  * t_gap  = (second SBWDIO edge) - (first SBWDIO edge)
             -> the gap between the two concurrently-triggered DMA channels.

Run the firmware once with OPT_TEST_TIM_DMA_TIMING=1 (normal channel order) and once
with =2 (swapped set/reset roles), capturing SBWCLK + SBWDIO each time. SBWDIO pulses
HIGH in mode 1 and LOW in mode 2; this script keys off the SBWCLK falling edge and the
*next two SBWDIO transitions*, so it is polarity-agnostic and handles both runs.

Input: a CSV exported from the logic analyzer with a time column (seconds) and two
digital channels. Saleae Logic 2 "Export Data" (per-sample) works directly. Columns may
be named (auto-detected from "clk"/"dio" substrings) or selected by --clk-col/--dio-col
(header name or 0-based index).

Pure standard library (csv, statistics, argparse) — no third-party deps.

Examples:
    python tools/tim_dma_timing.py capture_mode1.csv
    python tools/tim_dma_timing.py cap.csv --clk-col "Channel 1" --dio-col "Channel 0"
    python tools/tim_dma_timing.py cap.csv --clk-col 1 --dio-col 2 --time-col 0
"""

import argparse
import csv
import math
import statistics
import sys


def _resolve_col(header, spec, default_idx, kind):
    """Return a 0-based column index from a header-name-or-index spec."""
    if spec is None:
        return default_idx
    # Integer index?
    try:
        return int(spec)
    except (TypeError, ValueError):
        pass
    # Header name (exact, then case-insensitive substring).
    if spec in header:
        return header.index(spec)
    low = [h.lower() for h in header]
    if spec.lower() in low:
        return low.index(spec.lower())
    for i, h in enumerate(low):
        if spec.lower() in h:
            return i
    sys.exit(f"error: could not find {kind} column {spec!r} in header {header}")


def _autodetect(header, *substrings):
    low = [h.lower() for h in header]
    for i, h in enumerate(low):
        if any(s in h for s in substrings):
            return i
    return None


def load(path, time_spec, clk_spec, dio_spec):
    with open(path, newline="") as fh:
        reader = csv.reader(fh)
        rows = list(reader)
    if not rows:
        sys.exit("error: empty CSV")

    # Detect a header row (first cell not parseable as float).
    header = rows[0]
    has_header = False
    try:
        float(header[0])
    except (ValueError, IndexError):
        has_header = True
    data_rows = rows[1:] if has_header else rows
    if not has_header:
        header = [f"col{i}" for i in range(len(rows[0]))]

    t_col = _resolve_col(header, time_spec, 0, "time")
    c_col = clk_spec if clk_spec is not None else _autodetect(header, "clk", "sbwclk", "tck")
    d_col = dio_spec if dio_spec is not None else _autodetect(header, "dio", "sbwdio", "tms")
    c_col = _resolve_col(header, c_col, 1, "clk")
    d_col = _resolve_col(header, d_col, 2, "dio")

    samples = []
    for r in data_rows:
        if len(r) <= max(t_col, c_col, d_col):
            continue
        try:
            t = float(r[t_col])
            clk = 1 if float(r[c_col]) >= 0.5 else 0
            dio = 1 if float(r[d_col]) >= 0.5 else 0
        except ValueError:
            continue
        samples.append((t, clk, dio))
    if len(samples) < 2:
        sys.exit("error: fewer than 2 usable samples (check column selection)")
    return samples, (header[t_col], header[c_col], header[d_col])


def edges(samples, ch_index, direction=None):
    """Timestamps where channel ch_index transitions. direction: 'fall', 'rise', or None."""
    out = []
    prev = samples[0][ch_index]
    for s in samples[1:]:
        cur = s[ch_index]
        if cur != prev:
            if direction is None \
               or (direction == "fall" and cur == 0) \
               or (direction == "rise" and cur == 1):
                out.append(s[0])
            prev = cur
    return out


def summarize(name, unit_scale, unit, values):
    if not values:
        print(f"  {name:<28} (no measurements)")
        return
    vals = [v * unit_scale for v in values]
    mn, mx, mean = min(vals), max(vals), statistics.fmean(vals)
    sd = statistics.pstdev(vals) if len(vals) > 1 else 0.0
    print(f"  {name:<28} n={len(vals):>4}  "
          f"min={mn:8.2f}  max={mx:8.2f}  mean={mean:8.2f}  "
          f"std={sd:7.2f}  jitter(max-min)={mx-mn:8.2f}  [{unit}]")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv", help="logic-analyzer CSV export (time in seconds + 2 digital channels)")
    ap.add_argument("--time-col", default=None, help="time column (name or index; default 0)")
    ap.add_argument("--clk-col", default=None, help="SBWCLK column (name or index; default auto/1)")
    ap.add_argument("--dio-col", default=None, help="SBWDIO column (name or index; default auto/2)")
    ap.add_argument("--clk-edge", choices=("fall", "rise"), default="fall",
                    help="SBWCLK anchor edge (default: fall, matches the probe's idle-high PWM)")
    args = ap.parse_args()

    samples, (tn, cn, dn) = load(args.csv, args.time_col, args.clk_col, args.dio_col)

    # Timing resolution: GCD of all timestamp deltas (in ps). Robust for both
    # per-sample exports (= sample period) and transition-list exports (where the
    # row spacing is irregular but every edge still lands on the timebase grid).
    ps = [round(s[0] * 1e12) for s in samples]
    g = 0
    for i in range(len(ps) - 1):
        d = ps[i + 1] - ps[i]
        if d > 0:
            g = math.gcd(g, d)
    res_ns = (g / 1000.0) if g else float("nan")

    anchors = edges(samples, 1, args.clk_edge)          # SBWCLK chosen edge
    dio_edges = edges(samples, 2, None)                 # all SBWDIO transitions

    lags, gaps = [], []
    j = 0
    for k, a in enumerate(anchors):
        nxt = anchors[k + 1] if k + 1 < len(anchors) else float("inf")
        while j < len(dio_edges) and dio_edges[j] <= a:
            j += 1
        # first SBWDIO edge after the anchor, within this cycle
        if j < len(dio_edges) and dio_edges[j] < nxt:
            e1 = dio_edges[j]
            # second SBWDIO edge, still within this cycle
            if j + 1 < len(dio_edges) and dio_edges[j + 1] < nxt:
                e2 = dio_edges[j + 1]
                lags.append(e1 - a)
                gaps.append(e2 - e1)

    print(f"file        : {args.csv}")
    print(f"columns     : time={tn!r}  clk={cn!r}  dio={dn!r}")
    print(f"rows        : {len(samples)}   timing resolution (GCD) ~ {res_ns:.3f} ns"
          + (f" ({1000.0/res_ns:.0f} MHz)" if res_ns == res_ns and res_ns > 0 else ""))
    print(f"anchors     : {len(anchors)} SBWCLK {args.clk_edge} edges, "
          f"{len(lags)} complete pulses matched\n")
    summarize("compare->DMA->BSRR lag", 1e9, "ns", lags)
    summarize("inter-DMA gap (pulse width)", 1e9, "ns", gaps)
    if lags:
        print(f"\n  In timer ticks (mult/cycle), divide by the tick period; at the "
              f"configured wire freq one tick = 1/(mult*f).")


if __name__ == "__main__":
    main()
