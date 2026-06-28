#!/usr/bin/env python3
"""Check that the shared firmware source list is in sync between the two build
systems:

    Firmware.shared/Firmware.shared.vcxitems   (Visual Studio / VisualGDB)
    Firmware.shared/meson.build                (Meson CLI)

Both must list the exact same set of .cpp files, or one build system will compile
a different binary than the other. Run after adding/removing/renaming any shared
source file (see the .claude/skills/sync-build-files skill).

Exit code 0 = in sync, 1 = drift detected.

Usage:  python scripts/check_build_sync.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VCXITEMS = ROOT / "Firmware.shared" / "Firmware.shared.vcxitems"
MESON = ROOT / "Firmware.shared" / "meson.build"


def vcxitems_cpp(path: Path) -> set[str]:
    """Extract shared .cpp paths from a .vcxitems, normalized to forward slashes
    relative to Firmware.shared/ (drops the $(MSBuildThisFileDirectory) prefix)."""
    text = path.read_text(encoding="utf-8-sig")
    out = set()
    for m in re.finditer(r'ClCompile\s+Include="([^"]+\.cpp)"', text, re.IGNORECASE):
        inc = m.group(1).replace("$(MSBuildThisFileDirectory)", "")
        out.add(inc.replace("\\", "/").lstrip("/"))
    return out


def meson_cpp(path: Path) -> set[str]:
    """Extract the files() list from `firmware_sources = files( ... )`."""
    text = path.read_text(encoding="utf-8-sig")
    m = re.search(r"firmware_sources\s*=\s*files\((.*?)\)", text, re.DOTALL)
    if not m:
        print(f"ERROR: could not find `firmware_sources = files(...)` in {path}")
        sys.exit(2)
    return set(re.findall(r"'([^']+\.cpp)'", m.group(1)))


def main() -> int:
    vcx = vcxitems_cpp(VCXITEMS)
    mes = meson_cpp(MESON)

    only_vcx = sorted(vcx - mes)
    only_mes = sorted(mes - vcx)

    if not only_vcx and not only_mes:
        print(f"OK: shared source lists in sync ({len(vcx)} .cpp files).")
        return 0

    print("DRIFT: Firmware.shared.vcxitems and Firmware.shared/meson.build disagree.\n")
    if only_vcx:
        print("  In .vcxitems but NOT in meson.build (add to firmware_sources):")
        for f in only_vcx:
            print(f"    + {f}")
    if only_mes:
        print("  In meson.build but NOT in .vcxitems (add ClCompile, or remove from meson):")
        for f in only_mes:
            print(f"    - {f}")
    print("\nAlso update Firmware.shared.vcxitems.filters to match any .vcxitems change.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
