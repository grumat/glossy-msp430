# Why `JtagDev.tim.cpp` (TIM_DMA / TIM_DMA_SLOW) was removed in favour of DTRIG

Status: variant removed 2026-05-10 alongside the async `JtagPending<T>` shift
refactor. Companion to [DTRIG_JTAG_DRIVER.md](DTRIG_JTAG_DRIVER.md) and
[SPI_VARIANT_REMOVED.md](SPI_VARIANT_REMOVED.md).

## TL;DR

The TIM+DMA variant shipped two flavours:

- `OPT_JTAG_IMPL_TIM_DMA` — fastest path, used a single timer + DMA chain to
  drive JTCK/JTDI/JTDO/JTMS in lockstep. Worked only when all four JTAG signals
  shared the same GPIO port (true on bluepill, false on STLinkV2 / nucleos).
- `OPT_JTAG_IMPL_TIM_DMA_SLOW` — split-bus variant for STLinkV2 hardware. Used
  three separate DMA channels driven from TIM1 (CH1N → JTCK PWM, CH2 → JTMS
  trigger, CH3 → JTDI shift, CH4 → JTDO read). Per-bit DMA cost capped the
  achievable JTCK rate at ~1 Mbps on the F103.

DTRIG superseded both because it:

1. Drops per-bit TMS DMA — TMS is a single-channel TIM1 PWM with at most two
   CCR-reload DMA writes per frame, regardless of frame width. (See
   [DTRIG_JTAG_DRIVER.md](DTRIG_JTAG_DRIVER.md) §"DMA budget vs old DtrigJtag".)
2. Uses SPI1 hardware to clock JTCK + JTDI/JTDO together, which gets to ~9 Mbps
   on the F103 — an order of magnitude faster than the TIM_DMA_SLOW variant.
3. Works on every target the firmware supports (no split-bus restriction):
   bluepill, STLinkV2, and nucleos all map JTCK/JTDI/JTDO onto SPI1 PA5/PA6/PA7.
4. Makes async-shift overlap practical (see `Firmware.shared/util/JtagPending.h`),
   because the SPI hardware naturally exposes a per-frame DMA TC the next render
   can wait on.

## Last-working git references

If you need to revive any of this code, check out:

| What | Last-working commit | Notes |
|------|---------------------|-------|
| Tip of `JtagDev.tim.cpp` history | `af121e4` (`JtagDev: hoist common OnEnterTap into shared TU`) | Compiles cleanly under the pre-async ITapInterface |
| `7f2cb07` "A quite stable configuration for STLinkV2 hardware capable of 9 Mbps JTAG frames" | The architectural high-water mark for the TIM_DMA_SLOW variant on STLinkV2 |
| `Firmware.shared/util/TimDmaWave.h` final state | `6d3742c` ("TimDmaWave / TmsAutoShaper: explicit master/slave clock binding") | The deleted timer/DMA helper |
| `Firmware.shared/util/TmsAutoShaper.h` final state | `6d3742c` (same) | Shared with the SPI variant — see `SPI_VARIANT_REMOVED.md` |

## What was deleted

- `Firmware.shared/drivers/JtagDev.tim.cpp`
- `Firmware.shared/util/TimDmaWave.h`
- `Firmware.shared/util/TmsAutoShaper.h` (also used by the removed SPI variant)
- `OPT_JTAG_IMPL_TIM_DMA` and `OPT_JTAG_IMPL_TIM_DMA_SLOW` selectors in `platform-defs.h`
- All `#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA*` branches in every `target.*/platform.h`
- `OPT_INCLUDE_JTAG_TIM_DMA_` plumbing in `Firmware.shared/stdproj.h`
- `OPT_BUFFER_LAYOUT_TRIPLE` is *kept* in `stdproj.h` for future use (was the
  TIM_DMA_SLOW carrier for the per-bit TMS AUX buffer); no current variant
  selects it.

## When you would still want this code

- A future port to a target where the SPI peripheral cannot be allocated to JTAG
  (e.g. it's needed for an external memory or codec). In that case
  `JtagDev.tim.cpp` is your reference — it shows how to drive JTAG entirely from
  a single advanced timer + DMA, no SPI.
- A target with very large JTAG frames (>40 SPI bytes per scan) where the
  ping-pong buffer becomes the bottleneck. The TIM_DMA path used a 40-element
  per-bit buffer with `OPT_BUFFER_LAYOUT_TRIPLE` for the TMS AUX channel.

For everyday work on the supported targets, DTRIG is strictly faster, smaller,
and simpler.
