# TRACESWO Initialization for STM32 Bare Metal C Code

## Overview

TRACESWO (Trace Serial Wire Output) is an ARM CoreSight debug feature that allows real-time trace output from STM32 microcontrollers via the SWD (Serial Wire Debug) interface. This document explains the initialization process for TRACESWO in bare metal C code, including differences between original STM32 and clones like Geehy APM32F103CB.

## Key Components

### 1. ARM CoreSight Registers

TRACESWO uses several ARM CoreSight debug registers:

- **ITM** (Instrumentation Trace Macrocell): Handles stimulus ports for trace output
- **TPI** (Trace Port Interface): Configures trace protocol and bitrate
- **DBGMCU**: MCU-specific debug configuration
- **CoreDebug**: Core debug enable control

### 2. GPIO Configuration

The TRACESWO signal typically uses **PB3** pin. The configuration differs between original STM32 and clones:

#### Original STM32F103 (Standard Configuration):
```c
// PB3 configured as floating input with SWD alternate function
using TRACESWO_PB3 = Floating<Port::PB, 3, AfSwd3>;
```
Where `AfSwd3` maps to `AFIO_MAPR_SWJ_CFG_JTAGDISABLE` (2-pin SWD with optional SWO).

#### Geehy APM32F103CB (Clone Workaround):
```c
// PB3 must be explicitly configured as output pin
#define OPT_GEEGY_APM32F103CB 1
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
```

**Critical Difference**: Original STM32 debug hardware takes full control of the GPIO pin automatically when TRACESWO is enabled. Geehy clones require manual GPIO output configuration.

## Initialization Sequence

### Step 1: Enable Trace Access

Enable trace functionality in the Core Debug unit:

```c
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
```

### Step 2: Configure Trace Port Interface (TPI)

Set up the TPIU (Trace Port Interface Unit):

```c
TPI->CSPSR = 1;                     // Protocol width = 1 bit
TPI->SPPR = protocol;               // 1 = Manchester, 2 = Asynchronous

// Configure bitrate based on protocol
if (protocol == SwoProtocol::kManchester)
    TPI->ACPR = system_clock / (2 * desired_bitrate) - 1;
else
    TPI->ACPR = system_clock / desired_bitrate - 1;

TPI->FFCR = 0;                      // Disable formatter, discard ETM output
```

### Step 3: Unlock and Configure ITM

Unlock and enable the ITM (Instrumentation Trace Macrocell):

```c
ITM->LAR = 0xC5ACCE55;              // Unlock access to ITM registers
ITM->TCR = ITM_TCR_SWOENA_Msk | ITM_TCR_ITMENA_Msk;
ITM->TPR = 0;                       // Privileged access off
ITM->TER = channel_mask;            // Enable specific stimulus channels
```

### Step 4: Enable Trace IO Pins

Enable the TRACESWO output pin in the debug MCU configuration:

```c
DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN; // Enable IO trace pins
```

### Step 5: Configure SWJ Debug Port (STM32F1 Specific)

For STM32F1 series, configure AFIO to enable SWD and release TRACESWO:

```c
RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO clock
AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE; // Disable JTAG, enable SWD
```

## Complete Initialization Code Example

Here's a complete initialization template using the BMT library:

```c
#include "bmt.h"
#include "trace.h"
#include "f1xx/gpio-types.h"

using namespace Bmt;

// System clock configuration
typedef Clocks::AnyHse<8000000UL> HSE;
typedef Clocks::AnyPll<HSE, 72000000UL> PLL;
typedef Clocks::AnySycClk<PLL> SysClk;

// Define SWO channel for ITM stimulus port 0
using SwoChannel0 = SwoChannel<0>;

// Configure TRACESWO setup
typedef SwoTraceSetup<
    SysClk,                     // System clock
    SwoProtocol::kAsynchronous, // Protocol (async recommended for SWO)
    2000000,                    // Bitrate (2 Mbps)
    SwoChannel0                 // Enabled channels
> TraceConfig;

void InitTraceSwo()
{
    // Configure GPIO pin (Geehy clone specific)
    #ifdef OPT_GEEGY_APM32F103CB
    using TRACESWO_Pin = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
    TRACESWO_Pin::Init();
    #endif
    
    // Initialize trace configuration
    TraceConfig::Init();
    
    // Initialize SWO channels
    SwoChannel0::Init();
}

// Usage example
void TraceOutput(const char* message)
{
    SwoChannel0::PutS(message);
    SwoChannel0::Flush();
}
```

## Troubleshooting Common Issues

### 1. No TRACESWO Signal on Clone MCUs
**Problem**: Geehy APM32F103CB doesn't produce TRACESWO output.
**Solution**: Manually configure PB3 as an output pin before enabling TRACESWO.

### 2. Incorrect Bitrate
**Problem**: Trace data appears corrupted or doesn't synchronize.
**Solution**: Verify system clock frequency matches `SysClk::kFrequency_`. Use async protocol for simpler bitrate calculation.

### 3. No Trace Output
**Problem**: ITM registers show enabled but no data appears.
**Solution**:
- Verify `DBGMCU_CR_TRACE_IOEN` is set
- Check AFIO configuration for SWD mode
- Ensure debugger supports SWO trace capture

### 4. Pin Conflict with JTAG
**Problem**: TRACESWO pin conflicts with JTAG functionality.
**Solution**: Use `AFIO_MAPR_SWJ_CFG_JTAGDISABLE` to disable JTAG and free PB3 for SWO.

## Protocol Selection Guidelines

| Protocol | Pros | Cons | Use Case |
|----------|------|------|----------|
| **Manchester** | Built-in clock recovery, robust | Complex bitrate calculation | Noisy environments |
| **Asynchronous** | Simple bitrate calculation, common | Requires accurate baud rate | Most applications |

**Recommended**: Asynchronous mode at 2 Mbps for typical SWO usage.

## Channel Configuration

ITM provides 32 stimulus channels (0-31). Common usage:
- **Channel 0**: General debug messages
- **Channel 1**: Error/Exception logging  
- **Channel 2**: Performance measurements
- **Channel 31**: System events

Enable only necessary channels to reduce overhead:
```c
ITM->TER = (1 << 0) | (1 << 1); // Enable channels 0 and 1 only
```

## References

- ARM® CoreSight™ Architecture Specification
- STM32F1 Reference Manual (RM0008) - Debug support chapter
- ARM® v7-M Architecture Reference Manual
- BMT Library: `bmt/include/trace.h`
- BMT Library: `bmt/include/f1xx/gpio-types.h`

## Notes for Clone MCUs

Geehy APM32F103CB and similar STM32 clones may require:
1. Explicit GPIO output configuration for TRACESWO
2. Verification of DBGMCU register compatibility
3. Testing with lower bitrates initially (1 Mbps)
4. Checking SWD/SWO pin mapping in chip datasheet

Always test TRACESWO functionality with a logic analyzer or compatible debug probe before relying on it for critical debugging.