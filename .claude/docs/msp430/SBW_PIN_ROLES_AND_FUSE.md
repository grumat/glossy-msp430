# SBW vs JTAG pin roles, fuse-burn, and the 330 Ω

Working note on *why* Spy-Bi-Wire is sometimes 2-wire and sometimes effectively
3-wire, why TI reference boards put a 330 Ω resistor on a JTAG-14 connector used
for SBW, and how all of that maps onto the Glossy-MSP430 *repurposed-SWD*
hardware (STLinkV2 design). This is the connector / electrical layer — the TAP
state machine and frame timing live in
[`MSP430_JTAG_SBW_IMPLEMENTATION.md`](MSP430_JTAG_SBW_IMPLEMENTATION.md) and the
driver mechanics in [`../drivers/TIM_SBW_DRIVER.md`](../drivers/TIM_SBW_DRIVER.md).

> This note exists because a bring-up session burned time on exactly this
> confusion: the SBW entry sequence was bit-banged on the JTAG-14 nRST/TEST
> pins, which are **not in the SBW circuit at all** on this hardware.

## 1. Naming convention (use this to avoid mixing layers)

The same logical signal has up to three names depending on *where* you mean:

| Prefix | Means | Example |
|--------|-------|---------|
| (none) | the **signal / role**, abstractly | TEST, TCK, RST |
| `J…`   | that signal **on the JTAG-14 / JTAG-20 connector** (no qualitative difference between the two connectors) | JTEST, JTCK, JRST |
| `T…`   | that signal **at the target**, which may be re-routed by the target board's configuration jumpers | TTEST, TTCK |

Things "get mixed" precisely when you cross from connector to target through
jumpers — keep the prefix explicit.

## 2. Pins are multi-function; SBW reuses two of them

Every SBW-capable MSP430 multiplexes the SBW interface onto two existing pins:

### 2.1 The TEST pin  →  SBWTCK
- During **"Acquire SBW"**, the pin plays the **TEST role**: it applies the
  entry sequence **and** is used to sense the security fuse. The fuse-sense
  **draws ~1 mA** — this current draw is the part that matters for the resistor
  discussion below.
- Once SBW is acquired, the clock (`TCLK` in the abstract) is **relabelled
  `SBWTCK`** and the same pin carries the bit clock.
- **This pin is the master switch for the whole interface.** Per SLAU320AJ
  §2.3.1 / §2.4.1, the SBW (and JTAG) interface is **disabled whenever
  TEST/SBWTCK is held low** — the device has an internal pulldown, so a
  floating or low pin = no debug. Pulling it **high enables** SBW and, while
  active, holds the internal reset high and disables the RST/NMI function of the
  SBWTDIO pin (see §2.2).

#### Two different SBWTCK-low timings — do not conflate them
There are **two** distinct "hold SBWTCK low" rules in SLAU320AJ with very
different magnitudes and meanings:

| Duration | Meaning | Source |
|----------|---------|--------|
| **> 100 µs** | **Deliberate exit** of SBW mode (and likewise of 4-wire JTAG). Hold TEST/SBWTCK low for more than 100 µs to drop the interface cleanly. | SLAU320AJ §2.4.1 ("Exit the SBW mode by holding the TEST/SBWTCK low for more than 100 µs.") |
| **≤ 7 µs** | **In-frame ceiling**, NOT an exit: during an active SBW transaction the SBWTCK **low phase of any single clock cycle** must not exceed 7 µs, or the SBW state machine deactivates mid-transaction and must be re-acquired. This is why frame code disables interrupts across the low phase. | SLAU320AJ Fig. 2-10 note |

> **Correction (this note previously got this wrong):** an earlier draft / the
> original `SbwDev.cpp` comment described "SBWTCK held low ≥ 7 µs" as an *entry*
> conditioning step. It is not — 7 µs is the per-cycle low-phase *ceiling* during
> a live frame, and the *deactivation/exit* timing is **100 µs**, on the
> TEST/SBWTCK pin. Entry is the TEST glitch sequence in §5.1, not a timed low.

**Firmware consequence:** the SBW teardown in `SbwDev::OnReleaseJtag()` (today a
TODO) is simply *drive PB13 (TEST/SBWTCK) low and hold it > 100 µs* before
returning the pins to Hi-Z — no special waveform, just the documented 100 µs
low on the clock pin.

### 2.2 The ~RST / NMI / SBWDIO pin  →  SBWTDIO
One physical pin, three behaviours, gated differently:
- **~RST**: asserted only if the pin is held long enough (order of ~100 ms — not
  precisely characterised here; treat as "long").
- **NMI**: an interrupt mode, gated by target firmware.
- **SBWDIO**: used as the data line during the SBW entry sequence, and as the
  half-duplex input/output once SBW is acquired.

So **in SBW there are no other pins** — it is fundamentally a two-wire interface
(SBWTCK + SBWTDIO). A "third wire" only appears for the fuse-burn feature
(§3).

## 3. Why TI boards expose ~~two~~ three pins + a 330 Ω

TI reference boards (and the standard JTAG-14 → SBW wiring) drive **both** the
`JTEST` pin and the `JTCK` (TCLK-entry) pin, with a **330 Ω between them**:

- **Clock role (SBWTCK):** either pin works — through the 330 Ω you can take the
  clock off `JTEST` *or* `JTCK` with no problem.
- **TEST role (fuse-sense, ~1 mA):** driving this **through the 330 Ω is not
  stable enough** — the IR drop causes **misdetection of the fuse state**. So
  the TEST role must be driven on the real `JTEST` pin directly.
  - *(This is why Olimex simplifies: it uses only the `JTEST` pin and ignores
    the other.)*
- **Fuse-burn (the real reason for the dual drive):** burning the fuse applies a
  **high voltage** to the TEST pin from the debug unit's "Burn Fuse" circuit.
  Burning is **permanent** — it closes debug access forever. The **330 Ω
  protects the `JTCK` pin** from the return-current overload during that HV
  event.

### 3.1 The hardware fuse is a 1xx/2xx/4xx thing — 5xx+ is a software lock

Everything in §3 (the ~1 mA fuse-sense, the 330 Ω, the HV burn) applies to the
**1xx/2xx/4xx** families, which have a real one-time hardware fuse. The
**5xx/6xx/FRxx (Xv2)** families replaced it with a software **JTAG lock key**
(32-byte password at 0xFF80) — there is **no fuse-sense current and no
fuse-check waveform** on those parts.

The TI Xv2 reference proves it: `Replicator430Xv2/JTAGfunc430Xv2.c` removed the
fuse check from `ResetTAP()` in SBW mode (changelog: *"Fuse check is not required
for 5xx"*), and its SBW `ResetTAP()` is just the `DoGoIdle` TAP reset (6×
`TMSH_TDIH` + `TMSL_TDIH`). The lock is read as an ordinary register scan. So
for the Glossy probe driving an Xv2 target over SBW, **do not look for a
fuse-check signal — there isn't one** (a missing-frames symptom is a transport
bug, not a fuse gap). See
[`MSP430_JTAG_SBW_IMPLEMENTATION.md`](MSP430_JTAG_SBW_IMPLEMENTATION.md) §10.2.

## 4. 2-wire vs 3-wire — pick by whether you burn fuses

| Capability | Wires | Extra parts |
|------------|-------|-------------|
| Debug only (no fuse-burn) | **2** (SBWTCK, SBWTDIO) | none |
| Debug + fuse-burn | effectively **3** (dual TEST/TCK drive) | 330 Ω, HV burn circuit |

Simpler boards — e.g. the **TI LaunchPad** — **do not burn fuses** and are driven
purely **2-wire**. The 330 Ω / dual-drive is only a fuse-burn concern.

## 5. Glossy-MSP430 (STLinkV2 design) — pure 2-wire on the SWD pins

This is **not** a TI dongle: it repurposes the **ARM SWD** pins. We **do not
offer fuse-burn** (no HV circuitry), so it is a clean **2-wire** SBW.

Mapping (see `Hardware/STLink-Adapter/README.md` SBW table and
`target.stlinv2/platform.h`):

| SBW signal | role | STLinkV2 pin | notes |
|------------|------|--------------|-------|
| **SBWTCK** | TEST-role pin → clock | **PB13** (SWCLK; shorted to PA5 on PCB) | bit-banged GPIO for entry, then TIM1_CH1N for frames |
| **SBWTDIO**| ~RST/NMI/SBWDIO → data | **PB14** (SWDIO/TMS) | half-duplex; tri-state to read |
| (read-back)| echo of SBWDIO | **PB12** (SWD_IN) | level-translated echo — always reads true bus level |

Adapter SBW port (the *dedicated* SBW connector — **the JTAG-14 connector must
NOT be used for SBW**):

|      |    TI    |  Olimex  | **STLink (Glossy)** |
|------|:--------:|:--------:|:-------------------:|
| DIO  | TDO      | RST      | **TMS** (→ PB14)    |
| CLK  | TCK/TEST | TCK/TEST | **TCK** (→ PB13)    |

### 5.1 Firmware consequence (the bug this note prevents)

`JRST` = PB0 and `JTEST` = PB1 are the dedicated **JTAG-14** nRST/TEST lines.
They are **not routed to the SBW connector**, so the **SBW activation sequence
must be bit-banged on PB13 (TEST role) and PB14 (~RST role)** — never on
PB0/PB1. Driving PB0/PB1 toggles pins the target never sees in SBW mode, so the
device never enters SBW even though a clean frame burst then appears on
PB13/PB14.

Because PB13 also serves the frame clock, `SbwDev::OnEnterTap()` must:
1. flip PB13 from TIM1 AF to a **GPIO output** for the handshake (it is left in
   AF by `SbwBusOn()`, which runs earlier in `OnConnectJtag`),
2. bit-bang TEST on PB13 / ~RST on PB14,
3. hand PB13 back to TIM1_CH1N (`SbwClkToAf`, which touches **only** PB13 so the
   just-released ~RST level on PB14 is preserved).

See `target.stlinv2/platform.h` (`SBWTEST_Bb` / `SBWRST_Bb` / `SbwClkToAf`) and
`Firmware.shared/drivers/SbwDev.cpp`.

## 6. References
- `Hardware/STLink-Adapter/README.md` — SBW port wiring table; "DO NOT USE
  JTAG-14 FOR SBW"; TEST-pin pull-down note.
- `Hardware/STLinkV2/README.md` — SBW read-back topology (PB14 out / PB12 echo).
- `target.stlinv2/platform.h` — pin aliases and `SbwBusOn`/`SbwBusOff`.
- SLAU320AJ — MSP430 programming with the JTAG interface (fuse, SBW entry; §2.4.1
  enable/exit semantics — 100 µs exit; Fig. 2-10 note — 7 µs in-frame low ceiling).
