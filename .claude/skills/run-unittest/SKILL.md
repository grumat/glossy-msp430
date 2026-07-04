---
name: run-unittest
description: >-
  Use the C# UnitTest console tool (UnitTest/UnitTest.csproj) to exercise the
  glossy-msp430 GDB RSP firmware against real MSP430 hardware over a serial
  port. Use whenever asked to "test the firmware", "try it on hardware", "run
  the unittest tool", or to validate a firmware change end-to-end on a
  physical probe. Covers the COM-port map per probe board, starting TRACESWO
  before every session (required — the probe freezes otherwise), the mandatory
  reflash-before-test step, safe vs. destructive test numbers, the chip-identity
  prerequisite gate, and how to send one-off raw `monitor` commands without
  extending the C# tool.
---

# Running the UnitTest GDB RSP tool against real hardware

`UnitTest/UnitTest.csproj` is a C# console app that speaks GDB Remote Serial
Protocol to a **glossy-msp430** probe over a serial port, the same protocol a
real GDB client would use. It's how firmware changes get validated against an
actual MSP430 target instead of just compiling.

## Before anything: which port is which

Each physical probe board exposes **two** COM ports that do completely
different jobs. Don't confuse them.

| Board | ARM upload port (flash the probe's own STM32 firmware) | MSP430 GDB RSP port (UnitTest talks here) |
|---|---|---|
| STLinkV2 | **COM3** | **COM4** |
| BluePill | **COM5** | **COM6** |

- The ARM port is a Black Magic Probe-style ARM GDB server, used to flash new
  glossy-msp430 firmware onto the probe's STM32/Geehy MCU. **UnitTest never
  talks to this port.**
- The MSP430 port is the firmware-under-test's own GDB RSP server — this is
  what implements `monitor jtag_scan`, `g`/`G` register packets, `m`/`M`
  memory packets, etc., and what UnitTest connects to.

If a task involves flashing new firmware and you need the actual ARM or
MSP430 toolchain install paths (e.g. to flash from the command line, or to
invoke `msp430-elf-gdb` directly), **ask the user** rather than guessing —
paths vary by machine and weren't meant to be hardcoded here.

## The one rule that matters most: reflash before you test

**Building new firmware does not update what's running on the probe.** The
user flashes firmware manually via VisualGDB (see `CLAUDE.md` — headless
firmware compile-checks work, but flashing is an IDE action). If you changed
firmware code this session, **confirm the user has rebuilt-and-reflashed
before drawing any conclusion from a UnitTest run** — otherwise you're testing
stale firmware and any pass/fail result is meaningless. This is not a
hypothetical: it caused real confusion earlier in this project's history
(chasing a bug that had already been fixed in source but not yet flashed).

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

If `BmpTrace2Win.exe` (TRACESWO monitor) is running, its log lands at
`%TEMP%\BmpTrace2Win.log` and is usually exclusively locked while the tool is
writing — a plain file read will hit `EBUSY`/"used by another process". Ask
the user before assuming it's readable; if it's not, that's expected, not a
bug. The log correlates firmware-side `Msg`/`Err`/`Dbg` trace with whatever
GDB RSP traffic you just generated — very useful for figuring out *why* a
UnitTest failure happened, not just *that* it happened. Note: SWO trace
volume differs a lot between Debug and Release firmware builds (see
`stdproj.h` — the verbose `Dbg` channel compiles out entirely in Release),
which affects both trace log verbosity and, in rare cases, real-time
behavior if trace calls sit in a timing-critical code path.
