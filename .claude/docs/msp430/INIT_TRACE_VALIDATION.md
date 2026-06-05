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

> **Ordering convention:** rows are sorted by **ascending SLAUxxx users-guide
> number** (049 → 056 → 144 → 208 → 259 → 272 → 335 → 367) — the same convention as
> the wiring guide §4. This tracks TI's release timeline *and* the firmware layer
> history, so a reviewer validating one protocol layer can scan a whole generation
> at once. Within a generation, rows are grouped by part (a part's SBW + JTAG rows
> sit together). New captures slot into SLAU order, not append order.

| MCU | Family / SLAU | Probe | Transport | jtag_id | coreip_id | raw signature | HW bkpts | Result | Dump |
|-----|---------------|:-----:|:---------:|:-------:|:---------:|---------------|:--------:|--------|------|
| MSP430F1121 *(Olimex breakout; profile F11x1A)* | legacy CPU / SLAU049 | **STLinkV2** | **JTAG** | `0x89` | `0x0000` | device_id `0x12f1` @ `0x0ff0` | 2 | ✅ identify + GDB loop — **oldest/smallest part** (4 KB flash, 256 B RAM) | [`…/f1121_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f1121_jtag_stlinkv2_init.txt) |
| MSP430F1611 | legacy CPU / SLAU049 | **STLinkV2** | **JTAG** | `0x89` | `0x0000` | device_id `0x6cf1` @ `0x0ff0` | 8 | ✅ identify + GDB loop — **first JTAG-transport success** | [`…/f1611_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f1611_jtag_stlinkv2_init.txt) |
| MSP430F449 *(Olimex breakout; profile F44x)* | legacy CPU / SLAU056 | **STLinkV2** | **JTAG** | `0x89` | `0x0000` | device_id `0x49f4` @ `0x0ff0` | 8 | ✅ identify + GDB loop — **first `SLAU056` / x4xx** (first LCD part) | [`…/f449_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f449_jtag_stlinkv2_init.txt) |
| MSP430F247 *(no-name China board)* | legacy CPU / SLAU144 | **STLinkV2** | **JTAG** | `0x89` | `0x0000` | device_id `0x49f2` @ `0x0ff0` | 3 | ✅ identify + GDB loop — **first `EMEX_MEDIUM`** (3 bkpt) | [`…/f247_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f247_jtag_stlinkv2_init.txt) |
| MSP430F2131 *(Olimex breakout)* | legacy CPU / SLAU144 | **STLinkV2** | **JTAG** | — | — | — | — | ❌ `jtag_init: no device found` — **[#42](https://github.com/grumat/glossy-msp430/issues/42)** (siblings work; likely blown JTAG fuse / chip / board) | [`…/f2131_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f2131_jtag_stlinkv2_init.txt) |
| MSP430F2418 | **CPUX** / SLAU144 | **STLinkV2** | **JTAG** | `0x89` | `0x0000` | device_id `0x6ff2` @ `0x0ff0` | 8 | ✅ identify + GDB loop — **first `[CPUX]`** (20-bit, >64 KB flash) | [`…/f2418_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f2418_jtag_stlinkv2_init.txt) |
| MSP430G2211 *(profile F20x1_G2x0x_G2x1x)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x01f2` @ `0x0ff0`, cfg `01` | 2 | ✅ identify + GDB loop | [`…/g2211_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2211_sbw_stlinkv2_init.txt) |
| MSP430G2231 *(profile F20x2_G2x2x_G2x3x)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x01f2` @ `0x0ff0`, cfg `02` | 2 | ✅ identify + GDB loop | [`…/g2231_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2231_sbw_stlinkv2_init.txt) |
| MSP430G2452 *(profile G2xx2)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x5224` @ `0x0ff0` | 2 | ✅ identify + GDB loop | [`…/g2452_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2452_sbw_stlinkv2_init.txt) |
| MSP430G2553 *(profile G2xx3)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x5325` @ `0x0ff0` | 2 | ✅ identify + GDB loop | [`…/g2553_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2553_sbw_stlinkv2_init.txt) |
| MSP430G2955 *(profile G2x55)* | legacy CPU / SLAU144 | **STLinkV2** | **SBW** | `0x89` | `0x0000` | device_id `0x5529` @ `0x0ff0` | 2 | ✅ identify + GDB loop (fixed: SWDIO→pin 1/TDO, [#41](https://github.com/grumat/glossy-msp430/issues/41)) | [`…/g2955_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2955_sbw_stlinkv2_init.txt) |
| MSP430G2955 *(JTAG on an SBW-only part)* | legacy CPU / SLAU144 | **STLinkV2** | **JTAG** | — | — | — | — | 🟡 **expected fail (negative control)** — G2x55 has no JTAG TAP; `no device found`, transport fails cleanly (retries, no hang) | [`…/g2955_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2955_jtag_stlinkv2_init.txt) |
| MSP430F5418A | CPUXv2 / SLAU208 | **STLinkV2** | **SBW** | `0x91` | `0x0103` | `0606 2929 8000 1515` | 8 | ✅ identify + GDB loop | [`…/f5418a_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f5418a_sbw_stlinkv2_init.txt) |
| MSP430F5529 | CPUXv2 / SLAU208 | **STLinkV2** | **SBW** | `0x91` | `0x0103` | `0606 3deb 2955 1217` | 8 | ✅ identify + GDB loop | [`…/f5529_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f5529_sbw_stlinkv2_init.txt) |
| CC430F5137 *(Olimex MSP430-CCRF)* | CPUXv2 / SLAU259 | **STLinkV2** | **SBW** | `0x91` | `0x1101` | `0606 16c7 3751 1212` | 3 | ✅ identify + GDB loop | [`…/cc430f5137_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/cc430f5137_sbw_stlinkv2_init.txt) |
| CC430F5137 *(Olimex MSP430-CCRF)* | CPUXv2 / SLAU259 | **STLinkV2** | **JTAG** | `0x91` | `0x1101` | `0606 16c7 3751 1212` (**= SBW**) | 3 | ✅ identify + GDB loop — **same part, both transports, identical descriptor** | [`…/cc430f5137_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/cc430f5137_jtag_stlinkv2_init.txt) |
| MSP430FR5739 | CPUXv2 FRAM / SLAU272 | **STLinkV2** | **SBW** | `0x91` | `0x1106` | `0505 311e 8103 2626` | 3 | ✅ identify + GDB loop (⚠ DB tags `[SLAU321]`) | [`…/fr5739_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5739_sbw_stlinkv2_init.txt) |
| MSP430FR5739 *(SLAU272/367 proto-board)* | CPUXv2 FRAM / SLAU272 | **STLinkV2** | **JTAG** | `0x91` | `0x1106` | `0505 311e 8103 2626` (**= SBW**) | 3 | ✅ identify + GDB loop — **both transports, identical descriptor** (⚠ DB tags `[SLAU321]`) | [`…/fr5739_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5739_jtag_stlinkv2_init.txt) |
| MSP430i2041 | i20xx / SLAU335 | **STLinkV2** | **SBW** | — | — | — | — | ❌ `jtag_init: no device found` — **[#40](https://github.com/grumat/glossy-msp430/issues/40)** (root cause **[#43](https://github.com/grumat/glossy-msp430/issues/43)**: JTAG access password) | [`…/i2041_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/i2041_sbw_stlinkv2_init.txt) |
| MSP430i2041 | i20xx / SLAU335 | **STLinkV2** | **JTAG** | — | — | — | — | ❌ `jtag_init: no device found` — **both transports fail** → device-access, not wiring (**[#43](https://github.com/grumat/glossy-msp430/issues/43)**) | [`…/i2041_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/i2041_jtag_stlinkv2_init.txt) |
| MSP430FR5858 | CPUXv2 FRAM / SLAU367 | **STLinkV2** | **SBW** | `0x99` | `0x1106` | `0606 77ba 8158 3040` | 3 | ✅ identify + GDB loop | [`…/fr5858_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5858_sbw_stlinkv2_init.txt) |
| MSP430FR5858 *(SLAU272/367 proto-board)* | CPUXv2 FRAM / SLAU367 | **STLinkV2** | **JTAG** | `0x99` | `0x1106` | `0606 77ba 8158 3040` (**= SBW**) | 3 | ✅ identify + GDB loop — **both transports, identical descriptor; first `0x99` over JTAG** | [`…/fr5858_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5858_jtag_stlinkv2_init.txt) |
| MSP430FR5994 | CPUXv2 FRAM / SLAU367 | **STLinkV2** | **SBW** | `0x99` | `0x1106` | `0606 9b74 82a1 1021` (**= golden**) | 3 | ✅ identify + GDB loop — **#19/#20 fix confirmed** | [`…/fr5994_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5994_sbw_stlinkv2_init.txt) |

> **Scope note — why the LaunchPad parts have no JTAG row.** The JTAG-transport
> batch only covers boards with a physical **TI JTAG-14 connector** (proto-boards
> + third-party breakouts). The TI **LaunchPads** (MSP-EXP430G2, F5529LP, FR5994)
> break out only the **SBW isolation pads** — they have **no JTAG-14 connector** —
> so those parts are validated over SBW only and are intentionally absent from the
> JTAG rows above. (The same silicon's JTAG identify path is exercised by other
> boards: e.g. CPUXv2 JTAG via the CC430/FR58xx, legacy JTAG via the F1/F2 parts.)

## Entries

### MSP430F1121 — SLAU049 (oldest MSP430 generation) — ✅ JTAG, resolves to `F11x1A` family profile

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** **Olimex MSP430F1121 breakout** (third-party), standard TI JTAG-14
  socket — plain JTAG path (STLink-Adapter 20→14 + ribbon). Second Olimex board
  (the CC430-CCRF was the first, over SBW).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/f1121_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f1121_jtag_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy TAP class
coreip_id   0x0000        (no Xv2 core IP block)
device_id   0x12f1        (F1121 silicon ID — legacy 0x0ff0 path)
id_data_addr 0x0ff0
mcu_ver/rev/fab/cfg/fuse 12f1 / 13 / 40 / 00 / 00
profile     MSP430F11x1A [EMEX_LOW] [SLAU049]   (family profile — covers F1101A/F1111A/F1121A)
HW bkpts    2
```

Memory map reported: RAM `0x0200-0x02ff` (**256 B**), BSL `0x0c00-0x0fff`, Info
`0x1000-0x10ff`, Main Flash `0xf000-0xffff` (**4 KB**), **no RAM2**. The smallest
footprint captured — about as minimal as an MSP430 gets.

> **The oldest silicon generation in the collection — and why it's JTAG-only.**
> The original **F1xx** family predates Spy-Bi-Wire entirely: SBW was introduced
> with the later F2xx/G2xx low-pin-count parts. So the F1121 has **no SBW at any
> pin count** — JTAG is the *only* transport, not a packaging trade-off. It still
> rides the same legacy `TapDev430` identify path (`0x89`, ID at `0x0ff0`) as
> every other legacy part, confirming that path reaches back to the earliest
> devices.
>
> **Family-profile resolution.** The `device_id 0x12f1` resolves to the *grouped*
> profile **`MSP430F11x1A`** (not an exact "F1121" entry) — same family-profile
> behaviour seen on the G2 parts (`G2xx3`/`G2xx2`). Second **SLAU049 / x1xx** part
> after the F1611, and another `EMEX_LOW` (2-bkpt) data point — now confirmed on a
> JTAG part, not just the G2 SBW parts.

### MSP430F1611 — SLAU049 (legacy CPU, x1xx) — ✅ **first JTAG-transport success**

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** SLAU049/144 generic proto-board, **LQFP64 F1611 (U1)** — the JTAG
  part of this board (the 38-pin G2x55 in U2 is the SBW-only one).
- **Wiring:** **standard JTAG path, no hand-wiring** — STLink-Adapter (JTAG-20→14)
  + a plain 14-wire flat cable into the board's J1 connector. (Contrast the SBW
  hand-wire trick: full 4-wire JTAG just uses the ribbon directly.)
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors. **This is the first end-to-end validation of the JTAG transport.**
- **Dump:** [`INIT_TRACE_VALIDATION/f1611_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f1611_jtag_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy CPU (NOT CPUXv2) — same TAP class as the G2 parts
coreip_id   0x0000        (no Xv2 core IP block)
device_id   0x6cf1        (F1611 silicon ID — read directly, legacy 0x0ff0 path)
id_data_addr 0x0ff0       (legacy ID location, not the Xv2 0x1a00 TLV)
mcu_ver/rev/fab/cfg/fuse 6cf1 / 20 / 40 / 00 / 80
profile     MSP430F1611 [EMEX_HIGH] [SLAU049]
HW bkpts    8
```

Memory map reported: RAM `0x0200-0x09ff` (2 KB) + **RAM2 `0x1100-0x38ff` (10 KB)**,
BSL `0x0c00-0x0fff` (4 banks), Info `0x1000-0x10ff`, Main Flash `0x4000-0xffff`
(48 KB) — matches the F1611 (the largest x1xx part).

> **Why this case matters:**
> 1. **First JTAG transport pass.** Same legacy `TapDev430` identify flow as the
>    G2 parts (`0x89`, ID at `0x0ff0`), but reached over **4-wire JTAG** instead of
>    SBW — validating that transport end-to-end on real silicon.
> 2. **Disproves "STLinkV2 is SBW-only."** A JTAG-*only* LQFP64 part comes up over
>    the STLinkV2 + STLink-Adapter + plain ribbon. The probe maps all four JTAG
>    signals (§2b) and the firmware now has a JTAG build. The wiring-guide §4.3
>    "needs a JTAG-capable probe (BluePill-G431)" caveat was wrong for 3.3 V parts
>    and is corrected; the BluePill-G431 is only needed for **sub-3.3 V** targets
>    (STLinkV2 outputs are fixed at 3.3 V, §6).
> 3. **New coverage:** first **SLAU049 / x1xx** part, first **`EMEX_HIGH`** EEM
>    grade, and **8 HW breakpoints** on a *legacy* part (the G2 EMEX_LOW parts had
>    only 2) — a useful upper-bound anchor for the legacy breakpoint path.

### MSP430F449 — SLAU056 (x4xx, LCD generation) — ✅ JTAG, first `SLAU056` family

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** **Olimex MSP430F449 breakout** (third-party), standard TI JTAG-14
  socket — plain JTAG path (STLink-Adapter 20→14 + ribbon). Third Olimex board.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/f449_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f449_jtag_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy TAP class
coreip_id   0x0000        (no Xv2 core IP block)
device_id   0x49f4        (F449 silicon ID — legacy 0x0ff0 path)
id_data_addr 0x0ff0
mcu_ver/rev/fab/cfg/fuse 49f4 / 01 / 40 / 00 / 00
profile     MSP430F44x [EMEX_HIGH] [SLAU056]   (family profile — covers F447/F448/F449)
HW bkpts    8
```

Memory map reported: **LCD `0x0090-0x00a4` (21 B)** ← first part with a hardware
LCD controller block, RAM `0x0200-0x09ff` (2 KB), BSL `0x0c00-0x0fff`, Info
`0x1000-0x10ff` (2 banks), Main Flash `0x1100-0xffff` (59.7 KB), no RAM2.

> **New family — `SLAU056` / x4xx.** First **SLAU056** part in the collection,
> the LCD-driver MSP430 generation (FLL+, segment-LCD). Per the ascending-SLAU
> ordering convention, x4xx (`SLAU056`) sorts right after x1xx (`SLAU049`) — both
> are early-2000s silicon, contemporaries of the F1xx. The **LCD memory block** is
> the family fingerprint — first time the memory-map decode reports an `LCD`
> region. Still the same legacy `TapDev430` identify (`0x89`, ID at `0x0ff0`) and
> a *family* profile (`MSP430F44x`, like `F11x1A`/`G2xx3`); `EMEX_HIGH` / 8 HW
> breakpoints.

### MSP430F247 — SLAU144 (legacy CPU, x2xx) — ✅ JTAG, first no-name board + `EMEX_MEDIUM`

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** **no-name Chinese MSP430F247 board** (bought off-the-shelf), standard
  **TI JTAG-14 socket** — the **first non-repo, non-LaunchPad, non-Olimex** board in
  this collection. Plain JTAG path: STLink-Adapter (20→14) + 14-wire ribbon.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/f247_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f247_jtag_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy TAP class
coreip_id   0x0000        (no Xv2 core IP block)
device_id   0x49f2        (F247 silicon ID — legacy 0x0ff0 path)
id_data_addr 0x0ff0
mcu_ver/rev/fab/cfg/fuse 49f2 / 04 / 60 / 00 / 82
profile     MSP430F247 [EMEX_MEDIUM] [SLAU144]
HW bkpts    3
```

Memory map reported: RAM `0x0200-0x09ff` (2 KB) + RAM2 `0x1100-0x20ff` (4 KB),
BSL `0x0c00-0x0fff` (4 banks), Info `0x1000-0x10ff` (4 banks), Main Flash
`0x8000-0xffff` (**32 KB**). Flash top at `0xffff` (≤64 KB) → **plain 16-bit
legacy CPU, not `[CPUX]`** (contrast the F2418's `0x1ffff`), even though both are
F2xx / SLAU144.

> **Two firsts:**
> 1. **First third-party "found in the wild" board** — a generic Chinese F247
>    breakout with a stock JTAG-14 socket comes up with zero fuss on the standard
>    JTAG path, a good portability signal beyond the hand-built proto-boards.
> 2. **First `EMEX_MEDIUM` EEM grade (3 HW breakpoints)** — fills the gap between
>    `EMEX_LOW` (2 bkpt, the G2 parts) and `EMEX_HIGH` (8 bkpt, F1611/F2418). The
>    EEM-grade ladder now seen on the legacy `0x89` path: **LOW(2) → MEDIUM(3) →
>    HIGH(8)**.

### ❌ MSP430F2131 — SLAU144 (Olimex breakout) — FAILED (`no device found`), under investigation

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** **Olimex MSP430F2131 breakout**, standard TI JTAG-14 socket; same
  plain JTAG path (STLink-Adapter 20→14 + ribbon) as every other JTAG capture.
- **Result:** ❌ **`jtag_init: no device found`** — the device-detect loop never
  reads a valid TAP ID (no `jtag_id`/`device_id` ever printed); transport retries
  cleanly, no hang. Multiple attempts, none worked.
- **Dump:** [`INIT_TRACE_VALIDATION/f2131_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f2131_jtag_stlinkv2_init.txt)

```
Starting JTAG
jtag_init: no device found
initialization failed
… (retries, no valid TAP ID)
```

> Tracked in **[GH issue #42](https://github.com/grumat/glossy-msp430/issues/42)**.
>
> **Why this is almost certainly hardware, not firmware.** The identical
> STLinkV2 + STLink-Adapter + ribbon + JTAG build identifies **every other legacy
> `0x89` part** — including the **F247** (same F2xx / SLAU144 family as the F2131)
> and the **F1121** (a *second Olimex breakout* on this exact cabling). The
> TAP-level identify runs *before* any chip-DB lookup, so a profile gap can't be
> it either. **Top suspect: a blown JTAG security fuse** (one-time, permanently
> disables the TAP → reads as absent even on a live chip — common on
> salvaged/resold parts); then a dead chip or a broken TEST/RST track on the
> breakout. Confirm with TI tooling (reports a blown fuse) or a second F2131; an
> LA capture of TDO during init would separate "dead/disconnected" (flatline)
> from "alive but invalid ID" (toggling). Contrast **[#40](https://github.com/grumat/glossy-msp430/issues/40)**
> (i2041 SBW no-device-found) — same failure *class*, different transport/part.

### MSP430F2418 — SLAU144 (CPUX, x2xx) — ✅ JTAG, first `[CPUX]` part

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** SLAU049/144 generic proto-board, **LQFP64 part (U1)** — same JTAG
  socket/cable as the F1611 (STLink-Adapter 20→14 + plain 14-wire ribbon, no
  hand-wiring).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors. Second end-to-end JTAG-transport pass.
- **Dump:** [`INIT_TRACE_VALIDATION/f2418_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/f2418_jtag_stlinkv2_init.txt)

```
jtag_id     0x89          → legacy TAP class (same as F1611 / G2 parts)
coreip_id   0x0000        (no Xv2 core IP block)
device_id   0x6ff2        (F2418 silicon ID — legacy 0x0ff0 path)
id_data_addr 0x0ff0
mcu_ver/rev/fab/cfg/fuse 6ff2 / 07 / 60 / 00 / 8d
profile     MSP430F2418 [CPUX] [EMEX_HIGH] [SLAU144]
HW bkpts    8
```

Memory map reported: RAM `0x0200-0x09ff` (2 KB) + **RAM2 `0x1100-0x30ff` (8 KB)**,
BSL `0x0c00-0x0fff` (4 banks), Info `0x1000-0x10ff` (4 banks), **Main Flash
`0x3100-0x1ffff` (115.7 KB)** — note the Flash top at **`0x1ffff` (>64 KB)**, the
giveaway of 20-bit CPUX addressing.

> **Key distinction — `[CPUX]` ≠ `[CPUXv2]`.** This is the first **`[CPUX]`** part
> in the matrix: the **20-bit extended** MSP430 CPU (`TapDev430X` / SLAU208 core),
> but it still uses the **legacy identify flow** — `jtag_id 0x89`, `coreip_id
> 0x0000`, ID read directly at `0x0ff0`, *not* the Xv2 `0x1a00` TLV. So three CPU
> generations are now visible in this collection on the legacy `0x89` TAP alone:
> **plain legacy** (G2, F1611, 16-bit ≤64 KB), **`[CPUX]`** (F2418, 20-bit >64 KB),
> and **`[CPUXv2]`** (5xx/FRAM, `0x91`/`0x99` + TLV). The >64 KB Flash window
> (`…-0x1ffff`) is the visible marker of the CPUX 20-bit address space. Same
> `EMEX_HIGH` / 8-breakpoint grade as the F1611.

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

### MSP430G2955 — SLAU144 (legacy CPU) — ✅ after wiring fix (resolved #41)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU049/144 generic proto-board, 38-pin part (U2). The **G2x55
  family is SBW-only** (no JTAG, like the other G2 parts); the board's LQFP64
  F1xx/F2xx parts are the JTAG ones.
- **Result:** ✅ clean once the SBWDIO wiring was corrected (below) — profile
  resolved, GDB reader loop entered.
- **Dump:** [`INIT_TRACE_VALIDATION/g2955_sbw_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2955_sbw_stlinkv2_init.txt)
  (the earlier pin-11 *failure* LA capture is kept at `supp/docs-ai/g2955-sbw-fail.csv`)

```
jtag_id     0x89          → legacy CPU
coreip_id   0x0000
device_id   0x5529        (G2955 silicon ID — legacy path)
id_data_addr 0x0ff0
mcu_ver/fab 5529 / a0
profile     MSP430G2x55 [EMEX_LOW] [SLAU144]
HW bkpts    2
```

Memory map: RAM `0x0200-0x09ff` (2 KB) + RAM2 `0x1100-0x20ff` (4 KB), BSL
`0x0c00-0x0fff`, Info `0x1000-0x10ff`, Main Flash `0x2100-0xffff` (55.7 KB).

> **RESOLVED ([#41](https://github.com/grumat/glossy-msp430/issues/41)) — wiring,
> not the RC.** The `no device found` was a **wrong connector pin**. This board
> (no jumper block) carries **SBWDIO on the TI connector position, pin 1 (TDO)** —
> *not* pin 11. Fix: **SWDIO → J1 pin 1 (TDO)** (SWCLK stays on pin 8/TEST, GND
> pin 9, VCC pin 2). The pin-11/RST node does carry the board's heavy 100 kΩ/47 nF
> reset RC — which is *why* driving SBWDIO there failed — but that's simply the
> wrong pin, **not** an SBW blocker: on pin 1 the board does SBW fine. *(My earlier
> "extreme-RC → JTAG-only" conclusion was wrong.)* **Lead for
> [#40](https://github.com/grumat/glossy-msp430/issues/40):** try SWDIO on pin 1
> (TDO) on the SLAU335 board too — it may be the same pin mismatch.

### MSP430G2955 — JTAG transport on an SBW-only part — 🟡 expected fail (negative control)

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire) — *first JTAG-transport
  capture in this collection* (firmware recompiled for JTAG).
- **Board:** SLAU049/144 generic proto-board, 38-pin G2955 (U2).
- **Result:** 🟡 **`jtag_init: no device found` — and this is the correct
  outcome.** The **G2x55 family is SBW-only** (no JTAG TAP at all, like the other
  G2 parts), so there is nothing for the 4-wire JTAG path to find. This is a
  **negative control**, not a defect: it confirms the freshly-built JTAG transport
  **runs and fails gracefully** — it retries the full `Starting JTAG → no device
  found → initialization failed` cycle and never hangs.
- **Dump:** [`INIT_TRACE_VALIDATION/g2955_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/g2955_jtag_stlinkv2_init.txt)

```
Glossy MSP430
Starting...
Starting JTAG
jtag_init: no device found      ← expected: G2x55 has no JTAG TAP
initialization failed
Starting JTAG                    ← retries cleanly (no hang/crash)
jtag_init: no device found
initialization failed
Starting JTAG …
```

> **What this validates / does not.** ✅ The JTAG transport's *failure path* is
> clean (orderly retry loop, no freeze) — the same negative-control value the
> i2041 SBW failure has, but here the failure is **by design**. ❌ It does **not**
> yet validate a JTAG *success* — that needs a **JTAG-capable** target. On this
> board those are the **LQFP64 F1xx/F2xx parts**: **F1611 ✅** and **F2418 ✅** now
> do exactly that (entries above). *(The F2618 sample is dead — marked "Bad
> Tracks" — so it won't be sampled.)* Contrast the same G2955 over **SBW**, which
> identifies cleanly (entry above) — so the chip and STLinkV2 wiring are good; only
> the *transport* is wrong for this part.
>
> ⚠ **Doc note (now resolved — see F1611 above):** the wiring guide used to
> describe the STLinkV2 path as "SBW-only." That is **disproven** by the F1611
> JTAG success above — the STLinkV2 maps all four JTAG signals (JTCK/JTDO/JTDI/JTMS
> + RST/TEST, §2b) and debugs a JTAG-only LQFP64 part fine. §4.3 corrected.

### MSP430F5418A — SLAU208 (F5418 proto-board)

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Wiring:** the SLAU208 STLinkV2 SBW-wire path — STLink-Adapter (JTAG-20→14),
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

### CC430F5137 — SLAU259 (CPUXv2, CC430 RF SoC) — first third-party board

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** **Olimex MSP430-CCRF** (non-repo third-party). JTAG-14 connector;
  the only config jumper selects **self-powered vs probe-powered**. STLinkV2 via
  the same hand-wire as the proto-boards — **bench-confirmed**.
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

### CC430F5137 (JTAG) — SLAU259 — ✅ **same part over BOTH transports; first CPUXv2 over JTAG**

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** **Olimex MSP430-CCRF** (CC430F5137 RF SoC). This board supports **both
  transports** — the [SBW entry above](#cc430f5137--slau259-cpuxv2-cc430-rf-soc--first-third-party-board)
  was the same chip over 2-wire SBW. JTAG here is the plain ribbon path
  (STLink-Adapter 20→14 + 14-wire ribbon, no hand-wiring).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/cc430f5137_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/cc430f5137_jtag_stlinkv2_init.txt)

```
jtag_id     0x91          → CPUXv2
coreip_id   0x1101        → CC430 core IP (same as the SBW capture)
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00       (Xv2 TLV — NOT the legacy 0x0ff0)
raw[0..3]   0606 16c7 3751 1212   ← IDENTICAL to the SBW capture, word-for-word
mcu_ver/rev/cfg 3751 / 12 / 12
profile     CC430F5137 [CPUXv2] [EMEX_SMALL_5XX] [SLAU259]
HW bkpts    3
```

Memory map matches the SBW capture (BSL `0x1000-0x17ff`, Info `0x1800-0x19ff`,
Boot ROM `0x1a00-0x1aff`, RAM `0x1c00-0x2bff` 4 KB, Main Flash `0x8000-0xffff`
32 KB).

> **Two firsts, one important.**
> 1. **First cross-transport consistency check.** The *same* CC430F5137 was read
>    over **SBW** and now **JTAG**, and both return the **identical four-word
>    descriptor `0606 16c7 3751 1212`** from the `0x1a00` TLV. This proves the two
>    transports' identify front-ends produce the same result on the same silicon —
>    a strong validation that neither path corrupts or transforms the read.
> 2. **First CPUXv2 over JTAG.** Every prior JTAG capture was legacy `0x89` (ID at
>    `0x0ff0`); every prior CPUXv2 (`0x91`/`0x99`, TLV at `0x1a00`) was SBW. This
>    closes the gap — the **Xv2 TLV identify path is now validated over JTAG too**,
>    so all four (legacy/SBW, legacy/JTAG, Xv2/SBW, Xv2/JTAG) identify-path × CPU-class
>    combinations are confirmed.

### MSP430FR5739 — SLAU272 (CPUXv2 FRAM, FR57xx) — ⚠ DB tags wrong family UG

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU272_SLAU367 proto-board (2nd sample; the FR57xx / SLAU272
  family). Same hand-wire as the FR5858 — **bench-confirmed**.
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

### MSP430FR5739 (JTAG) — SLAU272 — ✅ third dual-transport part (FR57xx, `raw[0]=0x0505`)

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** SLAU272_SLAU367 proto-board (FR57xx sample). Same chip as the FR5739
  **SBW entry above**; **both transports** confirmed. JTAG = J3 in the JTAG jumper
  layout + plain ribbon.
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/fr5739_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5739_jtag_stlinkv2_init.txt)

```
jtag_id     0x91          → 0x91 (NOT 0x99) yet still CPUXv2 — coreip_id is the discriminator
coreip_id   0x1106        → CPUXv2 FRAM
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0505 311e 8103 2626   ← IDENTICAL to SBW; raw[0]=0x0505 (FR57xx marker, vs 0x0606)
mcu_ver/rev/cfg 8103 / 26 / 26
profile     MSP430FR5739 [CPUXv2] [FRAM] [EMEX_SMALL_5XX] [SLAU321]   ← ⚠ wrong UG tag (should be SLAU272)
HW bkpts    3
```

Memory map matches the SBW capture (TinyRAM `0x0006-0x001f`, BSL ROM, Info FRAM
`0x1800-0x18ff`, RAM `0x1c00-0x1fff` 1 KB, Main FRAM `0xc200-0xffff` 15.5 KB).

> **Third cross-transport anchor — pattern now solid.** Same FR5739, identical
> descriptor `0505 311e 8103 2626` over SBW and JTAG, with the FR57xx `raw[0]=0x0505`
> signature (vs `0x0606` on FR58xx/59xx). Two reinforcing details: (1) `jtag_id`
> alone still doesn't classify the core — this FR57xx returns `0x91` like a Flash
> part, and `coreip_id 0x1106` is the discriminator, over JTAG just as over SBW;
> (2) the **`[SLAU321]` DB-mislabel persists over JTAG** — as expected, since it's
> a `ChipInfoDB` family-UG bug independent of transport (FR5739 is SLAU272 per the
> wiki `Home.md`). The three SLAU272/367-board FRAM parts (FR5858 `0x99`, FR5739
> `0x91`) plus the CC430 now give three byte-identical SBW↔JTAG descriptor matches.

### ❌ MSP430i2041 — SLAU335 (i20xx metering) — FAILED, under investigation

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU335 proto-board. Mode jumper **J3** (6 jumpers: top 4 = JTAG,
  last 2 = SBW); STLinkV2 uses the SLAU208-style exception (J3 in JTAG layout +
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
> Contrast: the G2xxx parts (also legacy SBW) and the F5418A (SLAU208
> hand-wire) both identify fine, so this is i20xx- and/or SLAU335-board-specific.
>
> **Update (root cause reframed — see the i2041 JTAG entry below + [#43](https://github.com/grumat/glossy-msp430/issues/43)):**
> the i2041 now also fails over **JTAG**. Failing on *both* transports rules out
> the SBW hand-wire / RC / wiring hypotheses above — the cause is **device-access**:
> the i20xx family requires a **JTAG access password / key** that Glossy doesn't yet
> present. The "physical layer works but no valid ID" symptom here is exactly that.

### ❌ MSP430i2041 (JTAG) — SLAU335 — FAILED on BOTH transports → device-access (password)

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire), J3 in the JTAG jumper
  layout + plain ribbon (STLink-Adapter 20→14).
- **Result:** ❌ **`jtag_init: no device found`** — identical symptom to the i2041
  **SBW failure entry above** (#40).
- **Dump:** [`INIT_TRACE_VALIDATION/i2041_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/i2041_jtag_stlinkv2_init.txt)

```
Starting JTAG
jtag_init: no device found
initialization failed
… (retries, no valid TAP ID)
```

> Tracked in **[#43](https://github.com/grumat/glossy-msp430/issues/43)** (root
> cause), the broadened companion to **[#40](https://github.com/grumat/glossy-msp430/issues/40)** (SBW symptom).
>
> **This is the decisive datapoint.** The i2041 datasheet confirms it supports
> **both SBW and JTAG**, and the part now fails identically on *both* — the 2-wire
> SBW path **and** the 4-wire JTAG path (different pins, different cabling, JTAG on
> the plain ribbon). A wiring / RC / transport fault cannot explain both, so the
> earlier #40 SBW-hand-wire hypotheses are excluded. **Root cause: the MSP430i20xx
> family gates JTAG/SBW access behind a password / access key** that must be
> presented during init before the TAP returns a valid ID — which Glossy does not
> yet do. (Confirm the exact key/sequence against the MSPDebugStack i20xx device
> handling, SLAU335, or an eZ-FET golden-reference capture — see #43.) The i2041 is
> the **only** part that fails on either transport; every other legacy/Xv2,
> Flash/FRAM part identifies cleanly both ways.
### MSP430FR5858 — SLAU367 (CPUXv2 FRAM)

*(Family UG **SLAU367** per the wiki `Home.md`; firmware chip-DB prints
`[SLAU378]`, same FR58xx/59xx/6xx family guide as the FR5994.)*

- **Probe:** **STLinkV2**. **Transport:** **SBW** (2-wire).
- **Board:** SLAU272_SLAU367 proto-board (TSSOP-38). STLinkV2 via the proto-board
  JTAG-14 hand-wire (STLink-Adapter 20→14, J3 in JTAG layout, SWDIO→J7 pin 11,
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

### MSP430FR5858 (JTAG) — SLAU367 — ✅ second dual-transport part; first `0x99` (FRAM-Xv2) over JTAG

- **Probe:** **STLinkV2**. **Transport:** **JTAG** (4-wire).
- **Board:** SLAU272_SLAU367 proto-board (TSSOP-38). Same chip as the
  [SBW entry above](#msp430fr5858--slau367-cpuxv2-fram); this board does **both
  transports**. For JTAG, set **J3 to the JTAG jumper layout** and use a plain
  ribbon — so this is also the **first capture exercising that proto-board's
  actual JTAG jumper mode** (the SBW entry used the J3-in-JTAG + hand-wire trick).
- **Result:** ✅ clean — TAP identified, profile resolved, GDB reader loop entered,
  no errors.
- **Dump:** [`INIT_TRACE_VALIDATION/fr5858_jtag_stlinkv2_init.txt`](INIT_TRACE_VALIDATION/fr5858_jtag_stlinkv2_init.txt)

```
jtag_id     0x99          → CPUXv2 (FRAM 0x99 variant — the #19/#20 ReadWords branch)
coreip_id   0x1106        → CPUXv2 FRAM
device_id   0x0000        (Xv2: real ID from the TLV)
id_data_addr 0x1a00
raw[0..3]   0606 77ba 8158 3040   ← IDENTICAL to the SBW capture, word-for-word
mcu_ver/rev/cfg 8158 / 40 / 30
profile     MSP430FR5858 [CPUXv2] [FRAM] [EMEX_SMALL_5XX] [SLAU378]
HW bkpts    3
```

Memory map matches the SBW capture (TinyRAM `0x0006-0x001f`, BSL ROM
`0x1000-0x17ff`, Info `0x1800-0x19ff`, Boot ROM `0x1a00-0x1aff`, RAM
`0x1c00-0x23ff` 2 KB, Main FRAM `0x4400-0xffff` 47 KB).

> **Second cross-transport anchor, and it lands on the `0x99` branch.** Like the
> CC430, the *same* FR5858 returns the **identical descriptor `0606 77ba 8158
> 3040`** over SBW and JTAG. Crucially this is the **`jtag_id 0x99` FRAM-Xv2
> path** — the exact `ReadWords` branch that was the subject of issues **#19/#20**
> and the FR5994 golden-reference work — now confirmed correct **over JTAG** as
> well as SBW. Combined with the CC430 (`0x91`/JTAG), both CPUXv2 TAP flavours
> (`0x91` and `0x99`) are now validated on the JTAG transport.

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

