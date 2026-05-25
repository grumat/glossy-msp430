# GPIO G4xx — Hardware Features Not Covered by the Library

**File:** `bmt/include/g4xx/gpio.h`  
**Reference:** STM32G4xx Reference Manual RM0440

The current `g4xx/gpio.h` uses 9 of the 10 GPIO registers. One is completely ignored — `LCKR` — and one hardware capability (output-mode pull resistors) is partially exposed but not fully leveraged. This document describes each gap, its hardware basis, and a proposed API.

> **Correction (2026-05-25):** An earlier revision of this note claimed a third
> gap — a missing `ASCR` (analog switch) write that was a "functional bug for
> ADC." **That was wrong: the STM32G4 family has no `ASCR` register at all.**
> The G4 `GPIO_TypeDef` ends at `BRR` (offset 0x28) and `stm32g4xx.h` contains
> zero `ASCR` symbols. The analog switch is an **L4/L5** feature; on G4 a pin in
> analog mode (`MODER=11`) is connected to the ADC/DAC bus directly, so
> `AnyAnalog<Port,Pin>` is already correct for G4 ADC use. See the struck-out
> section below for the (now mostly historical) details — the only real ASCR
> gap is in `l4xx/gpio.h`, and the L4 target has been removed from the solution.

---

## 1. ~~`ASCR` — Analog Switch Control (Functional Bug for ADC)~~ — DOES NOT APPLY TO G4

> **This whole section was based on a false premise and is retained only for
> the record.** The STM32G4 family has **no `ASCR` register** (verified against
> `stm32g431xx.h`: the `GPIO_TypeDef` has no `ASCR` member and the header
> defines no `ASCR` symbols). `ASCR` / the I/O analog switch is an **STM32L4 /
> L5** feature. On G4, `MODER=11` (analog) connects the pad to the ADC/DAC bus
> directly — there is nothing extra to do, and `AnyAnalog<Port,Pin>` works as-is
> for G4 ADC pins. No `kConnectAdc` parameter is needed or possible on G4.
>
> The genuine gap is that `l4xx/gpio.h` likewise never writes `ASCR` (0
> references), so on **L4** `AnyAnalog` would leave the pad disconnected. That
> is the place to add a `kConnectAdc` param — but the L4 target has been removed
> from the solution, so it is not currently actionable.

The original (incorrect, G4-attributed) analysis follows:

### What the hardware does — *(describes L4, not G4)*

On L4/L5, every GPIO pin has an analog switch (`ASC` bit in `GPIOx_ASCR`) that connects or disconnects the I/O pad from the analog bus feeding the ADC/DAC inputs. This switch is **independent of MODER**.

```
         I/O pad
            │
   ┌────────┴──────────────────────────────┐
   │ MODER=11 (analog)                     │
   │  → Schmitt trigger off               │
   │  → digital path disabled             │
   └───────────────────────────────────────┘
            │
         ASC bit (ASCR register)
         ┌──┴──┐
      0=open  1=closed
            │
      ADC/DAC analog input bus
```

| `MODER` | `ASCR` bit | Result |
|---------|-----------|--------|
| 11 (analog) | 0 (default) | Pin is analog (no Schmitt, no leakage), but **NOT connected to ADC**. Minimum capacitance. |
| 11 (analog) | 1 | Pin is connected to the ADC input. This is required to actually sample the pin. |
| 00/01/10 (digital) | 0 | Normal digital I/O. Analog switch open — no effect. |
| 00/01/10 (digital) | 1 | Digital I/O + analog switch closed. ADC can sample while pin is driven digitally (usually undesirable). |

### What the library does today

`AnyAnalog` sets `MODER=11` and forces `PUPDR=0` and `OSPEEDR=0`, but **never touches `ASCR`**. The reset value of `ASCR` is `0` (switch open). This means:

```cpp
using MY_ADC_PIN = AnyAnalog<Port::PA, 0>;
MY_ADC_PIN::Setup();
// MODER = 11 ✅  PUPDR = 00 ✅  ASCR = 0 ← switch is OPEN, ADC cannot sample this pin
```

**On L4 this would be a functional bug for any ADC use case** (pins that feed the ADC must have `ASCR=1`). **It does not apply to G4** — see the correction banner above. The proposed API below would belong in `l4xx/gpio.h`.

For pins parked in analog mode purely to save power (no ADC needed), `ASCR=0` is the correct and desired state — this is the G4xx's way of minimising pad capacitance on unused pins, which is better than F1xx's floating-input parking.

### Proposed API

Add a `bool kConnectAdc` template parameter to `AnyAnalog`, defaulting to `true`:

```cpp
template<
    const Port kPort
    , const uint8_t kPin
    , const bool kConnectAdc = true   // true = connect pad to ADC (ASCR=1)
    , typename Map = AfNoRemap
>
class AnyAnalog : public Private::Implementation_<...>
{};
```

Inside `Implementation_`, add `kASCR_` and `kASCR_Mask_` constants, and write `ASCR` in `Setup()`:

```cpp
static constexpr uint32_t kASCR_ =
    (kImpl == Impl::kUnchanged)         ? 0UL
    : (kMode_ != Mode::kAnalog)         ? 0UL          // irrelevant for digital pins
    /*analog*/                          : (kConnectAdc ? (1UL << kPin) : 0UL);

static constexpr uint32_t kASCR_Mask_ =
    (kImpl == Impl::kUnchanged)         ? ~0UL
    : (kMode_ != Mode::kAnalog)         ? ~0UL
    /*analog*/                          : ~(1UL << kPin);
```

And in `AnyPinGroup`, combine across pins like any other register. `Setup()` / `Enable()` then writes `ASCR` alongside `MODER`.

**Consequence of not fixing this:** All ADC use on G4xx through `AnyAnalog` currently requires the caller to manually set `ASCR` bits after `Setup()`, which defeats the zero-overhead abstraction.

---

## 2. `LCKR` — Pin Configuration Lock

### What the hardware does

`GPIOx_LCKR` locks the configuration of individual pins (their `MODER`, `OTYPER`, `OSPEEDR`, `PUPDR`, `AFR` bits) until the next MCU reset. Once a pin is locked, any subsequent writes to those configuration registers for that pin are silently ignored by the hardware.

The lock is activated by a specific write sequence (write key):
1. Write `LCKR` with `LCK16=1` and the desired pin bits set
2. Write `LCKR` with `LCK16=0` and same pin bits
3. Write `LCKR` with `LCK16=1` and same pin bits
4. Read `LCKR` (optional verify: `LCK16` should now be `1`, confirming lock active)

This is a hardware-enforced one-way door — no software can unlock it.

### Why it matters

For safety-critical applications — motor drives, power converters, fault-output pins, fail-safe signals — accidental reconfiguration of a GPIO by a software bug (wild pointer, stack overflow touching peripheral space, runaway ISR) is a real hazard. Locking pins after startup eliminates this class of fault.

The G4xx is a motor-control-focused MCU (high-resolution timer, comparators, op-amps). Locking PWM output pins and fault-detection inputs after init is idiomatic for this family.

### Proposed API

A `Lock()` static method on any pin type or group:

```cpp
// On a single pin type
MY_PWM_OUT::Lock();

// On a group
using MOTOR_PINS = AnyPinGroup<Port::PA, PH1_OUT, PH2_OUT, PH3_OUT>;
MOTOR_PINS::Lock();
```

Implementation:

```cpp
// In Implementation_ or AnyPin
ALWAYS_INLINE static void Lock()
{
    volatile GPIO_TypeDef& port = Io();
    const uint32_t pin_bit = kBitValue_;
    port.LCKR = (1UL << 16) | pin_bit;   // step 1
    port.LCKR = pin_bit;                  // step 2
    port.LCKR = (1UL << 16) | pin_bit;   // step 3
    (void)port.LCKR;                      // step 4 (read-back)
}
```

For `AnyPinGroup`, combine the `kBitValue_` aggregate:

```cpp
ALWAYS_INLINE static void Lock()
{
    volatile GPIO_TypeDef& port = Io();
    const uint32_t bits = kBitValue_;
    port.LCKR = (1UL << 16) | bits;
    port.LCKR = bits;
    port.LCKR = (1UL << 16) | bits;
    (void)port.LCKR;
}
```

A compile-time variant could also be added: `AnyLockedPortSetup` that calls `Init()` followed immediately by `Lock()`.

---

## 3. Pull Resistors on Output Pins

### What the hardware does

On G4xx, `PUPDR` is fully independent of `MODER`. A push-pull output (`MODER=01`, `OTYPER=0`) can simultaneously have `PUPDR=01` (pull-up) or `PUPDR=10` (pull-down). The pull resistor (~40 kΩ typical) operates in addition to the strong driver.

On F1xx, pull-up/down is only meaningful when the pin is configured as input — for outputs, it is irrelevant because the strong driver overpowers the pull.

### When this is useful on G4xx

**Open-drain output with internal pull-up** (`MODER=01`, `OTYPER=1`, `PUPDR=01`)  
The G4xx's `AnyOutOD` already uses this — it is the right behaviour and removes the need for an external pull-up resistor on many I2C/1-Wire/open-collector buses at moderate speeds.

**Push-pull output with pull-up/down**  
Less common, but useful when the output is shared with an input path (e.g., a bidirectional single-wire bus that the MCU both drives and samples). The pull ensures a defined level when the driver is in a brief tri-state window between transactions.

**Weak drive backup**  
Some bus designs rely on a pull to hold the line when no device is driving. Using the internal pull on a push-pull output guarantees the bus is pulled to a safe level even if the driver is momentarily switching, without needing an external resistor.

### Current state

G4xx's `AnyOut` exposes `kPuPd` as a parameter (position 5). The previous comparison document flagged this as a portability break and recommended removing it. That recommendation is correct for cross-platform code, but the parameter itself represents genuine G4xx hardware capability.

**Proposed resolution**: Keep `kPuPd` in G4xx's `AnyOut` but move it to the last position so it cannot be confused with a positional F1xx call:

```cpp
// Proposed G4xx AnyOut — kPuPd last, safe default
template<
    const Port kPort
    , const uint8_t kPin
    , const Speed kSpeed = Speed::kFast
    , const Level kLevel = Level::kLow
    , const Mode kMode = Mode::kOutput
    , typename Map = AfNoRemap
    , const PuPd kPuPd = PuPd::kFloating   // G4xx-only; last, can't collide with F1xx call
>
class AnyOut ...
```

Calls using only positional parameters (up to `Map`) are then identical between F1xx and G4xx. G4xx-only code that needs a pull can add `kPuPd` at the end.

---

## 4. `Speed::kFastest` and the I/O Compensation Cell

### What the hardware does

`Speed::kFastest` (`OSPEEDR=0b11`, "Very High Speed") enables the G4xx I/O cell for switching up to the MCU's maximum GPIO frequency (up to ~100 MHz actual toggle rate on G491/G4A1 at 170 MHz HCLK). This is not just a slew-rate setting — at very high speeds, the I/O drivers draw significant current transients that create noise.

To mitigate this, G4xx has an **I/O Compensation Cell** (SYSCFG_CCCSR register, bit `EN`). When enabled, the compensation circuit tracks supply voltage variations and adjusts the output driver strength dynamically, reducing EMI and supply bounce. The datasheet recommends enabling it whenever GPIO speeds exceed ~50 MHz.

### Current state

`Speed::kFastest` is correctly in the `Speed` enum and maps to `OSPEEDR=0b11` in `kOSPEEDR_Bits_`. It works. However, the library does not mention or manage the compensation cell at all.

### Proposed addition

This is a system-level concern (SYSCFG, not GPIO), but the library could provide a companion call or document the pairing. For example, in `AnyPortSetup` or a separate `SysConfig` namespace:

```cpp
// Enable I/O compensation cell (call once at startup when using Speed::kFastest)
// SYSCFG clock must be enabled before this call.
ALWAYS_INLINE static void EnableIoCompensation()
{
    SYSCFG->CCCSR |= SYSCFG_CCCSR_EN;
    while ((SYSCFG->CCCSR & SYSCFG_CCCSR_READY) == 0) {}  // wait for trim to settle
}
```

At minimum, a comment on `Speed::kFastest` in the Speed enum should warn that the compensation cell is recommended.

---

## 5. Analog Mode — Stronger Isolation Than F1xx

### What the hardware does

On G4xx, `MODER=11` (analog mode) completely disconnects the Schmitt trigger and the entire digital input path from the I/O pad. Current leakage through the digital buffer drops to essentially zero. This is the recommended "park" state for unused pins on G4xx, not floating-input.

On F1xx, "analog mode" (`CNF=00`, `MODE=00`) similarly disables the input buffer, but the architecture is different — the register encoding is shared with the output driver speed bits.

### Impact on `Unused<>` pin handling

On F1xx, `Unused<>` configures the pin as input with pull-up or pull-down (this is the F1xx recommended practice — floating unused inputs consume current due to the Schmitt trigger oscillating). On G4xx, `Unused<>` also currently uses input-with-pull.

**A better G4xx `Unused<>`** would use `AnyAnalog` (`MODER=11`, `PUPDR=00`). This gives minimum leakage with no pull resistor needed — the analog mode itself eliminates the floating-input problem. (No `ASCR` is involved on G4; see the §1 correction.) Changing `Unused<>` to use analog mode instead of input-with-pull is therefore a G4xx-specific improvement worth considering.

---

## 6. Summary

| Feature | Register | Currently in library | Priority | Notes |
|---------|----------|---------------------|----------|-------|
| ~~ADC analog switch~~ | ~~`ASCR`~~ | n/a | — | **No `ASCR` on G4** (L4/L5 only); `AnyAnalog` is already correct for G4 ADC. See §1 correction. |
| Pin configuration lock | `LCKR` | ❌ Missing | Medium | Safety feature for motor control / safety-critical pins |
| Pull on output pins | `PUPDR` via `AnyOut` | ⚠️ Partially (parameter exists but was flagged for removal) | Medium | Move `kPuPd` to last position to avoid portability collision |
| `Speed::kFastest` + compensation cell | SYSCFG `CCCSR` | ⚠️ Speed enum exists; compensation cell not mentioned | Low | Add documentation / companion call |
| Analog mode for unused pin parking | `MODER` | ❌ `Unused<>` still uses input-with-pull | Low | Better leakage profile than input+pull on G4xx (no `ASCR` involved) |
