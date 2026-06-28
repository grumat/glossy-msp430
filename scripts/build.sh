#!/usr/bin/env bash
# Build Glossy MSP430 firmware using Meson (Linux/macOS).
#
# Usage:
#   ./scripts/build.sh [target] [config]
#
#   target  - bluepill, g431kb, stlinv2, funclets, tools, all (default: bluepill)
#   config  - debug, release (default: debug)
#
# Requires: meson (pip install meson ninja)
#           arm-none-eabi-g++ (apt install gcc-arm-none-eabi / brew install arm-none-eabi-gcc)
#           msp430-elf-g++    (for funclets)

set -euo pipefail

TARGET="${1:-bluepill}"
CONFIG="${2:-debug}"
ROOTDIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILDDIR="$ROOTDIR/build/$TARGET-$CONFIG"

resolve_crossfile() {
    case "$1" in
        bluepill|g431kb|stlinv2) echo "$ROOTDIR/cross/arm-none-eabi.ini" ;;
        funclets)                echo "$ROOTDIR/cross/msp430-elf.ini" ;;
        tools)                   echo "" ;;
        *)                       echo "" ;;
    esac
}

meson_setup() {
    local dir="$1" cross="$2" target="$3" config="$4"
    local args=("setup" "$dir" "--wipe")
    if [ -n "$cross" ]; then
        args+=("--cross-file" "$cross")
    fi
    args+=("-Dtarget=$target" "-Dbuildtype=$config" "-Dwarning_level=3")
    meson "${args[@]}"
}

meson_compile() {
    local dir="$1"
    meson compile -C "$dir"
}

# Check for meson
if ! command -v meson &>/dev/null; then
    echo "Error: meson not found. Install it: pip install meson ninja"
    exit 1
fi

CROSSFILE=$(resolve_crossfile "$TARGET")
NEED_SETUP=false
if [ ! -f "$BUILDDIR/meson-info/intro-buildoptions.json" ]; then
    NEED_SETUP=true
fi

if [ "$TARGET" = "all" ]; then
    for t in bluepill g431kb stlinv2 funclets; do
        echo "=== Building $t ($CONFIG) ==="
        d="$ROOTDIR/build/$t-$CONFIG"
        cf=$(resolve_crossfile "$t")
        if [ ! -f "$d/meson-info/intro-buildoptions.json" ]; then
            meson_setup "$d" "$cf" "$t" "$CONFIG"
        fi
        meson_compile "$d"
    done
    # Build tools (no cross file)
    td="$ROOTDIR/build/tools-release"
    if [ ! -f "$td/meson-info/intro-buildoptions.json" ]; then
        meson_setup "$td" "" "tools" "release"
    fi
    meson_compile "$td"
else
    if [ "$NEED_SETUP" = true ]; then
        meson_setup "$BUILDDIR" "$CROSSFILE" "$TARGET" "$CONFIG"
    fi
    meson_compile "$BUILDDIR"
fi

echo "=== Build complete: $TARGET ($CONFIG) ==="
