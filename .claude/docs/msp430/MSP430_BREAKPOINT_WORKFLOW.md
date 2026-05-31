# MSP430 Hardware Breakpoint Management Workflow

## Overview

This document describes the complete workflow for managing hardware breakpoints on MSP430 microcontrollers using the Enhanced Emulation Module (EEM). The workflow covers breakpoint programming, CPU control, state management, and single-stepping operations.

## 1. Hardware Breakpoint Architecture

### 1.1 EEM Trigger Blocks

MSP430 devices contain dedicated EEM (Enhanced Emulation Module) hardware with trigger blocks:

| Component | Description | MSP430 Implementation |
|-----------|-------------|----------------------|
| **Memory Trigger Blocks** | Compare MAB/MDB with values | 2-8 blocks, depending on device |
| **CPU Register Triggers** | Monitor CPU register writes | 0-2 blocks |
| **Combination Triggers** | Logical AND of triggers | 2-10 combinations |
| **Trigger Sequencer** | State machine for complex sequences | Available in M/L EEM levels |
| **State Storage** | 8-entry trace buffer | Available in L EEM level |

### 1.2 Breakpoint Types

1. **Code Breakpoints** - Trigger on instruction fetch at specific address
2. **Data Breakpoints** - Trigger on read/write to memory address (with optional value match)
3. **Register Breakpoints** - Trigger on write to CPU register
4. **Range Breakpoints** - Trigger on access to memory range
5. **Complex Breakpoints** - Combine multiple triggers with sequencer

## 2. Breakpoint Management Workflow

### 2.1 Complete Breakpoint Lifecycle

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│    Setup    │────▶│  Program    │────▶│   Release   │
│  Breakpoint │     │ Breakpoint  │     │    CPU      │
└─────────────┘     └─────────────┘     └─────────────┘
                                                   │
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Handle    │◀────│  Detect     │◀────│ Breakpoint  │
│ Breakpoint  │     │ Breakpoint  │     │    Hit      │
└─────────────┘     └─────────────┘     └─────────────┘
                                                   │
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Resume    │────▶│   Single    │────▶│   Halt      │
│ Execution   │     │   Step      │     │ Execution   │
└─────────────┘     └─────────────┘     └─────────────┘
```

## 3. Step-by-Step Implementation

### 3.1 Step 1: Stop the MCU and Enter JTAG Control

```c
bool HaltCpuAndEnterJtagControl(CpuContext &ctx, const ChipProfile &prof) {
    // 1. Synchronize JTAG and save current CPU context
    if (!SyncJtagConditionalSaveContext(ctx, prof)) {
        return false;
    }
    
    // 2. Verify CPU is halted
    uint16_t sig_reg = ReadJtagControlSignal();
    if (!(sig_reg & 0x0800)) {  // Check HALT bit
        // Force halt if needed
        WriteJtagControlSignal(0xA55A);  // STOP_DEVICE pattern
        WaitForCpuHalt();
    }
    
    // 3. Save watchdog state (prevents reset during debug)
    ctx.wdt_ = ReadByte(GetWdtAddress(prof));
    if (ctx.wdt_ != 0x5A) {
        WriteWord(GetWdtAddress(prof), WDT_PASSWD | WDT_HOLD);
    }
    
    return true;
}
```

### 3.2 Step 2: Program Hardware Breakpoint

```c
bool ProgramHardwareBreakpoint(address_t break_address, 
                               BreakpointType type,
                               const ChipProfile &prof) {
    // 1. Check available trigger resources
    if (prof.n_breakpoints_used >= prof.max_breakpoints) {
        return false;  // No free triggers
    }
    
    // 2. Configure trigger block for breakpoint
    uint8_t trigger_block = FindFreeTriggerBlock();
    
    // 3. Set trigger configuration based on type
    switch (type) {
    case BP_CODE:
        // Code breakpoint: match address on instruction fetch
        WriteEemRegister(TRIGGER_BLOCK_BASE(trigger_block) + TB_TRIGGER_CTRL,
                         TRIGGER_CTRL_ENABLE | 
                         TRIGGER_CTRL_FETCH_COMPARE |
                         TRIGGER_CTRL_ON_CPU);
        
        WriteEemRegister(TRIGGER_BLOCK_BASE(trigger_block) + TB_VALUE,
                         break_address);
        
        // Set mask for address match (all bits)
        WriteEemRegister(TRIGGER_BLOCK_BASE(trigger_block) + TB_MASK,
                         (prof.cpu_arch_ == CPU_ARCH_XV2) ? 0x000FFFFF : 0x0000FFFF);
        break;
        
    case BP_DATA_READ:
    case BP_DATA_WRITE:
        // Data breakpoint: match address on read/write
        uint32_t ctrl = TRIGGER_CTRL_ENABLE | TRIGGER_CTRL_ON_CPU;
        ctrl |= (type == BP_DATA_READ) ? TRIGGER_CTRL_READ : TRIGGER_CTRL_WRITE;
        
        WriteEemRegister(TRIGGER_BLOCK_BASE(trigger_block) + TB_TRIGGER_CTRL, ctrl);
        WriteEemRegister(TRIGGER_BLOCK_BASE(trigger_block) + TB_VALUE, break_address);
        break;
    }
    
    // 4. Configure breakpoint reaction
    WriteEemRegister(EEM_CPU_STOP, 1 << trigger_block);  // Stop CPU on trigger
    
    // 5. Update breakpoint tracking
    prof.n_breakpoints_used++;
    SaveTriggerAllocation(trigger_block, break_address, type);
    
    return true;
}
```

### 3.3 Step 3: Release CPU with Breakpoints Active

```c
void ReleaseCpuWithBreakpoints(CpuContext &ctx, 
                               const ChipProfile &prof,
                               bool start_execution) {
    // 1. Restore CPU context (except PC/SR if we want to start from breakpoint)
    if (!start_execution) {
        // Keep CPU halted
        ctx.is_running_ = false;
    } else {
        // Mark as running (breakpoints will stop execution)
        ctx.is_running_ = true;
    }
    
    // 2. Set up clock control for debugging
    if (prof.eem_version_ >= EEM_VERSION_M) {
        // Configure per-module clock control
        WriteEemRegister(EEM_CLK_CTRL0, ctx.eem_clk_ctrl_);
        
        // For M/L EEM: Enable clocks needed during debug halt
        uint32_t clk_ctrl = 0x0417;  // Default UIF value
        if (prof.has_uart_ || prof.has_timer_) {
            clk_ctrl |= (1 << 7) | (1 << 9);  // Keep UART/Timer clocks running
        }
        WriteEemRegister(EEM_MCLK_CTRL, clk_ctrl);
    }
    
    // 3. Release device from JTAG control
    ReleaseDevice(ctx, prof, false, kSwBkpInstr);
    
    // 4. If starting execution, issue run command
    if (start_execution) {
        // Write to JTAG control register to start execution
        WriteJtagControlSignal(0x0000);  // Clear HALT bit
        
        // Release JTAG signals
        TapPlayer::ReleaseJtag();
        
        // The CPU will now run until breakpoint hit
    }
}
```

> **Per-module clock control on CPUXv2 — `ETKEYSEL`/`ETCLKSEL`.** The
> `EEM_CLK_CTRL0`/`EEM_MCLK_CTRL` writes above are the *legacy* CPU/CPUX model
> (a single `MODCLKCTRL0` bitmap, default `0x0417`). On **CPUXv2** the fixed
> bitmap is replaced by an enumerated per-module scheme: for each module slot
> the probe writes `ETKEYSEL = ETKEY | PID` then `ETCLKSEL = run/stop`, driven
> by bit *i* of `ctx.eem_clk_ctrl_`. This is implemented in
> `TapDev430Xv2::SyncJtagAssertPorSaveContext()` (the PID table lives in the
> chip DB as `EtwCodes::etw_codes_[]`). These registers are undocumented by TI
> and were reverse-engineered from the MSP-FET/UIF source — see the wiki page
> **"The Missing EEM Documentation" → `ETKEYSEL`/`ETCLKSEL`** for the full
> protocol, the `ETWPID_*` table, and source references.

### 3.4 Step 4: Query CPU State and Detect Breakpoint Hit

```c
DeviceStatus QueryCpuStateAndDetectBreakpoint(CpuContext &ctx,
                                             const ChipProfile &prof) {
    // 1. Check if we can communicate with device
    if (!TapPlayer::IsConnected()) {
        return DEVICE_STATUS_ERROR;
    }
    
    // 2. Read EEM status registers
    uint32_t eem_status = ReadEemRegister(EEM_STATUS);
    uint32_t trigger_status = ReadEemRegister(EEM_TRIGGER_STATUS);
    
    // 3. Check for breakpoint hit
    if (trigger_status & 0x0000FFFF) {  // Any trigger active
        // 4. Determine which breakpoint was hit
        uint16_t active_triggers = trigger_status & 0xFFFF;
        uint8_t hit_trigger = 0;
        
        while (active_triggers) {
            if (active_triggers & 1) {
                // Found active trigger
                address_t bp_addr = GetBreakpointAddress(hit_trigger);
                printf("Breakpoint hit at 0x%08lX (trigger %d)\n", 
                      bp_addr, hit_trigger);
                break;
            }
            hit_trigger++;
            active_triggers >>= 1;
        }
        
        // 5. Clear trigger status
        WriteEemRegister(EEM_TRIGGER_STATUS, trigger_status);
        
        return DEVICE_STATUS_HALTED;
    }
    
    // 6. Check if CPU is executing
    uint16_t sig_reg = ReadJtagControlSignal();
    bool is_halted = (sig_reg & 0x0800) != 0;  // HALT bit
    bool instr_load = (sig_reg & 0x0080) != 0; // INSTR_LOAD bit
    
    if (is_halted) {
        ctx.is_running_ = false;
        
        // Check if halted in interrupt context
        ctx.in_interrupt_ = CheckInterruptContext(ctx, prof);
        
        return DEVICE_STATUS_HALTED;
    } else {
        ctx.is_running_ = true;
        return DEVICE_STATUS_RUNNING;
    }
}
```

### 3.5 Step 5: Handle Breakpoint Hit - Load Register Values

```c
bool HandleBreakpointHitAndLoadRegisters(CpuContext &ctx,
                                        const ChipProfile &prof,
                                        uint32_t *registers) {
    // 1. Halt CPU if not already halted
    if (ctx.is_running_) {
        HaltCpu();
        WaitForCpuHalt();
        ctx.is_running_ = false;
    }
    
    // 2. Save breakpoint context
    SaveBreakpointContext(ctx);
    
    // 3. Read all CPU registers
    bool success = ReadAllCpuRegisters(ctx, prof, registers);
    if (!success) {
        return false;
    }
    
    // 4. Update PC and SR in context
    ctx.pc_ = registers[0];  // R0 is PC
    ctx.sr_ = registers[2];  // R2 is SR
    
    // 5. Read additional CPU state if available
    if (prof.eem_version_ >= EEM_VERSION_M) {
        // Read pipeline state for Xv2 devices
        uint32_t pipe_state = ReadEemRegister(EEM_PIPE_STATUS);
        ctx.pipeline_depth_ = (pipe_state >> 8) & 0x03;
    }
    
    // 6. Read memory around breakpoint for context
    ReadMemoryContext(ctx.pc_, prof);
    
    return true;
}
```

### 3.6 Step 6: Single Step to Next Instruction

```c
bool SingleStepToNextInstruction(CpuContext &ctx,
                                const ChipProfile &prof,
                                uint32_t *registers_after) {
    // 1. Prepare for single step
    if (!PrepareSingleStep(ctx, prof)) {
        return false;
    }
    
    // 2. Different approaches based on CPU architecture
    if (prof.cpu_arch_ == CPU_ARCH_ORIGINAL) {
        // Original MSP430: Use instruction fetch tracking
        
        // 2a. Set PC to current address
        SetPC(ctx.pc_);
        
        // 2b. Enable single step mode
        WriteJtagControlSignal(0x0008);  // Set single step bit
        
        // 2c. Execute one instruction
        ReleaseDevice(ctx, prof, true, kSwBkpInstr);
        
        // 2d. Wait for instruction completion
        WaitForInstructionCompletion();
        
    } else if (prof.cpu_arch_ == CPU_ARCH_XV2) {
        // Xv2 with pipeline: More complex handling
        
        // 2a. Empty the pipeline first
        EmptyPipeline(ctx, prof);
        
        // 2b. Set up single instruction execution
        SetupSingleInstructionExecution(ctx, prof);
        
        // 2c. Execute and wait
        ExecuteSingleInstruction(ctx, prof);
    }
    
    // 3. Halt CPU after step
    HaltCpuAfterStep(ctx, prof);
    
    // 4. Read new register values
    ReadAllCpuRegisters(ctx, prof, registers_after);
    
    // 5. Update context
    ctx.pc_ = registers_after[0];
    ctx.sr_ = registers_after[2];
    
    // 6. Check for interrupt entry/exit
    UpdateInterruptState(ctx, prof);
    
    return true;
}
```

## 4. Complete Breakpoint Manager Class

```cpp
class HardwareBreakpointManager {
public:
    HardwareBreakpointManager(ITapDev &tap_dev);
    
    // Core operations
    bool SetBreakpoint(address_t address, BreakpointType type);
    bool ClearBreakpoint(address_t address);
    bool ClearAllBreakpoints();
    
    // Execution control
    bool RunToBreakpoint(CpuContext &ctx, const ChipProfile &prof);
    bool SingleStep(CpuContext &ctx, const ChipProfile &prof);
    bool ContinueExecution(CpuContext &ctx, const ChipProfile &prof);
    
    // State query
    DeviceStatus GetDeviceStatus(CpuContext &ctx, const ChipProfile &prof);
    bool GetRegisters(CpuContext &ctx, const ChipProfile &prof, uint32_t *regs);
    
    // Breakpoint info
    int GetActiveBreakpointCount() const;
    const std::vector<BreakpointInfo> &GetBreakpoints() const;
    
private:
    // Internal implementation
    bool ConfigureTriggerBlock(uint8_t block_idx, 
                              address_t address,
                              BreakpointType type,
                              const ChipProfile &prof);
    
    bool UpdateEemBreakpoints(const ChipProfile &prof);
    void ClearTriggerBlock(uint8_t block_idx);
    
    // State tracking
    struct TriggerAllocation {
        uint8_t block_idx;
        address_t address;
        BreakpointType type;
        bool enabled;
    };
    
    ITapDev &tap_dev_;
    std::vector<TriggerAllocation> allocations_;
    std::bitset<8> used_blocks_;  // Max 8 hardware breakpoints
    uint32_t current_trigger_status_;
};
```

## 5. Practical Implementation Example

### 5.1 Complete Debug Session Example

```c
void DebugSessionExample() {
    // Initialize
    TapMcu tap_mcu;
    ChipProfile prof;
    CpuContext ctx;
    
    // 1. Open connection to device
    if (!tap_mcu.Open()) {
        printf("Failed to open device\n");
        return;
    }
    
    // 2. Get device information
    prof = tap_mcu.GetChipProfile();
    
    // 3. Halt CPU and enter JTAG control
    if (!HaltCpuAndEnterJtagControl(ctx, prof)) {
        printf("Failed to halt CPU\n");
        return;
    }
    
    // 4. Program breakpoint at address 0x4400
    if (!ProgramHardwareBreakpoint(0x4400, BP_CODE, prof)) {
        printf("Failed to set breakpoint\n");
        return;
    }
    
    // 5. Release CPU to run
    ReleaseCpuWithBreakpoints(ctx, prof, true);
    printf("CPU released, waiting for breakpoint...\n");
    
    // 6. Poll for breakpoint hit
    DeviceStatus status;
    do {
        usleep(10000);  // 10ms poll interval
        status = QueryCpuStateAndDetectBreakpoint(ctx, prof);
    } while (status == DEVICE_STATUS_RUNNING);
    
    if (status == DEVICE_STATUS_HALTED) {
        printf("Breakpoint hit!\n");
        
        // 7. Load registers
        uint32_t regs[DEVICE_NUM_REGS];
        if (HandleBreakpointHitAndLoadRegisters(ctx, prof, regs)) {
            printf("PC: 0x%08lX, SR: 0x%04lX\n", regs[0], regs[2]);
            
            // 8. Single step
            uint32_t regs_after[DEVICE_NUM_REGS];
            if (SingleStepToNextInstruction(ctx, prof, regs_after)) {
                printf("Stepped to PC: 0x%08lX\n", regs_after[0]);
            }
        }
    }
    
    // 9. Cleanup
    tap_mcu.Close();
}
```

### 5.2 Multi-Breakpoint Management

```c
class AdvancedBreakpointManager {
public:
    // Complex breakpoint: Stop when variable reaches value
    bool SetDataWatchpoint(address_t var_addr, uint16_t value) {
        // Use two trigger blocks:
        // 1. Address match on variable
        // 2. Data match on value
        // Combine with AND trigger
        
        uint8_t addr_trigger = AllocateTrigger();
        uint8_t data_trigger = AllocateTrigger();
        
        if (addr_trigger == 0xFF || data_trigger == 0xFF) {
            return false;
        }
        
        // Configure address trigger
        WriteEemRegister(TRIGGER_BLOCK_BASE(addr_trigger) + TB_TRIGGER_CTRL,
                         TRIGGER_CTRL_ENABLE | 
                         TRIGGER_CTRL_WRITE |
                         TRIGGER_CTRL_ON_CPU);
        WriteEemRegister(TRIGGER_BLOCK_BASE(addr_trigger) + TB_VALUE, var_addr);
        
        // Configure data trigger  
        WriteEemRegister(TRIGGER_BLOCK_BASE(data_trigger) + TB_TRIGGER_CTRL,
                         TRIGGER_CTRL_ENABLE | 
                         TRIGGER_CTRL_ON_MDB);
        WriteEemRegister(TRIGGER_BLOCK_BASE(data_trigger) + TB_VALUE, value);
        
        // Combine with AND in combination register
        uint32_t comb_mask = (1 << addr_trigger) | (1 << data_trigger);
        WriteEemRegister(EEM_COMBINATION0, comb_mask);
        
        // Set CPU stop on combination
        WriteEemRegister(EEM_CPU_STOP_COMB, 1 << 0);  // Stop on combination 0
        
        return true;
    }
    
private:
    uint8_t AllocateTrigger() {
        for (uint8_t i = 0; i < 8; i++) {
            if (!used_blocks_.test(i)) {
                used_blocks_.set(i);
                return i;
            }
        }
        return 0xFF;  // No free triggers
    }
};
```

## 6. EEM Register Definitions

Key EEM registers for breakpoint management:

```c
// EEM Control Registers (device-dependent addresses)
#define EEM_CTRL                0x180
#define EEM_STATUS              0x182
#define EEM_TRIGGER_STATUS      0x184
#define EEM_CPU_STOP            0x186
#define EEM_CLK_CTRL0           0x188
#define EEM_MCLK_CTRL           0x18A

// Trigger Block Registers (offsets from base)
#define TB_TRIGGER_CTRL         0x00
#define TB_VALUE                0x02
#define TB_MASK                 0x04
#define TB_COMBINATION          0x06

// Trigger Control Bits
#define TRIGGER_CTRL_ENABLE     0x8000
#define TRIGGER_CTRL_FETCH      0x4000
#define TRIGGER_CTRL_READ       0x2000
#define TRIGGER_CTRL_WRITE      0x1000
#define TRIGGER_CTRL_ON_CPU     0x0800
#define TRIGGER_CTRL_ON_DMA     0x0400
#define TRIGGER_CTRL_ON_MAB     0x0200
#define TRIGGER_CTRL_ON_MDB     0x0100

// Combination Registers
#define EEM_COMBINATION0        0x1C0
#define EEM_COMBINATION1        0x1C2
// ... up to EEM_COMBINATION9

#define EEM_CPU_STOP_COMB       0x1E0
```

## 7. Architecture-Specific Considerations

### 7.1 Original MSP430 (F1xx/F2xx/F4xx)

```c
// Simple pipeline, direct breakpoint implementation
bool SetupBreakpointOriginalMSP430(address_t addr) {
    // 1 trigger = 1 breakpoint
    WriteEemRegister(TB0_TRIGGER_CTRL, 
                     TRIGGER_CTRL_ENABLE | 
                     TRIGGER_CTRL_FETCH |
                     TRIGGER_CTRL_ON_CPU);
    WriteEemRegister(TB0_VALUE, addr);
    WriteEemRegister(EEM_CPU_STOP, 0x0001);
    
    return true;
}
```

### 7.2 MSP430Xv2 (F5xx/F6xx)

```c
// Pipeline-aware breakpoint handling
bool SetupBreakpointXv2(address_t addr, const ChipProfile &prof) {
    // Xv2 has pipeline - breakpoint hits after instruction completes
    
    // 1. Empty pipeline first
    uint16_t sig_reg = ReadJtagControlSignal();
    if (sig_reg & 0x0001) {  // CPUSUSP bit
        // Pipeline already empty
    } else {
        // Need to empty pipeline
        WriteJtagControlSignal(0x0100);  // Set CPUSUSP
        WaitForPipelineEmpty();
    }
    
    // 2. Set breakpoint
    WriteEemRegister(TB0_TRIGGER_CTRL,
                     TRIGGER_CTRL_ENABLE |
                     TRIGGER_CTRL_FETCH |
                     TRIGGER_CTRL_ON_CPU);
    WriteEemRegister(TB0_VALUE, addr);
    WriteEemRegister(EEM_CPU_STOP, 0x0001);
    
    // 3. Resume pipeline
    WriteJtagControlSignal(0x0000);  // Clear CPUSUSP
    
    return true;
}
```

## 8. Error Handling and Recovery

### 8.1 Common Error Conditions

```c
enum BreakpointError {
    BP_OK = 0,
    BP_NO_TRIGGERS_AVAILABLE,
    BP_ADDRESS_OUT_OF_RANGE,
    BP_DEVICE_NOT_HALTED,
    BP_EEM_NOT_SUPPORTED,
    BP_REGISTER_ACCESS_FAILED,
    BP_TIMEOUT
};

BreakpointError ValidateBreakpointSetup(address_t addr, 
                                       BreakpointType type,
                                       const ChipProfile &prof) {
    // Check address range
    if (addr < prof.ram_start_ || addr > prof.main_end_) {
        if (!(addr >= prof.info_start_ && addr <= prof.info_end_)) {
            return BP_ADDRESS_OUT_OF_RANGE;
        }
    }
    
    // Check available resources
    if (prof.n_breakpoints_used >= prof.max_breakpoints) {
        return BP_NO_TRIGGERS_AVAILABLE;
    }
    
    // Check EEM support
    if (prof.eem_version_ == EEM_VERSION_NONE) {
        return BP_EEM_NOT_SUPPORTED;
    }
    
    return BP_OK;
}
```

### 8.2 Recovery Procedures

```c
bool RecoverFromBreakpointError(CpuContext &ctx, const ChipProfile &prof) {
    // 1. Attempt to re-establish JTAG control
    if (!SyncJtagAssertPorSaveContext(ctx, prof)) {
        // Hard reset may be needed
        if (!PerformHardReset(prof)) {
            return false;  // Unrecoverable
        }
    }
    
    // 2. Clear all breakpoints
    WriteEemRegister(EEM_CPU_STOP, 0x0000);
    WriteEemRegister(EEM_CPU_STOP_COMB, 0x0000);
    
    // Reset all trigger blocks
    for (int i = 0; i < 8; i++) {
        WriteEemRegister(TRIGGER_BLOCK_BASE(i) + TB_TRIGGER_CTRL, 0x0000);
    }
    
    // 3. Reset breakpoint tracking
    prof.n_breakpoints_used = 0;
    
    return true;
}
```

## 9. Performance Optimization

### 9.1 Minimizing Debug Overhead

```c
class OptimizedBreakpointManager {
public:
    // Batch breakpoint operations
    bool SetMultipleBreakpoints(const std::vector<BreakpointRequest> &requests,
                               CpuContext &ctx,
                               const ChipProfile &prof) {
        // 1. Halt once for all operations
        if (!HaltCpuAndEnterJtagControl(ctx, prof)) {
            return false;
        }
        
        // 2. Configure all triggers in single JTAG session
        TapPlayer::BeginTransaction();
        
        for (const auto &req : requests) {
            uint8_t block = AllocateTrigger();
            if (block == 0xFF) break;
            
            ConfigureTriggerBlock(block, req.address, req.type, prof);
        }
        
        TapPlayer::EndTransaction();
        
        // 3. Release once
        ReleaseCpuWithBreakpoints(ctx, prof, true);
        
        return true;
    }
    
    // Cache frequently accessed registers
    bool CacheRegisters(CpuContext &ctx, const ChipProfile &prof) {
        if (!reg_cache_valid_) {
            if (!ReadAllCpuRegisters(ctx, prof, reg_cache_)) {
                return false;
            }
            reg_cache_valid_ = true;
        }
        return true;
    }
    
private:
    uint32_t reg_cache_[DEVICE_NUM_REGS];
    bool reg_cache_valid_ = false;
};
```

## 10. Testing and Validation

### 10.1 Breakpoint Test Suite

```c
void RunBreakpointTests(TapMcu &tap_mcu) {
    ChipProfile prof = tap_mcu.GetChipProfile();
    CpuContext ctx;
    
    printf("Testing breakpoint functionality on %s\n", prof.name_);
    
    // Test 1: Simple code breakpoint
    printf("Test 1: Code breakpoint...\n");
    if (TestCodeBreakpoint(0x4400, tap_mcu, ctx, prof)) {
        printf("  PASS\n");
    } else {
        printf("  FAIL\n");
    }
    
    // Test 2: Data write breakpoint
    printf("Test 2: Data write breakpoint...\n");
    if (TestDataBreakpoint(0x2400, tap_mcu, ctx, prof)) {
        printf("  PASS\n");
    } else {
        printf("  FAIL\n");
    }
    
    // Test 3: Single step
    printf("Test 3: Single step...\n");
    if (TestSingleStep(tap_mcu, ctx, prof)) {
        printf("  PASS\n");
    } else {
        printf("  FAIL\n");
    }
    
    // Test 4: Multiple breakpoints
    printf("Test 4: Multiple breakpoints...\n");
    if (TestMultipleBreakpoints(tap_mcu, ctx, prof)) {
        printf("  PASS\n");
    } else {
        printf("  FAIL\n");
    }
}
```

## Summary

The hardware breakpoint management workflow for MSP430 involves:

1. **Halt and Control**: Stop CPU, enter JTAG control, save context
2. **Breakpoint Programming**: Configure EEM trigger blocks for address/value matching
3. **Execution Release**: Restart CPU with breakpoints active
4. **Breakpoint Detection**: Poll EEM status for trigger activation
5. **State Capture**: Halt CPU, read registers, save context
6. **Single Stepping**: Execute one instruction at a time

Key implementation details:
- Device-specific EEM capabilities determine available features
- Pipeline handling differs between MSP430 architectures
- Combination triggers enable complex breakpoint conditions
- Proper error recovery ensures robust debugging sessions

This workflow forms the foundation for MSP430 debugger implementations in tools like MSPDebug, Code Composer Studio, and the glossy-msp430 project.