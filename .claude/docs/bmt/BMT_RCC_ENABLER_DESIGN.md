# bmt `Clocks::Enabler` — Design Note

## Problem

Every peripheral template in `bmt/` does its own RCC enable + reset inside
`Init()`. Examples (STM32F1):

| File | Code |
|---|---|
| `f1xx/dma.h:169` | `RCC->AHBENR \|= RCC_AHBENR_DMA1EN;` |
| `f1xx/gpio.h:1000` | `RCC->APB2ENR \|= (1 << ...);` |
| `f1xx/uart.h:118-121` | `APB2ENR \|= ...; APB2RSTR \|= ...; APB2RSTR &= ~...;` |
| `f1xx/exti.h:496` | `RCC->APB2ENR \|= RCC_APB2ENR_AFIOEN;` |
| `f1xx/timer.h` | `Timer::Any::Init()` does RCC reset → `Setup()` |

This causes two recurring failure modes:

1. **Double-`Init()` resets shared state.** Calling `Init()` twice on a
   peripheral (or on two peripherals sharing a bus reset) pulses `APBxRSTR`
   and wipes any configuration applied between the two calls. We hit this
   with `TIM4` / `TIM3` / `TIM1` recently — the `MasterSlaveTimers::Setup()`
   write to `SMCR/CR2.MMS` was clobbered by a later `Timer::Any::Init()`.

2. **No way to enable a group atomically.** Today's pattern produces N
   separate RMW writes to the same `APBxENR` register at boot. That's
   harmless functionally but obscures the *intent* (which peripherals does
   this firmware actually use?) and makes shared resources like DMA and
   AFIO awkward — every user enables them independently.

## Goal

A `Bmt::Clocks::Enabler<Items...>` that:

- Takes a **variadic** list of peripherals (C++20 fold expressions).
- Computes per-RCC-register OR-masks at compile time.
- Exposes `Enable()` / `Disable()` / `Reset()` as **separate** entry points
  that emit one RMW per touched RCC register.
- Lets `Peripheral::Init()` be redefined as the composition
  `Enabler<Self>::Reset(); Setup();` while shared-bus callers can do
  `Enabler<Tim1, Dma1, GpioA, GpioB>::Enable()` once at boot and never reset.

## Variadic templates — yes, C++20 supports them

```cpp
template <typename... Items>
class Enabler { ... };

// Fold expressions:
(Items::Enable(), ...);          // unary right fold over comma
constexpr uint32_t mask = (Items::kBit_ | ...);  // unary fold over |
```

This is strictly cleaner than the `AnyPinGroup` "16 slots with neutral
default" simulation, because RCC bits have no positional identity (unlike
`Pin0..Pin15`) — there is no natural arity cap.

## Per-peripheral trait

Each peripheral template grows one nested type describing where its enable
and reset bits live:

```cpp
// bmt/include/shared/RccTrait.h  (new)
namespace Bmt::Clocks {

enum class RccReg : uint8_t {
    kAhb1En,  kAhb1Rst,
    kApb1En,  kApb1Rst,
    kApb2En,  kApb2Rst,
    // ... family-specific extras (AHB2, AHB3, IOP, ...)
};

template <RccReg kReg, uint32_t kBit>
struct RccBit
{
    static constexpr RccReg  kReg_ = kReg;
    static constexpr uint32_t kBit_ = kBit;
};

// Composite for peripherals that occupy bits in multiple registers
// (e.g. enable in APB1ENR, reset in APB1RSTR).
template <typename EnBit, typename RstBit = void>
struct RccTrait
{
    using Enable_ = EnBit;
    using Reset_  = RstBit;   // void => no reset bit (e.g. AFIO on some lines)
};

} // namespace
```

Each peripheral exposes:

```cpp
// f1xx/dma.h
template<...> class AnyChannel {
public:
    using RccTrait_ = Clocks::RccTrait<
        Clocks::RccBit<Clocks::RccReg::kAhb1En, RCC_AHBENR_DMA1EN>
        // no APBxRSTR bit for DMA on F1 — leave Reset_ = void
    >;
    ...
};

// f1xx/timer.h  (TIM1 example)
using RccTrait_ = Clocks::RccTrait<
    Clocks::RccBit<Clocks::RccReg::kApb2En,  RCC_APB2ENR_TIM1EN>,
    Clocks::RccBit<Clocks::RccReg::kApb2Rst, RCC_APB2RSTR_TIM1RST>
>;
```

## `Enabler` skeleton

```cpp
namespace Bmt::Clocks {

template <typename... Items>
class Enabler
{
    // Group bits by RCC register, OR them together at compile time.
    template <RccReg kReg>
    static constexpr uint32_t MaskFor()
    {
        // Sum (over |) of every Item's enable bit that targets kReg.
        return ((Items::RccTrait_::Enable_::kReg_ == kReg
                 ? Items::RccTrait_::Enable_::kBit_ : 0u) | ...);
    }

    template <RccReg kReg>
    static constexpr uint32_t ResetMaskFor()
    {
        return ((!std::is_void_v<typename Items::RccTrait_::Reset_>
                 && Items::RccTrait_::Reset_::kReg_ == kReg
                 ? Items::RccTrait_::Reset_::kBit_ : 0u) | ...);
    }

public:
    ALWAYS_INLINE static void Enable()
    {
        if constexpr (constexpr auto m = MaskFor<RccReg::kApb2En>(); m)
            RCC->APB2ENR |= m;
        if constexpr (constexpr auto m = MaskFor<RccReg::kApb1En>(); m)
            RCC->APB1ENR |= m;
        if constexpr (constexpr auto m = MaskFor<RccReg::kAhb1En>(); m)
            RCC->AHBENR  |= m;       // F1: single AHBENR
        // ... family extras
        // Read-back barrier (per ST errata) on at least one touched reg.
    }

    ALWAYS_INLINE static void Disable() { /* &= ~m, same shape */ }

    ALWAYS_INLINE static void Reset()
    {
        // Pulse RSTR: |= mask, &= ~mask. One RMW pair per bus.
        if constexpr (constexpr auto m = ResetMaskFor<RccReg::kApb2Rst>(); m)
        { RCC->APB2RSTR |= m; RCC->APB2RSTR &= ~m; }
        // ...
    }

    ALWAYS_INLINE static void Init() { Enable(); Reset(); }
};

} // namespace
```

Notes:
- `if constexpr` on the per-register mask means the function body collapses
  to *exactly* the register writes that are actually needed — zero overhead.
- Order inside `Enable()` is intentional: enable clocks before any reset is
  possible (no peripheral can be reset while its clock is gated on F1).
- `Reset()` is opt-in. Migration plan keeps `Peripheral::Init()` doing the
  current "enable + reset + setup" so existing call sites don't break, but
  internally that becomes `Enabler<Self>::Init(); Setup();`.

## Migration

1. Land `RccTrait` + `Enabler` in `bmt/include/shared/`.
2. Add `using RccTrait_ = ...;` to each peripheral template (one target
   family at a time — start with `f1xx/`).
3. Rewrite each peripheral's existing inline `RCC->...` writes in terms of
   `Enabler<Self>::Enable()` / `Reset()`. Keeps behavior identical.
4. **New capability**: at the top of `main()` in each `target.*/`, call
   `Enabler<Tim1, Tim3, Tim4, Dma1, GpioA, GpioB, ...>::Enable()` once for
   shared-bus peripherals. Then individual `Peripheral::Init()` calls only
   do `Setup()` (no more RCC writes), which kills the double-init reset
   hazard entirely.
5. Step 4 is where `OPT_JTAG_IMPL_TIM_DMA` benefits: the `TIM4`/`TIM3` link
   set up by `Bridge::Setup()` survives because no later `Init()` pulses
   `APB1RSTR`.

## Open questions

- **`Reset()` granularity on shared buses.** Pulsing `APB2RSTR` for `TIM1`
  alone is fine; pulsing it for `TIM1 | USART1 | SPI1` together is also
  fine *if* the firmware genuinely owns all of them. But if a peripheral
  was already configured by an earlier call, batching its reset bit into
  a later `Reset()` will wipe it. **Rule**: `Reset()` is only safe at
  boot, before any `Setup()` has run on any peripheral in the mask.
  Document this and consider a debug-build assert.

- **AFIO on F1.** AFIO has an enable bit (`APB2ENR.AFIOEN`) but no
  dedicated reset. `RccTrait::Reset_ = void` handles this; `Reset()`
  silently skips it.

- **Family differences.** F1 has one `AHBENR`; G4/L4 split into
  `AHB1ENR..AHB3ENR` plus `IOPENR`. `RccReg` enum lives per-family inside
  `f1xx/`, `g4xx/`, `l4xx/` (or under `#if defined(STM32F1)` guards in a
  shared header). The `Enabler` template body branches on `if constexpr`
  so unused registers cost nothing.

- **Read-back barrier.** ST recommends a dummy read after `RCC->...ENR |= ...`
  before touching the peripheral. Today most bmt callsites do this with
  `volatile uint32_t delay = RCC->APB2ENR & ...;`. `Enabler::Enable()`
  should do one read-back at the end (of any one touched register) so the
  caller doesn't have to.

## Not in scope

- Runtime refcounting of shared peripherals. The firmware owns the whole
  MCU; `Enable()` is idempotent (`|= mask`) and that's enough.
- Power-domain / sleep-mode bits (`APBxSMENR`). Add later if needed.
