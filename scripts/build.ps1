#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build Glossy MSP430 firmware using Meson.
.DESCRIPTION
    Wrapper around meson setup + meson compile for ARM firmware targets, MSP430
    funclets, and C#/.NET tools. Supports Windows (SysGCC toolchain) as a
    first-class build platform.
.PARAMETER Target
    Build target: bluepill, g431kb, stlinv2, funclets, tools, or all.
.PARAMETER Config
    Build configuration: debug or release (default: debug).
.PARAMETER Reconfigure
    Force re-run meson setup even if build directory exists.
.EXAMPLE
    .\scripts\build.ps1 -Target bluepill -Config debug
    .\scripts\build.ps1 -Target all
#>

param(
    [ValidateSet('bluepill', 'g431kb', 'stlinv2', 'funclets', 'tools', 'all')]
    [string]$Target = 'bluepill',
    [ValidateSet('debug', 'release')]
    [string]$Config = 'debug',
    [switch]$Reconfigure
)

$ErrorActionPreference = 'Stop'
$rootDir = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $rootDir "build\$Target-$Config"

function Resolve-CrossFile {
    param([string]$Target)
    $crossDir = Join-Path $rootDir 'cross'
    switch ($Target) {
        'bluepill'   { return Join-Path $crossDir 'arm-none-eabi.ini' }
        'g431kb'     { return Join-Path $crossDir 'arm-none-eabi.ini' }
        'stlinv2'    { return Join-Path $crossDir 'arm-none-eabi.ini' }
        'funclets'   { return Join-Path $crossDir 'msp430-elf.ini' }
        'tools'      { return $null }
        default      { return $null }
    }
}

function Invoke-MesonSetup {
    param([string]$Dir, [string]$CrossFile, [string]$Target, [string]$Config)
    $args = @('setup', $Dir, "--wipe")
    if ($CrossFile) {
        $args += "--cross-file", $CrossFile
    }
    $args += "-Dtarget=$Target"
    $args += "-Dbuildtype=$Config"
    $args += "-Dwarning_level=3"
    & meson $args 2>&1 | ForEach-Object { $_ }
    if ($LASTEXITCODE -ne 0) {
        throw "meson setup failed for $Target"
    }
}

function Invoke-MesonCompile {
    param([string]$Dir)
    & meson compile -C $Dir 2>&1 | ForEach-Object { $_ }
    if ($LASTEXITCODE -ne 0) {
        throw "meson compile failed"
    }
}

# ---- main ----
# Check that meson is available
if (-not (Get-Command 'meson' -ErrorAction SilentlyContinue)) {
    Write-Error "Meson not found. Install it: pip install meson ninja"
    exit 1
}

# Check that SysGCC is on PATH for ARM/funclet targets
if ($Target -ne 'tools') {
    $compiler = if ($Target -eq 'funclets') { 'msp430-elf-g++' } else { 'arm-none-eabi-g++' }
    if (-not (Get-Command $compiler -ErrorAction SilentlyContinue)) {
        $sysgccBin = "C:\SysGCC\arm-eabi\bin"
        if (Test-Path $sysgccBin) {
            $env:PATH = "$sysgccBin;$env:PATH"
            Write-Host "Added SysGCC to PATH: $sysgccBin"
        } else {
            Write-Warning "$compiler not found on PATH. Set toolchain path or install SysGCC."
        }
    }
}

if ($Reconfigure -and (Test-Path $buildDir)) {
    Remove-Item -Recurse -Force $buildDir
}

$crossFile = Resolve-CrossFile $Target
$needSetup = $Reconfigure -or -not (Test-Path (Join-Path $buildDir 'meson-info'))

if ($Target -eq 'all') {
    # Build each ARM target + funclets + tools
    foreach ($t in @('bluepill', 'g431kb', 'stlinv2', 'funclets')) {
        Write-Host "=== Building $t ($Config) ===" -ForegroundColor Cyan
        $dir = Join-Path $rootDir "build\$t-$Config"
        $cf = Resolve-CrossFile $t
        if ($Reconfigure -and (Test-Path $dir)) { Remove-Item -Recurse -Force $dir }
        if ($Reconfigure -or -not (Test-Path (Join-Path $dir 'meson-info'))) {
            Invoke-MesonSetup -Dir $dir -CrossFile $cf -Target $t -Config $Config
        }
        Invoke-MesonCompile -Dir $dir
    }
    # Build tools (no cross file)
    $toolsDir = Join-Path $rootDir 'build\tools-release'
    if ($Reconfigure -and (Test-Path $toolsDir)) { Remove-Item -Recurse -Force $toolsDir }
    if ($Reconfigure -or -not (Test-Path (Join-Path $toolsDir 'meson-info'))) {
        Invoke-MesonSetup -Dir $toolsDir -Target 'tools' -Config 'release'
    }
    Invoke-MesonCompile -Dir $toolsDir
} else {
    if ($needSetup) {
        Invoke-MesonSetup -Dir $buildDir -CrossFile $crossFile -Target $Target -Config $Config
    }
    Invoke-MesonCompile -Dir $buildDir
}

Write-Host "=== Build complete: $Target ($Config) ===" -ForegroundColor Green
