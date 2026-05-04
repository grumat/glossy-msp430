# GPIO F1xx vs G4xx — Consumer API Comparison

**Files compared:**
- `bmt/include/f1xx/gpio.h` — STM32F1xx (reference, current)
- `bmt/include/g4xx/gpio.h` — STM32G4xx (outdated)

**Scope**: This document focuses exclusively on the public API seen by a `platform.h` that consumes the library. Internal register mappings differ by design — that is expected and not covered here.

---

## 1. Single-Pin Templates — Signature Compatibility

All single-pin helper templates are listed below with their parameter lists. Templates that share the same signature on both platforms are compatible at the call site.

### Fully compatible (identical signatures)

| Template | Parameters |
|----------|-----------|
| `AnyPin` | `<kPort, kPin, kMode, kSpeed, kPuPd, kLevel, Map>` |
| `Unused` | `<kPin, kPuPd>` |
| `Unchanged` | `<kPin>` |
| `AnyAnalog` | `<kPort, kPin, Map>` |
| `AnyIn` | `<kPort, kPin, kPuPd, Map>` |
| `Floating` | `<kPort, kPin, Map>` |
| `AnyInPu` | `<kPort, kPin, Map>` |
| `AnyInPd` | `<kPort, kPin, Map>` |
| `AnyFastOut0` | `<kPort, kPin, Map>` |
| `AnyFastOut1` | `<kPort, kPin, Map>` |
| `AnyOutOD` | `<kPort, kPin, kSpeed, kLevel, Map>` |
| `AnyAltOut` | `<kPort, kPin, Map, kSpeed, kLevel, kPuPd, kMode>` |
| `AnyAltOutPP` | `<kPort, kPin, Map, kSpeed, kLevel>` |
| `AnyAltOutOD` | `<kPort, kPin, Map, kSpeed, kLevel>` |

### **BREAK: `AnyOut` — parameter 5 differs**

```
F1xx:  AnyOut<kPort, kPin, kSpeed, kLevel, [kMode], [Map]>
G4xx:  AnyOut<kPort, kPin, kSpeed, kLevel, [kPuPd], [kMode], [Map]>
```

G4xx inserts `kPuPd` at position 5. F1xx hardcodes `PuPd::kFloating` internally and exposes no `PuPd` parameter.

| Usage pattern | F1xx | G4xx |
|---------------|------|------|
| `AnyOut<PA, 5>` | ✅ | ✅ |
| `AnyOut<PA, 5, Speed::kFast, Level::kHigh>` | ✅ | ✅ |
| `AnyOut<PA, 5, Speed::kFast, Level::kLow, Mode::kOpenDrain>` | ✅ pos5=Mode | ❌ pos5=PuPd, type mismatch |
| `AnyOut<PA, 5, Speed::kFast, Level::kLow, PuPd::kFloating, Mode::kOpenDrain>` | ❌ too many args | ✅ |

The type mismatch (`Mode` enum vs `PuPd` enum) produces a compile error rather than silent wrong behaviour, so the break is caught immediately.

**Fix for G4xx**: Remove the `kPuPd` parameter from `AnyOut` and hard-code `PuPd::kFloating`, matching F1xx. `AnyOutOD` already passes `PuPd::kPullUp` explicitly in both files — `AnyOut` itself does not need to expose `PuPd` to support that.

---

## 2. Pin Operation Methods — Fully Compatible

All runtime I/O methods are identical on both platforms. No porting changes needed.

| Method | F1xx | G4xx |
|--------|------|------|
| `Setup()` | ✅ | ✅ |
| `Setup(Mode, Speed, PuPd)` | ✅ | ✅ |
| `SetupPinMode()` | ✅ | ✅ |
| `SetHigh()` | ✅ | ✅ |
| `SetLow()` | ✅ | ✅ |
| `Set(bool)` | ✅ | ✅ |
| `Get()` | ✅ | ✅ |
| `IsHigh()` | ✅ | ✅ |
| `IsLow()` | ✅ | ✅ |
| `Toggle()` | ✅ | ✅ |

---

## 3. `AnyPinGroup` — Renamed and Missing Methods

This is the most significant consumer-visible break. `AnyPinGroup` is used directly when a port setup needs to be applied incrementally (partial group), without going through `AnyPortSetup::Init()`.

| Method | F1xx | G4xx | Notes |
|--------|------|------|-------|
| `Setup()` | ✅ | ❌ | **Renamed to `Enable()` in G4xx** |
| `SetupPinMode()` | ✅ | ❌ | **Dropped in G4xx — no equivalent** |
| `Enable()` | ❌ | ✅ | F1xx equivalent is `Setup()` |
| `Disable()` | ❌ | ✅ | **New in G4xx — no F1xx equivalent** |
| `TriState()` | ✅ | ❌ | **Dropped in G4xx** |

### Functional description of each:

**`Setup()` (F1xx) / `Enable()` (G4xx)**
Both apply the full group configuration (mode + ODR / initial levels). Functionally equivalent — only the name changed.

**`SetupPinMode()` (F1xx only)**
Applies CRL/CRH mode bits without touching ODR. Used when a caller needs to reconfigure pin direction while preserving current output levels. G4xx dropped this. The closest G4xx equivalent is `Enable()`, which does write ODR — so the semantic is slightly different. However, in most practical usage, the ODR write in `Enable()` simply restores the same compile-time initial value, which is harmless.

**`Disable()` (G4xx only)**
Resets all group pins to their datasheet power-on default state and disables the port's RCC clock gate. F1xx has no equivalent — neither a port reset nor clock gating. This is a useful operation for power management.

**`TriState()` (F1xx only)**
Floats all group pins (forces floating-input configuration) without touching other pins' modes. Primarily a debug/diagnostic tool. G4xx dropped it. A rough G4xx equivalent would be calling `AnyPinGroup` with all pins as `AnyIn<>`, but there is no direct replacement method.

### Recommended fixes:

| Action | Target |
|--------|--------|
| Rename `Setup()` → `Enable()` in **F1xx** to match G4xx naming | F1xx |
| Add `SetupPinMode()` to **G4xx** (applies MODER/OTYPER/OSPEEDR/PUPDR/AFR without ODR) | G4xx |
| Add `Disable()` to **F1xx** (reset CRL/CRH to float-input, disable RCC clock) | F1xx |
| Add `TriState()` to **G4xx** (write `0xFF` to MODER for all group pins, or just call `Disable()`) | G4xx |

---

## 4. `AnyPortSetup` — Compatible

`AnyPortSetup::Init()` has the same signature and behaviour on both platforms. This is the primary entry point used by `platform.h` and it works identically.

```cpp
// Both platforms — identical call site
PORTA::Init();
```

No changes needed for typical `platform.h` usage.

---

## 5. `SaveGpio` — Second Parameter Break

```cpp
// F1xx
template<const Port kPort, bool safe = true>
class SaveGpio { ... };

// G4xx
template<const Port kPort>
class SaveGpio { ... };
```

| Usage | F1xx | G4xx |
|-------|------|------|
| `SaveGpio<Port::PA> save;` | ✅ | ✅ |
| `SaveGpio<Port::PA, false> save;` | ✅ skips AFIO MAPR save/restore | ❌ compile error |

The `safe` parameter in F1xx controls whether the system-wide `AFIO->MAPR` register is included in the save/restore. G4xx has no MAPR — it saves per-port `AFR[0]`/`AFR[1]` unconditionally, so the parameter has no meaning there.

**Fix for G4xx**: Accept (and ignore) the `bool safe` template parameter to allow the same call sites to compile on both platforms:

```cpp
template<const Port kPort, bool safe = true>  // safe is accepted but unused on G4xx
class SaveGpio { ... };
```

---

## 6. `Speed::kFastest` — G4xx Only

G4xx hardware has a 4th speed grade (`0b11` in OSPEEDR = "very high speed"). F1xx only has 3 (`kSlow`, `kMedium`, `kFast`). There is no matching F1xx encoding.

| Usage | F1xx | G4xx |
|-------|------|------|
| `Speed::kFast` | ✅ | ✅ |
| `Speed::kFastest` | ❌ enum value missing | ✅ |

Platform-specific code using `Speed::kFastest` will not compile on F1xx — this is acceptable as a known hardware difference. No action needed unless cross-platform pin aliases exist that use `Speed::kFastest`.

---

## 7. `MapType` Type Alias — F1xx Only

F1xx `Implementation_` exposes `using MapType = Map`, making the remap type accessible through the pin type:

```cpp
using MY_PIN = AnyAltOutPP<Port::PA, 9, AfUsart1_PA9_10>;
using REMAP = MY_PIN::MapType;  // works on F1xx, breaks on G4xx
```

G4xx `Implementation_` does not have this alias.

**Fix for G4xx**: Add `using MapType = Map` to `Implementation_`.

---

## 8. Functional Behaviour Differences (Same API, Different Result)

These cases compile and link identically on both platforms, but produce different runtime behaviour.

### `AnyOutOD` — pull-up on G4xx, none on F1xx

```cpp
using MY_OD = AnyOutOD<Port::PB, 3>;
MY_OD::Setup();
```

- **F1xx**: Open-drain, no internal pull resistor. External pull-up required on the bus.
- **G4xx**: Open-drain, internal pull-up enabled via PUPDR. External pull-up is still recommended for I2C/1-Wire, but less critical.

The G4xx behaviour is strictly better for most use cases. No action needed — document it.

### `AnyAltOutOD` — same situation as `AnyOutOD`

F1xx sets `PuPd::kPullUp` in the template instantiation but the hardware does not honour it for outputs (the comment in F1xx code says "unsupported by this hardware"). G4xx does honour it. Same conclusion.

### `AnyPinGroup::Enable()` vs `Setup()` — ODR always written

On F1xx, `SetupPinMode()` exists as a way to change pin direction without touching ODR. `Setup()` writes both CRL/CRH and ODR. G4xx's `Enable()` always writes ODR. In practice this is the same for the `AnyPortSetup::Init()` path (full port initialisation from a cold state), but different if `AnyPinGroup` is used to reconfigure a pin at runtime while preserving a live output state.

---

## 9. Summary: Required Changes for API Parity

### Changes to `g4xx/gpio.h`

| # | Break type | Item | Fix |
|---|-----------|------|-----|
| **B1** | **Compile break** | `AnyOut` has extra `kPuPd` at position 5 | Remove `kPuPd` parameter; hard-code `PuPd::kFloating` internally |
| **B2** | **Compile break** | `AnyPinGroup::Setup()` missing (renamed to `Enable()`) | Add `Setup()` as an alias: `constexpr static void Setup() { Enable(); }` |
| **B3** | **Compile break** | `SaveGpio` has no `bool safe` parameter | Add `bool safe = true` as ignored second template parameter |
| **B4** | Missing method | `AnyPinGroup::SetupPinMode()` dropped | Add: apply MODER/OTYPER/OSPEEDR/PUPDR/AFR without writing ODR |
| **B5** | Missing method | `AnyPinGroup::TriState()` dropped | Add: write `0xFFFFFFFF` to MODER for all group pins (all-analog/input) |
| **B6** | Missing type | `using MapType = Map` missing in `Implementation_` | Add to `Implementation_` |

### Changes to `f1xx/gpio.h`

| # | Break type | Item | Fix |
|---|-----------|------|-----|
| **B7** | Missing method | `AnyPinGroup::Disable()` absent | Add: reset CRL/CRH to floating-input defaults, disable RCC clock |
| **B8** | Missing method | `AnyPinGroup::Enable()` absent (named `Setup()` here) | Add `Enable()` as an alias: `constexpr static void Enable() { Setup(); }` |

> **Note on B2 and B8**: The rename `Setup()` ↔ `Enable()` can be resolved by adding the alias on both sides rather than renaming, which avoids breaking existing call sites.
