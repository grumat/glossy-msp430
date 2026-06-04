# Missing Features for a Standalone GDB Server

Gap list to reach Black-Magic-Probe parity for MSP430. Compiled from
`README.md` status flags, in-tree TODOs, `platform-defs.h`, and the
`Firmware.shared/docs/` notes. Update in place as items land.

Each bullet carries its GitHub issue number (e.g. **#21**). Close the
issue and strike/remove the bullet here as items land.

## Transport / Host I/O

- **#21 — USB CDC-ACM (VCP)** — `OPT_GDB_IMPL_VCP` is defined in
  `Firmware.shared/platform-defs.h` but unimplemented; current GDB
  transport is `USART1` / `USART2` / `USART3` only.
- **#22 — USB DFU** for firmware self-upgrade (README [TBD]).

## Target Wire Protocol

- **#23 (epic #6, acquisition #20) — Spy-Bi-Wire (SBW)** — JTAG is the
  mature path (`OPT_JTAG_IMPL_DTRIG`). An SBW transport (`TimSbw`, the
  timdma model) is in active bring-up: device-ID and FR5994 descriptor
  read are bench-confirmed, with a 5-grade RC-bound wire-speed table
  (200 kHz–1.2 MHz). Flash program/erase over SBW and a full
  regression across families are still pending (**#23**). See
  `.claude/docs/drivers/TIM_SBW_DRIVER.md` and
  `.claude/docs/msp430/SBW-AI.md`.
- **#24 — Target voltage / supply control** — variable VTref + power
  switching is a BMP-style differentiator advertised in the README;
  hardware planned, no firmware glue.
- **#25 — Adjustable JTAG/SBW speed exposed to host** — internal grade
  tables exist (JTCK grades and the SBW `BusSpeed` 200–1200 kHz
  table), but the grade is hard-coded at the call site. `TapMcu.cpp`
  TODO: "expose speed selection through the debug session
  configuration" — no `monitor frequency` yet (depends on **#28**).

## GDB RSP Surface

- **#26 — `vFlashErase` / `vFlashWrite` / `vFlashDone`** — not handled
  in `Firmware.shared/ui/gdb.cpp`. Flash programming currently relies on
  `X` / `M` writes routed through `WriteMem`, which is slower and
  breaks `load` semantics in some toolchains.
- **#27 — `vRun` / `vAttach` / `vKill` / extended-remote attach
  lifecycle** — not present; only the basic continue/step state machine
  is there.
- **#28 — `qRcmd` (`monitor`) command set** — handler exists
  (`Gdb::MonitorCommand`) but is a thin stub; no command table for
  `monitor reset`, `monitor jtag_scan`, `monitor tpwr`,
  `monitor frequency`, etc. that BMP users expect.
- **#29 — `monitor`-style chip selection / mass-erase / unlock (JTAG
  password / BSL)** — required for locked FRAM parts; not wired.
- **#30 — 3rd parameter of breakpoint packet** (`gdb.cpp` TODO) —
  `Z` / `z` cond/commands ignored.
- **#31 — LPM5 wake / low-power debug** (`TapDev430Xv2.cpp` TODO).
- **#32 — CRC32 via STM32 hardware** (`util/crc32.cpp` TODO) —
  currently software, slows `qCRC`.

## Probe Intelligence (BMP parity)

- **#33 — Asynchronous JTAG pipeline at the protocol layer** —
  DMA/async is done at the wire layer (`JtagPending<T>`), but the README
  lists protocol-level async as [TBD].
- **#34 — JTAGONSBW activation / pure-JTAG init fix-up**
  (`TapDev430Xv2.cpp` TODO).
- **#35 — EEM full feature set** — `Firmware.shared/docs/EEM-docs.md`
  is full of TODOs: `EMU_FEAT_EN`, `DEB_TRIG_LATCH`, `EEM_RST`,
  `E_STOPPED`, trace buffer, complex triggers. HW breakpoints work;
  watchpoints / trace / state-storage do not.
- **#36 — TapDev430 activation-key path** (`TapDev430.cpp` TODOs around
  `activationKey == 0x20404020`).

## Tooling / Portability

- **#37 — Makefile / non-VisualGDB build** — README explicitly lists
  this as planned (`good first issue`).
- **#38 — Cross-platform host workflow validation** (Linux / macOS) —
  README [TBD].

## Hardware

- **STLink-V2-clone bring-up** — *done / no issue:* now a primary
  low-cost target (the final G431 fits a reused clone case and the F103
  clone runs the firmware directly). Documented compromises: 3.3 V only,
  SBW-only on 10-pin clones, UART hw-mod.
- **#39 — Final MSPBMP / MSPBMP2-stick re-spin** — schematics evolved,
  awaiting the VCP work (**#21**).
