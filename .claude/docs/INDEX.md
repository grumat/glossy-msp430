# `.claude/docs/` Index

Living working notes for this project. Treat each file as a refinable
draft, not a deliverable doc — update in place rather than starting
parallel files on the same topic. Repo root stays clean of markdown
besides `CLAUDE.md` and `CODE_GUIDELINES.md`.

## Layout

```
.claude/docs/
├── INDEX.md                 (this file — top-level map)
├── bmt/                     (bmt template library: peripheral reviews, design rationale, cross-port comparisons)
├── drivers/                 (JTAG/SBW driver implementation notes — DtrigJtag, ST-Link V2, deprecation rationale, …)
├── msp430/                  (MSP430 protocol: breakpoints, JTAG/SBW, EEM, flash workflows)
├── project/                 (cross-cutting project status: roadmaps, gap lists, missing-feature inventories)
└── traceswo/                (TraceSWO / SWO bring-up and clone-board workarounds)
```

When adding a doc, pick the closest category. Create a new category
folder only when nothing fits and the topic is broad enough to grow.
Always add an entry to the appropriate section of this index, with a
one-line hook so future-you (or future-Claude) can locate it.

## bmt/

- [BMT_RCC_ENABLER_DESIGN](bmt/BMT_RCC_ENABLER_DESIGN.md) — single-source-of-truth peripheral clock/reset enabler design
- [GPIO_F1XX_IMPLEMENTATION_REVIEW](bmt/GPIO_F1XX_IMPLEMENTATION_REVIEW.md) — F1 GPIO bmt review
- [GPIO_F1XX_VS_G4XX_COMPARISON](bmt/GPIO_F1XX_VS_G4XX_COMPARISON.md) — cross-family GPIO API comparison
- [GPIO_G4XX_MISSING_FEATURES](bmt/GPIO_G4XX_MISSING_FEATURES.md) — gap list on the G4 port
- [TIMER_F1XX_IMPLEMENTATION_REVIEW](bmt/TIMER_F1XX_IMPLEMENTATION_REVIEW.md) — F1 Timer bmt review

## drivers/

- [DTRIG_JTAG_DRIVER](drivers/DTRIG_JTAG_DRIVER.md) — Double-Trigger SPI+TIM1 driver design (the only supported transport variant)
- [TIM_SBW_DRIVER](drivers/TIM_SBW_DRIVER.md) — design plan for the SBW counterpart: TIM1_CH1N clock on PB13, BSRR+CRx DMA on PB14, sovereign-Init resource model (draft; skeleton in `Firmware.shared/util/TimSbw.h`)
- [SBW_ISSUES](drivers/SBW_ISSUES.md) — draft GitHub issue bodies for the SBW bring-up: epic + 5 sub-issues (SbwDev skeleton, TimSbw encoder, direction turnaround, TapMcu arbitration, bench validation)
- [DTRIG_SBW_SPI_ALT](drivers/DTRIG_SBW_SPI_ALT.md) — parked alternate: encode SBW as a 3×-clocked SPI stream with SBWO on a TIM channel, reusing the DtrigJtag template almost verbatim (buffered boards only; post-MVP)
- [SPI_VARIANT_REMOVED](drivers/SPI_VARIANT_REMOVED.md) — why `JtagDev.spi.cpp` was retired in favour of dtrig (TMS edge-alignment limit) + last-working git refs
- [TIM_VARIANT_REMOVED](drivers/TIM_VARIANT_REMOVED.md) — why `JtagDev.tim.cpp` (TIM_DMA / TIM_DMA_SLOW) was retired + last-working git refs

## msp430/

- [BREAKPOINTS_ANALYSIS](msp430/BREAKPOINTS_ANALYSIS.md) — breakpoint capability map across MSP430 families
- [MSP430_BREAKPOINT_WORKFLOW](msp430/MSP430_BREAKPOINT_WORKFLOW.md) — runtime workflow (insert/remove, EEM units)
- [MSP430_JTAG_SBW_IMPLEMENTATION](msp430/MSP430_JTAG_SBW_IMPLEMENTATION.md) — JTAG / Spy-Bi-Wire low-level protocol
- [SBW_PIN_ROLES_AND_FUSE](msp430/SBW_PIN_ROLES_AND_FUSE.md) — connector-level pin-role multiplexing: TEST/SBWTCK + ~RST/SBWDIO dual functions, signal naming (J…/T… prefixes), why fuse-burn needs 3 pins + 330 Ω, 2-wire LaunchPad vs TI; Glossy STLinkV2 PB13/PB14 mapping (entry must bit-bang the SBW pins, not JTAG-14 nRST/TEST)
- [SBW-AI](msp430/SBW-AI.md) — SBW notes
- [FR5994_SBW_GOLDEN_REFERENCE](msp430/FR5994_SBW_GOLDEN_REFERENCE.md) — known-good eZ-FET `MSP430_OpenDevice` SBW capture decoded to TAP-FSM + TCLK phase; function-boundary map vs our driver; kills the magic-pattern theory (#19/#20), pins the bug to `ReadWords` TCLK phasing + the 0x99 per-word-SetPC branch. Decoder: `supp/docs-ai/decode_sbw_fsm.py`
- [I2031_ACQUISITION_GOLDEN_REFERENCE](msp430/I2031_ACQUISITION_GOLDEN_REFERENCE.md) — corrected #43 reference (supersedes the old JTAG-only doc). The i20xx "password" is the **BSL Entry Sequence** (RST/TEST pulse train halting the CPU to LPM4) + **500 ms** settle + a `SPYBIWIREJTAG`/SBW open (TEST-pin activation), then jtag_id `0x89`. Confirmed by `MSP430_Configure(INTERFACE_MODE,…)`: plain `JTAG_IF` fails, `SPYBIWIREJTAG_IF`/`SPYBIWIRE_IF` work. glossy lacks the BSL preamble (the `slow_settle` tweak was wrong, reverted). Decoder: `supp/docs-ai/decode_jtag_fsm.py`
- [INIT_TRACE_VALIDATION](msp430/INIT_TRACE_VALIDATION.md) — collection of Glossy's own startup TRACE dumps per target+transport (validation matrix: jtag_id / raw signature / profile / HW bkpts) to validate the SBW and JTAG transport layers; companion to the wiring guide. Seeded with MSP430F5418A (STLinkV2/SBW); raw dumps tracked in `msp430/INIT_TRACE_VALIDATION/` (naming: `<mcu>_<transport>_<probe>_<kind>.txt`)
- **Wiki cross-ref:** EEM register map / undocumented emulation registers (incl. `ETKEYSEL`/`ETCLKSEL` per-module clock control, reverse-engineered from UIF source) live in the `glossy-msp430.wiki` repo: `The-Missing-EEM-Documentation.md`. Referenced from [MSP430_BREAKPOINT_WORKFLOW](msp430/MSP430_BREAKPOINT_WORKFLOW.md) §3.3.

## project/

- [MISSING_FEATURES](project/MISSING_FEATURES.md) — gap list to reach standalone-GDB-server / BMP parity (VCP, SBW, vFlash*, monitor commands, EEM, etc.); bullets annotated with GitHub issues #21–#39
- [MSP430_WIRING_GUIDE](project/MSP430_WIRING_GUIDE.md) — DRAFT probe↔target wiring guide: BluePill-G431 Jiga (TI SBW) vs STLinkV2 (ARM-remap SBW) pinouts, the 3 repo adapters, per-proto-board JTAG/SBW capability matrix, TI 14-pin reference, 3.3 V/RC cautions. Promote to `Hardware/WIRING.md` once the ⚠/🛠 items are confirmed

## traceswo/

- [TRACESWO_BARE_METAL_C](traceswo/TRACESWO_BARE_METAL_C.md) — minimal SWO driver in C
- [TRACESWO_CLONE_WORKAROUND](traceswo/TRACESWO_CLONE_WORKAROUND.md) — Geehy-clone PB3 GPIO-as-output workaround
- [TRACESWO_INITIALIZATION](traceswo/TRACESWO_INITIALIZATION.md) — SWO bring-up sequence on F1xx
