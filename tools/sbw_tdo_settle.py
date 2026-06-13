#!/usr/bin/env python3
"""Extract the SBW TDO read-settle time from a 2-channel LA capture (SBWDIO+SBWCLK).

The target drives TDO *on/after* the SBWCLK falling edge (SLAU320 Fig 2-8), while
the host sets up TMS/TDI *before* the fall. So a SBWDIO transition that lands in
the LOW phase (after a CLK fall, before the next CLK rise) is a target TDO-drive
edge; its delay from the fall is the settle (threshold-crossing) time. Host TMS/TDI
edges land in the HIGH phase and are excluded automatically.

Input: Saleae-style transition CSV "Time[s], <dio>, <clk>" (header auto-detected).
Usage: python sbw_tdo_settle.py capture.csv
"""
import csv, sys, statistics as st

def load(path):
    rows = []
    with open(path, newline="") as f:
        r = csv.reader(f)
        hdr = next(r)
        # find dio / clk columns by name; default cols 1,2
        dio_i, clk_i = 1, 2
        for i, name in enumerate(hdr):
            n = name.strip().lower()
            if "dio" in n: dio_i = i
            if "clk" in n or "tck" in n: clk_i = i
        for row in r:
            if len(row) <= max(dio_i, clk_i): continue
            try:
                t = float(row[0]); dio = int(row[dio_i]); clk = int(row[clk_i])
            except ValueError:
                continue
            rows.append((t, dio, clk))
    return rows

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "test-logic-analyzer.csv"
    rows = load(path)
    if not rows:
        print("no rows"); return

    # Edge lists (transition list already, but rebuild from level changes to be safe).
    clk_fall, clk_rise, dio_edge = [], [], []
    pdio, pclk = rows[0][1], rows[0][2]
    for t, dio, clk in rows[1:]:
        if clk != pclk:
            (clk_rise if clk == 1 else clk_fall).append(t)
        if dio != pdio:
            dio_edge.append((t, dio))
        pdio, pclk = dio, clk

    # Dominant clock period from consecutive rising edges (active region only).
    rises = clk_rise
    deltas = sorted(rises[i+1]-rises[i] for i in range(len(rises)-1))
    # ignore the big inter-frame / entry gaps: take the median of the small ones
    small = [d for d in deltas if d < 5e-6]
    period = st.median(small) if small else (deltas[len(deltas)//2] if deltas else 0)
    print(f"rows={len(rows)} clk_fall={len(clk_fall)} clk_rise={len(rises)} dio_edges={len(dio_edge)}")
    print(f"SBWCLK period ~ {period*1e9:.0f} ns  => wire ~ {1/period/1e3:.0f} kHz (low phase ~ {period/2*1e9:.0f} ns)")

    # For each falling edge, find the first DIO edge before the next rising edge.
    import bisect
    rise_ts = rises
    de_ts = [t for t, _ in dio_edge]
    settle = []
    for f in clk_fall:
        ri = bisect.bisect_right(rise_ts, f)
        nxt_rise = rise_ts[ri] if ri < len(rise_ts) else f + period  # window end
        di = bisect.bisect_right(de_ts, f)
        if di < len(de_ts):
            te = de_ts[di]
            if te < nxt_rise:                 # DIO edge inside this low phase
                settle.append((te - f, dio_edge[di][1]))
    if not settle:
        print("no post-fall DIO edges found"); return

    vals = sorted(d for d, _ in settle)
    rise01 = [d for d, lvl in settle if lvl == 1]   # target drove HIGH (RC-limited)
    fall10 = [d for d, lvl in settle if lvl == 0]   # target drove LOW (active)
    def stats(x):
        if not x: return "n=0"
        x = sorted(x)
        p = lambda q: x[min(len(x)-1, int(q*len(x)))]
        return (f"n={len(x)} min={x[0]*1e9:.0f} med={st.median(x)*1e9:.0f} "
                f"mean={st.mean(x)*1e9:.0f} p95={p(0.95)*1e9:.0f} max={x[-1]*1e9:.0f} ns")
    print(f"\nTDO-drive settle (CLK fall -> first SBWDIO edge in the low phase):")
    print(f"  all   : {stats(vals)}")
    print(f"  ->HIGH: {stats(rise01)}   (target drove 1; RC/pull-up edge = worst case)")
    print(f"  ->LOW : {stats(fall10)}   (target drove 0; actively driven = fast)")

    # crude histogram (ns buckets)
    print("\nhistogram (ns):")
    buckets = [0,20,40,60,80,100,150,200,300,500,800,1200,2000]
    counts = [0]*(len(buckets))
    for d, _ in settle:
        ns = d*1e9
        for i in range(len(buckets)-1, -1, -1):
            if ns >= buckets[i]:
                counts[i] += 1; break
    for i, b in enumerate(buckets):
        hi = buckets[i+1] if i+1 < len(buckets) else float("inf")
        if counts[i]:
            print(f"  [{b:>4}..{hi:>4}) {'#'*min(60,counts[i])} {counts[i]}")

if __name__ == "__main__":
    main()
