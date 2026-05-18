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

## Direction-flip strategies (pick one)

**A. Single-pin BSRR + CRH script (recommended for STLinkV2)**

SBWDIO sits on PB14. Direction toggled by rewriting `GPIOB->CRH`
for nibble 14 between push-pull output (`0x3`) and pull-up input
(`0x8`). A second DMA channel triggered by a TIM1 channel writes
the CRH value at the right cycle within each 3-frame.

Pros: works on the existing single-pin hardware. One BSRR DMA word
per cycle for data, one CRH DMA word per direction flip.

Cons: CRH is a full 32-bit register — DMA script must preserve the
other 7 nibbles. Either (a) precompute both full CRH values and
swap, or (b) accept that the firmware owns all of GPIOB high nibbles
during SBW mode.

**B. External mux + dedicated in/out pins**

Future board revision could add the PA9/PA7/PA6 mux from SBW-AI.md
(direction = single GPIO bit, data-in and data-out on separate pins).
Cleaner DMA encoding — direction is one bit in the same BSRR word as
data — but needs hardware change.

**Choice for v1:** strategy A on STLinkV2. The template will accept
the direction mechanism as a policy class so strategy B can drop in
later without rewriting the framing code.

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
