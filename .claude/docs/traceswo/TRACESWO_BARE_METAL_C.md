# TRACESWO Initialization - Bare Metal C Implementation

## Overview

This document explains how to initialize TRACESWO (Trace Serial Wire Output) on STM32 microcontrollers using plain C code, without template metaprogramming libraries. It covers the actual register operations that need to be performed.

## Required Header Files

```c
#include <stdint.h>
#include "stm32f1xx.h"  // Or appropriate header for your MCU
```

## 1. GPIO Configuration for PB3

### For Original STM32 Chips
```c
// PB3 is configured as Alternate Function Push-Pull, 50MHz
// This is what Floating<Port::PB, 3, AfSwd3> expands to:

// Enable GPIOB clock
RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

// Configure PB3: CNF = 10 (Alternate Function Output Push-pull)
// MODE = 11 (Output mode, max speed 50 MHz)
GPIOB->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
GPIOB->CRL |= (0x2 << GPIO_CRL_CNF3_Pos) | (0x3 << GPIO_CRL_MODE3_Pos);

// No pull-up/pull-down (floating)
GPIOB->ODR &= ~GPIO_ODR_ODR3;
```

### For Geehy APM32F103CB (Clone)
```c
// PB3 must be explicitly configured as General Purpose Output
// This is what AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh> expands to:

// Enable GPIOB clock
RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

// Configure PB3: CNF = 00 (General purpose output Push-pull)
// MODE = 11 (Output mode, max speed 50 MHz)
GPIOB->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
GPIOB->CRL |= (0x0 << GPIO_CRL_CNF3_Pos) | (0x3 << GPIO_CRL_MODE3_Pos);

// Set initial output to high
GPIOB->ODR |= GPIO_ODR_ODR3;
```

### AFIO Configuration (STM32F1 Specific)
```c
// Enable AFIO clock
RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

// Configure SWJ debug port: JTAG disabled, SWD enabled with 3 pins
// This corresponds to AfSwd3 = AFIO_MAPR_SWJ_CFG_JTAGDISABLE
AFIO->MAPR &= ~AFIO_MAPR_SWJ_CFG_Msk;
AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
// Value: 0x02000000
```

## 2. CoreSight Debug Registers Initialization

### Enable Debug Trace
```c
// Enable trace access in Debug Exception and Monitor Control Register
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
// Address: 0xE000EDFC, Bit 24
```

### Trace Port Interface (TPI) Configuration
```c
// Set protocol width to 1 bit (async mode)
TPI->CSPSR = 0x00000001;  // Current Supported Port Sizes Register

// Set protocol: 1=Manchester, 2=Async (recommended for SWO)
TPI->SPPR = 0x00000002;   // Selected Pin Protocol Register

// Set bitrate prescaler for async mode
// Formula: ACPR = (SystemClock / DesiredBitrate) - 1
// Example for 72MHz system clock, 2Mbps bitrate:
uint32_t system_clock = 72000000;
uint32_t desired_bitrate = 2000000;
TPI->ACPR = (system_clock / desired_bitrate) - 1;  // Async Clock Prescaler Register

// Disable formatter, discard ETM output
TPI->FFCR = 0x00000000;   // Formatter and Flush Control Register
```

### ITM (Instrumentation Trace Macrocell) Initialization
```c
// Unlock ITM registers (required for write access)
ITM->LAR = 0xC5ACCE55;    // Lock Access Register

// Enable ITM and SWO output
ITM->TCR = (1UL << 0)     // ITM Enable bit
         | (1UL << 22);   // SWO Enable bit
// Trace Control Register bits:
// Bit 0: ITMENA - ITM enable
// Bit 22: SWOENA - SWO output enable
// Bit 16: BUSY - indicates FIFO not empty
// Bits 3-0: TraceBusID - ATB ID

// Disable privilege filtering (allow all accesses)
ITM->TPR = 0x00000000;    // Trace Privilege Register

// Enable specific stimulus ports (channels)
// Example: Enable channel 0 for general debug output
ITM->TER = (1UL << 0);    // Trace Enable Register
// Enable more channels as needed: TER |= (1 << channel_number)
```

### DBGMCU Configuration (MCU Specific)
```c
// Enable trace I/O pins on the microcontroller
DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
// STM32F1: DBGMCU_CR = 0xE0042004
// Bit 5: TRACE_IOEN - Trace I/O enable
// Also available in newer STM32 as DBGMCU_APB1_FZ, DBGMCU_APB2_FZ
```

## 3. Complete Initialization Function

```c
void TRACESWO_Init(uint32_t system_clock_hz, uint32_t baud_rate, uint8_t is_clone_mcu)
{
    // 1. Configure GPIOB clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    
    // 2. Configure PB3 based on MCU type
    if (is_clone_mcu) {
        // For clone MCUs (Geehy APM32F103CB)
        GPIOB->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
        GPIOB->CRL |= (0x0 << GPIO_CRL_CNF3_Pos) | (0x3 << GPIO_CRL_MODE3_Pos);
        GPIOB->ODR |= GPIO_ODR_ODR3;  // Set high
    } else {
        // For original STM32
        GPIOB->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
        GPIOB->CRL |= (0x2 << GPIO_CRL_CNF3_Pos) | (0x3 << GPIO_CRL_MODE3_Pos);
        GPIOB->ODR &= ~GPIO_ODR_ODR3;  // Clear (floating)
    }
    
    // 3. Configure AFIO for SWD mode
    AFIO->MAPR &= ~AFIO_MAPR_SWJ_CFG_Msk;
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;  // 2-pin SWD with SWO
    
    // 4. Enable CoreSight trace
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    // 5. Configure TPI
    TPI->CSPSR = 0x00000001;        // 1-bit protocol width
    TPI->SPPR = 0x00000002;         // Async protocol
    TPI->ACPR = (system_clock_hz / baud_rate) - 1;
    TPI->FFCR = 0x00000000;         // Disable formatter
    
    // 6. Configure ITM
    ITM->LAR = 0xC5ACCE55;          // Unlock
    ITM->TCR = (1UL << 0) | (1UL << 22);  // ITMENA | SWOENA
    ITM->TPR = 0x00000000;          // No privilege filtering
    ITM->TER = 0x00000001;          // Enable channel 0
    
    // 7. Enable trace in DBGMCU
    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
}
```

## 4. ITM Output Functions

### Single Character Output
```c
void ITM_SendChar(uint8_t ch, uint8_t channel)
{
    // Check if ITM and channel are enabled
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) &&   // ITM enabled
        (ITM->TER & (1UL << channel)))       // Channel enabled
    {
        // Wait until FIFO not full
        while (ITM->PORT[channel].u32 == 0UL) {
            __NOP();  // Or implement timeout
        }
        // Write character to stimulus port
        ITM->PORT[channel].u8 = ch;
    }
}
```

### String Output
```c
void ITM_SendString(const char* str, uint8_t channel)
{
    while (*str) {
        ITM_SendChar(*str++, channel);
    }
}
```

### 32-bit Word Output (Efficient for multiple chars)
```c
void ITM_SendBuffer(const uint8_t* data, uint32_t len, uint8_t channel)
{
    uint32_t word = 0;
    uint32_t shift = 0;
    
    while (len--) {
        word |= (uint32_t)(*data++) << shift;
        shift += 8;
        
        if (shift == 32) {
            while (ITM->PORT[channel].u32 == 0UL) __NOP();
            ITM->PORT[channel].u32 = word;
            word = 0;
            shift = 0;
        }
    }
    
    // Send remaining bytes
    if (shift > 0) {
        while (ITM->PORT[channel].u32 == 0UL) __NOP();
        ITM->PORT[channel].u32 = word;
    }
}
```

## 5. Memory Mapped Register Addresses

These are fixed addresses in the ARM CoreSight memory map:

```c
// CoreSight Component addresses (same across all Cortex-M)
#define ITM_BASE   ((uint32_t)0xE0000000)
#define TPI_BASE   ((uint32_t)0xE0040000)
#define CoreDebug_BASE ((uint32_t)0xE000EDF0)

// Register pointers
ITM_Type* ITM = (ITM_Type*)ITM_BASE;
TPI_Type* TPI = (TPI_Type*)TPI_BASE;
CoreDebug_Type* CoreDebug = (CoreDebug_Type*)CoreDebug_BASE;

// MCU-specific (STM32F1 example)
#define DBGMCU_BASE ((uint32_t)0xE0042000)
DBGMCU_Type* DBGMCU = (DBGMCU_Type*)DBGMCU_BASE;
```

## 6. Typical Usage Example

```c
int main(void)
{
    // System initialization
    SystemClock_Config();  // Configure 72MHz clock
    
    // Initialize TRACESWO for 2Mbps
    // Third parameter: 1 for clone MCUs, 0 for original STM32
    TRACESWO_Init(72000000, 2000000, 1);
    
    // Send test message
    ITM_SendString("TRACESWO Initialized\r\n", 0);
    
    while (1) {
        // Application code
        ITM_SendString("Hello from TRACESWO\r\n", 0);
        Delay_ms(1000);
    }
}
```

## 7. Black Magic Probe (BMP) Configuration

The Glossy MSP430 project is an implementation of Black Magic Debug for MSP430 targets. When using BMP or BMP-compatible debugging, configure as follows:

### GDB Startup Commands for BMP
```gdb
# Connect to BMP on COM port (adjust COM port as needed)
target extended-remote COM4

# Allow memory reads/writes to all regions (required for some MSP430 chips)
set mem inaccessible-by-default off

# Scan for SWD devices
monitor swd_scan

# Enable SWO trace with UART decoding at 720000 baud
monitor swo enable uart 720000 decode

# Attach to device #1 (first device found by swd_scan)
attach 1

# Load the firmware
load
```

### Complete BMP Configuration Script
Save this as `bmp_trace.gdb`:
```gdb
# Black Magic Probe TRACESWO configuration
target extended-remote COM4
set mem inaccessible-by-default off

# Reset and halt the target  
monitor connect_srst enable
monitor jtag_scan
monitor swd_scan

# Configure TRACESWO
# Format: monitor swo enable <mode> <baudrate> <decode>
# mode: uart (async), manchester, raw
# decode: decode (parse as ASCII), nodecode (raw bytes)
monitor swo enable uart 720000 decode

# Select device and halt
attach 1
monitor halt

# Enable ITM channels in target (if not done in firmware)
# This writes to target's ITM->TER register
monitor mww 0xE0000E00 0x00000001  # Enable channel 0

# Set up trace file
set trace-commands on
set trace-file trace.log
tstart
```

### BMP Monitor Commands Reference
```gdb
# Basic BMP commands
monitor help                          # List all available commands
monitor swd_scan                      # Scan for SWD devices
monitor jtag_scan                     # Scan for JTAG devices
monitor swo                           # Show SWO status
monitor swo enable uart <baud> decode # Enable SWO trace
monitor swo disable                   # Disable SWO trace
monitor swo pins                      # Show SWO pin configuration
monitor swo frequency                 # Show current SWO frequency

# Target control
monitor halt                         # Halt target
monitor reset                        # Reset target
monitor step                         # Single step
monitor mww <addr> <value>          # Write 32-bit word
monitor mwh <addr> <value>          # Write 16-bit halfword
monitor mwb <addr> <value>          # Write 8-bit byte
```

### OpenOCD with BMP (Alternative)
```tcl
source [find interface/cmsis-dap.cfg]
transport select swd

# BMP acts as CMSIS-DAP adapter
adapter speed 4000

source [find target/stm32f1x.cfg]

# Configure TRACESWO
tpiu config internal uart off 72000000 2000000
itm port 0 on
```

### ST-Link Utility with SWO
- Enable "Serial Wire Viewer" (SWV)
- Set correct baud rate (must match TPI->ACPR calculation)
- Configure ITM stimulus ports
- BMP compatible mode may require CMSIS-DAP configuration

## 8. Troubleshooting

### No Signal on PB3
1. Verify GPIO configuration (output mode for clones)
2. Check `DBGMCU->CR` has `TRACE_IOEN` bit set
3. Verify AFIO configuration releases PB3 from JTAG

### Data Received but Corrupted
1. Check bitrate calculation matches debugger setting
2. Verify system clock frequency is correct
3. Try lower bitrate (1Mbps) for testing

### ITM Port Always Shows Full
1. Debugger may not be configured for SWO
2. Check ITM->TCR has both ITMENA and SWOENA bits set
3. Verify ITM->TER has your channel enabled

## 9. Important Notes

1. **Bitrate Accuracy**: Async mode requires accurate clock source (use HSE not HSI)
2. **Channel Selection**: ITM has 32 stimulus ports (0-31), enable only needed ones
3. **FIFO Depth**: ITM has small FIFO, check `ITM->PORT[ch].u32 == 0` before writing
4. **MCU Specific**: DBGMCU registers vary between STM32 families (F1, F4, G4, etc.)
5. **Clock Domain**: CoreSight registers are in DAP domain, not affected by MCU sleep modes

## Appendix: BMT Library Implementation

The Glossy MSP430 project uses the BMT (Bare Metal Templates) library for type-safe, zero-overhead hardware access. Here's how TRACESWO is implemented using BMT's template metaprogramming:

### BMT Template-Based Configuration
```c++
// In platform.h (target.stlinv2/platform.h:107-133)
#if OPT_GEEGY_APM32F103CB
// TRACESWO does not work reliably on Geehy MCU without configuring GPIO as output
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
#else
// Debugger of STM32F103 simply takes full control of GPIO, so this pin is passive
using TRACESWO = TRACESWO_PB3;
#endif
```

### Template Expansions
**TRACESWO_PB3** (standard STM32):
```c++
// Defined in bmt/include/f1xx/gpio-types.h:854
using TRACESWO_PB3 = Floating<Port::PB, 3, AfSwd3>;

// Expands to:
// Floating<Port::PB, 3, AfSwd3>
//   -> AnyIn<Port::PB, 3, PuPd::kFloating, AfSwd3>
//   -> Implementation_<kNormal, Port::PB, 3, Mode::kInput, Speed::kInput, 
//                     PuPd::kFloating, Level::kLow, AfSwd3>
```

**AnyOut** (Geehy clone workaround):
```c++
// Expands to:
// AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>
//   -> Implementation_<kNormal, Port::PB, 3, Mode::kOutput, Speed::kFastest,
//                     PuPd::kFloating, Level::kHigh, AfNoRemap>
```

### BMT Trace Initialization
```c++
// From bmt/include/trace.h
template<
    typename SysClk
    , const SwoProtocol proto
    , const uint32_t bitrate
    , typename Ch0
    , typename Ch1 = SwoDummyChannel
    // ... channels 2-31
>
class SwoTraceSetup
{
public:
    ALWAYS_INLINE static void Init()
    {
        if(kChannelMask_ != 0)
        {
            CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;
            TPI->CSPSR = 1;
            TPI->SPPR = uint32_t(kProto_);
            if (kProto_ == SwoProtocol::kManchester)
                TPI->ACPR = SysClk::kFrequency_ / (2 * bitrate) - 1;
            else
                TPI->ACPR = SysClk::kFrequency_ / bitrate - 1;
            TPI->FFCR = 0;
            ITM->LAR = 0xC5ACCE55;
            ITM->TCR = ITM_TCR_SWOENA_Msk | ITM_TCR_ITMENA_Msk;
            ITM->TPR = 0;
            ITM->TER = kChannelMask_;
            DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
        }
    }
};
```

### BMT SWO Channel Template
```c++
template <const int8_t channel>
class SwoChannel
{
public:
    static void PutChar(char ch) NO_INLINE
    {
        if(IsTracing())
        {
            word_ |= (uint32_t)ch << shift_;
            shift_ += 8;
            if (shift_ == 32)
            {
                while (ITM->PORT[kChannel_].u32 == 0UL)
                    __NOP();
                ITM->PORT[kChannel_].u32 = word_;
                Init();
            }
        }
    }
};
```

### Advantages of BMT Approach
1. **Type Safety**: Compile-time checking of pin configurations
2. **Zero Runtime Overhead**: All configuration resolved at compile time
3. **Consistency**: Uniform API across different STM32 families (F1, F4, G4, L4)
4. **Error Prevention**: Template constraints prevent invalid configurations
5. **Readability**: Semantic naming (`Floating`, `AnyOut`, `Speed::kFastest`)

### Using BMT in Glossy MSP430
```c++
// System clock configuration
typedef Clocks::AnyHse<8000000UL> HSE;
typedef Clocks::AnyPll<HSE, 72000000UL> PLL;
typedef Clocks::AnySycClk<PLL, Clocks::AhbPrscl::k1> SysClk;

// SWO channel for ITM
using SwoChannel0 = SwoChannel<0>;

// Complete trace setup
typedef SwoTraceSetup<SysClk, SwoProtocol::kAsynchronous, 2000000, SwoChannel0> Trace;

// Initialization (called from main)
Trace::Init();
SwoChannel0::Init();

// Usage
SwoChannel0::PutS("Debug message\r\n");
```

### Relationship to Bare Metal C
The BMT templates compile down to the exact same register operations shown in this document's main sections. The template metaprogramming provides:
- **Abstraction**: Hides register bit manipulation details
- **Portability**: Same code works across MCU families
- **Safety**: Compile-time validation prevents runtime errors
- **Maintainability**: Centralized configuration in `platform.h`

For developers who prefer explicit register access (for debugging, optimization, or education), the bare metal C implementation in this document shows exactly what the BMT templates generate.

This bare metal implementation replaces the template metaprogramming with explicit register operations that any C programmer can understand and modify.