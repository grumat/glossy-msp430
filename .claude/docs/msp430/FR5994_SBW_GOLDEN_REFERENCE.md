# FR5994 SBW "Golden Reference" — eZ-FET `MSP430_OpenDevice` capture

Authoritative on-wire reference for a **successful** MSP430FR5994 acquisition over
single-pin Spy-Bi-Wire. Captured from the **LaunchPad's on-board eZ-FET** (TI MSP430.dll
v31501001) driving the same two-wire SBW topology we use, while our own probe still
fails to read the descriptor. Use this when reviewing `TapDev430Xv2` / `TapMcu` SBW
code: every byte and **every TCLK phase** here is known-good.

- **Capture:** `supp/docs-ai/test-logic-analyzer.csv` (2 ch: SBWDIO, TEST/SBWCLK), 0.26 s, 46 631 SBWTCK pulses.
- **Decoder:** `supp/docs-ai/decode_sbw_fsm.py` — TAP-FSM decoder (see *Conventions* below). Re-run: `python supp/docs-ai/decode_sbw_fsm.py`.
- **Host app:** `supp/MinimalMSP430_dll/MinimalMSP430_dll.cpp` — only `MSP430_OpenDevice("MSP430FR5994", …)` produces SBW traffic; it reported `device.id=489 (0x1E9)`, `MSP430FR5994`, 3 breakpoints, emulation=5.
- **Target state:** the capture reads the reset vector at `0xFFFE` back as **`0xFFFF`** (seq 48–50) — i.e. the reset vector is **erased**. Corroborated out-of-band: the board has sat unused for years and shows **no LED activity on power-up**. Whether the whole part is blank or just the vector is moot here — the point is it still yields the descriptor with **no magic pattern** (see *Findings*).

## Conventions (standard JTAG — do not invent new ones)

- Names are the JTAG-standard **TMS / TDI / TDO / TCLK**. TDI is master(probe)-driven, TDO is target-driven. We keep these names everywhere; layering an OUT/IN convention on top only adds a third thing to confuse.
- **SBW slot structure:** every JTAG TCK = 3 SBWTCK low pulses, in order **TMS, TDI, TDO**. TMS/TDI are sampled while SBWTCK is high (host setup); TDO is sampled mid-low (target turnaround — its pulse is visibly wider in the capture).
- **TCLK is NOT a wire and NOT TCK.** TCLK is the **level on TDI while the TAP sits in Run-Test/Idle**; that level gates the MSP430 CPU clock so a memory fetch can advance. Each Idle visit applies one TCLK phase; toggling TDI across Idle visits clocks the core. This is what our `RenderTransaction(..., tclk_high)` argument tracks.

### TCLK notation (used in the function map below)

TCLK can change **before and after** each shift frame, so we annotate every frame
with the TCLK activity in the Run-Test/Idle TCK(s) that border it, read left→right
in time:

| symbol | meaning |
|---|---|
| `H` | TCLK driven **high** |
| `L` | TCLK driven **low** |
| `K` | **keep** — inherit the level the previous frame left (no re-drive). Only as the **leading** symbol at a **block entry**, e.g. `KH`, `KL`. |

Rules: each `H`↔`L` flip is **one CPU-clock edge** into the core; consecutive equal
levels (dwell) collapse to one symbol; a frame that does not change TCLK is **omitted**.
`K` alone = entered a block and kept the level unchanged. The decoder prints `|gap`
before the symbols at a >2 ms inter-transaction boundary.

> Example — the descriptor quick-read enters Idle **high** (`H` at ADDR_CAPTURE) and
> then per word: `L` (word 0, just the falling edge — already high), then `HL`, `HL`,
> `HL` (words 1–3, a full rise+fall each). One CPU edge per fetched word, exactly.
- **DR values are shifted MSB-first on the wire**; the decoder prints the bit-reversed (real) value plus the raw. **IR values are shown in slau320 doc-notation** (the pre-reversal opcode); our `Ir::` enum stores the reversed wire byte.

## IR opcode map (doc-notation ↔ `Ir::` enum ↔ TI macro)

| trace `IR=` | `Ir::` enum (wire) | meaning |
|---|---|---|
| 0x14 | `kCntrlSigCapture` (0x28) | capture control-signal reg / auto-presents jtag_id (0x99) |
| 0x13 | `kCntrlSig16Bit` (0xC8) | write 16-bit control-signal reg |
| 0x2F | `kTestReg3V` (0xF4) | 3V test reg (LPMx.5 disable, 0x40A0) |
| 0x2A | `kTestReg` (0x54) | 32-bit test reg (0x00010000) |
| 0x0D | `kEmexDataExchange32` (0xB0) | EEM/clock-control 32-bit exchange (GENCLKCTRL) |
| 0x0C | `kEmexWriteControl` (0x30) | EEM write-control (EMU_CLK_EN/EEM_EN) |
| 0x41 | `kData16Bit` (0x82) | 16-bit data register |
| 0x42 | `kDataCapture` (0x42) | capture data register |
| 0x43 | `kDataQuick` (0xC2) | CPU-fetch auto-increment read |
| 0x83 | `kAddr16Bit` (0xC1) | 16/20-bit address register |
| 0x84 | `kAddrCapture` (0x21) | capture MAB (address bus) |
| 0x85 | `kDataToAddr` (0xA1) | explicit data-to-address read |
| 0x17 | `kCoreIpId` (0xE8) | read coreip_id (0x1106) |
| 0x87 | `kDeviceId` (0xE1) | read id_data_addr pointer (→ 0x1A00) |
| 0x61 | `kJmbExchange` (0x86) | JTAG mailbox (0xA55A appears at seq 379, *after* ID) |

## Phase timeline

| phase | t | shifts | what |
|---|---|---|---|
| 0 | 51.7 ms | seq 0 | lone jtag_id probe → 0x99 |
| 1 | 56–59 ms | seq 1–22 | fuse/integrity check, LPMx.5 disable, clock+EEM enable, sync, **apply POR (0x0C01)** |
| — | *40 ms* | — | **`IHIL_Delay_1ms(40)`** POR settle (this is the "gap", *not* a reset) |
| 2 | 100–142 ms | seq 23–256 | release POR (0x0401) → SAFE_PC park → context save → coreip → device-ID ptr → **descriptor read** |
| 3 | 142 ms+ | seq 257–795 | register reads / memory dump / release (incl. mailbox 0xA55A at seq 379) |

## Function-boundary map (identify path, seq 0–78) — our driver vs. the wire

> This is the part to diff against `TapDev430Xv2` / `TapMcu` during review.

| seq | t (ms) | wire activity | TI macro | **our driver function** | key values |
|---|---|---|---|---|---|
| 0 | 51.7 | IR 0x14 | cntrl_sig_capture | `TapMcu::InitDevice` entry probe | jtag_id **0x99** |
| 1–7 | 56.1 | IR 0x14 + DR16 0x5555 ×3 | GetCoreID integrity/fuse | `TapMcu::InitDevice` / `GetDevice` pre-check | TDO 0x8483 stable |
| 8–9 | 57.3 | IR 0x2F → DR16 **0x40A0** | test_reg_3V | `DisableLpmx5` (jtag_id 0x99) | 0x40A0 |
| 10–11 | 57.5 | IR 0x2A → DR32 **0x00010000** | test_reg | `DisableLpmx5` (jtag_id 0x99) | 0x00010000 |
| 12–16 | 57.8 | IR 0x0D/0x0C → DR32 | eem_data_exchange32 / write_control | `SyncJtagAssertPorSaveContext` clock-enable preamble | GENCLKCTRL, EMU_CLK_EN |
| 17–18 | 58.7 | IR 0x13 → DR16 **0x1501** | cntrl_sig_16bit (TCE1+CPUSUSP+RW) | `SyncJtagAssertPorSaveContext` | 0x1501 |
| 19–20 | 58.9 | IR 0x14 → DR16 | wait_for_synch (poll bit 0x0200) | `WaitForSynch` | sync bit |
| 21–22 | 59.1 | IR 0x13 → DR16 **0x0C01** | **apply POR** | `SyncJtagAssertPorSaveContext` | 0x0C01 |
| *(gap)* | *40 ms* | — | `IHIL_Delay_1ms(40)` | (our POR settle delay) | — |
| 23 | 100.2 | DR16 **0x0401** | **release POR** | `SyncJtagAssertPorSaveContext` | 0x0401 |
| 24–25 | 100.5 | IR 0x41 → DR16 **0x0004** | force PC = SAFE_PC_ADDRESS | `SyncJtagAssertPorSaveContext` (SAFE_PC park) | 0x0004 = `JMP $` |
| 26–32 | 100.7 | IR 0x42/0x13/0x0C | data_capture, EEM feature enable | `SyncJtagAssertPorSaveContext` | EMU_FEAT_EN |
| 33–36 | 101.5 | IR 0x83 → **0x015C**, IR 0x42 read | hold Watchdog (WDTCTL@0x015C) | `SyncJtagAssertPorSaveContext` WDT-hold | 0x015C, read 0x6904 |
| 37–46 | 102.1 | addr_capture (MAB), ctrl-sig | save context / read PC (MAB−4) | `SyncJtagAssertPorSaveContext` | MAB |
| 47–50 | 103.3 | IR 0x83 → **0xFFFE**, read **0xFFFF** | Note-1495 ReadMemWord(0xFFFE) | `SyncJtagAssertPorSaveContext` reset-vector read | **0xFFFF ⇒ blank** |
| 51–53 | 104.0 | IR 0x0D → DR32 | restore GENCLKCTRL (MCLK_SEL0) | `SyncJtagAssertPorSaveContext` clock restore | — |
| 54–56 | 105.0 | IR 0x14, IR 0x17 → **0x1106** | GetCoreipIdXv2 | `GetDevice` (`kCoreIpId`) | coreip **0x1106** |
| 57–58 | 105.3 | IR 0x87 → **0x1A00** | device-ID pointer | `GetDevice` (`kDeviceId` → `id_data_addr_`) | **0x1A00** |
| 59–72 | 106.0 | IR 0x14/0x41/0x13/0x41 + ADDR_CAPTURE | SetPCXv2(0x1A00) | `SetPC(0x1A00)` | Mova 0x0080, Pc 0x1A00, MAB 0x1A000 ✔ bit-for-bit. TCLK: `L`(MOVA) … `H`(addr) `LH`(NOP) `L`(capture) |
| 73–74 | 107.7 | IR 0x84, IR 0x43 | addr_capture + **DATA_QUICK** | `ReadWords` | TCLK driven **`H`** entering the loop |
| **75–78** | 108.0 | DR16 ×4 | auto-increment quick read | `ReadWords` (the loop) | **0x0606, 0x9B74, `0x82A1`, 0x1021**; TCLK **`L · HL · HL · HL`** = 1 edge/word |

### Descriptor bytes read at 0x1A00 (seq 75–78)

```
0x1A00: 0x0606   (info_len=0x06, crc_len=0x06)
0x1A02: 0x9B74   (CRC16)
0x1A04: 0x82A1   <-- mcu_ver  == ChipInfoDB FR5994 entry
0x1A06: 0x1021   (fw/hw rev)
```

## Critical findings (where our driver diverges)

1. **No magic pattern is used to identify the device.** The 40 ms "gap" is the POR
   settle delay *inside* `SyncJtag_AssertPor` (0x0C01 → 40 ms → 0x0401), not a
   reset/relatch. The mailbox `0xA55A` appears only at seq 379 (t≈157 ms), ~50 ms
   **after** the descriptor was already read. ⇒ Issue #20 (MagicPattern acquisition)
   is **not** what blocks FR5994 identification on this device.

2. **The reset vector reads 0xFFFF (erased, seq 50) — yet the descriptor at 0x1A00
   reads fine.** (Out-of-band: years in a drawer, no power-up LED — consistent with an
   erased/idle part.) ⇒ The "blank → LPM4 → 0x1A00 vacated → needs magic pattern" theory
   does not hold for this part/this capture. 0x1A00 is reachable from a plain synced,
   POR'd, SAFE_PC-parked state regardless of whether user flash is programmed.

3. **`DATA_QUICK` auto-increment WORKS on jtag_id 0x99.** The eZ-FET does **one**
   `SetPC(0x1A00)` then a single `DataQuick`, then reads 4 words with **no SetPC between
   them** — auto-incrementing through 0x1A00→0x1A06, one TCLK edge per word. This
   contradicted our old `ReadWords` 0x98/0x99 branch, which did a **fresh `SetPC` per
   word** on the (disproven) assumption that auto-increment derails at 0x1A00. **Issue
   #19's per-word-SetPC fix was misdiagnosed; that branch is now removed** — all Xv2 use
   the single-SetPC auto-increment path.

4. **TCLK phase is exact: one CPU edge per fetched word.** The loop *enters* Idle high
   (`H` at ADDR_CAPTURE, seq 73), then per word: `L` (word 0 — already high, just the
   falling edge), `HL`, `HL`, `HL` (words 1–3 — full rise+fall). One `Tclk` edge per
   fetch, no more, no fewer — phase `H · L · HL · HL · HL`.

5. **Our `SetPC` and `GetDevice` match the wire bit-for-bit** (seq 54–72) — at the
   *TDI-slot* level. The actual bug lived in the **TMS slot**, invisible to a TDI-slot
   decode (see root cause below).

## Root cause & fix (BENCH-CONFIRMED 2026-06-03)

After collapsing `ReadWords` to auto-increment (fix #1), a bench capture still gave
`raw[0]=0x00f0, raw[1..3]=0x3fff`. Decoding **our** capture and diffing it against this
golden reference pinned it precisely:

- **`SetPC`'s `MOVA #0x1A00,PC` never executed.** The `ADDR_CAPTURE` after SetPC read
  MAB = `0x0020` (ours) vs **`0x1A000`** (golden) — PC was never loaded, so every fetch
  read vacant memory. The descriptor read failed *upstream* of the quick-read loop.
- The SetPC wire was **byte- and TDI-slot-identical** to the golden, and our SBW timing
  was actually *slower/safer* (2.0 µs period vs 1.76 µs). The divergence was a **spurious
  edge in the TMS slot**, which a TDI-slot decoder cannot see.
- **The bug (transport, `SbwDev.tim.cpp`):** the old `SbwBbSetTclk`/`SbwBbClrTclk`
  hardcoded the TMS-slot variant on the *transition direction* (`SetTclk`→`TMSL`,
  `ClrTclk`→`TMSLDH`). `OnPulseTclk`/`OnPulseTclkN` call them in a pair, and the first
  sub-bit's assumption is violated by the entry level. In `SetPC`, the first `OnPulseTclkN`
  is entered **LOW**, so `SbwBbClrTclk` ran `TMSLDH` (drives SBWTDIO low→**high** in the
  TMS slot) on a line that should stay low.
- **Per slau320aj §2.2.3.5.1, that rising edge in the TMS slot "is interpreted as a trigger
  for TMS = 1 and the TAP controller would leave Run-Test/Idle mode."** So the phantom edge
  didn't just add a clock — it kicked the TAP out of Idle mid-MOVA, leaving PC at `0x0020`.
- **The fix:** `SbwBbTclkBit(level)` chooses the TMS-slot variant from the **live level**
  `s_sbw_tclk_high` (`TMSLDH` to *maintain* high, `TMSL` to *maintain* low) and drives the
  new level only in the TDI slot — a faithful port of TI's `_hil_2w_Tclk` (hil_2w.c:377).
  No phantom TMS-slot edges; TMS stays honestly 0.

**Result:** `raw = 0x0606, 0x9b74, 0x82a1, 0x1021` — word-for-word match with this golden
reference; profile loads as `MSP430FR5994`.

### TCLK strobes (§2.2.3.5.2) — why they don't help here

slau320aj §2.2.3.5.2 allows a *custom number of TCLK clocks within a single TDI slot*
("TCLK strobes") — but **only for F1xx / F2xx / G2xx / F4xx**. It is **explicitly not
applicable to F5xx/F6xx/FR5xx/FR6xx** (FR5994 included); on those, flash timing comes from
the internal MODOSC. So FR5994 must use the standard §2.2.3.5.1 method: one TCLK cycle =
two TDI slots (Set + Clr) with the TMS slot holding the line. (The strobe shortcut is still
valid for the older-family flash path, e.g. `OnFlashTclk`.)

## See also

- slau320aj **§2.2.3.5.1** (SetTCLK/ClrTCLK over SBW — TMS-slot level hold; TMS=1 hazard)
  and **§2.2.3.5.2** (TCLK strobes; family restriction).
- `supp/.../Bios/src/hil/fet_generic/hil_2w.c` (`_hil_2w_Tclk`, `TMSLDH`/`TMSL`) — the reference the fix ports.
- `supp/.../Bios/src/hal/macros/SyncJtag_AssertPor_SaveContextXv2.c` / `ReadMemQuickXv2.c` — phases 1–2 replay.
- Issues #19 (descriptor garbage — **fixed**) / #20 (magic pattern — **not the cause; close/repurpose**).
- `memory/reference_xv2_vacant_memory_3fff.md` — supersede the "needs magic pattern / per-word SetPC" conclusion.
