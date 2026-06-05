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
