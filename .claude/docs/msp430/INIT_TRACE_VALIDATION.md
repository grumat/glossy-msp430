# MSP430 Initialization-Trace Validation (transport bring-up)

> **Purpose:** collect Glossy's own **startup TRACE** dumps across every
> target + transport, to validate the **SBW** and (later) **JTAG** transport
> layers end-to-end. Each entry is a known-good (or known-bad) capture of the
> `Starting JTAG → identify → profile select → GDB reader loop` sequence.

This is the companion to [`../project/MSP430_WIRING_GUIDE.md`](../project/MSP430_WIRING_GUIDE.md):
the wiring guide says *how to connect* a board; this doc records *that the
connection works* (which MCU it resolves to, on which transport).

## How to contribute a dump

Raw dumps are **tracked in git** under [`INIT_TRACE_VALIDATION/`](INIT_TRACE_VALIDATION/)
(the folder beside this file) — *not* under `supp/docs-ai/`, which is gitignored.

**Naming convention** (unique + self-describing, one file per capture):

```
<mcu>_<transport>_<probe>_<kind>.txt        e.g.  f5418a_sbw_stlinkv2_init.txt
```

| Field | Values |
|-------|--------|
| `mcu` | short part number, lowercase — `f5418a`, `fr5994`, `g2553`, … |
| `transport` | `sbw` \| `jtag` |
| `probe` | `stlinkv2` \| `bluepill-g431` \| `bluepill` |
| `kind` | `init` (startup/identify); reserve `flash`, `run`, … for later |
| (dup) | append `_NN` if the same combo is captured twice — `…_init_02.txt` |

Steps:

1. Save the raw startup TRACE into `INIT_TRACE_VALIDATION/` using the name above.
2. Tell me the **probe** and the **transport** — the TRACE prints "Starting JTAG /
   JTAG identify path" for *both* transports and never names the probe, so those
   two facts must be tagged by hand (the filename now carries them).
3. I parse it into the matrix + an entry below.

## What the TRACE exposes (field key)

| Field | Meaning |
|-------|---------|
| `jtag_id` | TAP identifier — `0x91` = CPUXv2, `0x89` = CPUX, `0x..` legacy |
| `coreip_id` | Xv2 core IP block id |
| `device_id` / `id_data_addr` | Xv2 reads the real ID from the TLV at `id_data_addr` (usually `0x1a00`), not from `device_id` |
| `raw[0..3]` | the 4-word device signature read from `id_data_addr` → drives profile selection |
| profile flags | `[CPUXv2]`, `[EMEX_*]`, `[SLAUxxx]`, breakpoint count |

## Validation matrix

| MCU | Family / SLAU | Probe | Transport | jtag_id | coreip_id | raw signature | HW bkpts | Result | Dump |
|-----|---------------|:-----:|:---------:|:-------:|:---------:|---------------|:--------:|--------|------|
| MSP430F5418A | CPUXv2 / SLAU208 | **STLinkV2** | **SBW** | `0x91` | `0x0103` | `0606 2929 8000 1515` | 8 | ✅ identify + GDB loop | [`…/f5418a_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f5418a_sbw_stlinkv2_init.txt) |
| MSP430G2553 *(profile G2xx3)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x5325` @ `0x0ff0` | 2 | ✅ identify + GDB loop | [`…/g2553_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2553_sbw_stlinkv2_init.txt) |
| MSP430G2452 *(profile G2xx2)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x5224` @ `0x0ff0` | 2 | ✅ identify + GDB loop | [`…/g2452_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2452_sbw_stlinkv2_init.txt) |
| MSP430G2211 *(profile F20x1_G2x0x_G2x1x)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x01f2` @ `0x0ff0`, cfg `01` | 2 | ✅ identify + GDB loop | [`…/g2211_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2211_sbw_stlinkv2_init.txt) |
| MSP430G2231 *(profile F20x2_G2x2x_G2x3x)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x01f2` @ `0x0ff0`, cfg `02` | 2 | ✅ identify + GDB loop | [`…/g2231_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2231_sbw_stlinkv2_init.txt) |
| MSP430FR5994 | CPUXv2 FRAM / SLAU367 | **STLinkV2** | **SBW** | `0x99` | `0x1106` | `0606 9b74 82a1 1021` (**= golden**) | 3 | ✅ identify + GDB loop — **#19/#20 fix confirmed** | [`…/fr5994_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5994_sbw_stlinkv2_init.txt) |
| MSP430F5529 | CPUXv2 / SLAU208 | **STLinkV2** | **SBW** | `0x91` | `0x0103` | `0606 3deb 2955 1217` | 8 | ✅ identify + GDB loop | [`…/f5529_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f5529_sbw_stlinkv2_init.txt) |
| MSP430i2041 | i20xx / SLAU335 | **STLinkV2** | **SBW** | — | — | — | — | ❌ `jtag_init: no device found` — **[#40](https://github.com/grumat/glossy-msp430/issues/40)** | [`…/i2041_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/i2041_sbw_stlinkv2_init.txt) |
| MSP430FR5858 | CPUXv2 FRAM / SLAU367 | **STLinkV2** | **SBW** | `0x99` | `0x1106` | `0606 77ba 8158 3040` | 3 | ✅ identify + GDB loop | [`…/fr5858_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5858_sbw_stlinkv2_init.txt) |
| MSP430FR5739 | CPUXv2 FRAM / SLAU272 | **STLinkV2** | **SBW** | `0x91` | `0x1106` | `0505 311e 8103 2626` | 3 | ✅ identify + GDB loop (⚠ DB tags `[SLAU321]`) | [`…/fr5739_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5739_sbw_stlinkv2_init.txt) |
| CC430F5137 *(Olimex MSP430-CCRF)* | CPUXv2 / SLAU259 | **STLinkV2** | **SBW** | `0x91` | `0x1101` | `0606 16c7 3751 1212` | 3 | ✅ identify + GDB loop | [`…/cc430f5137_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/cc430f5137_sbw_stlinkv2_init.txt) |

## Entries

### MSP430F5418A — SLAU208 (F5418 proto-board)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Wiring:** the §4.3 STLinkV2 SBW-wire path — STLink-Adapter (JTAG-20→14),
  J10 in the **JTAG** jumper layout, SWDIO→J19 pin 11, SWCLK→J19 pin 8,
  GND→pin 9, VCC→pin 2. **This trace bench-confirms that path.**
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/f5418a_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f5418a_sbw_stlinkv2_init.txt)

```
jtag_id     0x91          → CPUXv2
coreip_id   0x0103
device_id   0x0000        (Xv2: real ID comes from the TLV, not this field)
id_data_addr 0x1a00
raw[0..3]   0606 2929 8000 1515
profile     MSP430F5418A [CPUXv2] [EMEX_LARGE_5XX] [SLAU208] [1377]
HW bkpts    8
```

Memory map reported: BSL `0x1000-0x17ff`, Info `0x1800-0x19ff`, Boot ROM
`0x1a00-0x1aff`, RAM `0x1c00-0x5bff` (16 KB), Main Flash `0x5c00-0x25bff`
(128 KB). Consistent with the F5418A datasheet.

> Note: `raw[0]=0x0606` matches the FR5994 golden reference
> ([`FR5994_SBW_GOLDEN_REFERENCE.md`](FR5994_SBW_GOLDEN_REFERENCE.md)) — that
> word is the common Xv2 marker; `raw[1..3]` differ (different part, Flash vs
> FRAM). No vacant-memory / blank-device handling needed here.

### MSP430G2553 — SLAU144 (legacy CPU, low-pin-count)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/g2553_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2553_sbw_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy CPU (NOT CPUXv2)
coreip_id   0x0000        (no Xv2 core IP block)
device_id   0x5325        (G2553 silicon ID — read directly, legacy path)
id_data_addr 0x0ff0       (legacy ID location, not the Xv2 0x1a00 TLV)
mcu_ver/fab 5325 / 60
profile     MSP430G2xx3 [EMEX_LOW] [SLAU144]   (family profile covers G2553/G2453/…)
HW bkpts    2
```

Memory map reported: RAM `0x0200-0x03ff` (512 B), BSL `0x0c00-0x0fff`, Info
`0x1000-0x10ff`, Main Flash `0xc000-0xffff` (16 KB) — matches the G2553.

> **Why this case matters:** it's the **legacy `TapDev430` SBW path** (`jtag_id
> 0x89`, ID at `0x0ff0`), a completely different identify flow from the F5418A's
> CPUXv2 path (`0x91`, TLV at `0x1a00`). Both succeeding over SBW on the same
> STLinkV2 probe validates the transport across both protocol generations.
>
> **Board:** captured through the **MSP-EXP430G2 (1st-gen) LaunchPad** with the
> eZ-FET (USB FET) isolated and the STLinkV2 wired to the target-side isolation
> pads per the §4.2 G2 block — so that wiring is **bench-confirmed**.

### MSP430G2452 — SLAU144 (legacy CPU, low-pin-count)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **MSP-EXP430G2 (1st-gen) LaunchPad** (socket swap from the G2553; §4.2).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/g2452_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2452_sbw_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy CPU (NOT CPUXv2)
coreip_id   0x0000
device_id   0x5224        (G2452 silicon ID — legacy path)
id_data_addr 0x0ff0
mcu_ver/fab 5224 / a0
profile     MSP430G2xx2 [EMEX_LOW] [SLAU144]   (family profile covers G2452/G2412/…)
HW bkpts    2
```

Memory map reported: RAM `0x0200-0x02ff` (256 B), BSL `0x0c00-0x0fff`, Info
`0x1000-0x10ff`, Main Flash `0xe000-0xffff` (8 KB) — matches the G2452 (half the
G2553's RAM/Flash). Same legacy `TapDev430` SBW path as the G2553, distinct
`device_id`/profile (G2xx2 vs G2xx3).

### MSP430G2211 — SLAU144 (legacy CPU, smallest G2 part)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **MSP-EXP430G2 (1st-gen) LaunchPad** (socket swap; §4.2).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/g2211_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2211_sbw_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy CPU (NOT CPUXv2)
coreip_id   0x0000
device_id   0x01f2        (old F20xx-style ID — note the low value, not 0x5xxx)
id_data_addr 0x0ff0
mcu_ver/rev/fab/cfg 01f2 / 30 / 40 / 01
profile     F20x1_G2x0x_G2x1x [EMEX_LOW] [SLAU144]   (shared profile for the smallest parts)
HW bkpts    2
```

Memory map reported: RAM `0x0200-0x02ff` (256 B), BSL `0x0c00-0x0fff`, Info
`0x1000-0x10ff`, Main Flash `0xf800-0xffff` (**2 KB**) — matches the G2211 (the
smallest of the socket's parts).

> Profile contrast across the G2 socket: the larger parts resolve to distinct
> `G2xx3`/`G2xx2` profiles via a `0x5xxx` `device_id`, while the smallest parts
> carry a low old-style `device_id` (`0x01f2`) and are split by a **secondary
> field** — see the G2231 entry below, where the *same* `device_id` resolves to a
> *different* profile by `mcu_cfg`. All ride the same legacy `TapDev430` SBW
> identify path (`0x89`, ID at `0x0ff0`).

### MSP430G2231 — SLAU144 (legacy CPU; same device_id as G2211, split by mcu_cfg)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **MSP-EXP430G2 (1st-gen) LaunchPad** (socket swap; §4.2). **Last part
  for this board.**
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/g2231_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2231_sbw_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy CPU (NOT CPUXv2)
coreip_id   0x0000
device_id   0x01f2        (IDENTICAL to the G2211 — not enough on its own)
id_data_addr 0x0ff0
mcu_ver/rev/fab/cfg 01f2 / 30 / 40 / 02   ← cfg = 0x02 (G2211 was 0x01)
profile     F20x2_G2x2x_G2x3x [EMEX_LOW] [SLAU144]
HW bkpts    2
```

Memory map reported: RAM `0x0200-0x02ff` (256 B), Main Flash `0xf800-0xffff`
(2 KB) — same footprint as the G2211.

> **Key finding — `mcu_cfg` is the tiebreaker for the smallest G2 parts.** The
> G2211 and G2231 report the **same** `device_id 0x01f2`; the chip DB
> distinguishes them by **`mcu_cfg`** (`0x01` → F20x1_G2x0x_G2x1x, `0x02` →
> F20x2_G2x2x_G2x3x). Good evidence that Glossy's profile-resolution reads the
> full Xv1 ID block, not just `device_id` — and a useful regression anchor if
> profile selection ever changes.

### MSP430FR5994 — SLAU367 (CPUXv2 FRAM) — **#19/#20 regression anchor**

*(Family UG is **SLAU367** per the wiki `Home.md`; the firmware chip-DB prints
`[SLAU378]`, a later revision of the same FR58xx/59xx/6xx family guide.)*

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **MSP-EXP430FR5994 LaunchPad** (eZ-FET isolated, STLinkV2 on the
  target-side SBW pads per §4.2) — so that wiring is **bench-confirmed**.
- **Result:** ✅ clean — TAP identified, **descriptor read correct**, profile
  resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/fr5994_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5994_sbw_stlinkv2_init.txt)

```
jtag_id     0x99          → CPUXv2 (FR5994 variant — the 0x99 ReadWords branch from #19)
coreip_id   0x1106
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0606 9b74 82a1 1021       ← matches the eZ-FET golden reference word-for-word
mcu_ver/rev/cfg 82a1 / 21 / 10
profile     MSP430FR5994 [CPUXv2] [FRAM] [EMEX_SMALL_5XX] [SLAU378]
HW bkpts    3
```

Memory map reported: TinyRAM `0x0006-0x001f`, LEA peripheral/RAM blocks, BSL ROM
`0x1000-0x17ff`, Info FRAM `0x1800-0x19ff`, Boot ROM `0x1a00-0x1aff`, RAM
`0x1c00-0x2bff` (4 KB), LeaRAM `0x2c00-0x3bff` (4 KB), Main FRAM `0x4000-0x43fff`
(256 KB). Consistent with the FR5994.

> **This is the headline SBW validation.** The FR5994 descriptor read was the
> subject of issues **#19 / #20** and the eZ-FET golden-reference investigation
> ([`FR5994_SBW_GOLDEN_REFERENCE.md`](FR5994_SBW_GOLDEN_REFERENCE.md)). Glossy's
> own driver now returns the **identical** four-word signature
> (`0606 9b74 82a1 1021`) the eZ-FET produced, on the same `jtag_id 0x99` path
> whose `ReadWords` branch was suspected — **no vacant-memory / magic-pattern /
> per-word-SetPC symptom**. Treat this dump as the regression anchor for the
> Xv2-FRAM SBW identify path.

### MSP430F5529 — SLAU208 (CPUXv2, USB part)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **MSP-EXP430F5529LP LaunchPad** (eZ-FET lite isolated, STLinkV2 on the
  target-side `SBW RST`/`SBW TST` pads per §4.2) — so that wiring is
  **bench-confirmed**.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/f5529_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f5529_sbw_stlinkv2_init.txt)

```
jtag_id     0x91          → CPUXv2 (same family TAP as the F5418A)
coreip_id   0x0103
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0606 3deb 2955 1217
mcu_ver/rev/cfg 2955 / 17 / 12
profile     MSP430F5529 [CPUXv2] [EMEX_LARGE_5XX] [SLAU208] [1377]
HW bkpts    8
```

Memory map reported: BSL `0x1000-0x17ff`, Info `0x1800-0x19ff`, Boot ROM
`0x1a00-0x1aff`, **USBRAM `0x1c00-0x23ff` (2 KB)**, RAM `0x2400-0x43ff` (8 KB),
Main Flash `0x4400-0x243ff` (128 KB). The dedicated USBRAM block is the F5529's
USB-capable signature; otherwise it mirrors the F5418A's SLAU208 CPUXv2 layout
(`0x91`/`0x0103`, 8 HW bkpts, `raw[0]=0x0606`).

### ❌ MSP430i2041 — SLAU335 (i20xx metering) — FAILED, under investigation

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU335 proto-board. Mode jumper **J3** (6 jumpers: top 4 = JTAG,
  last 2 = SBW); STLinkV2 uses the §4.3-style exception (J3 in JTAG layout +
  hand-wire SWDIO→JTAG-14 pin 11, SWCLK→pin 8). **First time this board is driven
  by the STLinkV2 SBW path.**
- **Result:** ❌ **`jtag_init: no device found`** — aborts in the device-detect
  loop (`TapMcu.cpp:122–174`) after `kMaxEntryTry` (4) iterations; no `jtag_id`
  ever resolves to a valid MSP430 ID.
- **Dump:** [`INIT_TRACE_VALIDATION/i2041_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/i2041_sbw_stlinkv2_init.txt)
- **LA capture:** `supp/docs-ai/i2041-sbw-fail.csv` (2-wire SBWDIO + TEST/SBWCLK,
  ~0.63 s, 69k edges).

```
Starting JTAG
jtag_init: no device found
initialization failed
```

**Preliminary investigation (from the LA capture):**

- The SBW **bus is alive** — `SBWCLK` toggles throughout and `SBWDIO` goes high
  11,520× (first at t≈35 ms); not an open/dead line.
- The capture shows a **structured retry loop**: a ~100 ms cycle with timed
  delays (≈4/20/5/10 ms) interleaved with fast SBW bursts, repeating to the
  `kMaxEntryTry` limit. Each iteration = a **normal RST-high entry** then the
  **magic-pattern (RST-low, `0xA55A`) fallback** — both return an invalid ID, so
  it retries and finally aborts.
- Conclusion: physical layer works, but `IR_Shift(kCntrlSigCapture)` never yields
  a valid MSP430 `jtag_id` for the i2041 → **identification failure, not a dead
  bus**. (Wiring still not fully excluded — this board's STLinkV2 hand-wire is new.)

> Tracked in **[GH issue #40](https://github.com/grumat/glossy-msp430/issues/40)**.
> Contrast: the G2xxx parts (also legacy SBW) and the F5418A (SLAU208 §4.3
> hand-wire) both identify fine, so this is i20xx- and/or SLAU335-board-specific.

### MSP430FR5858 — SLAU367 (CPUXv2 FRAM)

*(Family UG **SLAU367** per the wiki `Home.md`; firmware chip-DB prints
`[SLAU378]`, same FR58xx/59xx/6xx family guide as the FR5994.)*

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU272_SLAU367 proto-board (TSSOP-38). STLinkV2 via the §4.4
  hand-wire (STLink-Adapter 20→14, J3 in JTAG layout, SWDIO→J7 pin 11,
  SWCLK→pin 8) — so that path is **bench-confirmed**.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/fr5858_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5858_sbw_stlinkv2_init.txt)

```
jtag_id     0x99          → CPUXv2 (same TAP/coreip as the FR5994)
coreip_id   0x1106
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0606 77ba 8158 3040
mcu_ver/rev/cfg 8158 / 40 / 30
profile     MSP430FR5858 [CPUXv2] [FRAM] [EMEX_SMALL_5XX]
HW bkpts    3
```

Memory map reported: TinyRAM `0x0006-0x001f`, BSL ROM `0x1000-0x17ff`, Info FRAM
`0x1800-0x19ff`, Boot ROM `0x1a00-0x1aff`, RAM `0x1c00-0x23ff` (2 KB), Main FRAM
`0x4400-0xffff` (47 KB). A second `0x99`/`0x1106` CPUXv2-FRAM identify alongside
the FR5994 — the smaller FR58xx sibling on the dual-family SLAU272/367 board, and
the first **non-LaunchPad** STLinkV2-SBW success (proto-board JTAG-14 hand-wire).

### MSP430FR5739 — SLAU272 (CPUXv2 FRAM, FR57xx) — ⚠ DB tags wrong family UG

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU272_SLAU367 proto-board (2nd sample; the FR57xx / SLAU272
  family). Same §4.4 hand-wire as the FR5858 — **bench-confirmed**.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/fr5739_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5739_sbw_stlinkv2_init.txt)

```
jtag_id     0x91          → note: 0x91, NOT 0x99 (yet still CPUXv2 — see coreip_id)
coreip_id   0x1106        → CPUXv2 (this, not jtag_id, marks the Xv2 FRAM core)
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0505 311e 8103 2626       ← raw[0] = 0x0505, not 0x0606 (FR57xx marker)
mcu_ver/rev/cfg 8103 / 26 / 26
profile     MSP430FR5739 [CPUXv2] [FRAM] [EMEX_SMALL_5XX] [SLAU321]   ← ⚠ wrong UG tag
HW bkpts    3
```

Memory map reported: TinyRAM `0x0006-0x001f`, BSL ROM `0x1000-0x17ff`, Info FRAM
`0x1800-0x18ff` (256 B), Boot ROM `0x1a00-0x1aff`, RAM `0x1c00-0x1fff` (1 KB),
Main FRAM `0xc200-0xffff` (15.5 KB). The smallest FRAM part captured so far.

> **Two findings:**
> 1. **`jtag_id` does not classify the core.** This FR57xx part returns `0x91`
>    (like the F5xx Flash parts) yet is CPUXv2-FRAM — the **`coreip_id 0x1106`**
>    is the discriminator, and `raw[0]=0x0505` (vs `0x0606` on FR58xx/59xx) marks
>    the FR57xx signature family.
> 2. ⚠ **Chip-DB family-UG mislabel.** The firmware prints **`[SLAU321]`**, but
>    SLAU321 is the *MSP430x09x* family (C092/L092). FR5739 is **SLAU272**
>    (MSP430FR57xx) per the wiki `Home.md`. The part still *resolves correctly*
>    (profile = MSP430FR5739) — only the printed users-guide tag is wrong. Looks
>    like a `ChipInfoDB` SLAU mapping bug for the FR57xx group; worth a follow-up.

### CC430F5137 — SLAU259 (CPUXv2, CC430 RF SoC) — first third-party board

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **Olimex MSP430-CCRF** (non-repo third-party). JTAG-14 connector;
  the only config jumper selects **self-powered vs probe-powered**. STLinkV2 via
  the same hand-wire as the proto-boards (§4.6) — **bench-confirmed**.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered, no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/cc430f5137_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/cc430f5137_sbw_stlinkv2_init.txt)

```
jtag_id     0x91
coreip_id   0x1101        ← NEW core-IP value (F5xx-Flash 0x0103, FRAM 0x1106, CC430 0x1101)
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0606 16c7 3751 1212
mcu_ver/rev/cfg 3751 / 12 / 12
profile     CC430F5137 [CPUXv2] [EMEX_SMALL_5XX] [SLAU259]
HW bkpts    3
```

Memory map reported: BSL `0x1000-0x17ff`, Info `0x1800-0x19ff`, Boot ROM
`0x1a00-0x1aff`, RAM `0x1c00-0x2bff` (4 KB), Main Flash `0x8000-0xffff` (32 KB) —
a Flash CC430 RF SoC. Adds a **third CPUXv2 `coreip_id` (`0x1101`)** to the set
and the first **SLAU259** part — all on the same STLinkV2 SBW front-end.
