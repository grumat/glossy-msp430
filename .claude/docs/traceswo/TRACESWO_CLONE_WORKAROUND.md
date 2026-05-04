# TRACESWO Workaround for Geehy APM32F103CB (STM32F103 Clone)

## Problem Statement

When enabling TRACESWO on Geehy APM32F103CB (an STM32F103CB clone), the trace signal doesn't appear on PB3. Unlike original STM32 chips where the debug unit automatically controls the GPIO pin, clone chips require manual GPIO configuration.

## Original vs Clone Behavior

### Original STM32F103
- Debug unit takes full control of PB3 when TRACESWO is enabled
- GPIO configuration in `Floating` mode works correctly
- No manual output configuration needed

### Geehy APM32F103CB (Clone)
- Debug unit doesn't automatically configure GPIO output
- PB3 must be explicitly configured as an output pin
- Manual GPIO initialization required before enabling TRACESWO

## Implementation Examples

### Standard STM32 Configuration
```c
// In platform.h (for original STM32)
using TRACESWO = TRACESWO_PB3;  // Defined as Floating<Port::PB, 3, AfSwd3>
```

### Geehy Clone Workaround
```c
// In platform.h (for Geehy APM32F103CB)
#define OPT_GEEGY_APM32F103CB 1

// PB3 configured as fast output, initially high
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
```

## Complete Implementation

### 1. Platform Configuration
```c
// target.stlinv2/platform.h lines 107-133
#if OPT_GEEGY_APM32F103CB
// TRACESWO does not work reliably on Geehy MCU, without configuring the GPIO as output
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
#else
// Debugger of STM32F103 simply takes full control of GPIO, so this pin is passive
using TRACESWO = TRACESWO_PB3;
#endif
```

### 2. Pin Definitions Explained

**TRACESWO_PB3** (standard STM32):
```c
// Defined in bmt/include/f1xx/gpio-types.h:854
using TRACESWO_PB3 = Floating<Port::PB, 3, AfSwd3>;
```
- Configures PB3 as floating input with SWD alternate function
- `AfSwd3` sets AFIO to `AFIO_MAPR_SWJ_CFG_JTAGDISABLE` (2-pin SWD with optional SWO)

**AnyOut** (Geehy workaround):
```c
// Configures PB3 as push-pull output, fast speed, initially high
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
```
- Explicitly sets PB3 as output pin
- Fastest slew rate for trace signals
- Initial high level ensures clean signal start

### 3. Port Configuration
```c
// In platform.h PORTB configuration
typedef AnyPortSetup <Port::PB
    , JRST_Init             // bit bang
    , JTEST_Init            // bit bang  
    , Unused<2>             // STM32 BOOT1
    , TRACESWO              // ARM trace pin (varies based on OPT_GEEGY_APM32F103CB)
    // ... rest of pins
> PORTB;
```

## Underlying BMT Library Differences

### Floating Input (Original)
```c
class Floating : public AnyIn<kPort, kPin, PuPd::kFloating, Map>
{
    // Configured as input with floating pull-up/down
    // Mode::kInput, Speed::kInput
};
```

### AnyOut Output (Clone)
```c
class AnyOut : public Private::Implementation_<
    Private::Impl::kNormal
    , kPort
    , kPin
    , Mode::kOutput          // Critical difference!
    , kSpeed
    , PuPd::kFloating
    , kLevel
    , Map
>
{
    // Configured as push-pull output
    // Mode::kOutput allows signal generation
};
```

## TRACESWO Initialization Sequence

The trace initialization in `bmt/include/trace.h` still works correctly with either configuration:

```c
// SwoTraceSetup::Init() - Line 174
DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN; // Enable IO trace pins
```

For Geehy clones, the GPIO is already configured as output before this point.

## Testing Procedure

1. **Confirm pin is working**: Use logic analyzer on PB3
2. **Verify signal quality**: Check for clean square wave at configured bitrate
3. **Test with debugger**: Ensure OpenOCD or ST-Link Utility can capture trace data
4. **Validate data**: Send known patterns via ITM and verify reception

## Troubleshooting Checklist

- [ ] `OPT_GEEGY_APM32F103CB` is defined as 1 for clone chips
- [ ] PB3 is configured as `AnyOut` (not `Floating`) for clones
- [ ] `DBGMCU_CR_TRACE_IOEN` is set during trace initialization  
- [ ] AFIO is configured for SWD mode (`AFIO_MAPR_SWJ_CFG_JTAGDISABLE`)
- [ ] Debug probe supports SWO trace capture
- [ ] Bitrate matches between MCU and debugger configuration

## Notes for Other Clone MCUs

This workaround may be necessary for other STM32 clones from:
- Geehy (APM32 series)
- MindMotion (MM32 series)  
- Puya (PY32 series)
- Any clone where debug features don't fully match ST originals

Always refer to the specific clone's datasheet and errata for debug interface limitations.