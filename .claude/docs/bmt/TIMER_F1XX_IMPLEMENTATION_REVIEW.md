# STM32F1 Timer Implementation Review

**File:** `bmt/include/f1xx/timer.h` (~1985 lines)

## Executive Summary

The STM32F1 timer module demonstrates a sophisticated use of C++17 templates and compile-time constants but suffers from **severe code redundancy**. The main sources are:

1. **Three `#if 0` blocks** with dead code that can be removed (lines 1219, 1236, 1300)
2. **Massive repetitive switch/case statements** for timer enumeration (TIM1-TIM17)
3. **Duplicated channel handling logic** across four channels (k1, k2, k3, k4)
4. **Template specialization boilerplate** for every timer-channel combination

This review identifies the issues and provides refactoring recommendations.

---

## 1. Dead Code: The Three `#if 0` Blocks

### Issue #1: Lines 1219–1226 (StartShot method)

```cpp
ALWAYS_INLINE static void StartShot()
{
    volatile TIM_TypeDef *timer = BASE::GetDevice();
#if 0
    if (kTimerMode_ == Mode::kSingleShot || kTimerMode_ == Mode::kUpCounter)
    {
        timer->CNT = 0;
        if (kBuffered_)
            timer->EGR = TIM_EGR_UG;  // UG Event
    }
#endif
    timer->EGR = TIM_EGR_UG;  // this clears CNT
    timer->CR1 |= TIM_CR1_CEN;
}
```

**Analysis**: The conditional code (lines 1220–1225) is redundant because:
- The method unconditionally executes `timer->EGR = TIM_EGR_UG` on line 1227.
- The `EGR` write already clears the counter, so the conditional `if` block is unnecessary.
- If `kBuffered_` is true, calling `EGR = TIM_EGR_UG` twice (once conditionally, once unconditionally) is wasteful.

**Action**: **Remove lines 1220–1225 entirely.** The unconditional `EGR` write is sufficient.

### Issue #2: Lines 1236–1243 (StartShot overload)

```cpp
ALWAYS_INLINE static void StartShot(const TypCnt ticks)
{
    volatile TIM_TypeDef *timer = BASE::GetDevice();
    timer->ARR = ticks;
#if 0
    if (kTimerMode_ == Mode::kSingleShot || kTimerMode_ == Mode::kUpCounter)
    {
        timer->CNT = 0;
        if (kBuffered_)
            timer->EGR = TIM_EGR_UG;  // UG Event
    }
#endif
    timer->EGR = TIM_EGR_UG;  // this clears CNT
    timer->CR1 |= TIM_CR1_CEN;
}
```

**Identical to Issue #1**: Same redundant conditional logic. 

**Action**: **Remove lines 1237–1242.**

### Issue #3: Lines 1300–1303 (Delay_ method)

```cpp
static void Delay_(const uint16_t num) NO_INLINE
{
    TIM_TypeDef* timer = (TIM_TypeDef*)Base::kTimerBase_;
    timer->ARR = num;
#if 0
    if (Base::kBuffered_)
        timer->EGR = TIM_EGR_UG;  // update ARR
#endif
    timer->EGR = TIM_EGR_UG;
    timer->CR1 |= TIM_CR1_CEN;
    // CEN is cleared automatically in one-pulse mode
    Base::WaitForAutoStop();
}
```

**Same Pattern**: Conditional update event generation is replaced by unconditional `EGR` write. The comment on line 1302 states "update ARR", suggesting the dead code intended to handle buffered mode differently, but the unconditional write accomplishes the same goal.

**Action**: **Remove lines 1301–1303.**

---

## 2. Massive Timer Enumeration Redundancy

### Problem: Lines 727–848 (Init method)

The `Init()` method contains a **122-line `switch` statement** with 17 cases (TIM1–TIM17), each following the same pattern:

```cpp
#ifdef TIMx_BASE
case kTimx:
    RCC->APB2ENR |= RCC_APB2ENR_TIMxEN;     // or APB1ENR
    RCC->APB2RSTR |= RCC_APB2RSTR_TIMxRST;  // or APB1RSTR
    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIMxRST;
    break;
#endif
```

**Similar issues exist in:**
- **Stop()** method (lines 891–945): 55 lines, same pattern
- **EnableIrq()** method (lines 948–1006): 59 lines, same pattern

### Root Cause

Each timer has a unique RCC enable/reset bit and NVIC IRQ number. The compiler can't automatically generate this mapping, so manual enumeration is necessary. However, the structure is highly predictable.

### Refactoring Options

**Option A: Helper Structure (Recommended)**

Create a compile-time timer descriptor:

```cpp
template<const Unit kTimerNum>
struct TimerDescriptor {
    static constexpr volatile uint32_t* pEnReg = 
        (kTimerNum == kTim1 || kTimerNum == kTim8) ? &RCC->APB2ENR : &RCC->APB1ENR;
    static constexpr uint32_t kEnBit = 
        (kTimerNum == kTim1) ? RCC_APB2ENR_TIM1EN :
        // ... etc for all timers
        0;
    static constexpr uint32_t kRstBit = 
        (kTimerNum == kTim1) ? RCC_APB2RSTR_TIM1RST :
        // ... etc
        0;
    static constexpr IRQn_Type kIrqNum = 
        (kTimerNum == kTim1) ? TIM1_UP_IRQn :
        // ... etc
        (IRQn_Type)-1;
};
```

Then simplify `Init()`:

```cpp
ALWAYS_INLINE static void Init()
{
    using Desc = TimerDescriptor<BASE::kTimerNum_>;
    *Desc::pEnReg |= Desc::kEnBit;
    
    volatile uint32_t* pRstReg = (Desc::pEnReg == &RCC->APB2ENR) 
        ? &RCC->APB2RSTR : &RCC->APB1RSTR;
    *pRstReg |= Desc::kRstBit;
    *pRstReg &= ~Desc::kRstBit;
    
    TimeBase::Setup();
    Setup();
}
```

**Pros:**
- Reduces ~120 lines to ~12 lines
- Centralizes timer metadata in one place
- Easier to maintain when adding new timers

**Cons:**
- Slightly more complex (requires helper struct)
- Still repetitive for the 17-timer case, just abstracted

**Option B: Macro-Based (Less Safe)**

Generate switch cases from a macro (but less type-safe than Option A).

**Recommendation:** Implement **Option A** with a `TimerDescriptor<>` template. This is the most C++-idiomatic and maintainable solution.

---

## 3. Channel Handling Redundancy

### Problem: Repetitive 4-Channel Switch Statements

Multiple methods repeat the same structure across 4 channels:

**EnableIrq() (Lines 1615–1634):**
```cpp
ALWAYS_INLINE static void EnableIrq()
{
    volatile TIM_TypeDef* timer = BASE::GetDevice();
    switch (kChannelNum)
    {
    case Channel::k1:
        timer->DIER |= TIM_DIER_CC1IE;
        break;
    case Channel::k2:
        timer->DIER |= TIM_DIER_CC2IE;
        break;
    case Channel::k3:
        timer->DIER |= TIM_DIER_CC3IE;
        break;
    case Channel::k4:
        timer->DIER |= TIM_DIER_CC4DE;
        break;
    }
}
```

**Same pattern repeated in:**
- `EnableIrq()` (lines 1615–1634)
- `DisableIrq()` (lines 1636–1655)
- `EnableDma()` (lines 1657–1676)
- `DisableDma()` (lines 1678–1697)
- `GenerateCaptureEvent()` (lines 1502–1512)
- `GenerateCompareEvent()` (lines 1514–1524)

Plus many more in `AnyOutputChannel` and `AnyInputChannel`.

### Root Cause

Each channel (k1, k2, k3, k4) has different bit positions and register fields in hardware registers (DIER, CCER, CCMR1, CCMR2). The compiler can't generate this automatically.

### Refactoring Strategy

**Option A: Channel Descriptor Template**

```cpp
template<const Channel kChannel>
struct ChannelDescriptor {
    static constexpr uint32_t kDierCcIe = 
        (kChannel == Channel::k1) ? TIM_DIER_CC1IE :
        (kChannel == Channel::k2) ? TIM_DIER_CC2IE :
        (kChannel == Channel::k3) ? TIM_DIER_CC3IE :
        TIM_DIER_CC4IE;
    
    static constexpr uint32_t kDierCcDe = 
        (kChannel == Channel::k1) ? TIM_DIER_CC1DE :
        (kChannel == Channel::k2) ? TIM_DIER_CC2DE :
        (kChannel == Channel::k3) ? TIM_DIER_CC3DE :
        TIM_DIER_CC4DE;
    
    // ... similar for other fields
};
```

Then simplify:

```cpp
ALWAYS_INLINE static void EnableIrq()
{
    using Desc = ChannelDescriptor<kChannelNum>;
    BASE::GetDevice()->DIER |= Desc::kDierCcIe;
}
```

**Pros:**
- Eliminates 5+ near-identical switch statements
- Single source of truth for channel bit mappings
- Easier to debug (one place to check mapping)

**Cons:**
- Requires additional helper struct
- Still~80 lines for the descriptor (but centralized)

**Recommendation:** Implement **ChannelDescriptor** to eliminate switch-based redundancy. This is especially beneficial for methods called frequently (EnableIrq, DisableIrq, EnableDma, DisableDma).

---

## 4. Code Quality Issues

### Issue A: Typo in AnyOutputChannel Setup (Line 1699)

```cpp
ALWAYS_INLINE static uint16_t GetCapture()
{
    volatile TIM_TypeDef* timer = BASE::GetDevice();
    switch (BASE::kChannelNum_)  // <-- Uses BASE::kChannelNum_
    {
    case Channel::k1: return timer->CCR1;
    // ...
    }
}
```

**Problem**: Uses `BASE::kChannelNum_` instead of `kChannelNum_`. This works because it's inherited, but it's inconsistent. In other methods (e.g., line 1577), the code uses `kChannelNum` directly.

**Fix**: Change line 1702 to use `kChannelNum_` for consistency:

```cpp
switch (kChannelNum_)
```

### Issue B: Switch Without Default Case (Lines 1332–1340)

```cpp
ALWAYS_INLINE static volatile void* GetCcrAddress()
{
    volatile TIM_TypeDef* timer = BASE::GetDevice();
    switch (kChannelNum_)
    {
    case Channel::k1: return &timer->CCR1;
    case Channel::k2: return &timer->CCR2;
    case Channel::k3: return &timer->CCR3;
    case Channel::k4: return &timer->CCR4;
    }
    return 0;  // Unreachable if static_asserts work correctly
}
```

**Observation**: The `return 0` on line 1339 is unreachable if all static assertions (lines 1324–1327) pass. However, relying on implicit `0` return is fragile. Better:

```cpp
switch (kChannelNum_)
{
case Channel::k1: return &timer->CCR1;
case Channel::k2: return &timer->CCR2;
case Channel::k3: return &timer->CCR3;
case Channel::k4: return &timer->CCR4;
default: return nullptr;  // Explicit, matches return type
}
```

### Issue C: Inverted CCER Mask for Channel 4 (Line 1746)

```cpp
static constexpr uint32_t kCcer_Mask =
    (BASE::kChannelNum_ == Channel::k1) ? TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk | TIM_CCER_CC1NE_Msk | TIM_CCER_CC1NP_Msk
    : (BASE::kChannelNum_ == Channel::k2) ? TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk | TIM_CCER_CC2NE_Msk | TIM_CCER_CC2NP_Msk
    : (BASE::kChannelNum_ == Channel::k3) ? TIM_CCER_CC3E_Msk | TIM_CCER_CC3P_Msk | TIM_CCER_CC3NE_Msk | TIM_CCER_CC3NP_Msk
    : (BASE::kChannelNum_ == Channel::k4) ? TIM_CCER_CC4E_Msk | TIM_CCER_CC4P_Msk  // <-- Missing NE/NP!
    : 0;
```

**Observation**: Channel 4 mask omits `CC4NE_Msk` and `CC4NP_Msk` (complementary/inverted outputs). This may be intentional if Channel 4 doesn't support these features on STM32F1, but it's inconsistent with Channels 1–3. 

**Action**: Add a comment clarifying whether this is intentional:

```cpp
: (BASE::kChannelNum_ == Channel::k4) ? TIM_CCER_CC4E_Msk | TIM_CCER_CC4P_Msk  // Channel 4: no complementary output on F1
```

---

## 5. Summary of Redundancy Metrics

| Component | Lines | Refactoring | Reduction |
|-----------|-------|-------------|-----------|
| Dead code (#if 0 blocks) | 27 | Remove | 27 lines |
| Timer enumeration (Init/Stop/EnableIrq) | 236 | TimerDescriptor<> | ~200 lines |
| Channel switch statements | 80+ | ChannelDescriptor<> | ~60 lines |
| Other small redundancies | 15 | Minor fixes | ~5 lines |
| **Total potential reduction** | — | — | **~290 lines (~15%)** |

---

## 6. Recommendations (Prioritized)

| Priority | Issue | Action | Impact |
|----------|-------|--------|--------|
| **P0 (Critical)** | Dead code (#if 0 blocks) | Remove lines 1220–1225, 1237–1242, 1301–1303 | 27 lines, clarity |
| **P1 (High)** | Timer enumeration redundancy | Implement `TimerDescriptor<>` helper | ~200 lines, maintainability |
| **P2 (High)** | Channel switch redundancy | Implement `ChannelDescriptor<>` helper | ~60 lines, maintainability |
| **P3 (Medium)** | Inconsistent var references | Fix `BASE::kChannelNum_` → `kChannelNum_` (line 1702) | 1 line, consistency |
| **P4 (Medium)** | Unreachable code clarity | Add explicit `default: return nullptr;` (line 1339) | 1 line, safety |
| **P5 (Low)** | Channel 4 mask clarity | Add comment explaining why CC4NE/CC4NP are omitted (line 1747) | 1 line, documentation |

---

## 7. Implementation Guide

### Step 1: Remove Dead Code (Immediate)

```cpp
// Remove #if 0 blocks at lines 1219–1226, 1236–1243, 1300–1303
```

### Step 2: Implement TimerDescriptor (Medium Effort)

Create a new file or add to timer.h:

```cpp
template<const Unit kTimerNum>
struct TimerDescriptor {
    // APB2 timers: TIM1, TIM8, TIM9-17
    // APB1 timers: TIM2-7, TIM12-14
    static constexpr volatile uint32_t* pEnReg = 
        (kTimerNum == kTim1 || kTimerNum == kTim8 || kTimerNum >= kTim9) ? &RCC->APB2ENR : &RCC->APB1ENR;
    
    static constexpr uint32_t kEnBit = 
        (kTimerNum == kTim1) ? RCC_APB2ENR_TIM1EN : // ... continue for all timers
        0;
    
    static constexpr IRQn_Type kIrqNum = 
        (kTimerNum == kTim1) ? TIM1_UP_IRQn : // ... continue for all timers
        (IRQn_Type)-1;
};
```

Then refactor `Init()`, `Stop()`, `EnableIrq()` methods to use it.

### Step 3: Implement ChannelDescriptor (Medium Effort)

```cpp
template<const Channel kChan>
struct ChannelDescriptor {
    static constexpr uint32_t kDierCcIe = 
        (kChan == Channel::k1) ? TIM_DIER_CC1IE : 
        (kChan == Channel::k2) ? TIM_DIER_CC2IE : 
        (kChan == Channel::k3) ? TIM_DIER_CC3IE : 
        TIM_DIER_CC4IE;
    // ... similar for CC1DE, CC2DE, etc.
};
```

Then refactor channel methods to use it.

---

## Conclusion

The timer.h module is functionally correct but **significantly over-engineered with redundant code**. The three `#if 0` blocks are immediately removable. The larger issues are the repetitive timer and channel enumeration code, which could be reduced by **~260 lines** using helper template structures. 

While C++ template metaprogramming can't entirely eliminate hardware mapping boilerplate, centralizing these mappings in descriptor structures would improve maintainability and reduce the risk of copy-paste errors.

**Recommended action timeline:**
1. **Immediately**: Remove #if 0 blocks (5 minutes)
2. **This sprint**: Implement TimerDescriptor and ChannelDescriptor (2–3 hours)
3. **Follow-up**: Add comments for non-obvious design choices (clarify why certain hardware features are missing from specific timers)
