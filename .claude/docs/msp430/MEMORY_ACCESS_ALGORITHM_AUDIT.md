# Memory-access algorithm audit: bus width / quick-read / quick-write per region, per SLAU family

Supersedes/broadens `.claude/issues/052-dataquick-fast-read-family-profiling.md` (that file's
DataQuick investigation stays as the historical record of how this started; this doc is the
forward-looking plan). Scope is restricted to **non-FRAM** parts — FRAM (SLAU272/SLAU367,
`fIsFram`) gets its own future ticket since large parts of the write/erase infrastructure for it
don't exist yet.

## The question

`TapMcu`'s memory access (`ReadMem`/`WriteMem` → `OnReadWords`/`OnWriteWords` →
`ITapDev::ReadWords`/`WriteWords`/`ReadBytes`) currently makes almost all of its access-strategy
decisions from **one global, per-chip flag** (`ChipProfile::fQuickMemRead`) plus a **memory-type
gate added this session** (RAM/Flash only, `TapDev430X::ReadWords`). Real MSP430 devices need a
finer decision per **(memory region, direction, family)**, and the flat flag is demonstrably
wrong for at least one region/family combination (classic `TapDev430` quick-read on F1611/SLAU049
produces garbage). The goal of this audit is to replace "one flag, hope it generalizes" with a
small number of explicit, bench-confirmed capability facts, filled in one SLAU family at a time.

## Reference memory map (MSP430F1611, SLAU049)

```
CPU:         0x0000-0x000f    16 B  [Regs]   -- off-bus, ULA-internal
EEM:         0x0000-0x007f   128 B  [Regs]   -- off-bus, EEM-block-internal
Periph8:     0x0000-0x00ff   256 B  [Regs]   -- 8-bit bus only
Periph16:    0x0100-0x01ff   256 B  [Regs]   -- 16-bit bus, byte access still legal
RAM:         0x0200-0x09ff    2 KB  [RAM]
BSL:         0x0c00-0x0fff  1024 B  [Flash - 4 banks]
Info:        0x1000-0x10ff   256 B  [Flash - 2 banks]
RAM2:        0x1100-0x38ff   10 KB  [RAM]   -- NOT a separate bank from RAM; F149-compat mirror
Main:        0x4000-0xffff   48 KB  [Flash]
```

Every non-FRAM family shares this same *shape* (CPU/EEM off-bus, Periph8, Periph16, RAM(+RAM2),
BSL, Info, Main) even where block sizes/counts differ. `RAM`/`RAM2` and `Info`/`Main` are
independently-keyed `MemInfo` records for map-printing convenience only — nothing in the firmware
should assume they're separately-addressed banks (confirmed: `FindMemByAddress` is a flat linear
scan over `mem_[]`, no bank concept).

## What's already correct today

- **`bitSize` is already per-region and already forced 8-bit for `Periph8`**:
  `ChipProfile.cpp:229` — `pTarget->bitSize = pTarget->memClass == kMkeyPeripheral8bit ? 8 : 16;`
  This is a single global rule, not per-family, which is fine — the 8-bit-only Periph8 bus width
  is an MSP430-architecture constant, not a family variable (SLAU049 through SLAU506 model
  Periph8bit identically; confirmed via device XML `Peripheral8bit_Default` reuse across F1xx/F2xx).
- **`ReadMem()` already consults `bitSize`** (`TapMcu.cpp:483`): `m->bitSize > 8 ? OnReadWords :
  OnReadBytes`. Reads into Periph8 already correctly split to byte access.
- **Erase paths already gate on `type == kMtypFlash`** (`EraseMain`/`EraseAll`/`EraseSegment`/
  `EraseRange`): non-flash addresses get a traced "not erasable" + silent success, never attempt
  an erase. This is the right shape to imitate for the write-side ROM guard below.
- **`accessMpu`/FRAM write-protect fields are already scoped to SLAU272/SLAU367 only** — no
  accidental bleed into the non-FRAM families this ticket covers.

## Concrete gaps found this pass (family-independent — fix once, not per-family)

**Status: done** — all three fixed in commit `9389a14` (GH #52, closed). Gaps 1 and 2
hardware-verified on F1611 (single-byte `P1OUT` write no longer disturbs `P1DIR`, both via the
RMW-boundary path and the aligned bulk-loop path). Gap 2's `kMtypRom` branch is code-complete and
mirrors the already-proven `EraseMain`/`EraseAll` pattern but isn't hardware-verified — F1611 has no
ROM-typed region reachable to exercise it against; deferred to whichever family sub-issue first has
one on the bench. Gap 3 verified by inspection (enum/array now aligned).

### 1. `WriteMem()` is not bus-width-aware — Periph8 writes are never split to 8 bits

Unlike `ReadMem()`, `WriteMem()` (`TapMcu.cpp:519`) never looks at `m->bitSize`. Every write —
including the unaligned-start/end Read-Modify-Write paths — goes through `OnWriteWords()` →
`WriteWords()` → per-word `WriteWord()`, which issues a 16-bit `IR_DATA_TO_ADDR` shift
unconditionally. A single-byte GDB write that lands in `Periph8` (e.g. `P1OUT` at 0x0021) gets
folded into a **16-bit** RMW that reads/writes its neighbor byte too. TI's own reference
(`JTAGfunc430.c::WriteMem`, `JTAGfunc430X.c::WriteMem_430X`) has an explicit byte-write mode we
never ported:

```c
IR_Shift(IR_CNTRL_SIG_16BIT);
if (Format == F_WORD) DR_Shift16(0x2408);   // Set word write
else                  DR_Shift16(0x2418);   // Set byte write   <-- we have no equivalent
```

Our `WriteWord()` (`TapDev430.cpp:843`, `TapDev430X.cpp:627`) always shifts `0x08`
("word write") into `kCntrlSigLowByte` — there is **no `WriteByte`/`WriteBytes` anywhere in the
codebase** (`ITapDev.h` declares `ReadByte`/`ReadBytes` but no write counterpart at all).

**Fix**: add `WriteByte(address, uint8_t)` / `WriteBytes(address, buf, count)` to `ITapDev` and
both `TapDev430`/`TapDev430X` (mirrors the existing `ReadByte`/`ReadBytes` shape exactly, using
control word `0x2418` in place of `0x2419`/`0x2409`'s low-byte-8 write variant — same shift
pattern as `WriteWord`, just the "byte write" control bit instead of "word write"). Then make
`TapMcu::WriteMem()`'s aligned-block loop and both RMW edges bitSize-aware, symmetric to
`ReadMem()`. This is architecture-constant (no per-family variation expected — byte-write mode is
a documented universal opcode, not a capability that varies by silicon generation), so it can be
implemented and hardware-verified once, ahead of any per-family sub-issue.

### 2. Writes to ROM-typed regions (`kMtypRom`) are not denied

`TapMcu::OnWriteWords()` (`TapMcu.cpp:510`):

```cpp
if (m->type != ChipInfoDB::kMtypFlash)
    pTraits_->WriteWords(addr, ...);   // <-- also taken for kMtypRom!
else
    pTraits_->WriteFlash(addr, ...);
```

Only `kMtypFlash` is special-cased; `kMtypRam` and `kMtypRom` both fall into the "regular write"
branch. On devices where BSL is ROM-typed (`kAccBslRomAccess`/`kAccBslRomAccessGR`, not the
4-bank flash variant) or on `Boot`/`Boot2`/`MidROM` blocks, a GDB write attempt currently issues a
raw `IR_DATA_TO_ADDR` write against ROM instead of being rejected — harmless on real ROM silicon
(write is simply ignored by the target) but incorrect/misleading at the protocol level: GDB is
never told the write didn't happen, and nothing distinguishes "wrote RAM" from "no-op'd against
ROM." **Fix**: add a `type == kMtypRom` guard in `WriteMem`/`OnWriteWords`, mirroring the existing
`EraseMain`/`EraseAll` "not erasable, silent acceptance" pattern (or fail loudly — worth a decision
call, see Open Questions).

### 3. `TapMcu::SlauName()` display array is shifted from SLAU272 onward

Found by cross-checking the enum against the display table while researching this doc — not
something a family audit would have caught on the bench, since it's a display-only bug. The enum
(`ChipInfoDB.h:983`) has 11 members: `049,056,144,208,259,272,321,335,367,445,506`. The name
table (`TapMcu.cpp:765`) has 11 strings too (`static_assert` on `kSlau_Last_+1` passes) but they're
misaligned starting at index 5:

| index | enum value | array string (wrong) |
|---|---|---|
| 5 | `kSLAU272` | `"SLAU321"` |
| 6 | `kSLAU321` | `"SLAU335"` |
| 7 | `kSLAU335` | `"SLAU367"` |
| 8 | `kSLAU367` | `"SLAU378"` (this SLAU number doesn't exist) |
| 9 | `kSLAU445` | `"SLAU445"` (coincidentally correct) |
| 10 | `kSLAU506` | `"SLAU506"` (coincidentally correct) |

Every `monitor chipinfo`/`jtag_scan` trace for a SLAU272/321/335/367 part currently prints the
wrong manual number. Out of this ticket's direct scope (those are FRAM-adjacent/rare families),
but it's a one-line fix in the same function this audit will be reading constantly for family
labels — do it now rather than let bad labels contaminate audit notes. **Fix**: insert `"SLAU272"`
at index 5, drop the fabricated `"SLAU378"`.

## Why the F1611 DataQuick failure probably isn't a code bug

Re-examined while writing this doc: `TapDev430X` does **not** override `SetPC`, `HaltCpu`, or
`ReleaseCpu` — it inherits all three unchanged from `TapDev430`. The reverted classic DataQuick
read (`TapDev430::ReadWords`) and the working extended one (`TapDev430X::ReadWords`) therefore
issue a **byte-for-byte identical** JTAG sequence (`SetPC(addr-4)` → shared `HaltCpu()` →
`kCntrlSig16Bit=0x2409` → `IR_DATA_QUICK` → per-word `SetTCLK`/`DR_Shift16`/`ClrTCLK` → shared
`ReleaseCpu()`). There is no code-path divergence left to find by further diffing the two drivers.
This reframes the open question: it's not "find the bug in `TapDev430::ReadWords`," it's "does
this specific silicon generation's JTAG controller actually support the auto-increment quick-read
fetch during a halted-CPU read the way TI's `quickMemRead` XML flag and slau320aj Table 2-14
claim" — i.e. treat it as an empirical hardware-capability question, not a firmware defect, and
budget the sub-issue accordingly (bench characterization, not code archaeology).

One relevant fact for that characterization: F2418 (the working case) is a **CPUX** part
(`kCpuX`, 20-bit addressing) even though it's in the SLAU144 family, while F1611 (the broken case)
is a plain **CPU** part (`kCpu`, 16-bit addressing) in SLAU049. Since `SetPC`'s 16-bit `mov
#addr,PC` priming instruction is identical either way (both devices' addresses fit in 16 bits, so
CPUX's 20-bit `MOVA` path is never engaged here), architecture width doesn't explain the
difference either — but it does mean SLAU144 sub-issue testing should include **both** a CPUX
part (F2418, already confirmed working) **and** a plain-CPU part (e.g. G2553) to separate "SLAU144
family" from "CPUX architecture" as the actual explanatory variable, since today's one data point
conflates them.

## Where the capability data does (and doesn't) come from

Confirmed by reading the raw device XML (`ChipInfo/ExtractChipInfo/MSP430-devices/devices/*.xml`)
directly, not just the imported DB: TI's `<features><quickMemRead>` is a **flat boolean**,
inherited from a per-family `<features id="Default_2xx">`-style base and overridden per device —
there is **no separate SRAM-quick / Flash-quick element anywhere in the source XML**. The
finer-grained split (SRAM independently R/W-quick vs Flash independently R-only-quick, per slau320aj
Tables 2-14/2-15/2-16) exists **only** in the human-authored PDF manual tables, never made it into
TI's machine-readable device database, and therefore cannot be derived — it must be captured by
hand, per family, from the manual + bench confirmation. This is the concrete reason the per-family
audit approach (rather than "just read another column out of the DB") is the only path forward.

## Proposed capability model

Add a small, hand-curated, per-`EnumSlau` capability table — **not** derived from
`fQuickMemRead` (that flag stays as the per-chip DB-sourced "family is quick-read-capable at all"
signal, used as one of several ANDed conditions, same role it plays today) — something like:

```cpp
enum class QuickCap : uint8_t { kUnknown, kNone, kSramOnly, kFlashReadOnly, kSramAndFlashRead };
// One entry per EnumSlau, default kUnknown until a sub-issue bench-confirms it.
// kUnknown must NOT silently fall back to "assume capable" -- treat as kNone until proven.
// kFlashReadOnly exists because F1611/kSLAU049 bench-confirmed exactly this split: RAM
// quick-read unreliable (needs LA capture), Main-flash quick-read fully reliable (once the
// unrelated EraseFlash() missing-kReleaseCpu bug, fixed in d1ef4a8, is out of the way).
```

`ReadWords()`'s existing gate (`fQuickMemRead && memType is RAM/Flash`) becomes a third AND term:
`quickCapFor(chipInfo_.slau)` includes the region's type. This is exactly what lets each
sub-issue below "fill in one row" without touching the read/write dispatch logic again. Write-side
quick (`WriteMemQuick`/`WriteMemQuick_430X`) is explicitly **out of scope for non-FRAM parts** —
confirmed via slau320aj Tables 2-14/2-15 that real NOR Flash is quick-**read**-only in every row
checked (quick-write is FRAM-exclusive, Table 2-16, which this ticket excludes), and `WriteWords`
never routes to Flash addresses anyway (`OnWriteWords` sends flash-typed writes to `WriteFlash()`).
So the write half of this table only matters for RAM, where quick-write is uncontroversial (no
erase/timing concerns) — audit it opportunistically, not as a blocking item.

## Test-snippet infrastructure

Follow the existing project pattern for driver-decoupled bring-up instrumentation (see
`OPT_TEST_TIM_DMA_TIMING` / `util/TimDmaTiming.h` — an `OPT_*`-gated, default-off diagnostic mode
with its own isolated code path). Proposed: an `OPT_TEST_MEM_ACCESS_AUDIT`-gated GDB monitor
command (or extend `UnitTest.exe`) that takes a region name + algorithm choice (quick vs safe,
word vs byte) and exercises **just that one region with just that one algorithm**, dumping
pass/fail + a byte-level diff on mismatch. This replaces today's only signal (the general RAM/flash
test in `UnitTest.exe COM6 <chip> 1`, which mixes regions and made the F1611 failure hard to
localize — it took explicit code reading to even guess "Main-only" vs "RAM-only" as the next
narrowing step). Minimum viable surface: RAM-quick-read, Flash-Main-quick-read, Periph8-byte-write,
Periph16-byte-write, BSL-write-denied — five isolated probes cover everything this doc identified.

## SLAU049 bench session (2026-07-19, F1611 on BluePill/COM6): confirmed a real timing race, not a logic bug

Re-enabled the classic quick-read path behind runtime-tunable knobs (SetPC backward offset,
warm-up pulse count) and a diagnostic monitor command, then drove it two ways: isolated
word-aligned probes (always matched, no exceptions found), and raw GDB `M`/`m` packets replicating
the exact byte sequence `UnitTest`'s "TEST RAM WRITE MIXED PATTERNS" sends (this reproduced the
original failure immediately, byte-for-byte, "Got 0xFF instead of 0xE5 in address 0x1110").

Bisecting further overturned the "some request shape triggers it" framing entirely:

- **Repeated quick reads of the same, unchanged memory return different garbage each call.**
  Two consecutive `m1300,4` packets (no write in between) returned `2eff0000` then `ff3f0000` —
  proof this is a live sampling race, not a deterministic offset/addressing bug (a wrong offset
  would reproduce the *same* wrong value every time).
- **Every bus-speed grade was tried (slowest 562 kHz through fastest 9 MHz) — none was reliably
  correct.** "Slowest" appeared to fix a write-then-read pair on the first attempt, but the same
  address failed differently on a later retest at the same speed. No speed grade is safe to
  recommend.
- **SetPC backward-offset and warm-up-pulse variations changed *what* came back wrong, never
  produced a working pattern.** Offset 4 (TI/X-mirrored default) plus 0 extra pulses is the
  configuration that happens to pass `UnitTest`'s Phase 1 (uniform data) most reliably, but still
  fails Phase 2 (mixed unaligned writes) at the very first batched multi-word read.

This rules out every hypothesis this doc raised earlier (pipeline-depth offset, RMW-before-quick
hazard, an unrelated preceding batched write leaving bad state) — isolated reproductions of all
three passed cleanly. The failure only shows up through the *real* dispatch path (`Gdb::ReadMemory`
→ `TapMcu::ReadMem` → `OnReadWords` with `word_count>1`) issued as **separate GDB packets**, never
through a single diagnostic monitor command that does everything back-to-back after one
`gTapMcu.Halt()` resync — suggesting the race is sensitive to real inter-packet timing/state that
a same-command loop doesn't reproduce, not to any parameter this session could tune from software.

### Follow-up same session: it's a warm-up/settle characteristic, not pure noise, but not fully fixable in software either

Also checked whether BSL specifically (Flash-*typed* in our DB but structurally different from Main
— smaller, 4-bank, bootloader ROM code) fails differently from RAM, since the memory-type gate
lumps both under `kMtypFlash`. It doesn't fail differently: reading BSL (`0x0C00`) or the device
descriptor area (`0x0FE0`) via raw, separate GDB `m` packets showed the **same** pattern as RAM —
the first 1-2 quick reads after a fresh `jtag_scan` return garbage, then *stabilize* to a value that
exactly matches the safe-loop reference on every subsequent attempt (confirmed byte-for-byte
against `testquick`'s safe reference, repeatable across independent fresh-connect trials). So this
isn't memory-region-specific, and it isn't pure random noise either — it's a genuine warm-up/settle
characteristic of entering `IR_DATA_QUICK` on this core, and the *stabilized* value is correct.

That looked fixable: priming with throwaway quick-read cycles at the target address before trusting
the result. It partially is — but not with a fixed, small constant:

- 2 or even 8 throwaway **quick-mode** priming cycles (full `SetPC`/`HaltCpu`/enter-quick/read/
  release, discarded) before the real read did **not** fix the original `UnitTest` write-then-read
  failure (`TEST RAM WRITE MIXED PATTERNS`) — still failed identically.
- **1-2 plain safe `ReadWord()` calls** (not quick-mode) immediately before the real quick burst
  *did* fix it — `TEST RAM WRITE MIXED PATTERNS` passed cleanly with this in place.
- But the very next test in the suite, `TEST RLE RESPONSE PACKETS` (a longer sequence: ~100
  iterations of write-then-verify with progressively larger buffers), then failed with the same
  signature — the 2-safe-read prime got further into the suite than 1 did (moved the failure from
  iteration ~200 to ~192 repeats of a pattern, and changed the wrong byte from `0xFF` to `0x77`) but
  did not eliminate it. More prior write/read activity apparently needs more priming to stay
  reliable — a fixed constant tuned against one test doesn't generalize to a longer one.

This is consistent with a real hardware settle/warm-up time (e.g. a charge pump, internal
synchronizer, or clock-domain-crossing latch feeding the auto-increment logic) that decays or needs
re-priming based on actual elapsed time / amount of prior toggling, not something a constant
software delay can fully characterize without knowing the real time constant involved.

**Conclusion unchanged: this needs a logic-analyzer capture of TDO during a quick-read burst** —
now with a much sharper target for what to look for: capture across a *sequence* of quick reads
(not just one) spanning a preceding write, and look for a settling/rise-time signature on TDO or
TCK that would explain why reliability depends on recent activity. See the `analyze-jtag-la` skill
and `reference_logic_analyzer_csv.md` for the existing capture/decode workflow.

**Design principle for whatever the eventual fix is** (raised directly by the user, and correct):
GDB can request *any* memory address at *any* time — a live `x` command, a breakpoint read, the
identification sequence, anything — so the read-algorithm decision must be a uniform,
capability-driven gate applied identically regardless of which code path triggered the read. The
existing `GetDeviceSignature()` bypass (always safe-loop, because the chip's family isn't known yet
at that point) is a narrow, justified exception for that one specific ordering problem — it must
not become a precedent for "hardcode the known trouble spots and assume quick is fine elsewhere."
Until a bench-confirmed capability exists, the only correct entry in the proposed `QuickCap` table
for `kSLAU049` is "none" (RAM included) — not a partial table with some regions allowed and others
denied — because no fixed software knob was found reliable for *any* region on this family yet.

`TapDev430::ReadWords()` stays on the safe per-word loop (unchanged from before this session,
re-verified clean end to end: both `TEST RAM WRITE MIXED PATTERNS` and `TEST RLE RESPONSE PACKETS`
pass). All diagnostic scaffolding used across this session (`ReadWordsQuickDiag`, `testquick`
monitor command, the various priming/runtime knobs) was removed from the tree afterward rather than
left in place, per project convention on temporary diagnostic code — reconstruct from this doc + GH
issue #53 if a future session wants to resume this way, or better, go straight to the LA capture.

### Follow-up bench session (2026-07-19, continued): found a real, unrelated bug — and Main-flash quick-read is actually reliable

Picked back up to finish #53's test plan: F1121 access became available, and rather than testing
Main flash on blank/erased content (uninformative — both safe and quick trivially read `0xFFFF`),
wrote known content into a scratch Main segment (`0x4000`) via the existing `EraseSegment`/
`WriteFlash` path, then compared safe vs. quick reads against that.

First attempt was alarming: **both** the safe loop and the quick path returned `0x3FFF` repeated
for every word — not garbage, not almost-right, just uniformly wrong, and initially inconsistent
between otherwise-identical runs. `0x3FFF` is exactly the "JMP $" instruction `HaltCpu()`
force-feeds the CPU to keep it stopped — too specific a coincidence to ignore. Narrowed it down:

- An **untouched** flash region (never erased/written this session) read back correctly throughout
  — so this wasn't a device-wide or read-mechanism-wide failure, it was confined to whatever segment
  had just been erased/written.
- A plain `jtag_scan` re-attach (no probe reflash, just a fresh JTAG connect/POR) cleared the stuck
  state on the poisoned segment — subsequent reads of it were then correct.

That pointed straight at the erase/write code leaving something unrestored. Found it:
`TapDev430::EraseFlash()`'s and `TapDev430X::EraseFlash()`'s final step sequence had `kReleaseCpu`
**commented out** — every erase left the CPU improperly halted/unreleased, unlike the sibling
`WriteFlash()` which correctly releases it. Fixed in commit `d1ef4a8` (uncommented in both). This is
a **real, previously-unknown correctness bug**, unrelated to the DataQuick capability question, that
affected **every non-Xv2 family** using this probe (both `TapDev430` and `TapDev430X` had it —
`TapDev430Xv2::EraseFlash()` is a separate funclet-based implementation that already ends with an
explicit resync, so Xv2 parts were never affected). Anyone erasing/writing flash and then reading
back within the same JTAG session — including GDB's own post-flash verification, if a client does
one — could have hit this.

With the fix in place: erase → immediate read (no re-attach) correctly shows `0xFFFF`; write →
immediate read correctly returns the written pattern, repeatably, both via isolated probes and real
GDB `m` packets; re-erasing the same segment and reading again stays correct. **This closes test
plan item 2 from #53 with a positive result**: Main-flash quick-read is fully reliable on F1611,
once this unrelated bug is out of the way. It does **not** touch the RAM finding above — re-tested
`TEST RAM WRITE MIXED PATTERNS` fresh with the `EraseFlash` fix in place and quick-read still
re-enabled, and it still failed on the very first attempt, exactly as before. RAM and Flash-Main
are genuinely different here: **RAM quick-read is unreliable and needs the LA capture; Flash-Main
quick-read works correctly.** Updates the `QuickCap` picture for `kSLAU049`: not "none" outright —
`kSramNone`-but-`kFlashReadOk` is the accurate shape once this data point is included, pending the
RAM root-cause.

### F1121 cross-check (2026-07-19): confirmed family-wide, not F1611-specific — closes #53

F1121 (F1121A, second SLAU049 device) became available and was tested the same way. Its memory map
is much smaller than F1611's — RAM `0x0200-0x02FF` (256 B, no RAM2), Main flash `0xF000-0xFFFF`
(4 KB, 512 B segments) — but the JTAG/quick-read behavior tracks F1611 exactly:

- **RAM**: replayed the exact `TEST RAM WRITE MIXED PATTERNS` byte sequence (Phase 1 uniform write,
  Phase 2 mixed unaligned writes) via raw GDB packets. Phase 1 passed; Phase 2's first batched quick
  read (the same shape that failed on F1611) returned wrong data, but the very next (larger) batched
  read of the same region was correct — same warm-up/settle signature as F1611, confirming this is a
  **family-wide SLAU049 characteristic, not an F1611 die-specific quirk**. Closes test plan item 3.
- **Main-flash erase**: works cleanly once isolated from write — erase → immediate read (same
  session, no re-attach) correctly and repeatably shows `0xFFFF`, safe and quick agree.
- **Main-flash write**: a new, separate finding — reports `OK` but the data never actually gets
  programmed (stays `0xFFFF` after write, both safe and quick agree it's still blank). Initially
  suspected as a target-voltage issue (probe sensed ~2570mV against a driven 3300mV — below
  F1121A's 2700mV `vccFlashMin`), but that turned out to be a **floating target-side sense pin**
  on this specific prototype fixture (no reference divider populated) giving a plausible-but-wrong
  reading, not a real voltage shortfall — confirmed once the user added a jumper to give the sense
  pin a real reference (reading corrected to ~3294mV, matching the driven level) and the write still
  silently failed. Test range verified valid (`0xF000`, Main's first 512 B segment, well clear of
  the vector table in the last segment). **Root cause not yet found** — next concrete step is
  checking `FCTL3`'s BUSY/FAIL status bits immediately after a write attempt. Tracked as a separate
  issue (not blocking #53's closure): flash *write* reliability is orthogonal to the memory-access
  *algorithm selection* question this doc is about.

**#53 closes here.** SLAU049 `QuickCap` conclusion: RAM unreliable (needs LA capture, tracked
generically — not per-device, since F1121 confirmed it's a family trait), Main-flash quick-**read**
reliable (once the `EraseFlash` release bug was out of the way). Main-flash *write* correctness is a
new, separate bug, filed on its own.

## Per-family sub-issues (bench-driven; #51 becomes the tracking/parent issue, closes when all land)

Each sub-issue's deliverable: bench log on the owned part(s) + one filled-in `QuickCap` table row
+ code changes only if that family needs something beyond the shared dispatch logic.

1. **SLAU049** (F1121, F1611 — both plain CPU architecture, currently the only bench-confirmed
   *failure*). Test snippets: RAM-only quick read, Flash-Main-only quick read, on **both** F1121
   and F1611 (two silicon families sharing the manual, in case it's device-specific rather than
   family-wide). Also exercise Periph8/Periph16 write-splitting and BSL write-denial once gaps #1
   and #2 above land, since those paths have literally never been exercised on this family before.
2. **SLAU056** (F449) — same classic-CPU shape and test plan as SLAU049, single device to confirm
   or deny; worth checking whether it shares SLAU049's quick-read failure or not (they're adjacent
   manuals, not identical).
3. **SLAU144** (F247, F2131, F2416, G2553, G2955 — mixed CPU/CPUX within one family; F2418 CPUX
   already bench-confirmed working). Priority: run the same RAM/Flash-Main isolation snippets on a
   **plain-CPU** SLAU144 part (G2553 is on hand) specifically to separate "SLAU144 family capable"
   from "CPUX architecture capable" — today's single F2418 data point can't distinguish the two,
   and SLAU049's failure is CPU-architecture, so this is the one test that actually discriminates
   the two hypotheses in this doc's "why probably not a code bug" section.
4. **SLAU208 and newer (Xv2)** — user's expectation is "everything here is quick-capable"; lower
   priority to formally re-derive since `TapDev430Xv2` already ships and is presumably exercised in
   normal use, but still run one confirmatory isolated-region pass on F5418A or F5529 to seed a
   real bench-confirmed table row instead of an assumption, and to shake out whether Xv2's EEM
   changes affect anything in this doc's scope.

## Explicitly out of scope this round

- FRAM (SLAU272/SLAU367) — quick-write semantics, MPU/write-protect (`accessMpu`), separate ticket.
- EEM/CPU "memory" entries — confirmed off-bus formalities (`fMapped=false` already marks them);
  no read/write algorithm applies, no firmware change expected, just noted as intentionally excluded
  so future readers don't wonder why they're missing from the capability table.
- `IR_DATA_QUICK` write mode entirely (see "Proposed capability model" above — FRAM-exclusive per
  the manual tables, and current dispatch never sends Flash writes through the quick path anyway).

## Decisions

- ROM-write guard (gap #2): **silent no-op**, matching the existing `EraseMain`/`EraseAll` "not
  erasable" convention.
- Sub-issues 1-4 are filed as real GitHub issues, children of #51 (retitled to the tracking issue
  for this doc).

## References

- `Firmware.shared/util/ChipProfile.h` (`MemInfo`), `ChipProfile.cpp` (`MemoryBlock_::Fill` —
  where `bitSize`/`accessType`/`fMapped`/`accessMpu` are derived, all currently global rules)
- `Firmware.shared/drivers/TapMcu.{h,cpp}` (`ReadMem`/`WriteMem`/`OnReadWords`/`OnWriteWords`/
  `CheckRange` — the dispatch layer this doc's fixes target)
- `Firmware.shared/drivers/ITapDev.h`, `TapDev430.cpp`, `TapDev430X.cpp` (`ReadByte`/`ReadBytes`
  exist; `WriteByte`/`WriteBytes` don't — gap #1)
- `supp/slau320aj/slau320/Replicator430/JTAGfunc430.c` (`WriteMem`, `F_BYTE`/`F_WORD`, `0x2418`
  vs `0x2408`), `Replicator430X/JTAGfunc430X.c` (`WriteMem_430X`, same split)
- `supp/slau320aj - MSP430™ Programming With the JTAG Interface.md` Tables 2-14/2-15/2-16
- `ChipInfo/ExtractChipInfo/MSP430-devices/devices/*.xml` (raw source of `quickMemRead`,
  confirmed flat/family-inherited, no per-region split)
- `ChipInfo/ImportDB/Model/Features.cs`, `ChipInfo/ImportDB/Results/results.db` (`Features` table)
- `.claude/docs/drivers/TIM_DMA_TIMING_PROBE.md` (pattern to imitate for the test-snippet infra)
- `.claude/issues/052-dataquick-fast-read-family-profiling.md` (predecessor write-up, historical)
- GitHub issue #51 (tracking issue) with sub-issues #52 (Phase 0 infra), #53 (SLAU049), #54
  (SLAU056), #55 (SLAU144), #56 (SLAU208+)
