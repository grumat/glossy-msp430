---
name: sync-build-files
description: >-
  Add, remove, or rename a firmware source/header/resource file in glossy-msp430
  while keeping BOTH build systems (Visual Studio/VisualGDB .vcxproj/.vcxitems AND
  the Meson CLI meson.build) in sync. Use whenever you create, delete, or rename a
  .cpp/.h/.c/.bin under Firmware.shared/, target.*/, or Funclets/. Covers the
  shared-vs-target scope rules, case-only renames, CRLF/BOM preservation, the
  drift-check script, and the headless verification build.
---

# Syncing build files across VisualGDB + Meson

This project tracks source files in **two** build systems. Every file add / remove /
rename must update both or the IDE and CLI builds compile different binaries.

- **VisualGDB / VS2022** — `*.vcxproj` (+ `.filters`) per target, and the shared
  `Firmware.shared/Firmware.shared.vcxitems` (+ `.vcxitems.filters`).
- **Meson CLI** — `meson.build` files (top-level dispatch, `Firmware.shared/meson.build`,
  one per `target.*/`, and under `Funclets/` and `ChipInfo/`).

`BUILD.md` (repo root) has the user-facing reference and the build commands. **Note:
BUILD.md's table is slightly wrong** — it says a shared `.cpp` is a `ClCompile` in
"3 target vcxprojs." It is not. Shared sources live in **one** place, the
`Firmware.shared.vcxitems` shared-items project, which all three ARM targets absorb
via the `.sln` (`{8BC9CEB8-…}` project GUID). Trust this skill over that row.

## Step 1 — classify the file by location

| Location | VS side | Meson side |
|----------|---------|------------|
| **`Firmware.shared/**`** (shared by all 3 ARM targets) | `Firmware.shared/Firmware.shared.vcxitems` **+** `.vcxitems.filters` | `Firmware.shared/meson.build` → `firmware_sources` |
| **`target.<t>/**`** (one target only, e.g. startup, platform glue) | `target.<t>/target.<t>.vcxproj` **+** `.filters` | `target.<t>/meson.build` (appended to `sources`, e.g. `startup`) |
| **`Funclets/<name>/**`** (MSP430) | `Funclets/<name>/<name>.vcxproj` **+** `.filters` | `Funclets/<name>/meson.build` |
| **`ChipInfo/**`** (host tools) | the tool's `.vcxproj`/`.csproj` | `ChipInfo/meson.build` |

Most firmware work is the **first row** (shared). Get the scope right first — putting
a shared file into a single target's `.vcxproj` is the most common mistake.

## Step 2 — apply by file kind

### A `.cpp` (compiled translation unit)
Update **both** sides for the scope from Step 1.

- **Shared `.cpp`** → touch exactly three files:
  1. `Firmware.shared.vcxitems`: a `ClCompile` line using the
     `$(MSBuildThisFileDirectory)` prefix and **backslashes**, e.g.
     `<ClCompile Include="$(MSBuildThisFileDirectory)util\NewThing.cpp" />`
  2. `Firmware.shared.vcxitems.filters`: matching `<ClCompile>` with its `<Filter>`
     (e.g. `util`) so it lands in the right IDE folder.
  3. `Firmware.shared/meson.build`: add `'util/NewThing.cpp',` (forward slashes,
     relative to `Firmware.shared/`) inside `firmware_sources = files( … )`. Keep it
     alphabetical-ish / grouped with its folder, matching existing order.
- **Target `.cpp`** → that target's `.vcxproj` + `.filters`, and append the path to
  `sources` in `target.<t>/meson.build`.

### B `.h` (header)
- VS side **only**: add a `<ClInclude>` to the `.vcxitems`/`.vcxproj` **and** its
  `.filters`. (Headers are for IDE visibility/IntelliSense.)
- Meson side: **nothing** — headers are auto-discovered via `include_directories()`.

### C `.bin` (embedded resource, `Firmware.shared/res/*.bin`)
- VS side: `<EmbeddedBinaryFile Include="…\New.bin" />` (display + VisualGDB embeds it).
- Meson side: a `custom_target(...)` that runs `tools/objcopy_binary.py` to turn the
  `.bin` into a `.o`, then add that `_o` object to the `executable(...)` source list in
  **each** `target.*/meson.build` that needs it. Copy an existing `erase_o` / `write_o`
  block and rename the symbol (`___Firmware_shared_res_<Name>.bin`).

## Step 3 — renames (and case-only renames)

The class-per-file rule means renames are common (e.g. `parser.cpp` → `Parser.cpp`).

- Move the file with **`git mv -f old new`**. The `-f` matters for **case-only**
  renames on Windows (NTFS is case-insensitive; without `-f` git no-ops).
- Then do a **find/replace of the path string** across: the `.vcxitems`/`.vcxproj`,
  the `.filters`, the `meson.build`, the file's own self-include, and every other
  source that `#include`s it (grep for the old name).
- **Preserve encoding when editing the XML project files.** They are UTF-8 *with BOM*
  and CRLF. If you edit them with PowerShell, read/write with the BOM and CRLF intact,
  and remember PowerShell `-replace`/`-eq` are **case-insensitive** — use the
  case-sensitive `-creplace` / `-cne` for case-only renames or you'll "change nothing."
  The Edit tool preserves bytes and is the safer default for these files.

## Step 4 — verify (headless, no VS2022 needed)

The ARM toolchain (SysGCC 15.2) is at `C:\SysGCC\arm-eabi\bin` but **not on PATH** —
prepend it. After **add/remove** of a file you must `--wipe` (Meson caches the source
list); after a pure edit a plain `meson compile` suffices.

```bash
# 1. cheap structural check first:
python scripts/check_build_sync.py        # exits 1 + a diff if vcxitems vs meson drift

# 2. then a real compile of all three ARM targets:
export PATH="$PATH:/c/SysGCC/arm-eabi/bin"
for t in bluepill:bluepill g431kb:g431kb stlinv2:stlinv2; do
  name="${t%%:*}"; tgt="${t##*:}"
  meson setup --wipe build/$name-debug --cross-file cross/arm-none-eabi.ini \
      -Dtarget=$tgt -Dbuildtype=debug && meson compile -C build/$name-debug
done
```

A clean build with no `1014`/`1015` (VS) or unresolved-symbol (Meson) errors confirms
both lists are consistent. `-Wmissing-field-initializers` warnings from
`TapPlayer.h` are pre-existing noise, not your change.

The `check_build_sync.py` script only validates the **shared** list (the highest-drift
surface). Target-specific and resource entries still need a manual eyeball against the
table in Step 1.

## Done-checklist

- [ ] Correct scope (shared `.vcxitems` vs a single `target.*.vcxproj`).
- [ ] `.filters` updated alongside every `.vcxitems`/`.vcxproj` change.
- [ ] meson.build updated (`.cpp` only; headers skip meson).
- [ ] `git mv -f` used for case-only renames; all `#include`s repointed.
- [ ] `python scripts/check_build_sync.py` → OK.
- [ ] `meson setup --wipe && meson compile` clean for all three ARM targets.
