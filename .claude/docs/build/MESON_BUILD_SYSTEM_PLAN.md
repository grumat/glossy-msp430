# Meson Build System — Plan

## Scope

This plan describes a parallel CLI build system for the Glossy MSP430 project using **Meson** + **Ninja**. It does **not** replace VisualGDB — both coexist.

Windows is a **first-class build platform** — the CLI build must work on Windows (via the SysGCC toolchain already installed by VisualGDB, or a standalone ARM GCC), not just as an IDE fallback. Linux/macOS are secondary targets.

The Meson build covers:

- **ARM firmware targets** (bluepill, g431kb, stlinv2) — ARM GCC 15.2.1 → `.elf` + `.bin` + `.hex`
- **MSP430 funclets** (EraseXv2, WriteFlashXv2) — MSP430 GCC 9.3.1 → `.elf` + `.bin`
- **Embedded binary embedding** — objcopy pipeline to link funclet `.bin` files into ARM firmware as `.rodata.funclet`
- **C#/.NET tools** — thin wrapper delegating to `dotnet build`
- **C++/MFC host tool** (ExtractChipInfo) — thin wrapper delegating to MSBuild (Windows only)

## New directory tree

```
glossy-msp430/
│
├── meson.build                            # Top-level entry point
├── meson.options.txt                      # Build options (target selection, paths)
├── meson-
│
├── cross/
│   ├── arm-none-eabi.ini                  # Cross file: ARM GCC toolchain
│   └── msp430-elf.ini                     # Cross file: MSP430 GCC toolchain
│
├── scripts/
│   ├── build.ps1                          # PowerShell wrapper: one-command build
│   └── build.sh                           # Bash wrapper (Linux/macOS)
│
├── cmsis/                                 # Vendored CMSIS headers (see § below)
│   ├── cmsis_version.h
│   ├── cmsis_compiler.h
│   ├── cmsis_gcc.h
│   ├── cmsis_armcc.h
│   ├── core_cm3.h
│   ├── core_cm4.h
│   ├── cmsis_armclang.h
│   ├── mpu_armv7.h
│   ├── stm32f103xb.h                      # STM32F1 device header (STM32F103CB derivative)
│   ├── system_stm32f1xx.h
│   ├── stm32g431xx.h                      # STM32G4 device header (STM32G431 derivative)
│   ├── system_stm32g4xx.h
│   └── startup/
│       ├── startup_stm32f103xb.c          # F1 startup file (from SysGCC BSP / ST CubeF1)
│       └── startup_stm32g431kb.c          # G4 startup file (from SysGCC BSP / ST CubeG4)
│
├── target.bluepill/
│   └── meson.build                        # Bluepill arm-none-eabi executable target
│
├── target.bluepill.g431kb/
│   └── meson.build                        # G431KB arm-none-eabi executable target
│
├── target.stlinv2/
│   └── meson.build                        # ST-Link V2 arm-none-eabi executable target
│
├── Funclets/
│   └── meson.build                        # Parent: dispatches to EraseXv2 + WriteFlashXv2
│
├── Funclets/EraseXv2/
│   └── meson.build                        # MSP430 elf + bin target
│
├── Funclets/WriteFlashXv2/
│   └── meson.build                        # MSP430 elf + bin target
│
├── Funclets/Interface/
│   └── MkAsm.bat                          # (unchanged — post-link disassembly via objdump)
│
├── Firmware.shared/
│   └── meson.build                        # Declares firmware_sources variable for targets to consume
│
├── ChipInfo/
│   └── meson.build                        # Parent: dotnet build + MSBuild wrappers
│
└── supp/                                  # (unchanged — off-limits per CLAUDE.md)
```

## File-by-file description

### Top-level files

| File | Purpose |
|---|---|
| `meson.build` | Root entry point. Reads `-Dtarget=` option. `subdir()` into the selected target directory. Also defines `run_target()` wrappers for C#/MFC projects. |
| `meson.options.txt` | Declares build options: `target` (choice: bluepill / g431kb / stlinv2 / funclets / all), `bsp_root` (optional, fallback for SysGCC BSP path), `cmsis_root` (points to `cmsis/` by default). |

### Cross files (`cross/`)

Cross files resolve toolchain binaries via `PATH` (looked up at `meson setup` time), which works on all platforms. On Windows the paths are set by SysGCC installer or added manually; on Linux/macOS they come from package managers.

| File | Purpose |
|---|---|
| `arm-none-eabi.ini` | Sets `c`/`cpp`/`ar`/`strip`/`objcopy` binaries, `cpu` flags (`-mcpu=cortex-m3` or `-mcpu=cortex-m4` via option), standard selection (`c_std = gnu11`, `cpp_std = c++20`), `--specs=nano.specs --specs=nosys.specs`. |
| `msp430-elf.ini` | Sets `c`/`cpp` binaries, target MCU flags (`-mlarge -msilicon-errata=cpu11,cpu12,cpu13,cpu19`), `cpp_std = c++11`, position-independent flags (`-fPIE`). |

### Wrapper scripts (`scripts/`)

Both scripts are **peer front-ends** — no hierarchy, no "primary" platform.

| File | Purpose |
|---|---|
| `build.ps1` | PowerShell front-end (Windows). Syntax: `.\scripts\build.ps1 -Target bluepill -Config debug`. Calls `meson setup` (if missing) then `meson compile`. Detects Visual Studio environment for MSBuild (MFC tool). Resolves ARM GCC from SysGCC path (`C:\SysGCC\arm-eabi\bin`) or PATH. |
| `build.sh` | Bash front-end (Linux/macOS). Same workflow. Skips MSBuild/MFC. Resolves ARM GCC from PATH (`arm-none-eabi-g++` via apt/brew). |

### Vendored CMSIS headers (`cmsis/`)

Extracted from STMicroelectronics/STM32CubeF1 and STM32CubeG4 and SysGCC BSP. Exact set:

- **Core CMSIS** (from ARM CMSIS_5): `cmsis_version.h`, `cmsis_compiler.h`, `cmsis_gcc.h`, `cmsis_armcc.h`, `core_cm3.h`, `core_cm4.h`, `cmsis_armclang.h`, `mpu_armv7.h`
- **STM32F1 device**: `stm32f103xb.h` (register map, peripheral definitions), `system_stm32f1xx.h`
- **STM32G4 device**: `stm32g431xx.h`, `system_stm32g4xx.h`
- **Startup files**: `startup_stm32f103xb.c` (F1), `startup_stm32g431kb.c` (G4)

All files are small (< 100 KB each), stable per MCU family, and redistributable under Apache 2.0 / BSD.

### meson.build files (per target)

| File | Responsibility |
|---|---|
| `Firmware.shared/meson.build` | Declares a `firmware_sources` list variable (all `.cpp` files) + `firmware_headers` list. Subprojects consume these via `subproject('firmware-shared').get_variable('firmware_sources')`. |
| `target.*/meson.build` | Defines one `executable()` target per firmware binary. Pulls sources from `Firmware.shared`, sets target-specific include dirs (including `platform.h` path), preprocessor defines, linker script, link flags. Adds a `custom_target()` for `objcopy` → `.bin` / `.hex` output. Adds a `custom_target()` for `objdump` → `.lst` disassembly. |
| `Funclets/meson.build` | Parent target that `subdir()` into each funclet. |
| `Funclets/*/meson.build` | Defines one `executable()` per funclet with MSP430 flags, then a `custom_target()` for `objcopy` → `.bin`. Output `bin` files are placed where `Firmware.shared/res/` expects them. |
| `ChipInfo/meson.build` | Defines `run_target('ImportDB', command: ['dotnet', 'build', ...])` and `run_target('ExtractChipInfo', command: ['msbuild', ...])` wrappers. |

## Pipeline: funclet binary embedding

This replicates what VisualGDB's `<EmbeddedBinaryFile>` + `TargetSectionName` does:

1. **Build funclets** → produces `EraseXv2.elf`, `WriteFlashXv2.elf`
2. **objcopy** → `EraseXv2.bin`, `WriteFlashXv2.bin` → output to `Firmware.shared/res/`
3. **Generate C source** from `.bin` (via `xxd -i` or a Python script) → produces `EmbeddedFunclets.c` with `__attribute__((section(".rodata.funclet")))`
4. **Compile `EmbeddedFunclets.c`** as part of each ARM firmware target
5. **Link** → the funclet data lands in `.rodata.funclet` section of the firmware `.elf`

The generated `.c` file replaces the existing `EmbeddedResources.h` byte-array declarations and is gitignored (regenerated from the committed `.bin` sources on each build).

## Build workflow

```powershell
# One-command wrapper
.\scripts\build.ps1 -Target bluepill -Config debug

# Manual Meson workflow (equivalent)
meson setup build\bluepill-debug --cross-file cross\arm-none-eabi.ini `
    -Dtarget=bluepill -Dbuildtype=debug
meson compile -C build\bluepill-debug

# Build all firmware targets
.\scripts\build.ps1 -Target all

# Build only funclets
meson setup build\funclets-release --cross-file cross\msp430-elf.ini `
    -Dtarget=funclets -Dbuildtype=release
meson compile -C build\funclets-release

# C# tools (no cross file needed)
meson setup build\tools -Dbuildtype=release
meson compile -C build\tools           # invokes dotnet build via run_target

# MFC tool (Windows only, requires VS developer prompt)
meson compile -C build\tools ExtractChipInfo
```

Build outputs go to `build/<target>-<config>/` (parallel to existing `VisualGDB/Debug/`).

## What stays unchanged

The following files are **not** touched — Meson reads them but does not modify:

- All `.vcxproj`, `.vcxitems`, `.sln` files (VisualGDB continues to work)
- `Funclets/Interface/Interface.h` and `Funclets/Interface/MkAsm.bat`
- `target.*/platform.h` per-target configuration
- All linker scripts (`*_flash.lds`)
- `Firmware.shared/res/*` (pre-committed funclets — Meson regenerates them, they remain as fallback)

## Future considerations (not in initial scope)

- **Docker/devcontainer** with ARM GCC + MSP430 GCC pre-installed for CI
- **GitHub Actions** workflow: `meson setup --cross-file ...` → `meson compile` → archive `.elf`/`.bin`/`.hex`
- **VS Code tasks.json** integration for `Ctrl+Shift+B` using the Meson build directory
