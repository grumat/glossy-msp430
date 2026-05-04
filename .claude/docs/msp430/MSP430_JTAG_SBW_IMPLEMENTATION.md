# MSP430 JTAG and Spy-Bi-Wire (SBW) Implementation Documentation

## Overview

This document provides comprehensive documentation of JTAG and Spy-Bi-Wire (SBW) implementation for TI MSP430 microcontrollers, based on analysis of Texas Instruments reference documentation, MSPDebugStack implementation, and the glossy-msp430 firmware project.

## 1. JTAG/SBW Interface Fundamentals

### 1.1 Supported Interfaces

MSP430 devices support two debugging interfaces:
1. **4-Wire JTAG** - Standard IEEE 1149.1 compliant with MSP430-specific restrictions
2. **2-Wire Spy-Bi-Wire (SBW)** - Reduced pin count interface using TEST and RST pins

### 1.2 Key Differences

| Interface | Pins Required | Speed | Pin Sharing | Compatibility |
|-----------|---------------|-------|-------------|---------------|
| 4-Wire JTAG | TMS, TCK, TDI, TDO | Faster | Conflicts with GPIO on shared-pin devices | All MSP430 devices |
| Spy-Bi-Wire | SBWTCK (TEST), SBWTDIO (RST) | Slower | No conflict with GPIO | SBW-capable devices only |

### 1.3 MSP430 JTAG Restrictions (SLAU320AJ Section 2.1.1)

1. **Shared pins**: JTAG pins are shared with port functions on devices with TEST pin
2. **Chain position**: MSP430 must be first device in JTAG chain
3. **Limited instructions**: Only BYPASS instruction supported (no SAMPLE, PRELOAD, or EXTEST)
4. **RTOS incompatibility**: Should not be used with real-time operating systems

## 2. Hardware Interface Details

### 2.1 4-Wire JTAG Signals

| Pin | Direction | Description |
|-----|-----------|-------------|
| TMS | IN | Controls JTAG TAP state machine |
| TCK | IN | JTAG clock input |
| TDI | IN | JTAG data input and TCLK input |
| TDO | OUT | JTAG data output |
| TEST | IN | Enable JTAG pins (shared JTAG devices only) |

### 2.2 Spy-Bi-Wire Signals

| Pin | JTAG Equivalent | Description |
|-----|-----------------|-------------|
| SBWTCK (TEST) | TCK | Spy-Bi-Wire test clock (dedicated pin) |
| SBWTDIO (RST) | TMS, TDI, TDO | Bi-directional data line (shared with RST/NMI) |

### 2.3 TCLK Signal

- TCLK is the system clock provided to target MSP430
- In 4-wire JTAG: TCLK is provided on TDI pin while in Run-Test/Idle state
- In SBW: TCLK is provided in TDI slot while in Run-Test/Idle state
- Used for CPU clocking during memory operations

## 3. TAP Controller State Machine

The MSP430 implements the standard IEEE 1149.1 TAP controller state machine:

```
Test-Logic-Reset
    ↓ (TMS=0)
Run-Test/Idle ←→ Select-DR-Scan ←→ Select-IR-Scan
    ↓           ↓ (TMS=0)        ↓ (TMS=0)
    ...     Capture-DR           Capture-IR
            Shift-DR             Shift-IR
            Exit1-DR             Exit1-IR
            Pause-DR             Pause-IR
            Exit2-DR             Exit2-IR
            Update-DR            Update-IR
```

## 4. JTAG Implementation Architecture

### 4.1 Hardware Abstraction Layer (glossy-msp430)

The glossy-msp430 project implements a layered architecture:

```
GDB RSP Protocol (Firmware.shared/ui/gdb.h)
    ↓
TapMcu (Firmware.shared/drivers/TapMcu.h) - Unified TAP abstraction
    ↓
ITapDev (Abstract Interface) 
    ├─ TapDev430 (MSP430 F1xx/F2xx/F4xx)
    ├─ TapDev430X (MSP430X architecture)
    └─ TapDev430Xv2 (MSP430Xv2 with EEM)
    ↓
ITapInterface (Firmware.shared/drivers/TapPlayer.h)
    ↓
JtagDev (Hardware-specific implementations)
    ├─ JtagDev_2..5 (Speed grades)
    ├─ SPI + DMA implementation
    └─ TIM + DMA implementation (fastest: 9 MHz on STM32F103)
```

### 4.2 Implementation Components

1. **ITapInterface** - Abstract JTAG hardware operations
2. **TapPlayer** - Executes JTAG sequences with timing control
3. **ITapDev** - Device-specific protocol implementation
4. **TapMcu** - High-level MCU control interface

### 4.3 Hardware Configurations

Configured via `Firmware.shared/platform-defs.h`:

```c
// JTAG Implementation (choose one)
#define OPT_JTAG_IMPLEMENTATION OPT_JTAG_IMPL_TIM_DMA  // Fastest
#define OPT_JTAG_IMPLEMENTATION OPT_JTAG_IMPL_SPI_DMA  // Alternative
#define OPT_JTAG_IMPLEMENTATION OPT_JTAG_IMPL_SPI      // Basic

// TCLK Implementation  
#define OPT_JTAG_TCLK_IMPLEMENTATION OPT_JTCLK_IMPL_TIM_DMA
```

## 5. JTAG Instructions and Registers

### 5.1 Key JTAG Instructions (SLAU320AJ Table 2-5)

| Instruction | Value (LSB) | Description |
|-------------|-------------|-------------|
| IR_ADDR_16BIT | 0x83 | Set Memory Address Bus (MAB) |
| IR_ADDR_CAPTURE | 0x84 | Capture MAB value |
| IR_DATA_TO_ADDR | 0x85 | Write MDB to address on MAB |
| IR_DATA_16BIT | 0x41 | Set Memory Data Bus (MDB) |
| IR_DATA_QUICK | 0x43 | Set MDB with PC auto-increment |
| IR_CNTRL_SIG_16BIT | 0x13 | Set JTAG control signal register |
| IR_CNTRL_SIG_CAPTURE | 0x14 | Read JTAG control signal register |
| IR_CNTRL_SIG_RELEASE | 0x15 | Release CPU from JTAG control |
| IR_BYPASS | 0xFF | Bypass mode |
| IR_JMB_EXCHANGE | 0x61 | JTAG mailbox exchange |

### 5.2 Data Formats

```c
#define F_BYTE  8    // 8-bit access
#define F_WORD  16   // 16-bit access  
#define F_ADDR  20   // 20-bit address (MSP430X)
#define F_LONG  32   // 32-bit access
```

### 5.3 JTAG Control Signal Register

**For 1xx/2xx/4xx Families:**
- Bit 0: R/W (1=Read, 0=Write)
- Bit 3: HALT_JTAG (1=CPU stopped)
- Bit 4: BYTE (1=Byte access, 0=Word access)
- Bit 9: TCE (CPU synchronization)
- Bit 10: TCE1 (CPU under JTAG control)
- Bit 11: POR (Power-on-reset control)
- Bit 14: SWITCH (Enable TDO as TDI input)

**For 5xx/6xx Families (Xv2):**
Additional bits for pipelined CPU:
- Bit 8: CPUSUSP (CPU suspend)
- Bits 12-13: RELEASE_LBYTE (Release control bits)
- Bits 14-15: INSTR_SEQ_NO (Instruction sequence number)

## 6. Memory Access Protocols

### 6.1 Basic Memory Write Sequence

```c
// Write word to memory address
1. IR_SHIFT(IR_ADDR_16BIT)      // Select address register
2. DR_SHIFT20(address)          // Set address (20-bit for Xv2)
3. IR_SHIFT(IR_DATA_TO_ADDR)    // Select data-to-address instruction  
4. DR_SHIFT16(data)             // Write data to address
```

### 6.2 Flash Programming Sequence

```c
// Write to flash memory
1. Set flash control registers (FCTL1/FCTL3)
2. Enable write mode (WRT bit)
3. Write data using IR_DATA_TO_ADDR
4. Provide TCLK pulses for flash timing
5. Disable write mode
6. Set LOCK bit
```

### 6.3 Flash Erase Sequences

Supported erase modes:
- **Segment Erase**: Erase single flash segment
- **Main Erase**: Erase all main memory
- **Mass Erase**: Erase main and info memory (device-dependent)

## 7. Spy-Bi-Wire Implementation

### 7.1 SBW Timing (SLAU320AJ Section 2.2.3)

SBW uses time-division multiplexing with three slots:
1. **TMS Slot**: Controls TAP state transitions
2. **TDI Slot**: Shifts data into target
3. **TDO Slot**: Reads data from target

```
SBWTCK ─────┐   ┌─────┐   ┌─────┐   ┌─────
            │   │     │   │     │   │
            └───┘     └───┘     └───┘
SBWTDIO ────┬───┬─────┬───┬─────┬───┬─────
            │TMS│ TDI │TDO│ TMS │TDI│TDO
```

### 7.2 SBW-to-JTAG Conversion Logic

Internal conversion logic translates 2-wire SBW to 4-wire JTAG internally:

```
SBWTCK ────┐  ┌─── Internal Logic ───┐  ┌── TAP Controller
SBWTDIO ───┼──┼───► TMS, TDI, TCK ───┼──┼──► JTAG Interface
           │  │                    │  │
           │  └── TDO ─────────────┘  │
           └───────────────────────────┘
```

### 7.3 SBW Entry Sequence

```c
// SBW activation sequence
1. Drive SBWTCK low for >7μs (deactivates SBW if previously active)
2. Drive TEST/RST pins to enter mode:
   - TEST=0, RST=0: Deactivate
   - TEST=1, RST=0: Activate SBW  
   - TEST=0, RST=1: Activate 4-wire JTAG on SBW-capable device
```

## 8. Enhanced Emulation Module (EEM)

### 8.1 EEM Features (SLAU414F/SLAA393F)

The EEM provides non-intrusive debugging capabilities:

| Feature | Description |
|---------|-------------|
| Hardware Breakpoints | 2-8 MAB/MDB triggers (device-dependent) |
| CPU Register Triggers | 0-2 register write triggers |
| Combination Triggers | 2-10 complex trigger combinations |
| Trigger Sequencer | State machine for trigger sequencing |
| State Storage | 8-entry trace buffer |
| Cycle Counter | 40-bit performance profiling |
| Clock Control | Per-module clock control during debug |

### 8.2 EEM Configuration Levels

Texas Instruments defines four EEM levels:

| Level | Triggers | Sequencer | State Storage | Cycle Counter |
|-------|----------|-----------|---------------|---------------|
| XS | 2 | No | No | 1 |
| S | 3 | No | No | 1 |
| M | 5 | Yes | No | 1 |
| L | 8 | Yes | Yes | 2 |

### 8.3 EEM Register Access

EEM registers are accessed through JTAG using mailbox system:
1. Write command to JTAG mailbox in register
2. Wait for acknowledgment in JTAG mailbox out register
3. Read response data

## 9. Implementation Examples

### 9.1 TI Reference Implementation (SLAU320AJ)

The Replicator example provides reference implementations:

```
slau320aj/slau320/
├── Replicator430/        # MSP430 F1xx/F2xx/F4xx
├── Replicator430X/       # MSP430X architecture  
├── Replicator430Xv2/     # MSP430Xv2 (F5xx/F6xx)
└── Replicator430FR/      # FRAM devices
```

Key files:
- `LowLevelFunc*.h/c` - Hardware-specific macros
- `JTAGfunc*.h/c` - JTAG protocol implementation
- `FlashWrite.c` - Flash programming algorithms
- `FlashErase.c` - Flash erase algorithms

### 9.2 MSPDebugStack Implementation

TI's production debug stack provides comprehensive implementation:

```
DLL430_v3/
├── include/
│   ├── MSP430.h          # Main API header
│   ├── MSP430_EEM.h      # EEM API
│   └── MSP430_FET.h      # FET hardware control
└── src/TI/DLL430/
    ├── EM/               # Emulation Manager
    ├── EEM/              # Enhanced Emulation Module
    └── DeviceDb/         # Device database
```

### 9.3 glossy-msp430 Implementation

Modern C++17 implementation with zero-overhead abstractions:

```
Firmware.shared/drivers/
├── TapMcu.cpp/h          # Unified TAP interface
├── TapDev430*.cpp/h      # Device-specific protocols
├── JtagDev.cpp/h         # Hardware JTAG driver
├── ITapDev.h             # Abstract interface
└── TapPlayer.cpp/h       # JTAG sequence player
```

## 10. Security and Protection

### 10.1 JTAG Security Fuse (1xx/2xx/4xx Families)

1. **Fuse Programming**:
   ```c
   IR_SHIFT(IR_PREPARE_BLOW);
   IR_SHIFT(IR_EX_BLOW);
   // Apply VPP voltage (11-13V) to TEST/TDI pin
   ```

2. **Fuse Verification**: Attempt JTAG access; blown fuse prevents communication

### 10.2 JTAG Lock Key (5xx/6xx/FRxx Families)

Software-based protection using 32-byte password at address 0xFF80:
- Password verification required for JTAG access
- Invalid password attempts can lock device permanently

### 10.3 FRAM Write Protection

FRAM devices implement Memory Protection Unit (MPU):
- Segment-based read/write/execute protection
- Configurable via MPU registers

## 11. Performance Considerations

### 11.1 JTAG Clock Speeds

| Implementation | Max Speed | Platform |
|----------------|-----------|----------|
| TIM + DMA | 9 MHz | STM32F103 BluePill |
| SPI + DMA | 4.5 MHz | STM32 with SPI |
| Bit-banged GPIO | ~1 MHz | Software-driven |

### 11.2 Flash Programming Speed

Factors affecting speed:
- **Flash timing**: Requires specific TCLK pulse counts
- **Memory type**: FRAM faster than Flash
- **Erase time**: Mass erase slower than segment erase
- **Verification**: PSA mode for faster verification

### 11.3 SBW Performance Limitations

- **Slower than 4-wire JTAG**: Time-division multiplexing adds overhead
- **Clock limitations**: SBWTCK low phase must be <7μs
- **Bidirectional signaling**: Requires precise timing for bus turnaround

## 12. Common Implementation Patterns

### 12.1 Device Initialization Pattern

```c
bool InitializeDevice(CoreId &coreid) {
    // 1. Reset and enter JTAG mode
    ResetTap();
    
    // 2. Shift JTAG ID to identify device
    uint8_t jtag_id = IrShift(IR_BYPASS);
    
    // 3. Enter JTAG control
    SyncJtag();
    
    // 4. Read device signature
    return GetDeviceSignature(coreid);
}
```

### 12.2 Memory Access Pattern

```c
uint16_t ReadMemory(address_t addr) {
    // Set address register
    IrShift(IR_ADDR_16BIT);
    DrShift20(addr);
    
    // Read data
    IrShift(IR_DATA_TO_ADDR);
    return DrShift16(0);  // Dummy data shifted in
}
```

### 12.3 Flash Programming Pattern

```c
void ProgramFlash(address_t addr, const uint16_t *data, uint32_t count) {
    // Unlock flash
    WriteWord(FCTL1_ADDR, FCTL1_UNLOCK | FCTL1_WRT);
    
    // Program words
    for (uint32_t i = 0; i < count; i++) {
        WriteWord(addr, data[i]);
        addr += 2;
        
        // Provide flash timing pulses
        PulseTclk(FLASH_TIMING_PULSES);
    }
    
    // Lock flash
    WriteWord(FCTL1_ADDR, FCTL1_UNLOCK);
}
```

## 13. Debug and Diagnostics

### 13.1 Common Error Conditions

| Error | Cause | Solution |
|-------|-------|----------|
| No JTAG ID | Fuse blown, wrong interface | Check security fuse, verify interface mode |
| Communication errors | Timing issues, signal integrity | Adjust clock speed, check connections |
| Flash programming fails | Incorrect timing, voltage | Verify TCLK pulses, check VCC |
| EEM access fails | Device not halted, wrong EEM version | Halt CPU before EEM access |

### 13.2 Diagnostic Techniques

1. **JTAG ID Check**: First diagnostic step
2. **Register Access Test**: Verify basic JTAG functionality
3. **Memory Access Test**: Test RAM access before flash
4. **PSA Verification**: Quick memory checksum verification

## 14. Platform Configuration

### 14.1 glossy-msp430 Configuration

Configure target-specific settings in `platform.h`:

```c
// Target.bluepill/platform.h example
#define OPT_JTAG_IMPLEMENTATION      OPT_JTAG_IMPL_TIM_DMA
#define OPT_JTAG_TCLK_IMPLEMENTATION OPT_JTCLK_IMPL_TIM_DMA  
#define OPT_JTAG_SPEED_GRADE         5  // Highest speed
#define OPT_JTAG_USE_SBW             0  // 4-wire JTAG
```

### 14.2 Pin Mapping

Hardware-specific pin assignments:

```c
// STM32F103 BluePill example
#define JTAG_TCK_PIN    GPIO_Pin_13  // PortB.13
#define JTAG_TMS_PIN    GPIO_Pin_14  // PortB.14  
#define JTAG_TDI_PIN    GPIO_Pin_15  // PortB.15
#define JTAG_TDO_PIN    GPIO_Pin_12  // PortB.12
#define JTAG_nTRST_PIN  GPIO_Pin_11  // PortB.11 (optional)
#define JTAG_nSRST_PIN  GPIO_Pin_10  // PortB.10 (optional)
```

## 15. References

### 15.1 TI Documentation

1. **SLAU320AJ**: "MSP430 Programming With the JTAG Interface" (Primary reference)
2. **SLAA393F**: "Advanced Debugging Using the Enhanced Emulation Module (EEM)"
3. **SLAU414F**: "Embedded Emulation Module (EEM)" excerpt
4. Device-specific datasheets and user guides

### 15.2 Implementation Resources

1. **MSPDebugStack**: TI's reference implementation (DLL430_v3)
2. **Replicator Example**: SLAU320AJ reference code
3. **glossy-msp430**: Modern C++ implementation
4. **MSPDebug**: Open-source debug tool reference

### 15.3 Tools and Hardware

1. **MSP-FET**: TI's USB JTAG debugger
2. **MSP-GANG**: Production programmer
3. **Third-party tools**: Compatible JTAG/SBW hardware

## Appendix A: JTAG Instruction Encoding

Complete instruction encoding (LSB-first as shifted):

```c
// Original values (from SLAU320AJ)
#define IR_ADDR_16BIT          0x83  // 11000001 (LSB: 1, MSB: C1)
#define IR_ADDR_CAPTURE        0x84  // 00100001 (LSB: 1, MSB: 21)
#define IR_DATA_TO_ADDR        0x85  // 10100001 (LSB: 1, MSB: A1)
#define IR_DATA_16BIT          0x41  // 10000010 (LSB: 2, MSB: 82)
#define IR_DATA_QUICK          0x43  // 11000010 (LSB: 2, MSB: C2)
#define IR_CNTRL_SIG_16BIT     0x13  // 11001000 (LSB: 8, MSB: C8)
#define IR_CNTRL_SIG_CAPTURE   0x14  // 00101000 (LSB: 8, MSB: 28)
#define IR_CNTRL_SIG_RELEASE   0x15  // 10101000 (LSB: 8, MSB: A8)
#define IR_BYPASS              0xFF  // 11111111 (LSB: FF, MSB: FF)
```

**Note**: Many implementations swap bit order for easier shifting.

## Appendix B: Device Family Support

| Architecture | JTAG ID | CoreIP ID | Features |
|--------------|---------|-----------|----------|
| Original MSP430 | 0x89 | N/A | 16-bit, basic EEM |
| MSP430X | 0x8D | N/A | 20-bit address, enhanced EEM |
| MSP430Xv2 | 0x91 | 0x1A | Pipeline, advanced EEM |
| MSP430Xv2 (FRAM) | 0x99 | 0xXX | FRAM, MPU, advanced EEM |

## Appendix C: Timing Requirements

### C.1 Minimum Timing

- **TCK period**: Device-dependent (check datasheet)
- **TCLK setup/hold**: Relative to TCK edges
- **SBWTCK low phase**: Must be <7μs
- **Flash programming**: Requires specific TCLK pulse counts

### C.2 Maximum Speeds

- **4-wire JTAG**: Up to device TCK maximum (typically 5-10 MHz)
- **Spy-Bi-Wire**: Limited by 2-wire multiplexing overhead
- **Practical limits**: Signal integrity, cable length, driver capability

---

*Document compiled from analysis of TI documentation, MSPDebugStack source, and glossy-msp430 implementation.*
*Last updated: Based on SLAU320AJ (2021), SLAU414F (2018), SLAA393F (2016) revisions.*