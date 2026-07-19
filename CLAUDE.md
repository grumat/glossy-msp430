# CLAUDE.md - Glossy MSP430

This is the authoritative agent instructions file for this project. Detailed coding standards live in [`CODE_GUIDELINES.md`](CODE_GUIDELINES.md) — read it before making non-trivial changes.

## Project Overview

**Glossy MSP430** is a GDB remote serial protocol (RSP) debug probe for Texas Instruments MSP430 microcontrollers, running on STM32 hardware. It is analogous to the Black Magic Probe for ARM. The probe connects via USB/UART and talks GDB RSP, allowing full debugging of MSP430 targets over JTAG or Spy-Bi-Wire.

## Build System

**Primary**: Visual Studio 2022 + VisualGDB (commercial plugin)
- Open `glossy-msp430.sln` and build from there
- No Makefile alternative yet
- Build configurations: `Debug|VisualGDB` and `Release|VisualGDB` for firmware
- For .NET tools: `Debug|x86` / `Release|Any CPU`

**Headless firmware build (CLI, for compile-checking without the IDE)** — verified
on this machine; the ARM toolchain (`com.visualgdb.arm-eabi`) resolves, but two
caveats apply when invoking MSBuild on a single `target.*/*.vcxproj`:

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild "target.stlinv2\target.stlinv2.vcxproj" /t:Build `
    /p:Configuration=Debug /p:Platform=VisualGDB `
    "/p:SolutionDir=P:\github.com\glossy-msp430\"
```

- **`SolutionDir` must be passed explicitly.** The include dirs use
  `$(SolutionDir)/Firmware.shared` and `$(SolutionDir)/bmt/include`; building a
  bare `.vcxproj` leaves `$(SolutionDir)` empty and `stdproj.h` won't be found.
  (Building the `.sln` in the IDE sets it automatically.)
- **The MSP430 Funclet `ProjectReference`s fail headless** ("No toolchain
  found") — they are pinned to an old GUID-registered MSP430 GCC that isn't in
  VisualGDB's CLI toolchain DB. The firmware itself does not need them (it embeds
  the committed pre-built `Firmware.shared/res/*.bin`); they only need rebuilding
  when their MSP430 source changes.
  - **Do NOT edit the `.vcxproj` to work around this** (e.g. dropping the two
    `<ProjectReference>` entries). The user keeps the solution open in VS, and any
    project-file edit pops a "reload?" dialog every iteration. `git checkout --`
    to restore still dirties the file twice. `/p:BuildProjectReferences=false`
    does **not** help — VisualGDB's `SysprogsPlatform.targets` builds the
    referenced projects regardless and still errors "No toolchain found".
  - **Prefer the IDE build** as the compile-check for `target.*` C++ changes — the
    user rebuilds before flashing anyway. If a non-invasive headless syntax check
    is ever needed, invoke `arm-none-eabi-g++ -fsyntax-only` directly on the one
    TU (locate the VisualGDB toolchain first; it is not on PATH).

**C# tools** (.NET 9.0 SDK):

```bash
dotnet build ChipInfo/ImportDB/ImportDB.csproj
dotnet build ChipInfo/MkChipInfoDbV2/MkChipInfoDbV2.csproj
dotnet build UnitTest/UnitTest.csproj
dotnet run --project UnitTest/UnitTest.csproj
```

## Architecture Key Points

- **bmt/**: Bare Metal Templates submodule — C++20 template library for STM32 HAL (zero-overhead `constexpr` abstractions)
- **Firmware.shared/**: Platform-independent firmware logic
- Each target has its own `platform.h` in `target.*/` directories
- **Funclets/**: MSP430 assembly/C binaries embedded as byte arrays for flash operations (uploaded to MSP430 RAM at runtime)
- **ChipInfo/**: Toolchain to generate embedded chip database

## Critical Constraints

1. **Chip database is pre-generated**: `Firmware.shared/drivers/ChipInfoDB.h` (370KB) — do not edit by hand
2. **Regeneration process** (only when adding new chip support):

   ```bash
   ScrapeDataSheet                          # requires all TI PDFs locally
   ImportDB "ExtractChipInfo\MSP430-devices\devices" "ScrapeDataSheet\Results\All Data.csv" "ImportDB\Results\results.db"
   MkChipInfoDbV2 "ImportDB\Results\results.db" "GDB430\ChipInfoDB.h"
   ```

3. **Platform configuration** via preprocessor constants in `Firmware.shared/platform-defs.h`:
   - `OPT_JTAG_IMPLEMENTATION`: `OPT_JTAG_IMPL_DTRIG` (the only supported variant; the legacy SPI / TIM_DMA paths were removed — see `.claude/docs/drivers/SPI_VARIANT_REMOVED.md` and `TIM_VARIANT_REMOVED.md`)
   - `OPT_JTAG_TCLK_IMPLEMENTATION`: `OPT_JTCLK_IMPL_TIM_DMA` or `OPT_JTCLK_IMPL_SPI`
   - `OPT_GDB_IMPLEMENTATION`: `OPT_GDB_IMPL_VCP` (TBD), `USART1`, `USART2`, `USART3`

## Code Conventions

**Comprehensive guidelines**: See [`CODE_GUIDELINES.md`](CODE_GUIDELINES.md) for detailed coding standards.

Key conventions:
- C++20 with heavy `constexpr`/`consteval` and template metaprogramming
- No RTOS; bare-metal interrupt-driven design
- `stdproj.h` is common precompiled header for all firmware TUs
- `<main.inl>` (angle-bracket include) provides platform-specific `main()` scaffold
- **Windows line endings** (CRLF) for all files
- **Tab size: 4 spaces** (real tabs, not converted to spaces)
- **Naming**: PascalCase for types, camelCase for globals, snake_case for locals
- **Organization**: Context-based class layout with thorough documentation

### Use bmt abstractions, not raw register access (REQUIRED)

Firmware code **must** use the `bmt/` template library (`Bmt::Gpio`, `Bmt::Timer`, `Bmt::Dma`,
`Bmt::Spi`, `Bmt::Clocks`, etc.) for all peripheral access. Direct manipulation of CMSIS
register pointers (`TIMx->CR1`, `GPIOA->BSRR`, `DMA1_Channel7->CCR`, `RCC->APB1ENR`, …) is
forbidden in normal driver code.

**Why**: bmt encodes peripheral configuration as `constexpr` template parameters validated
at compile time, produces zero-overhead code, and keeps register addresses, bit masks, and
clock-enable wiring consistent across targets. Open-coded register access bypasses these
guarantees, duplicates configuration in two places (template + raw write), and is the
top source of subtle bugs when peripherals are reconfigured (e.g. a raw `TIMx->SMCR = ...`
write that gets clobbered by a later `Timer::Any::Init()` RCC reset).

**How to apply**:
- New code uses `Bmt::*` templates exclusively. If the abstraction is missing a method
  you need, add it to `bmt/` (or extend the relevant template) rather than reaching into
  raw registers from the caller.
- The narrow exceptions are: (a) debug/trace dumps that read registers read-only for
  inspection, (b) ISR fast paths where a specific bmt method does not yet exist — in
  that case, prefer adding the method to bmt. Both cases must be commented with the
  reason.
- When reviewing or modifying existing code, replace raw register writes with the
  matching bmt call when touching nearby code; do not introduce new raw writes.
- Clearing status flags (`timer->SR = 0`) and similar one-liners that bmt does not
  expose are acceptable inline only when annotated and ideally promoted to a bmt helper
  in a follow-up.

### Allocate MCU resources wisely (REQUIRED)

The firmware owns the entire MCU — there is no OS, no other application, and no
external arbiter. Every timer, DMA channel, GPIO, and peripheral is ours to assign.
When two features collide on the same resource (e.g. TIM3_CH1 and TIM1_CH3 both
mapping to DMA1_CH6 on STM32F1), the **first remedy is to reassign one of them to
a free resource**, not to invent runtime hand-off / claim-release dances.

**Why**: Time-multiplexing a shared resource adds ordering bugs (stale flags,
half-configured channels, missed interrupts), runtime cost, and code that is hard
to reason about. Static, exclusive ownership is cheaper and provably correct at
compile time. We just spent a debugging session on exactly this failure mode
(DtrigJtag and JtclkWaveGen fighting over DMA1_CH6).

**How to apply**:
- Before adding a peripheral, check what is already in use in the relevant
  `target.*/platform.h` and pick a free channel/timer.
- If a clash is unavoidable on the current target, document it in `platform.h`
  with a comment explaining why no alternative exists, and encapsulate the
  hand-off in a single helper rather than spreading claim/release calls across
  call sites.
- Prefer moving the *less timing-critical* user of the resource to a different
  channel/timer — the hot path keeps its static configuration.
- When porting to a new target, remap resources in that target's `platform.h`
  rather than introducing `#ifdef` branches in shared driver code.

## Layer Overview

```
GDB host (PC)
    ↕ RSP over UART / USB-CDC
Firmware.shared/ui/gdb.h          ← GDB RSP state machine
Firmware.shared/drivers/TapMcu.h  ← unified TAP abstraction
    ↕
ITapDev (abstract)
  ├─ TapDev430    (legacy F1/F2/G/F4 — SLAU049/SLAU144)
  ├─ TapDev430X   (extended MSP430 — SLAU208)
  └─ TapDev430Xv2 (Xv2 with EEM — SLAU272/SLAU367)
    ↕
JtagDev (hardware JTAG driver — DTRIG only)
  └─ SPI1 (JTCK/JTDI/JTDO) + TIM1 PWM (TMS), phase-locked
     OPT_JTAG_IMPL_DTRIG — 9 MHz on F103, async shifts via JtagPending<T>
```

JTAG shifts are asynchronous: every `OnXxxShift()` returns a `JtagPending<T>`
(see `Firmware.shared/util/JtagPending.h`) that fires-and-returns. The next
shift's render overlaps with the previous frame's DMA. Implicit conversion to
`T` blocks; statement-only calls are fire-and-forget.

### bmt template library

`bmt/` is a C++20 template library providing zero-overhead peripheral abstractions for STM32: GPIO, clocks, USART, SPI, DMA, timers. All register addresses and bit masks are `constexpr`; the compiler eliminates all abstraction overhead. Templates are instantiated per-peripheral in `platform.h` for each target.

### Funclets

`Funclets/` contains small MSP430 assembly/C programs compiled as raw binaries and embedded as byte arrays in `Firmware.shared/res/`. They are uploaded to MSP430 RAM at runtime to perform flash erase and write operations — **the MSP430 must execute flash operations from RAM**, so the probe cannot do it directly over JTAG.

## Important Locations

| Path | Purpose |
|------|---------|
| `bmt/` | Bare Metal Templates — compile-time HAL for STM32 (submodule) |
| `Firmware.shared/` | Platform-independent firmware logic |
| `Firmware.shared/drivers/` | JtagDev (HW), TapDev430* (protocol), TapMcu (top-level) |
| `Firmware.shared/ui/gdb.h` | GDB RSP protocol implementation |
| `Firmware.shared/util/` | ChipProfile, Breakpoints, WaveJtag, WaveSet |
| `Firmware.shared/drivers/ChipInfoDB.h` | 370 KB pre-generated chip database (do not edit by hand) |
| `target.bluepill/platform.h` | Platform config for STM32F103 BluePill |
| `target.bluepill.g431kb/platform.h` | Platform config for STM32G431 (LQFP48, BluePill-G431 board) |
| `target.nucleo-l432kc/platform.h` | **OBSOLETE** — STM32L432 (LQFP32), removed from the solution; pin count too low |
| `target.stlinv2/platform.h` | Platform config for ST-Link V2 clone hardware |
| `Funclets/EraseXv2/` | MSP430 flash-erase routine (compiled as bare MSP430 binary) |
| `Funclets/WriteFlashXv2/` | MSP430 flash-write routine (compiled as bare MSP430 binary) |
| `ChipInfo/` | Toolchain to build the chip database from TI sources |

## Verification

- No lint or typecheck commands defined — check for compilation errors in Visual Studio
- Unit tests are C# based: `dotnet run --project UnitTest/UnitTest.csproj`
- Always verify chip database generation commands work before modifying database-related code

## Git Workflow

- **Never create a new branch** (`git branch`, `checkout -b`, `switch -c`, etc.) unless the user explicitly asks for one in that conversation. Switching between branches that already exist is fine. The user works from an SVN background and wants to opt into every new branch, not have one appear as a side effect of "cleaning up" a commit.
- If a commit ends up on the wrong branch, move it between **existing** branches (e.g. cherry-pick onto the target + reset the source branch) rather than proposing a new one.
- When asked to "show log", run `git log main` — show `main`'s history specifically, not whatever branch happens to be checked out.

## Documentation Rules

- **Knowledge base index**: [`.claude/docs/INDEX.md`](.claude/docs/INDEX.md) is the top-level map of all working notes — consult it before writing a new doc to find what already exists on the topic, and add a one-line entry to it whenever you create or rename a file.
- **AI-generated documentation**: All markdown files must be written under `./.claude/docs/<category>/` to keep them out of the repo proper. These are working notes / refinement targets for future development sessions, not deliverable docs.
- **Categories** (use the closest match; create a new one only if nothing fits — see `INDEX.md` for the current list):
  - `.claude/docs/bmt/` — bmt template library (peripheral reviews, design rationale, cross-port comparisons)
  - `.claude/docs/drivers/` — JTAG/SBW driver implementation notes (DtrigJtag, ST-Link V2, etc.)
  - `.claude/docs/msp430/` — MSP430 protocol topics (breakpoints, JTAG/SBW, EEM, flash workflows)
  - `.claude/docs/traceswo/` — TraceSWO / SWO bring-up and clone-board workarounds
- Example: a new "EEM trace buffer" note belongs at `.claude/docs/msp430/EEM_TRACE_BUFFER.md` and gets a line in `INDEX.md` under the `msp430/` section.
- **Exception**: `CODE_GUIDELINES.md` and `CLAUDE.md` (this file) live at the repo root.
- Keep the repo root clean of other documentation files.
- Treat existing docs under `.claude/docs/` as living drafts — many describe topics still to be refined or implemented; update them in place rather than starting parallel files on the same topic.

### Alpha stage: write comments and docs as current facts, not history

*(Remove this section once the project reaches its first release — the user will do this manually.)*

This project is still alpha/prototype. Historic and obsolete information ("this used
to be X", "was superseded by Y during the Z migration", "older revisions said...")
is usually just noise at this stage — it costs a reader time and rarely changes what
they should do next. Prefer writing code comments and `.claude/docs/` content as
**plain current-state facts**: what's true now and the rule/value that follows from
it, not the journey that produced it or what it replaced. When you touch a comment
or doc section that's carrying old/superseded context, feel free to strip it down to
the current fact rather than layering a new note on top.

Once there's a real "release" state, history regains value (changelog-worthy, audit
trail, etc.) — but that's a decision for the user to make and apply themselves.

## `supp/` folder scanning policy

`supp/` contains large third-party / reference code dumps used as research material,
not part of the build. **Do not scan or grep `supp/` subfolders by default** — they
will pollute search results and waste context.

- AI-generated docs now live under `.claude/docs/<category>/` (see "Documentation Rules" above), **not** under `supp/`. Older sessions wrote to `supp/docs-ai/`; that path no longer exists.
- All `supp/*` subfolders are off-limits unless the user explicitly points you
  at one (e.g. "look in `supp/ti-slau320/...`"). In that case, scope the search to
  the named path, not the whole `supp/` tree.
- When using Glob/Grep across the repo, exclude `supp/` (or restrict to specific
  paths) so reference dumps don't drown out project code.
- **slau320aj is shallow and introductory only.** It covers the basic JTAG protocol
  (read/write/erase memory, PSA verify) but does **not** deal with breakpoints, EEM,
  or any advanced debugging feature — don't treat its silence on a topic as "not
  supported" or extrapolate its patterns (e.g. "reset before verify") to scenarios
  it never addresses, like live breakpoint patching.
- **The authoritative reference is the official DLL + firmware** (the user calls this
  "uif") at `supp/MSPDebugStack_OS_Package_3_15_1_1/` — real, shipped TI code, not a
  teaching sample. Prefer it over slau320aj whenever the question involves anything
  beyond basic memory access: breakpoints (`DLL430_v3/src/TI/DLL430/EM/SoftwareBreakpoints/`),
  EEM/trigger internals, or other emulation-layer behavior.
