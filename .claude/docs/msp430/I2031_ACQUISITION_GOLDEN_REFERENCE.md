# MSP430i2031 — Acquisition Golden Reference (issue #43)

Bench- and source-confirmed reference for acquiring an **MSP430i2031** (jtag_id
`0x89`, legacy CPU) with the genuine TI FET, decoded against the TI MSPDebugStack
firmware. This **supersedes** the earlier `I2031_JTAG_GOLDEN_REFERENCE.md`, whose
"timing tweak in `OnEnterTap`" conclusion was wrong.

> **✅ BENCH-CONFIRMED 2026-06-07:** with the RST-high faithful entry fix below, glossy
> acquires the i2030/i2031 through the **normal entry path** (no BSL train): `jtag_id
> 0x89`, `device_id 0x2040`, profile `MSP430I204x_I203x_I202x`, full memory map +
> breakpoints. **Both the 4-wire JTAG and the SBW path acquire.** Tier-1 (minimal
> acquire) is **proven sufficient for a quiescent part**.
>
> **TL;DR of #43 (corrected 2026-06-07):** there are **two tiers**.
> 1. **Minimal acquire (quiescent part):** a normal TAP open whose entry sequence
>    runs the **TEST-pin activation** with RST held **high** throughout and an
>    "activate TEST logic" dwell at //3. The two things that were wrong:
>    - **Shape:** glossy pulled RST **low** during activation (a ~14 ms pre-pulse
>      from `OnConnectJtag`'s `JtagOn` JRST-default-low + `OnEnterTap` //1). UIF keeps
>      RST high. **Fixed** by RST-high faithful `OnEnterTap` + `JRST_On` (RST-high
>      level) inside each target's `JtagOn` pin group — see "JRST level fix" below.
>    - **Dwell:** glossy used 20 ms; TI uses 100 ms. We first tried 100 ms (worked),
>      then learned the i2030 datasheet specs **`t_SBW,En` = 1 µs max** ("TEST high to
>      acceptance of first clock edge"), so 100 ms was heavy overkill. **Final value:
>      25 ms** (`JtagDev::OnEnterTap` //3 and `SbwDev::OnEnterTap` //3), both
>      bench-confirmed. Can likely go far lower toward the 1 µs spec.
> 2. **Robust acquire (running/locked part):** additionally prefix the **BSL Entry
>    Sequence** (RST/TEST pulse train → halt CPU into LPM4) + ~500 ms settle. This
>    is the `Reset430I.c` path; still additive, not yet ported.
>
> The earlier claim that "BSL is *the* key and is always required" was an
> over-statement — the quiescent i2030/i2031 in the bench test acquired through the
> plain activation path with **no** BSL train.
>
> **JRST level fix (2026-06-07):** the RST-high requirement is now declarative in
> `platform.h`, not patched in the driver. Each target defines
> `JRST_On = AnyOut<…, Level::kHigh>` and lists it (not the default-low `JRST`) in its
> `JtagOn` pin group, so `JtagOn::Setup()` brings RST up as part of activating the bus.
> Applied to **all three live targets**: `target.stlinv2` (PB0), `target.bluepill` and
> `target.bluepill.g431kb` (PA1). The old `JRST::SetHigh()` workaround in the shared
> `JtagDev::OnConnectJtag` was removed.

## How the DLL decides what to run (the dispatch, corrected)

The acquisition path is a **host-side (DLL) policy**, keyed off the **device name**,
not anything read from the bus. The UIF firmware only executes whatever HAL macro the
DLL hands it.

- `OpenDevice(Device, …, DeviceCode, setId)` — `DLL430_OldApiV3.cpp:482`:
  ```cpp
  if (tmpName.find("MSP430I") == 0 && DeviceCode == 0)
      DeviceCode = 0x20404020;     // i-family sentinel — comes from the NAME prefix
  ```
- `0x20404020` is the **only** gate to the i-family `Reset430I` path. It is reachable
  *only* when the host names an `MSP430I*` device (or passes the code explicitly).
- `Reset430I` is **not** run for all chips. Per-family dispatch (`ConfigManager::reset`,
  `:552`): i-family → `ID_Reset430I`; L092/C092 → `ID_ResetL092`; other Xv2 →
  per-device `rstHalId` from the DB; classic 430/430X → just an RST-pin BOR.

### What the bench `MinimalMSP430_dll` test actually exercised

The active call is `MSP430_OpenDevice("DEVICE_UNKNOWN", "", 0, 0, DEVICE_UNKNOWN)`:

- **`Device` name = the literal string `"DEVICE_UNKNOWN"`** → `find("MSP430I") != 0`
  → `DeviceCode` stays `0` → **`Reset430I` is never reached.** (The `"MSP430I2031"`
  name was on a *commented* line.)
- `setId = DEVICE_UNKNOWN` → DLL runs the **generic** identify path:
  `ConfigManager::start()` sends one `ID_StartJtag` element with the mode byte from
  `Configure(INTERFACE_MODE,…)` (= `2`, SPYBIWIREJTAG), then `identifyDevice()` does
  the normal IR/DR + descriptor reads.

So the captured waveform is the **generic `StartJtag`** sequence, **not** the i-family
`Reset430I` recipe:

```c
// Bios/src/hal/macros/StartJtag.c  (_hal_StartJtag, MSP430_UIF)
IHIL_SetProtocol(protocol);   // 2 = SPYBIWIREJTAG -> 4-wire funcs + SBW-style entry
IHIL_Open(RSTHIGH);           // <-- the activation entry sequence lives here
IHIL_TapReset();              // TMS=1 x5
IHIL_CheckJtagFuse();
// returns chainLen = 1
```

The earlier "~1 s reset / BSL train" reading of this capture was therefore wrong: a
DEVICE_UNKNOWN open has no BSL train and no 500 ms settle. Any long idle in that
trace belongs to the activation dwell / retries, not `Reset430I`.

## How the transport was isolated (the decisive experiment)

`Configure(INTERFACE_MODE, value)` (after `Initialize`, before `OpenDevice`) pins the
transport instead of letting the DLL auto-detect. Results on the i2031:

| `INTERFACE_TYPE` value | `OpenDevice` | LA shows |
|------------------------|--------------|----------|
| `JTAG_IF` (0) — plain 4-wire | **fails** | — |
| `SPYBIWIRE_IF` (1) — 2-wire SBW | works | clean SBW |
| `SPYBIWIREJTAG_IF` (2) — 4-wire + SBW-style activation | works | clean 4-wire JTAG, **TDO live** |
| `AUTOMATIC_IF` (3) — default | works (via SBW/SPYBIWIREJTAG) | mixed attempts |

**Plain `JTAG_IF` fails; `SPYBIWIREJTAG_IF` succeeds** — and it succeeds through the
*generic* `StartJtag` path, i.e. with **no** BSL train. That proves the quiescent
i2031 only needs the **activation entry sequence**, which `JTAG_IF` skips.

## Why `JTAG_IF` fails and `SPYBIWIREJTAG_IF` works — it's the entry sequence

From `Bios/src/hil/uifv1/hil430.c`, interface mode controls **two decoupled** things:

1. **Shift/transport level** (`_hil_SetProtocol`, line 219): `JTAG` **and**
   `SPYBIWIREJTAG` use the **identical 4-wire** method set (`_hil_4w_*`). Only
   `SPYBIWIRE` uses 2-wire. → SPYBIWIREJTAG is real 4-wire on the wire.
2. **Entry/activation level** (`_hil_EntrySequences`, line 551):

   | Mode | Entry | Effect |
   |------|-------|--------|
   | `JTAG` | `default:` → just `TSTset1;` — **no activation, no dwell** | assumes TAP already alive → i2031 stays dark → **fail** |
   | `SPYBIWIREJTAG` | `_hil_EntrySequences_RstHigh_JTAG()` — full TEST/RST activation with a **100 ms** dwell, then 4-wire | wakes the TEST-pin-gated TAP → **works** |
   | `SPYBIWIRE` | `_hil_EntrySequences_RstHigh_SBW()` + 2-wire | SBW activation + SBW transport |

The i2031's JTAG pins are **gated behind the TEST pin**; they only become active after
the TEST/RST activation handshake. `JTAG_IF`'s bare `TSTset1;` never performs it.

## glossy `OnEnterTap` vs TI's working entry (the actual fix)

glossy's `JtagDev::OnEnterTap` (slau320aj-derived) is **already the same sequence
shape** as TI's `_hil_EntrySequences_RstHigh_JTAG` (`hil430.c:454`). Step-for-step:

| step | glossy `OnEnterTap` (was) | TI `RstHigh_JTAG` |
|---|---|---|
| //1 reset TEST logic | TEST low — 4 ms | TEST low — 1 ms |
| //2 | RST high | RST high |
| //3 **activate TEST logic** | TEST high — **20 ms** ❌ | TEST high — **100 ms** |
| //4 | RST low — 50 µs | RST low — 40 µs |
| //5 | TEST low — 1 µs | TEST low — 1 µs |
| //7 | TEST high — 60 µs | TEST high — 40 µs |
| end | RST high, 5 ms | RST high, 5 ms |

Same pin *edges*, but two things were wrong: the //3 dwell was short (20 ms), and —
more importantly — **glossy pulsed RST low at the start** (//1 `JRST::SetLow`, plus
`OnConnectJtag`'s `JtagOn` whose `JRST` defaults to `Level::kLow`, held 10 ms). On the
bench that showed as a **14 ms RST-low pre-pulse** (10 ms connect + 4 ms //1) that UIF
does not have — UIF keeps **RST high** through the whole TEST-low(reset)→TEST-high(100 ms
activate) window and dips RST only ~40 µs at //4.

**Fix applied 2026-06-07** (two edits):
1. `JtagDev::OnEnterTap` rewritten as a faithful port of `_hil_EntrySequences_Rst{High,
   Low}_JTAG`: RstHigh keeps **RST high** through //1/#3 (no leading RST-low), //1 dwell
   1 ms, //3 dwell **100 ms**, //4 dip 40 µs, //7 40 µs. RstLow (MagicPattern path) holds
   RST low across activation. The misleading `#if 0` "RstLow_JTAG" sketch (5 µs TEST
   pulse, no activation dwell — not a faithful UIF reading) was removed.
2. `JtagDev::OnConnectJtag` now drives `JRST::SetHigh()` after `JtagOn::Setup()` so the
   10 ms settle holds RST high (TEST stays low) — entry starts from RST=H like
   `_hil_Connect`, eliminating the 10 ms RST-low pre-window.

**Hardware note (TEST polarity, corrected in code 2026-06-07):** the old platform.h:200
comment claimed a TEST **pull-up** — wrong per schematic review: there is no PU; the
adapter has a 10K **pull-down** and the MSP430 TEST pin has a weak internal PD, so TEST
idles **low** passively. Comment fixed. The firmware drives PB1 low in every
Open/Close/idle state and high only at `OnEnterTap` //3/#7 (activation) — matching "TEST
high only for activation". A captured TEST-high *outside* activation therefore cannot
come from this firmware (suspect the LA TEST-channel mapping or a probe issue).

**SBW path (mirrored 2026-06-07):** `SbwDev::OnEnterTap` was already RST-high (fixed
during FR5994 bring-up); its //3 activation dwell was bumped **20 ms → 100 ms** to match
`_hil_EntrySequences_RstHigh_SBW` so the i2031 acquires over SBW too. (Bench-confirmed on
4-wire; SBW i2031 acquire still to be checked on the bench.)

## The robust tier — full i20xx recipe (`Reset430I.c`), still to port

Reached only via the `"MSP430I*"` name (`0x20404020`). For a part that is *running
user code* or *locked*, the plain activation races a live CPU and can lose; TI prefixes
a BSL halt:

```c
// Bios/src/hal/macros/Reset430I.c  (_hal_Reset430I)
IHIL_BSL_EntrySequence(1);   // RST/TEST pulse train -> halts CPU, forces LPM4
IHIL_Delay_1ms(500);         // settle in LPM4
IHIL_Open(RSTHIGH);          // SPYBIWIREJTAG / SBW activation entry (the same as above)
IHIL_TapReset();
IHIL_CheckJtagFuse();
id = cntrl_sig_capture();
if (id == JTAGVERSION /*0x89*/) { ... return SPYBIWIREJTAG; }
```

`_hil_BSL_EntrySequence` (`hil430.c:860`) pulse train scales with transport
(4-wire/else ≈ 71 ms with 10 ms dwells; SBW ≈ 701 ms with 100 ms dwells). With the
500 ms settle that is the genuine "~1 s reset" — but it only appears on an
**`MSP430I*`-named** open, not the DEVICE_UNKNOWN bench capture.

## Bench decode of the clean 4-wire session (2026-06-05, caveat)

Capture `supp/docs-ai/test-logic-analyzer.csv`. **Channel map was never verified** and
parts of the trace are untrustworthy (see the on-hold note below); treat the TI source
above as authoritative. Where the decode was legible: IR captures returned raw TDO
`0x91` = bit-reversed `0x89` ⇒ `jtag_id 0x89`, and the shifted instructions were the
standard legacy-MSP430 (`TapDev430`) acquisition (`0x14` CNTRL_SIG_CAPTURE, `0x2A`
DATA_16BIT, `0x41` ADDR_16BIT, …). Nothing exotic once the TAP is awake.

## Corrected port plan for #43

| TI piece | glossy today | action |
|----------|--------------|--------|
| activation entry: RST-high, 100 ms dwell | `OnEnterTap` pulsed RST low + 20 ms dwell | **done** — RST-high faithful port + `OnConnectJtag` RST-high settle |
| generic `StartJtag` acquire of a quiescent i2031 | `TapDev430` legacy path exists | should now succeed on bench with the 100 ms dwell — **first experiment** |
| `IHIL_BSL_EntrySequence` (robust tier) | no primitive | new `JtagDev`/`SbwDev` method emitting the RST/TEST pulse train |
| `_hal_Reset430I` orchestration (robust tier) | absent | BSL-entry → 500 ms → `OnEnterTap(false)` → `OnResetTap()` → fuse-check → `IR_Shift(kCntrlSigCapture)` == `0x89` |
| `0x20404020` family selector | absent | i20xx branch in `TapMcu::InitDevice()` to choose the robust tier |

**Out of scope:** C092 / L092 (`ID_ResetL092`, `ID_UnlockC092`,
`ID_StartJtagActivationCode`) — no samples available, will not be supported. The only
family-specific acquisition glossy needs is the i-family one.

## Status / caveats

- Bench investigation was **on hold pending hardware check** — the original i2031 board
  ran warm under FET-UIF (back-power symptom; see
  `project_i20xx_self_powered_capture` memory). Capture self-/target-powered only (the
  `0x20404020` connect drops VCC to 0 V for ~5 s).
- The 100 ms `OnEnterTap` change is the **first thing to bench** when hardware is
  confirmed clean: re-run the i2031 acquire and check for `jtag_id 0x89`.

## See also
- `Firmware.shared/docs/TI-step-by-step.md` — "MSP430i20xx Acquisition" section.
- `project_i20xx_self_powered_capture` memory — capture must be self/target-powered.
