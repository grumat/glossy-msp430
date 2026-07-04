---
name: run-unittest
description: >-
  Use the C# UnitTest console tool (UnitTest/UnitTest.csproj) to exercise the
  glossy-msp430 GDB RSP firmware against real MSP430 hardware over a serial
  port. Use whenever asked to "test the firmware", "try it on hardware", "run
  the unittest tool", or to validate a firmware change end-to-end on a
  physical probe. Covers the COM-port map per probe board, the full build
  (meson) -> flash (arm-none-eabi-gdb + UnitTest/batch scripts) -> TRACESWO
  workflow, safe vs. destructive test numbers, the chip-identity prerequisite
  gate, and how to send one-off raw `monitor` commands without extending the
  C# tool.
---

# Running the UnitTest GDB RSP tool against real hardware

`UnitTest/UnitTest.csproj` is a C# console app that speaks GDB Remote Serial
Protocol to a **glossy-msp430** probe over a serial port, the same protocol a
real GDB client would use. It's how firmware changes get validated against an
actual MSP430 target instead of just compiling.

## Toolchain paths (already known — don't ask, use these)

- **ARM toolchain**: `C:\SysGCC\arm-eabi\bin\` (`arm-none-eabi-gdb.exe`, `arm-none-eabi-g++.exe`, etc.) — used both for the meson cross build and for flashing.
- **TI MSP430 toolchain**: `c:\ti\msp430-gcc\bin\` (`msp430-gdbproxy.exe`, `gdb_agent_console.exe`) — TI's own reference GDB bridges, useful as an alternate/reference target distinct from glossy-msp430 itself (see `UnitTest/batch/run-msp430-gdbproxy*.bat`, `run-gdb-agent-console.bat`). Not needed for the glossy-msp430 workflow itself.

If some *other* path turns out to be needed and isn't one of these two, ask —
but these two are confirmed and stable across sessions.

## Before anything: which port is which

Each physical probe board exposes **two** COM ports that do completely
different jobs. Don't confuse them.

| Board | ARM upload port (flash the probe's own STM32/Geehy firmware) | MSP430 GDB RSP port (UnitTest talks here) |
|---|---|---|
| STLinkV2 | **COM3** | **COM4** |
| BluePill | **COM5** | **COM6** |

- The ARM port is a Black Magic Probe-style ARM GDB server, used to flash new
  glossy-msp430 firmware onto the probe's STM32/Geehy MCU. **UnitTest never
  talks to this port.**
- The MSP430 port is the firmware-under-test's own GDB RSP server — this is
  what implements `monitor jtag_scan`, `g`/`G` register packets, `m`/`M`
  memory packets, etc., and what UnitTest connects to.

Each board has its **own** flash script: `UnitTest/batch/flash.stlinkv2.txt`
(COM3) and `UnitTest/batch/flash.bluepill.txt` (COM5) — matching the port
table above exactly. Don't merge them back into one shared file.

## Start TRACESWO before anything else, every session

The Black Magic Probe side of the probe **freezes** if its TRACESWO buffer
fills up and isn't drained — this is a hard prerequisite for the probe
staying responsive, not just a diagnostics nicety. Two independent things
need to be true for TRACESWO to actually work end-to-end:

1. **Probe-side arm**: `mon traceswo enable <baud>` (a GDB monitor command)
   turns on the ITM/TPIU stream on the probe itself. Both `flash.stlinkv2.txt`
   and `flash.bluepill.txt` already do this as part of every flash
   (`mon traceswo enable 1125000`) — so flashing through either script arms
   it automatically.
2. **PC-side listener**: `BmpTrace2Win.exe` is the separate process that
   actually reads and decodes the resulting SWO byte stream and writes
   `%TEMP%\BmpTrace2Win.log`. Arming SWO on the probe without a listener
   draining it doesn't prevent the freeze — **both halves need to be
   running.**

If a UnitTest run or raw serial command hangs/times out unexpectedly, check
both of these before assuming it's a firmware bug.

## The full workflow: build -> flash -> test

You (the agent) can now do this **entirely without the VisualGDB IDE**,
using meson to build and the `UnitTest/batch/run-glossy-flash-*.bat` scripts
to flash. This is a real capability, not just a compile-check — confirm the
result with the user if the stakes are high, but you don't need to ask them
to flash manually every time. In the IDE this is just pressing F5 on the
selected target/config — the scripts below are the headless equivalent, kept
in sync with meson's output paths (not VisualGDB's).

### 1. Build with meson

Four canonical build directories exist, one per board x config, matching the
`.bat` script names exactly:

```bash
export PATH="$PATH:/c/SysGCC/arm-eabi/bin"
meson compile -C build-bluepill-debug     # or: build-bluepill-release
meson compile -C build-stlinv2-debug      # or: build-stlinv2-release
```

(First-time setup, only if a build dir doesn't exist yet:
`meson setup build-<board>-<config> --cross-file cross/arm-none-eabi.ini -Dtarget=<board> --buildtype=<debug|release>`.)

- `--buildtype=debug` → firmware gets `-DDEBUG=1` (verbose SWO `Dbg` channel
  compiled in). `--buildtype=release` → `-DNDEBUG=1 -DRELEASE=1` (much
  quieter SWO, matches the IDE's "Release|VisualGDB" config). This mapping
  lives in each `target.*/meson.build` (`firmware_defines`).
- Output ELF lands at `build-<board>-<config>/target.<board>/target.<board>.elf`
  — this is a **different location than VisualGDB's own IDE output**
  (`VisualGDB/Debug/target.<board>.elf` / `VisualGDB/Release/...`). The
  `.bat` scripts below point at the `build-*` paths, not the IDE's — don't
  confuse the two, and don't overwrite the IDE's output directory.

### 2. Flash it

```bash
"UnitTest/batch/run-glossy-flash-bluepill-debug.bat"     # or -release
"UnitTest/batch/run-glossy-flash-stlinkv2-debug.bat"     # or -release
```

Each board has its own flash script (`flash.stlinkv2.txt` on COM3,
`flash.bluepill.txt` on COM5 — see the port table above). Both do, in order:
`target extended-remote COM<n>` → `mon swdp_scan` → `mon traceswo enable
1125000` (arms SWO, see above) → `attach 1` → `load` (flashes the ELF) →
`detach` → `exit`. Confirm `BmpTrace2Win.exe` is already running (or start
it) before/around this step so the SWO stream has somewhere to drain to.

If a `.bat` script's ELF path ever needs to change (new build dir naming,
new target), edit the script directly — these exist specifically so you can
flash headlessly, so keep them pointing at whatever you actually build with.

### 3. Test

Now proceed to the "Building the tool" / "Usage" sections below against the
board's **MSP430 GDB RSP port** (COM4 for STLinkV2, COM6 for BluePill —
never the ARM port you just flashed through).

### The one rule that still matters most: reflash before you test

**A meson/IDE build alone does not update what's running on the probe** —
you (or the flash step above) must actually flash it. If firmware code
changed this session, confirm the flash step above actually ran against the
current build before drawing any conclusion from a UnitTest run — otherwise
you're testing stale firmware and any pass/fail result is meaningless. This
is not hypothetical: it caused real confusion earlier in this project's
history (chasing a bug that had already been fixed in source but not yet
flashed).

## Building the tool

```bash
dotnet build UnitTest/UnitTest.csproj
```

Binary lands at `UnitTest/bin/Debug/net9.0/UnitTest.exe`. It also copies
`ChipDB.xml`, `nlog.config`, and `TestFunclets/LoadRegs.bin` next to itself —
run it from that output directory (or reference those files by relative path)
so it can find them.

## Usage

```
UnitTest.exe <COMn> <chip> <test_num> [<test_num> ...]
UnitTest.exe list      # prints all test numbers — this is the source of truth,
                        # don't hardcode test numbers from memory, they can change
UnitTest.exe mcus      # prints chip names known to ChipDB.xml — use one of these
                        # exactly as <chip>, e.g. MSP430F5418A
```

Example: `UnitTest.exe COM4 MSP430F5418A 1`

### Safe vs. destructive tests

- **Test 1** — general GDB v7 protocol test. **Non-destructive.** Safe default
  for "does this work at all" checks.
- **Test 2** — erase-all-flash test. **Destructive** — wipes the attached
  chip's flash. Only run this if the user has explicitly said the attached
  chip has nothing important on it (or explicitly asks for this test).
- **Test 3** — runs funclets (flash write/erase routines uploaded to MSP430
  RAM).
- Finer-grained numbered sub-tests (100, 110, 120, …, 500, 510, 9999) exist
  for individual protocol steps — run `UnitTest.exe list` to see the current
  set rather than assuming these don't change.

When unsure whether a test number is destructive, check `Tests.cs`/`Tests.1.cs`/
`Tests.2.cs`/`Tests.3.cs` for what it actually does before running it, or ask.

### The chip-identity prerequisite gate

`TestsBase.ConnectTarget()` sends `monitor jtag_scan`, then
`VerifyChipIdentity()` sends `monitor chipinfo` and compares the detected
device name against the `<chip>` argument you passed on the command line. If
they don't match (e.g. the probe fell back to `DefaultChip` because of a
JTAG/SBW acquisition problem), the test fails **immediately** with a clear
error instead of silently running the rest of the suite against the wrong
chip profile and producing confusing downstream failures. If you see this
gate fail, the problem is in JTAG/SBW acquisition (driver/hardware/signal
integrity), not in whatever test number you asked for — don't chase the
originally-requested test until this passes.

## Ad-hoc raw `monitor` commands without touching the C# tool

Sometimes you need to try a single `monitor` command interactively (e.g.
"does `jtag_scan slow` behave differently from the default speed grade?")
without adding a permanent code path to `UnitTest`. GDB RSP framing is simple
enough to send directly over the serial port with a short PowerShell snippet
— this was worked out and validated in practice for speed-grade experiments:

```powershell
function Send-GdbPacket($port, [string]$payload) {
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($payload)
    $sum = 0
    foreach ($b in $bytes) { $sum = ($sum + $b) -band 0xFF }
    $chk = "{0:x2}" -f $sum
    $port.Write("`$$payload#$chk")
}

function Read-GdbReply($port) {
    $sb = New-Object System.Text.StringBuilder
    $start = Get-Date
    while (((Get-Date) - $start).TotalSeconds -lt 5) {
        try { $ch = [char]$port.ReadChar() } catch { break }
        if ($ch -eq '+' -or $ch -eq '-') { continue }
        $sb.Append($ch) | Out-Null
        if ($ch -eq '#') {
            $port.ReadChar() | Out-Null; $port.ReadChar() | Out-Null
            $port.Write("+")
            break
        }
    }
    return $sb.ToString()
}

function Decode-HexReply([string]$raw) {
    $hex = $raw.Trim('$','#')
    $bytes = for ($i=0; $i -lt $hex.Length; $i+=2) { [convert]::ToByte($hex.Substring($i,2),16) }
    return [System.Text.Encoding]::ASCII.GetString($bytes)
}

# Example: send "monitor jtag_scan slow"
$port = New-Object System.IO.Ports.SerialPort("COM4", 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.ReadTimeout = 5000
$port.Open(); $port.DiscardInBuffer(); $port.DiscardOutBuffer()
Start-Sleep -Milliseconds 300
$port.Write("+")
$cmd = "jtag_scan slow"
$hex = -join ($cmd.ToCharArray() | ForEach-Object { "{0:x2}" -f [byte][char]$_ })
Send-GdbPacket $port "qRcmd,$hex"
Decode-HexReply (Read-GdbReply $port)
$port.Close()
```

`monitor <cmd>` over GDB RSP is always `qRcmd,<hex(cmd)>` wrapped in
`$<payload>#<2-hex-checksum>`; the reply is hex-encoded ASCII text. This is
useful for one-off experiments (speed grades, `chipinfo`, `power`, etc.)
without needing a C# rebuild — but if the same check will be needed
repeatedly, promote it into `TestsBase.cs` instead (see how `SendMonitor` /
`DecodeHexToString` / `ConnectTarget` are already structured there).

## Reading firmware-side trace during a test

`BmpTrace2Win.exe`'s log lands at `%TEMP%\BmpTrace2Win.log` and is usually
exclusively locked while the tool is writing — a plain file read will hit
`EBUSY`/"used by another process". That's expected, not a bug, when the tool
is actively running. The log correlates firmware-side `Msg`/`Err`/`Dbg` trace
with whatever GDB RSP traffic you just generated — very useful for figuring
out *why* a UnitTest failure happened, not just *that* it happened. Note: SWO
trace volume differs a lot between Debug and Release firmware builds (the
verbose `Dbg` channel compiles out entirely under `-DRELEASE=1`/without
`-DDEBUG=1`), which affects both trace log verbosity and, in rare cases,
real-time behavior if trace calls sit in a timing-critical code path.
