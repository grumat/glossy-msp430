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
