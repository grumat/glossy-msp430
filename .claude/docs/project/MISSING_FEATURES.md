# Missing Features for a Standalone GDB Server

Gap list to reach Black-Magic-Probe parity for MSP430. Compiled from
`README.md` status flags, in-tree TODOs, `platform-defs.h`, and the
`Firmware.shared/docs/` notes. Update in place as items land.

## Transport / Host I/O

- **USB CDC-ACM (VCP)** — `OPT_GDB_IMPL_VCP` is defined in
  `Firmware.shared/platform-defs.h` but unimplemented; current GDB
  transport is `USART1` / `USART2` / `USART3` only.
- **USB DFU** for firmware self-upgrade (README [TBD]).

## Target Wire Protocol

- **Spy-Bi-Wire (SBW)** — only JTAG is implemented
  (`OPT_JTAG_IMPL_DTRIG`). SBW is referenced in docs but no driver
  path exists. See `.claude/docs/msp430/SBW-AI.md`.
- **Target voltage / supply control** — variable VTref + power
  switching is a BMP-style differentiator advertised in the README;
  hardware planned, no firmware glue.
- **Adjustable JTAG/SBW speed exposed to host** — `TapMcu.cpp:114`
  TODO: "expose speed selection through the debug session
  configuration".

## GDB RSP Surface

- **`vFlashErase` / `vFlashWrite` / `vFlashDone`** — not handled in
  `Firmware.shared/ui/gdb.cpp`. Flash programming currently relies on
  `X` / `M` writes routed through `WriteMem`, which is slower and
  breaks `load` semantics in some toolchains.
- **`vRun` / `vAttach` / `vKill` / extended-remote attach lifecycle**
  — not present; only the basic continue/step state machine is there.
- **`qRcmd` (`monitor`) command set** — handler exists
  (`Gdb::MonitorCommand`) but is a thin stub; no command table for
  `monitor reset`, `monitor jtag_scan`, `monitor tpwr`,
  `monitor frequency`, etc. that BMP users expect.
- **`monitor`-style chip selection / mass-erase / unlock (JTAG
  password / BSL)** — required for locked FRAM parts; not wired.
- **3rd parameter of breakpoint packet** (`gdb.cpp:401` TODO) —
  `Z` / `z` cond/commands ignored.
- **LPM5 wake / low-power debug** (`TapDev430Xv2.cpp:1285` TODO).
- **CRC32 via STM32 hardware** (`util/crc32.cpp:5` TODO) — currently
  software, slows `qCRC`.

## Probe Intelligence (BMP parity)

- **Asynchronous JTAG pipeline at the protocol layer** — DMA/async is
  done at the wire layer (`JtagPending<T>`), but the README lists
  protocol-level async as [TBD].
- **JTAGONSBW activation / pure-JTAG init fix-up**
  (`TapDev430Xv2.cpp:890` TODO).
- **EEM full feature set** — `Firmware.shared/docs/EEM-docs.md` is
  full of TODOs: `EMU_FEAT_EN`, `DEB_TRIG_LATCH`, `EEM_RST`,
  `E_STOPPED`, trace buffer, complex triggers. HW breakpoints work;
  watchpoints / trace / state-storage do not.
- **TapDev430 activation-key path** (`TapDev430.cpp:124`, `:294`
  TODOs around `activationKey == 0x20404020`).

## Tooling / Portability

- **Makefile / non-VisualGDB build** — README explicitly lists this
  as planned.
- **Cross-platform host workflow validation** (Linux / macOS) —
  README [TBD].

## Hardware

- **STLink-V2-clone bring-up** — planned cheap option with documented
  compromises: 3.3 V only, SBW-only on 10-pin clones, UART hw-mod.
- **Final MSPBMP / MSPBMP2-stick re-spin** — schematics evolved,
  awaiting the VCP work.
