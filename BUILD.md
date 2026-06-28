# Building Glossy MSP430 Firmware

## Build Options Overview

| Method | ARM Firmware | MSP430 Funclets | C# Tools | C++/MFC Tool | Platform |
|--------|-------------|-----------------|----------|--------------|----------|
| **Meson (recommended)** | ✅ | ✅ | ✅ | ✅ | Win/Linux/macOS |
| **VisualGDB (VS2022)** | ✅ | ✅ | ❌ | ✅ | Windows only |

---

## Meson Build (All Platforms)

### Prerequisites

```bash
pip install meson ninja
```

**Windows (SysGCC toolchain):**
- Install [SysGCC ARM](https://sysgcc.com/) to `C:\SysGCC\arm-eabi\bin\` (provides `arm-none-eabi-g++`)
- For MSP430 funclets: install [MSP430 GCC](https://www.ti.com/tool/MSP430-GCC-OPENSOURCE) and add `msp430-elf-g++` to PATH
- .NET 9 SDK (for C# tools): `winget install Microsoft.DotNet.SDK.9`
- MSBuild ships with Visual Studio 2022 (for ExtractChipInfo)

**Linux (apt):**
```bash
sudo apt install gcc-arm-none-eabi
# MSP430: install from TI or build msp430-elf-gcc from source
```

**macOS (Homebrew):**
```bash
brew install arm-none-eabi-gcc
# MSP430: typically not available — will skip funclets
```

### Quick Start

```bash
# Debug build for Blue Pill (STM32F103CB)
meson setup build --cross-file cross/arm-none-eabi.ini -Dtarget=bluepill -Dbuildtype=debug
meson compile -C build
```

### Available Targets

Pass `-Dtarget=<name>` to `meson setup`:

| Target | What It Builds |
|--------|---------------|
| `bluepill` | ARM firmware for STM32F103CB (Blue Pill) |
| `g431kb` | ARM firmware for STM32G431KB (Nucleo-32) |
| `stlinv2` | ARM firmware for STM32F103CB (ST-Link/V2) |
| `funclets` | MSP430 funclets (EraseXv2, WriteFlashXv2) |
| `tools` | Host tools (C# + C++/MFC) |
| `all` | All of the above |

### Build Configurations

| Config | Flag |
|--------|------|
| Debug | `-Dbuildtype=debug` |
| Release | `-Dbuildtype=release` |

### Complete Examples

```bash
# Blue Pill, Debug
meson setup build/bluepill-debug --cross-file cross/arm-none-eabi.ini -Dtarget=bluepill -Dbuildtype=debug
meson compile -C build/bluepill-debug

# G431KB, Release
meson setup build/g431kb-release --cross-file cross/arm-none-eabi.ini -Dtarget=g431kb -Dbuildtype=release
meson compile -C build/g431kb-release

# Funclets (requires msp430-elf-g++)
meson setup build/funclets-debug --cross-file cross/msp430-elf.ini -Dtarget=funclets -Dbuildtype=debug
meson compile -C build/funclets-debug

# Tools only (no cross file — native build)
meson setup build/tools -Dtarget=tools -Dbuildtype=release
meson compile -C build/tools

# Individual tool targets
meson compile -C build/tools ImportDB
meson compile -C build/tools MkChipInfoDbV2
meson compile -C build/tools ScrapeDataSheet
meson compile -C build/tools UnitTest           # runs tests (needs COM3 hardware)
meson compile -C build/tools ExtractChipInfo     # MSBuild, Windows only
```

**To rebuild after changing sources**, just re-run `meson compile`. Meson automatically detects changes. If you add new source files (see below), run `meson setup --wipe` to regenerate the build system.

### Output Artifacts

Each ARM target produces in its build subdirectory:
- `target.<name>.elf` — ELF executable
- `target.<name>.bin` — raw binary (for flashing)
- `target.<name>.hex` — Intel HEX
- `target.<name>.lst` — disassembly listing

Each funclet produces:
- `EraseXv2.elf` / `WriteFlashXv2.elf` — ELF executables
- `EraseXv2.bin` / `WriteFlashXv2.bin` — raw binaries
- `EraseXv2.ASM` / `WriteFlashXv2.ASM` — disassembly

### Wrapper Scripts

```bash
# Windows (PowerShell 7+)
.\scripts\build.ps1 -Target bluepill -Config debug

# Linux/macOS
./scripts/build.sh bluepill debug

# Build everything
.\scripts\build.ps1 -Target all
```

---

## Visual Studio 2022 + VisualGDB (Windows Only)

### Prerequisites

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload
2. Install [VisualGDB](https://visualgdb.com/) extension — provides the ARM/MSP430 cross-toolchain integration
3. Install [SysGCC ARM EABI](https://sysgcc.com/) — ARM toolchain (or let VisualGDB manage it)
4. Open `glossy-msp430.sln`

### Debug/Release Configurations

Each target in the solution has two configurations:

| Solution Configuration | Build Type |
|----------------------|------------|
| `Debug` | Debug symbols, no optimization |
| `Release` | Optimized, no debug asserts |

Each target maps to a VisualGDB project:
- `target.bluepill` → `target.bluepill.vcxproj` (STM32F103CB)
- `target.bluepill.g431kb` → `target.bluepill.g431kb.vcxproj` (STM32G431KB)
- `target.stlinv2` → `target.stlinv2.vcxproj` (STM32F103CB)
- `EraseXv2` → `Funclets/EraseXv2/EraseXv2.vcxproj` (MSP430)
- `WriteFlashXv2` → `Funclets/WriteFlashXv2/WriteFlashXv2.vcxproj` (MSP430)
- `ExtractChipInfo` → `ChipInfo/ExtractChipInfo/ExtractChipInfo.vcxproj` (Win32 MFC)

C# projects (ImportDB, MkChipInfoDbV2, ScrapeDataSheet, UnitTest) are not part of the VisualGDB solution; build them with `dotnet build` or via Meson (`meson compile -C build/tools ImportDB`).

### Debugging (VisualGDB)

1. Set the target project as **Startup Project**
2. Select the correct configuration & platform (e.g., `Debug | ARM`)
3. Press **F5** — VisualGDB handles flashing and debugging via ST-Link, J-Link, etc.
4. VGDB settings per target are stored in `*.vgdbsettings` files

---

## Keeping vcxproj and Meson Files in Sync

### Overview

Source files are tracked in two places:
- **`.vcxproj` files** — used by Visual Studio / VisualGDB
- **`meson.build` files** — used by the Meson build system

When you add or remove a `.cpp` file, you must update **both**. Headers (`.h`) only need updating in the `.vcxproj`; Meson discovers headers automatically via `include_directories()`.

### Source File Map

```
Shared ARM firmware sources (all 3 ARM targets):
  Firmware.shared/meson.build       ←→  each target.*/target.*.vcxproj

ARM target bluepill:
  target.bluepill/meson.build       ←→  target.bluepill/target.bluepill.vcxproj

ARM target g431kb:
  target.bluepill.g431kb/meson.build  ←→  target.bluepill.g431kb/target.bluepill.g431kb.vcxproj

ARM target stlinv2:
  target.stlinv2/meson.build        ←→  target.stlinv2/target.stlinv2.vcxproj

MSP430 funclets:
  Funclets/EraseXv2/meson.build     ←→  Funclets/EraseXv2/EraseXv2.vcxproj
  Funclets/WriteFlashXv2/meson.build  ←→  Funclets/WriteFlashXv2/WriteFlashXv2.vcxproj
```

### Adding a Source File

**In the `.vcxproj`** (all 3 ARM targets if shared, or individual target):

```xml
<ClCompile Include="..\Firmware.shared\util\NewFile.cpp" />
```
Edit the `.vcxproj` file in a text editor, or use **Visual Studio → Add → Existing Item** (right-click the project in Solution Explorer). VisualGDB projects opened through Visual Studio also support this.

**In the `meson.build`** (same scope — shared or per-target):

```meson
# Firmware.shared/meson.build
firmware_sources = files(
    ...
    'util/NewFile.cpp',
)
```

```meson
# For a target-specific file (not shared):
# target.bluepill/meson.build
sources = firmware_sources + [startup, '../target.bluepill/NewFile.cpp']
```

### Removing a Source File

1. Delete or comment the `ClCompile` line in each affected `.vcxproj`
2. Remove the `'path/file.cpp'` entry from the corresponding `meson.build`

### Headers (`.h`) and Other Non-Compiled Files

Headers need **vcxproj only**:

```xml
<ClInclude Include="..\Firmware.shared\util\NewHeader.h" />
```

Meson finds `.h` files automatically through the `include_directories()` call — no meson.build change needed for headers.

Resource files (.bin) used as embedded binaries:

```xml
<!-- vcxproj: for IDE display only -->
<EmbeddedBinaryFile Include="..\Firmware.shared\res\NewResource.bin" />
```

```meson
# meson.build: must add a custom_target to convert .bin → .o
new_resource_o = custom_target('newresource.bin.o',
    input: project_src / 'Firmware.shared/res/NewResource.bin',
    output: '____Firmware_shared_res_NewResource.bin.o',
    command: [python, objcopy_script, '@INPUT@', '@OUTPUT@', '___Firmware_shared_res_NewResource.bin'],
    build_by_default: true,
)
target_elf = executable(..., new_resource_o, ...)
```

### Verification Checklist

After adding/removing a file:

1. **VisualGDB**: Open the solution → Build → Confirm no 1014/1015 errors
2. **Meson**: `meson setup --wipe build/test --cross-file cross/arm-none-eabi.ini -Dtarget=bluepill -Dbuildtype=debug && meson compile -C build/test`
3. If only headers changed, the Meson rebuild is optional — Meson watches headers via dependency scanning

### Quick Reference: Meson ↔ vcxproj

| Item | vcxproj element | meson.build |
|------|----------------|-------------|
| Shared `.cpp` | `ClCompile` in 3 target vcxprojs | `files(...)` in `Firmware.shared/meson.build` |
| Startup `.c` | `ClCompile` per target | Added to `sources` per `target.*/meson.build` |
| Headers `.h` | `ClInclude` | Auto-discovered via `include_directories()` |
| Embedded `.bin` | `EmbeddedBinaryFile` (display only) | `custom_target()` + `objcopy_binary.py` |
| Linker script `.lds` | `LinkerScript` in vcxproj props | `-Wl,-Tpath/to/file.lds` in link_args |
| Preprocessor defines | `PreprocessorDefinitions` in vcxproj props | `c_args` / `cpp_args` in `executable()` |
| Include paths | `AdditionalIncludeDirectories` in vcxproj props | `include_directories()` |
| Compiler flags | `AdditionalOptions` in vcxproj props | `c_args` / `cpp_args` in `executable()` |
| Linker flags | `AdditionalOptions` / `LinkerScript` in vcxproj props | `link_args` in `executable()` |
