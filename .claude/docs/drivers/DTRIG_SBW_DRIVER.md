# DtrigSbw — autonomous SBW transport (design plan)

Companion to [DTRIG_JTAG_DRIVER](DTRIG_JTAG_DRIVER.md). This is the
design for the SBW equivalent of `DtrigJtag<>`: an autonomous Spy-Bi-Wire
transport layer with the same async pipeline contract (return a
`JtagPending<T>`, overlap CPU rendering with DMA flush).

Status: **draft / not implemented**. Skeleton template lives at
`Firmware.shared/util/DtrigSbw.h`.

## Scope

- Target hardware first: **STLinkV2 clone** (`target.stlinv2/platform.h`).
  Pins are constrained by the existing PCB — SBWCLK and SBWDIO share
  traces with JTCK and JTMS respectively.
- Wire rate goal: **1–2 MHz SBW** initially (5 MHz is the MSP430 spec
  limit, but the F103 SPI/TIM path tops out earlier). Speed grades
  mirror the JTAG ones (`JTCK_Speed_1..5` already defined).
- The active mode is exclusive: SBW and JTAG never run concurrently
  on the same target. **No claim/release dance** — each `Init()` is
  destructive (force-reconfigures GPIO, force-disables DMA channels,
  force-resets the relevant peripheral via APBxRSTR) and asserts
  exclusive ownership over its resources, regardless of prior state.

## SBW frame in one sentence

Each logical bit becomes a **3-phase frame** clocked by SBWCLK rising
edges:

1. **TMS phase** — host drives SBWDIO with the TMS value for this bit
2. **TDI phase** — host drives SBWDIO with the TDI value
3. **TDO phase** — host releases SBWDIO (set as input), samples TDO
   from the target near the falling edge

That gives one logical JTAG bit per 3 wire cycles. A 14-bit IR scan
on JTAG becomes 14 × 3 = 42 SBW wire cycles; a 32-bit DR is 96.

## Board hardware requirements for SBW

The MSP430 SBW spec re-uses the TEST and RST pins from the JTAG connector:

- After the entry handshake, **TEST pin becomes SBWTCK** (clock from probe).
- After the entry handshake, **RST pin becomes SBWTDIO** (bidirectional data).

On every Glossy probe variant, the TEST line is also wired to the JTAG
connector's TCK pin through a 220-330R series resistor at the target side
(so the resistor presents TEST = TCK to the target, with "TEST always
wins" because of impedance). The implication for the probe is that to use
TCK as a clean SBWCLK output, **the probe's TEST pin must be tri-stated
while TCK is driven**. If both are driven, TEST overpowers TCK and the
target never sees a real clock — only the held TEST state.

This makes board-level **TEST / TCK driver-enable independence** a hard
requirement for SBW. Boards whose output-buffer scheme gangs TEST with
TCK (or with RST) cannot run SBW without hardware modification.

### Per-board status

| Board    | TEST enable  | TCK enable   | RST enable   | SBW capable?            |
|----------|--------------|--------------|--------------|-------------------------|
| Bluepill | PB12 (ENA1N) | PB12 (ENA1N) | PB12 (ENA1N) | **NO** — all ganged     |
| STLinkV2 | (PB1 direct) | (PA5/PB13)   | (PB0 direct) | Yes — independent pads  |
| G431     | TBD          | TBD          | TBD          | TBD — verify schematic  |
| L432     | TBD          | TBD          | TBD          | TBD — verify schematic  |

**Bluepill** is the canonical broken case — bench captures (May 2026)
showed TEST/TRST entry pulses fine, then zero SBWCLK reached the target
because PB12 keeps TEST driven during the clock phase. Fixing it requires
a PCB cut to separate ENA1N into independent TEST and TCK/RST enables.
SBW on bluepill is therefore frozen until that hardware change happens.

**STLinkV2** does not use the ENAxN buffer scheme — TEST is on PB1 with
its own direct pad, and TCK lives on PA5/PB13 with its own independent
SPI/TIM driver. The "tri-state TEST during clock phase" requirement is
satisfiable. STLinkV2 is the next SBW bring-up target.

**G431 / L432** schematics need a manual check before any SBW work on
those boards. They use the same ENA1N/ENA2N/ENA3N grouping pattern as
bluepill, so the same risk exists; the exact pin allocation per ENA is
not currently documented in `platform.h`.

## SBWCLK source per board

The BSRR-script encoder needs SBWCLK to come from a **TIM channel on a
pin electrically connected to the SBWCLK trace**. SPI1_SCK alone can't
emit the non-multiple-of-8 clock counts we need without padding tricks
that would conflict with the buffered SBWDIO_Out pin (PA7 = SPI1_MOSI).

| Board    | SBWCLK trace             | TIM channel on trace                  | Encoder      |
|----------|--------------------------|----------------------------------------|--------------|
| Bluepill | PA5 ↔ PA8 (PCB bridge)   | **PA8 = TIM1_CH1** (regular CH)        | BSRR-script  |
| STLinkV2 | PA5 / PB13 trace short   | **PB13 = TIM1_CH1N** (complementary)   | BSRR-script  |
| G431     | PA5 only                 | none — PA8 not bridged                 | Needs SPI-stream alt |
| L432     | PA5 only                 | none — PA8 not bridged                 | Needs SPI-stream alt |

Bluepill and STLinkV2 both route SBWCLK through TIM1_CH1 (regular vs
complementary depending on which pin is the AF). The encoder takes
`kCmpComplementary` as a template flag to cover both.

G431 and L432 cannot use this encoder as-is — their SBWCLK trace is on
PA5 only, with no TIM channel routed to it. Bringing up SBW on those
boards will require the [SPI-stream alternate](DTRIG_SBW_SPI_ALT.md),
which generates SBWCLK from SPI1_SCK on PA5 directly. That work is
parked until G431/L432 SBW is on the roadmap; not blocking bluepill.

## Pin / Resource plan (STLinkV2)

The PA5↔PB13 PCB short used in JTAG mode is reused in mirror:

| Signal      | Pin   | Driver in JTAG mode | Driver in SBW mode             |
|-------------|-------|---------------------|--------------------------------|
| SBWCLK / JTCK | PA5 / PB13 trace | SPI1_SCK (PA5 AF) | TIM1_CH1N (PB13 AF), PA5 released |
| SBWDIO / JTMS | PB14 | TIM1_CH2N PWM (TMS) | TIM1 DMA → BSRR / direction (bidirectional) |
| JTDI        | PA7   | SPI1_MOSI           | unused (Hi-Z)                  |
| JTDO        | PA6   | SPI1_MISO           | unused (Hi-Z)                  |

Why PB13/TIM1_CH1N for SBWCLK: PA5 has no native TIM alt-function on
F103; SPI1 cannot easily produce non-multiple-of-8 clock bursts.
PB13 is TIM1_CH1N, available, and electrically connected to PA5 via
the same trace. JTAG releases PB13 to pull-up so SPI on PA5 can drive
the trace; SBW does the inverse — releases PA5 to pull-up so TIM1_CH1N
on PB13 drives it.

TIM1 stays the master timer in both modes — `Timer::Unit` doesn't
change, only the channel set in use:

- JTAG mode: CH2N=TMS PWM, CH3=CCR2 reload trigger
- SBW mode:  CH1N=SBWCLK PWM, CH2/CH3/CH4=DMA triggers for SBWDIO
  data / direction / sample

DMA channel map (proposed; verify against `target.stlinv2/platform.h`
DMA1 budget):

| TIM1 trigger | DMA1 channel | Purpose                          |
|--------------|--------------|----------------------------------|
| TIM1_CH2     | CH4          | BSRR write — SBWDIO data bit     |
| TIM1_CH3     | CH6          | CRH/CRL write — SBWDIO direction |
| TIM1_CH4     | CH4 alias    | IDR read — sample TDO            |

(STM32F103 TIM1 DMA map: CH1=DMA1_CH2, CH2=DMA1_CH3, CH3=DMA1_CH6,
CH4=DMA1_CH4, UP=DMA1_CH5, TRG/COM=DMA1_CH4. Final channel choice
depends on which conflicts least with JTCLK flash-burst peripherals
— same constraint solved for JTAG in DTRIG_JTAG_DRIVER.md.)

## Direction-flip strategies (per-board)

There are two physical realisations of the SBW direction flip in this
codebase, with very different hardware cost. Both strategies share the
same `DirPolicy` interface (see *DirPolicy contract* below) so
`DtrigSbw<>` doesn't have to know which one a given board uses.

### Hardware split

| Board                | SBWDIO pin(s)        | Direction control                 | Direction DMA write |
|----------------------|----------------------|-----------------------------------|---------------------|
| **STLinkV2 (legacy)** | PB14 (bidir)         | flip GPIO mode (CRH on F1)        | 32-bit MODER/CRH overwrite |
| Bluepill / BlackPill-BMP | PA7 (out) + PA6 (in) | PA9 buffer-enable mux            | BSRR set/reset of PA9 |
| Nucleo-G431          | (out) + (in)         | PA10 buffer-enable mux            | BSRR set/reset of PA10 |
| Nucleo-L432          | (out) + (in)         | PA10 buffer-enable mux            | BSRR set/reset of PA10 |

The STLinkV2 case is the *un-optimized* path: there is one
bidirectional wire and no external buffer to gate it, so the only way
to release the trace to the target is to switch the MCU pin from
push-pull-output to floating-input via the mode register. On F1 that
register is `GPIOx->CR{L,H}` (4 bits per pin: MODE[1:0] + CNF[1:0]);
on F4/G4/L4 it is `GPIOx->MODER` (2 bits per pin).

A naive implementation would have the DMA write the full 32-bit
register and rely on precomputed snapshots, which forces the firmware
to "own" every other pin on the same mode-register half. The better
approach on F1 is **peripheral bit-banding**: each bit of CRL/CRH has
a unique 32-bit alias word at a consecutive 4-byte offset in the
`0x42000000` region. A single DMA stream with both source and
destination incrementing, NDTR = 4, sweeps the four alias words for
the SBWDIO pin's nibble (`MODE[0]`, `MODE[1]`, `CNF[0]`, `CNF[1]`) in
one armed transfer — touching exactly those four bits, leaving every
sibling pin physically undisturbed.

For SBWDIO on PB14 (push-pull output @ 10 MHz ↔ floating input):

| Bit       | Output value | Input value |
|-----------|--------------|-------------|
| MODE[0]   | 1            | 0           |
| MODE[1]   | 0            | 0           |
| CNF[0]    | 0            | 1           |
| CNF[1]    | 0            | 0           |

The dir-script for the un-optimized path is therefore two 4-word
blocks (one for "drive output", one for "release to input"), 8
transfers per scan total — vs 2 transfers for the buffered path.
That cost is paid once per scan, not per bit, so the impact on wire
rate is negligible.

The buffered boards are the *optimized* path: separate output and
input physical pins feed an external buffer, and a single mux GPIO
gates which one electrically reaches the trace. Flipping that GPIO is
a one-bit BSRR write — same DMA shape as the data script, no
register-preservation gymnastics.

**A. Single-pin CRH swap (STLinkV2)**

SBWDIO sits on PB14. Direction toggled by rewriting `GPIOB->CRH`
for nibble 14 between push-pull output (`0x3`) and pull-up input
(`0x8`). A second DMA channel triggered by a TIM1 channel writes
the CRH value at the right cycle within each 3-frame.

Pros: works on the existing single-pin hardware. One BSRR DMA word
per cycle for data, one CRH DMA word per direction flip.

Cons: CRH is a full 32-bit register — DMA script must precompute
*both* full CRH values (output-mode and input-mode) and swap. The
firmware effectively owns the GPIOB high nibbles during SBW mode.

**B. External mux + dedicated in/out pins (Bluepill / BlackPill-BMP) — PRIMARY**

Hardware mux on PA9 (`SBWO`) selects which physical pin drives the
SBWDIO trace: PA7 (SPI1_MOSI) when output, PA6 (SPI1_MISO) when
input. PA9 itself is just a GPIO; flipping direction is a single
BSRR write — one bit, no register-preservation gymnastics.

Pros: direction is a 32-bit BSRR word with one bit set (or one bit
in BR), same DMA-write shape as the data BSRR script. Atomic,
zero-latency, matches the JTAG encoding style exactly.

Cons: requires the PA9 mux on the PCB (already present on Bluepill
and BlackPill-BMP).

**Selection at compile time:** each target's `platform.h` aliases a
concrete `DirPolicy_*` type (see *Bluepill: DirPolicy_PA9_BsrrMux*
and *STLinkV2: DirPolicy_PB14_CrhSwap* below) and passes it as the
`DirPolicy` template parameter when instantiating `DtrigSbw<...>`.

## DirPolicy contract

Every `DirPolicy` type — whether it writes BSRR or sweeps bit-band
alias words — exposes the same shape so the encoder doesn't need to
know which mechanism is in play. The two variants differ only in the
**direction of `DirRegister()`** and the **per-flip word count**:

```cpp
struct DirPolicy_Example
{
    /// Number of 32-bit DMA transfers per direction flip.
    ///   - BSRR-mux variants:  1 (write to a fixed BSRR)
    ///   - Bit-band variants:  4 (write MODE[1:0] + CNF[1:0] alias words
    ///                            via dest-incrementing DMA)
    static constexpr unsigned kWordsPerFlip = 1;

    /// Setup hook called once from DtrigSbw::Init(). No-op for mux
    /// variants; bit-band variants may use it to lazily compute their
    /// kWordsPerFlip-word "drive output" / "drive input" arrays.
    static void Init();

    /// Pointer to a kWordsPerFlip-word array consumed by the dir-script
    /// DMA to drive the SBWDIO trace from the probe (master driving TDI).
    static const uint32_t* DriveOutput();

    /// Pointer to a kWordsPerFlip-word array that releases SBWDIO
    /// (target driving TDO).
    static const uint32_t* DriveInput();

    /// Destination for the direction-script DMA.
    ///   - BSRR-mux:  &GPIOx->BSRR        (DMA: dest-fixed,    NDTR = 1)
    ///   - Bit-band:  alias of MODE[0]    (DMA: dest-increment, NDTR = 4)
    static volatile uint32_t* DirRegister();
};
```

The encoder reads `kWordsPerFlip` at compile time and instantiates the
right DMA template (dest-fixed vs dest-incrementing). The direction
script is the concatenation of `DriveOutput()` then `DriveInput()` —
total `2 × kWordsPerFlip` transfers per scan — armed once per scan
with the appropriate `NDTR`.

### Bluepill / BlackPill-BMP: DirPolicy_PA9_BsrrMux

```cpp
template <uint32_t kMuxBit>
struct DirPolicy_BsrrMux_GPIOA
{
    static constexpr unsigned kWordsPerFlip = 1;
    static void Init() {}
    static const uint32_t* DriveOutput()
    {
        static constexpr uint32_t v[1] = { 1u << kMuxBit };           // BSRR set
        return v;
    }
    static const uint32_t* DriveInput()
    {
        static constexpr uint32_t v[1] = { 1u << (kMuxBit + 16) };    // BSRR reset
        return v;
    }
    static volatile uint32_t* DirRegister() { return &GPIOA->BSRR; }
};
using DirPolicy_PA9_BsrrMux  = DirPolicy_BsrrMux_GPIOA<9>;     // Bluepill, BlackPill-BMP
using DirPolicy_PA10_BsrrMux = DirPolicy_BsrrMux_GPIOA<10>;    // Nucleo-G431, Nucleo-L432
```

The actual SBWDIO data still goes through PA7/PA6; the mux just gates
which one electrically reaches the trace. The data BSRR script targets
the same GPIOA register but a different bit.

### STLinkV2: DirPolicy_PB14_BitBand (F1)

Bit-banding lets the dir-script touch *only* the PB14 nibble of
`GPIOB->CRH` — every other pin's mode is preserved by hardware, with
no runtime snapshot or precompute needed. The DMA destination address
is the bit-band alias of `MODE[0]`, with dest-incrementing across the
four consecutive alias words covering `MODE[1:0]` + `CNF[1:0]`.

```cpp
struct DirPolicy_PB14_BitBand
{
    static constexpr unsigned kWordsPerFlip = 4;
    static void Init() {}            // values are constexpr — nothing to do

    // PB14: bit-band aliases for GPIOB->CRH bits 24..27 (MODE0, MODE1, CNF0, CNF1).
    static volatile uint32_t* DirRegister()
    {
        constexpr uintptr_t kCRH    = 0x40010C04;                      // &GPIOB->CRH
        constexpr uintptr_t kAlias0 = 0x42000000 + (kCRH - 0x40000000) * 32 + 24 * 4;
        return reinterpret_cast<volatile uint32_t*>(kAlias0);
    }

    static const uint32_t* DriveOutput()
    {
        // PP-Output @ 10 MHz: MODE=01, CNF=00  → { 1, 0, 0, 0 }
        static constexpr uint32_t v[4] = { 1, 0, 0, 0 };
        return v;
    }
    static const uint32_t* DriveInput()
    {
        // Floating-Input: MODE=00, CNF=01      → { 0, 0, 1, 0 }
        static constexpr uint32_t v[4] = { 0, 0, 1, 0 };
        return v;
    }
};
```

The DMA template for this variant is `mem→periph`, source incrementing,
**destination incrementing**, `NDTR = 8` for the full per-scan script
(`2 × kWordsPerFlip`), triggered by TIM1 at each phase boundary. Each
of the 8 transfers writes one bit; hardware-atomic, no RMW on the CPU
side, no other pin touched.

Floating-input is chosen over input-pull-up here because the SBW PCB
has a defined external pull on the SBWDIO net (slau320 wire spec). If
a future board needs internal pull, swap CNF[1] to 1 and set
`ODR.PB14 = 1` in `Init()`.

### F4 / G4 / L4 equivalent

`MODER` is 2 bits per pin instead of CRL/CRH's 4. The bit-band approach
still applies, just with 2 alias words per flip (MODER[2n], MODER[2n+1])
and `kWordsPerFlip = 2`. A `DirPolicy_BitBand_MODER<Port, Pin>` template
can factor it when an F4/G4/L4 board ever needs the un-buffered SBW
path.

## Sampling strategy

Sample SBWDIO IDR on **every** SBWCLK falling edge via DMA into a
ring buffer; discard the 2-of-3 samples that don't carry TDO in
software. At 5 MHz worst case that's 5 Msamples/s of IDR DMA —
within F103 AHB headroom, and avoids burning TIM2 as a /3 prescaler.

(The "TIM1 RCR = 2 → every-3rd update event" trick from SBW-AI.md is
unavailable here because TIM1's RCR is already in use by the
single-shot scan-length mechanism we inherit from DtrigJtag.)

## Async pipeline / overlap

Reuse `Firmware.shared/util/JtagPending<T>` verbatim. Each
`OnXxxShift()` in `SbwDev` returns a `JtagPending<T>` whose:

- `Wait()` waits for TIM1 single-shot completion + final IDR sample
- `Get()` decodes the sampled TDO bits out of the IDR ring buffer
- conversion-to-T blocks; statement-only is fire-and-forget

CPU renders the next scan's BSRR data+direction script and IDR sample
buffer while the current frame is DMA-flushing — same ping-pong as
JTAG, just 3× the buffer entries per logical bit.

## Resource-ownership rules (codify the user's point)

`SbwDev::Init()` and `JtagDev::Init()` are each **sovereign**:

- force all GPIO pins they own to the configured mode (overwrite
  whatever the previous mode set)
- force-disable every DMA channel they will configure before
  re-`Setup()`ing it
- pulse APBxRSTR for SPI1 (JTAG mode) or for TIM1 if switching
  channel configurations significantly (SBW mode)
- leave the *other* mode's pins in their `*_Init` (release) state

`TapMcu::Open(mode)` calls exactly one of `JtagDev::Init()` /
`SbwDev::Init()`. There is no `Release()` / `Claim()` API and no
runtime arbitration. Mode change = call the other mode's `Init()`.

## Template skeleton

See `Firmware.shared/util/DtrigSbw.h` for the draft. Signature
mirrors `DtrigJtag<>` with two extra parameters:

```cpp
template <
    typename SysClk
    , Timer::Unit kTim                  // advanced timer (TIM1)
    , Timer::Channel kSbwClk            // CH driving SBWCLK PWM (CH1N on STLinkV2)
    , Timer::Channel kSbwDataTrig       // CH triggering SBWDIO BSRR DMA
    , Timer::Channel kSbwDirTrig        // CH triggering SBWDIO CRH DMA
    , Timer::Channel kSbwSampleTrig     // CH triggering IDR sample DMA
    , typename SbwDioPin                // Bmt::Gpio pin alias (PB14 here)
    , typename DirPolicy                // strategy A (CRH swap) vs B (mux pin)
    , uint32_t kFreq                    // SBWCLK rate (1×, not multiplied)
    , JtagFrame::Scan kScan
    , JtagFrame::NumBits kNumBits
    , bool kCmpComplementary = true     // SBWCLK on CHN
>
class DtrigSbw;
```

Same surface as DtrigJtag: `Init`, `SetupDma`, `ReleaseDma`,
`RenderTransaction`, `DoGoIdle`, `Start`, `Wait`, `GetResult`. The
encoder is the part that diverges — see next section.

## RenderTransaction encoding (per bit)

For each logical JTAG bit `i ∈ [0, kBitCount)`:

- 3 BSRR words: `[TMS_i, TDI_i, X]` (3rd is don't-care; direction
  flips to input here so SBWDIO is released)
- 1 CRH word at cycle 3i+2: switch SBWDIO to input
- 1 CRH word at cycle 3(i+1): switch SBWDIO back to output (or skip
  on the last bit to leave SBWDIO Hi-Z)

The pack-and-`__REV` trick from DtrigJtag stays viable for the BSRR
data column if we encode TMS+TDI as two bit-planes packed into a
single uint32_t per bit, expanded into 3 BSRR words by a small loop
or LUT in `RenderTransaction`. For 32-bit DR that's 96 BSRR words +
~64 CRH words — well within DMA budget at 1 MHz wire rate.

## What this draft commits to and what it leaves open

**Committed**:
- Reuse JtagPending<T> and the Render/Start/Wait/GetResult surface.
- Sovereign-Init resource ownership model.
- PB13/TIM1_CH1N for SBWCLK on STLinkV2.
- Sample-every-cycle + discard for TDO.

**Open** (to be settled during implementation):
- Final DMA channel assignment (verify no clash with JTCLK
  flash-burst peripherals).
- DirPolicy concrete API (Setup/RenderBit/FlipToInput/FlipToOutput).
- BSRR script encoding details (loop vs LUT, packed bit-plane
  representation).
- `cnt_offset` calibration knobs per speed grade (will need LA
  verification on real hardware once SBW signals are exposed).

## File layout when implemented

| File | Role |
|------|------|
| `Firmware.shared/util/DtrigSbw.h` | Template class (skeleton lives here as draft) |
| `Firmware.shared/drivers/SbwDev.dtrig.cpp` | Concrete `ISbwDev` (or `JtagDev`-subclass) virtual methods |
| `target.stlinv2/platform.h` | Add `kWaveSbw*` constants, `SbwOn`/`SbwOff` pin groups, `kDtrigSbwCntOffset_*` |
| `Firmware.shared/platform-defs.h` | Add `OPT_SBW_IMPL_DTRIG` (separate from `OPT_JTAG_IMPLEMENTATION`) |
| `Firmware.shared/stdproj.h` | Guard `OPT_INCLUDE_SBW_DTRIG_` |
