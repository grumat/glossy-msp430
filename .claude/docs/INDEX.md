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

- [DTRIG_JTAG_DRIVER](drivers/DTRIG_JTAG_DRIVER.md) — Double-Trigger SPI+TIM1 driver design (current best path)
- [SPI_VARIANT_DEPRECATION](drivers/SPI_VARIANT_DEPRECATION.md) — why `JtagDev.spi.cpp` is being phased out in favour of dtrig (TMS edge-alignment limit)
- [STLinV2-AI](drivers/STLinV2-AI.md) — ST-Link V2 clone hardware notes

## msp430/

- [BREAKPOINTS_ANALYSIS](msp430/BREAKPOINTS_ANALYSIS.md) — breakpoint capability map across MSP430 families
- [MSP430_BREAKPOINT_WORKFLOW](msp430/MSP430_BREAKPOINT_WORKFLOW.md) — runtime workflow (insert/remove, EEM units)
- [MSP430_JTAG_SBW_IMPLEMENTATION](msp430/MSP430_JTAG_SBW_IMPLEMENTATION.md) — JTAG / Spy-Bi-Wire low-level protocol
- [SBW-AI](msp430/SBW-AI.md) — SBW notes

## traceswo/

- [TRACESWO_BARE_METAL_C](traceswo/TRACESWO_BARE_METAL_C.md) — minimal SWO driver in C
- [TRACESWO_CLONE_WORKAROUND](traceswo/TRACESWO_CLONE_WORKAROUND.md) — Geehy-clone PB3 GPIO-as-output workaround
- [TRACESWO_INITIALIZATION](traceswo/TRACESWO_INITIALIZATION.md) — SWO bring-up sequence on F1xx
