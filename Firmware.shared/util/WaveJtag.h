/*
# WaveJtag - High-Performance JTAG Frame Generation using Timer and DMA

This module implements JTAG TAP (Test Access Port) frame generation using hardware
timers and DMA for maximum performance and precise timing. Two template classes
are provided for different hardware configurations:

## 1. Generator<> - Optimized Single-Port Implementation
   For hardware where all JTAG signals (JTCK, JTMS, JTDO, JTDI) share the same
   GPIO port. Uses 3 DMA channels with precise timer synchronization.

## 2. GeneratorSTLinkPWM<> - Split-Port Implementation with PWM Clock
   For hardware constraints like ST-Link V2 clones where signals are split across
   multiple ports. Uses PWM for automatic 50% duty cycle JTCK generation, freeing
   DMA channels for data transfers.

## Operating Principles:
- **Timer-Driven Waveforms**: An advanced timer (TIM1) generates precise timing
  base with repetition counter for exact bit counts
- **DMA Synchronization**: Multiple DMA channels triggered by timer compare events
  for zero-CPU-overhead data transfers
- **BSRR Register Manipulation**: Direct GPIO bit-set/reset register writes for
  atomic signal updates
- **PWM Clock Generation**: For ST-Link variant, TIM1_CH1N produces 50% duty cycle
  clock automatically while DMA handles data

## Timing Diagrams:

### Generator<> (Single-Port):
Timer Count: 0 1 2 3 4 5 6 7
		    ┌─┬─┬─┬─┬─┬─┬─┬─┐
Write Ch 1  │ │█│ │ │ │ │ │ │ → JTMS/JTDI data written
		    ├─┼─┼─┼─┼─┼─┼─┼─┤
Rise Ch  5  │ │ │ │ │█│ │ │ │ → JTCK rising edge
		    ├─┼─┼─┼─┼─┼─┼─┼─┤
Read Ch  7  │ │ │ │ │ │ │█│ │ → JTDO sampled
		    └─┴─┴─┴─┴─┴─┴─┴─┘

### GeneratorSTLinkPWM<> (Split-Port with PWM):
Timer Count: 0 1 2 3 4 5 6 7
		    ┌─┬─┬─┬─┬─┬─┬─┬─┐
PWM (CH1N)  │█│█│█│█│ │ │ │ │ → 50% duty cycle (4 high, 4 low)
		    ├─┼─┼─┼─┼─┼─┼─┼─┤
Read Ch 6   │█│ │ │ │ │ │ │ │ → JTDO sampled (Port A)
		    ├─┼─┼─┼─┼─┼─┼─┼─┤
Write Ch 2  │ │█│ │ │ │ │ │ │ → JTDI data written (Port A)
		    ├─┼─┼─┼─┼─┼─┼─┼─┤
TMS Ch 5    │ │ │█│ │ │ │ │ │ → JTMS control (Port B)
		    └─┴─┴─┴─┴─┴─┴─┴─┘
*/

#pragma once

namespace WaveJtag
{


/**
 * JTAG scan operation type
 * Determines whether to scan Data Register (DR) or Instruction Register (IR)
 */
enum class Scan : uint8_t
{
	kDR = 0,		///< Data Register scan (JTAG state: Shift-DR)
	kIR = 1,		///< Instruction Register scan (JTAG state: Shift-IR)
	kGoIdle,		///< Forces JTAG TAP to Run-Test/Idle state (5 TMS=1 cycles)
};

/**
 * Number of bits to shift in a JTAG scan operation
 * Supports common MSP430 bit widths and special GoIdle operation
 */
enum class NumBits : uint8_t
{
	kGoIdle = 0,	///< Special value for GoIdle operation (not a bit count)
	k8 = 8,		///< 8-bit data transfer (common for byte operations)
	k16 = 16,	///< 16-bit data transfer (MSP430 word size)
	k20 = 20,	///< 20-bit data transfer (MSP430 extended addressing)
	k32 = 32,	///< 32-bit data transfer (double word operations)
};


/**
# Generator<> - Optimized Single-Port JTAG Frame Generator

This class implements high-performance JTAG frame generation for hardware where
all JTAG signals (JTCK, JTMS, JTDO, JTDI) share the same GPIO port. It achieves
maximum performance through tightly synchronized DMA transfers triggered by
timer compare events.

## Operating Principles:

### Timer Configuration:
- **8-cycle waveform**: Each JTAG clock cycle spans 8 timer counts for precise
  control of setup/hold times
- **Single-shot mode with repetition**: Timer runs once for exact bit count,
  using advanced timer's repetition counter
- **Compare events trigger DMA**: Three compare points at counts 1, 5, and 7
  trigger DMA transfers for write, rise, and read operations

### DMA Channel Usage:
1. **DmaWrData**: Writes JTMS/JTDI data to GPIO BSRR (bit-set/reset register)
   - Triggered at count 1 (before clock rising edge)
   - Memory → Peripheral, circular mode
   
2. **DmaRise**: Generates JTCK rising edge by setting clock bit
   - Triggered at count 5 (clock transition point)
   - Memory → Peripheral, circular mode
   
3. **DmaRdData**: Reads JTDO data from GPIO IDR (input data register)
   - Triggered at count 7 (after clock rising edge)
   - Peripheral → Memory, circular mode

### Signal Timing (8-count cycle):
```
Count: 0   1   2   3   4   5   6   7
	   ┌───┬───┬───┬───┬───┬───┬───┬───┐
Write  │   │█W │   │   │   │   │   │   │ → JTMS/JTDI data written
	   ├───┼───┼───┼───┼───┼───┼───┼───┤
Rise   │   │   │   │   │   │█R │   │   │ → JTCK rising edge (clock high)
	   ├───┼───┼───┼───┼───┼───┼───┼───┤
Read   │   │   │   │   │   │   │   │█S │ → JTDO sampled
	   └───┴───┴───┴───┴───┴───┴───┴───┘
JTCK   ───┐            ┌───────────────
		  │            │
		  └────────────┘
```

## Template Parameters:
@tparam SysClk          System clock configuration
@tparam kTim            Master timer unit (must be advanced timer with repetition counter)
@tparam kWriteCh        Timer channel for data write DMA trigger
@tparam kRise           Timer channel for clock rising edge DMA trigger  
@tparam kReadCh         Timer channel for data read DMA trigger
@tparam kFreq           JTAG clock frequency (wave DMA trigger frequency)
@tparam kScan           Scan type (DR, IR, or GoIdle)
@tparam kNumBits        Number of bits to shift (8, 16, 20, 32, or GoIdle)

## Hardware Requirements:
- All JTAG pins must be on the same GPIO port (for atomic BSRR operations)
- Advanced timer with repetition counter (TIM1 on STM32)
- 3 independent DMA channels
- Timer must support output compare with DMA request generation
*/
template <
	typename SysClk							///< System clock that drives timers
	, const Timer::Unit kTim				///< Master timer (must be advanced timer)
	, const Timer::Channel kWriteCh			///< Timer channel for the write cycle (data to JTMS/JTDI)
	, const Timer::Channel kRise			///< Timer channel for the rising edge (JTCK toggle)
	, const Timer::Channel kReadCh			///< Timer channel for the read cycle (JTDO sample)
	, const uint32_t kFreq					///< Frequency of JTAG clock (wave DMA trigger)
	, const Scan kScan						///< Type of JTAG scan operation
	, const NumBits kNumBits				///< Number of bits to shift
>
class Generator
{
public:
	/// Time Base for the JCLK generation (8 cycles are needed by timer for a complete cycle)
	using MasterClock = Timer::InternalClock_Hz<kTim, SysClk, 8 * kFreq>;
	/// Generates the beat that issues a DMA request
	using CycleTimer = Timer::Any<MasterClock, Timer::Mode::kSingleShot, 7, false, true>;

	/// Timer cycle where clock asserts (and other signals are prepared)
	using TriggerWrite = Timer::AnyOutputChannel<CycleTimer, kWriteCh>;
	/// DMA channel that writes data to BSRR
	using DmaWrData = Dma::AnyChannel
		<
		typename TriggerWrite::DmaChInfo_
		, Dma::Dir::kMemToPerCircular
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::PtrPolicy::kLongPtr
		, Dma::Prio::kMedium
		>;

	/// Timer cycle where clock rises (and target samples other signals)
	using TriggerRise = Timer::AnyOutputChannel<CycleTimer, kRise>;
	/// DMA channel that just produces a rising edge (when all data is fetched)
	using DmaRise = Dma::AnyChannel
		<
		typename TriggerRise::DmaChInfo_
		, Dma::Dir::kMemToPerCircular
		, Dma::PtrPolicy::kLongPtr
		, Dma::PtrPolicy::kLongPtr
		, Dma::Prio::kHigh
		>;

	/// Timer cycle after clock rises dat input is sampled
	using TriggerRead = Timer::AnyOutputChannel<CycleTimer, kReadCh>;
	/// DMA channel that reads IDR
	using DmaRdData = Dma::AnyChannel
		<
		typename TriggerRead::DmaChInfo_
		, Dma::Dir::kPerToMemCircular
		, Dma::PtrPolicy::kLongPtr
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::Prio::kHigh
		>;
	// Type of JTAG scan
	static constexpr Scan kScan_ = kScan;
	// Bit count
	static constexpr NumBits kNumBits_ = kNumBits;

public:
	/// Hardware initialization
	static ALWAYS_INLINE void Init()
	{
		// Repetition counter is only available on the advanced timer
		static_assert(CycleTimer::HasRepetitionCounter(), "Timer needs to be the advanced timer (TIM1)");
		static_assert(DmaWrData::kChan_ != DmaRise::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(DmaWrData::kChan_ != DmaRdData::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(DmaRise::kChan_ != DmaRdData::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");

		CycleTimer::Init();				// Timer generates time base
		DmaWrData::Init();
		DmaRise::Setup();
		DmaRdData::Setup();
		TriggerWrite::Setup();
		TriggerRise::Setup();
		TriggerRead::Setup();
		// Set compare values for 8-count timing cycle:
		// Count:  0   1   2   3   4   5   6   7
		//         ┌───┬───┬───┬───┬───┬───┬───┬───┐
		// Write   │   │█W │   │   │   │   │   │   │ → JTMS/JTDI @1 (data setup)
		//         ├───┼───┼───┼───┼───┼───┼───┼───┤
		// Rise    │   │   │   │   │   │█R │   │   │ → JTCK @5 (rising edge)
		//         ├───┼───┼───┼───┼───┼───┼───┼───┤
		// Read    │   │   │   │   │   │   │   │█S │ → JTDO @7 (data sample)
		//         └───┴───┴───┴───┴───┴───┴───┴───┘
		// JTCK    ───┐               ┌────────────
		//            │               │
		//            └───────────────┘
		//              Low (0-4)       High (5-7)
		TriggerWrite::SetCompare(1);	// Data write at count 1
		TriggerRise::SetCompare(5);	// Clock rising edge at count 5
		TriggerRead::SetCompare(7);	// Data sample at count 7
	}
	// Restores DMA Setup for this use (when sharing DMA with other peripherals)
	static ALWAYS_INLINE void SetupDma()
	{
		DmaWrData::Setup();
		DmaRise::Setup();
		DmaRdData::Setup();
	}
	// Restores DMA channels for repurpose, when sharing DMA with other peripherals
	static ALWAYS_INLINE void ReleaseDma()
	{
		DmaWrData::Disable();
		DmaRise::Disable();
		DmaRdData::Disable();
	}
	/// Prepares a buffer with a JTAG transaction
	static ALWAYS_INLINE void RenderTransaction(
		uint32_t* buffer
		, const uint32_t tclk_level			// BSSR value for the initial TCLK level
		, const uint32_t data_out			// Data to be sent
	)
	{
		static_assert(kScan_ != Scan::kGoIdle && kNumBits_ != NumBits::kGoIdle, "This method cannot be used to render kGoIdle");

		constexpr uint32_t tms0 = JTMS::kBitValue_ << 16;
		constexpr uint32_t tms1 = JTMS::kBitValue_;
		constexpr uint32_t tdi0 = JTDI::kBitValue_ << 16;
		constexpr uint32_t tdi1 = JTDI::kBitValue_;
		constexpr uint32_t tck0 = JTCK::kBitValue_ << 16;
		constexpr uint32_t tms0tck0 = tms0 | tck0;
		constexpr uint32_t tms1tck0 = tms1 | tck0;

		// Select DR-Scan
		*buffer++ = tms1tck0 | tclk_level;
		if (kScan_ == Scan::kIR)
		{
			// Select IR-Scan
			*buffer++ = tms1tck0 | tclk_level;
		}
		// Capture-DR / Capture-IR
		*buffer++ = tms0tck0 | tclk_level;
		*buffer++ = tms0tck0 | tclk_level;
		uint32_t mask = 0x0001U << ((uint8_t)kNumBits_ - 1);
		for (; mask > 1; mask >>= 1)
		{
			// Shift-DR / Shift-IR
			if ((data_out & mask) != 0)
				*buffer++ = tms0tck0 | tdi1;
			else
				*buffer++ = tms0tck0 | tdi0;
		}
		// Exit1-DR / Exit1-IR; The last bit
		if ((data_out & mask) != 0)
			*buffer++ = tms1tck0 | tdi1;
		else
			*buffer++ = tms1tck0 | tdi0;
		// Update-DR / Update-IR
		*buffer++ = tms1tck0 | tclk_level;
		// Run-test/IDLE
		*buffer++ = tms0tck0 | tclk_level;
	}
	static constexpr ALWAYS_INLINE uint8_t GetCount()
	{
		if (kScan_ == Scan::kGoIdle)
			return 8;
		else
			return (5 + (uint8_t)kNumBits_ + (uint8_t)kScan_);
	}
	static ALWAYS_INLINE void Start(
		uint32_t* buffer
		, uint32_t& rise_buffer		// CLK rise static buffer
	)
	{
		rise_buffer = JTCK::kBitValue_;
		uint16_t cnt = GetCount();
		// Buffer writing data
		DmaWrData::Start(buffer, &JTCK::Io().BSRR, cnt);
		// This channel always transmit the same data (the rising edge)
		DmaRise::Start(&rise_buffer, &JTCK::Io().BSRR, cnt);
		// Reuse the write stream for input, as write happens before read
		DmaRdData::Start(&JTDO::Io().IDR, buffer, cnt);
		CycleTimer::SetupRepetition(cnt);
		TriggerWrite::EnableDma();
		TriggerRise::EnableDma();
		TriggerRead::EnableDma();
		CycleTimer::CounterResume();
	}
	static ALWAYS_INLINE void Wait()
	{
		// Wait for timer to complete all cycles
		CycleTimer::WaitForAutoStop();
		/* JTAG state = Run-Test/Idle */
		TriggerWrite::DisableDma();
		TriggerRise::DisableDma();
		TriggerRead::DisableDma();
	}
	static ALWAYS_INLINE uint32_t GetResult(uint32_t* buffer)
	{
		size_t pos = 3 + (uint8_t)kScan_;
		uint32_t mask = 0x0001U << ((uint8_t)kNumBits_ - 1);
		uint32_t data = 0;
		for (; mask != 0; mask >>= 1, ++pos)
		{
			if ((buffer[pos] & JTDO::kBitValue_) != 0)
				data |= mask;
		}
		return data;
	}
	// Prepare a buffer to reset state of the JTAG state machine
	static ALWAYS_INLINE void DoGoIdle(
		uint32_t* buffer
		, uint32_t& rise_buffer		// CLK rise static buffer
	)
	{
		static_assert(kScan_ == Scan::kGoIdle && kNumBits_ == NumBits::kGoIdle, "This method cannot be used to render kGoIdle");

		constexpr uint32_t tms0 = JTMS::kBitValue_ << 16;
		constexpr uint32_t tms1 = JTMS::kBitValue_;
		constexpr uint32_t tck0 = JTCK::kBitValue_ << 16;
		constexpr uint32_t tck1 = JTCK::kBitValue_;
		constexpr uint32_t tms0tck0 = tms0 | tck0;
		constexpr uint32_t tms1tck0 = tms1 | tck0;
		constexpr uint32_t tms1tck1 = tms1 | tck1;

		uint32_t* wave = buffer;
		// First bit is dummy, just to ensure TMS is up without a clock change
		*wave++ = tms1tck1;
		*wave++ = tms1tck0;
		*wave++ = tms1tck0;
		*wave++ = tms1tck0;
		*wave++ = tms1tck0;
		*wave++ = tms1tck0;
		*wave++ = tms1tck0;
		*wave++ = tms0tck0;
		uint32_t tmp;
		// Change default prescaler to a slower one
		if (kScan_ == Scan::kGoIdle)
		{
			tmp = CycleTimer::GetPrescaler();
			CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		}
		Start(buffer, rise_buffer);
		Wait();
		if (kScan_ == Scan::kGoIdle)
			CycleTimer::SetPrescaler(tmp);
	}
};


/**
# GeneratorSTLinkPWM<> - Split-Port JTAG Generator with PWM Clock

This class implements JTAG frame generation for hardware with split JTAG buses,
specifically designed for ST-Link V2 clone hardware constraints. It uses PWM
(Pulse Width Modulation) to generate a precise 50% duty cycle JTCK clock
automatically, freeing DMA channels for data transfers across multiple ports.

## Operating Principles:

### PWM Clock Generation:
- **TIM1_CH1N PWM**: Uses complementary channel output for automatic 50% duty
  cycle clock generation without CPU or DMA intervention
- **8-cycle period**: Same as Generator<> for timing compatibility
- **50% duty cycle**: Compare value 4 gives 4 counts high, 4 counts low
- **Automatic toggling**: PWM hardware handles clock transitions, no DMA needed

### Split-Port Architecture:
- **Port B**: JTCK (PWM) and JTMS (GPIO with DMA)
- **Port A**: JTDI (output) and JTDO (input)
- **Separate DMA channels**: Each port requires independent DMA due to different
  peripheral addresses (BSRR/IDR on different GPIO ports)

### DMA Channel Usage:
1. **DmaTms**: Writes JTMS control signals to Port B BSRR
   - Triggered at count 5
   - Memory → Peripheral, single transfer mode
   
2. **DmaWrite**: Writes JTDI data to Port A BSRR
   - Triggered at count 2 (early in cycle)
   - Memory → Peripheral, single transfer mode
   
3. **DmaRead**: Reads JTDO data from Port A IDR
   - Triggered at count 1 (after clock rise)
   - Peripheral → Memory, single transfer mode

### Signal Timing (8-count cycle with PWM):
```
Count: 0   1   2   3   4   5   6   7
	   ┌───┬───┬───┬───┬───┬───┬───┬───┐
PWM    │███│███│███│███│   │   │   │   │ → JTCK 50% duty (PWM automatic)
	   ├───┼───┼───┼───┼───┼───┼───┼───┤
Write  │   │█W │   │   │   │   │   │   │ → JTDI data (Port A)
	   ├───┼───┼───┼───┼───┼───┼───┼───┤
Read   │   │   │█S │   │   │   │   │   │ → JTDO sample (Port A)
	   ├───┼───┼───┼───┼───┼───┼───┼───┤
TMS    │   │   │   │   │█T │   │   │   │ → JTMS control (Port B)
	   └───┴───┴───┴───┴───┴───┴───┴───┘
JTCK   ┌─────────────┐  ┌───────────────┐
	   │             │  │               │
	   └─────────────┘  └───────────────┘
		 PWM High (4)      PWM Low (4)
```

## Key Innovations:
1. **PWM-based clock**: Eliminates need for DMA channel for clock toggling
2. **Dynamic pin mode switching**: JTCK pin switches between GPIO (bit-bang)
   and PWM (automatic clock) modes at runtime
3. **Split-port optimization**: Accommodates ST-Link V2 hardware constraints
   where JTAG signals are physically separated across connectors

## Template Parameters:
@tparam SysClk          System clock configuration  
@tparam kTim            Master timer unit (must be TIM1 for CH1N PWM)
@tparam kTck            Timer channel for PWM clock output (must be channel 1)
@tparam kTms            Timer channel for JTMS DMA trigger
@tparam kWriteCh        Timer channel for JTDI DMA trigger
@tparam kReadCh         Timer channel for JTDO DMA trigger
@tparam kFreq           JTAG clock frequency
@tparam kScan           Scan type (DR, IR, or GoIdle)
@tparam kNumBits        Number of bits to shift

## Hardware Requirements:
- TIM1 with CH1N complementary output support (for PWM clock)
- JTDI and JTDO must be on same GPIO port (Port A)
- JTCK and JTMS typically on different port (Port B) 
- 3 independent DMA channels
- ST-Link V2 compatible pin mapping

## Limitations:
- This approach saturates DMA at aproximatelly 2.5 MHz, when JTMS delays so much that 
  frame is malformed
- Applying a safety margin we consider 2,250,000 MHz the topmost speed we can handle.
- Output polarity was chosen so that the output is high when the count is 0, since
  in JTAG bus clock rests on the upper level.
- So the rising edge occurs when counter reloads and this is the edge used to acquire 
  data and update states.
- Since data is more important than control, priorities were choosen so that DMA 
  happens on a deterministic order, in case of overloads.
- Above the 2 MHz JTMS signal is dangerously aproaching the clock rising edge, which 
  means that bus saturation delays are at limit.
*/
template <
	typename SysClk							///< System clock that drives timers
	, const Timer::Unit kTim				///< Master timer (must be TIM1 for CH1N PWM)
	, const Timer::Channel kTck				///< Timer channel for PWM clock output (typically CH1)
	, const Timer::Channel kTms				///< Timer channel for JTMS control DMA trigger
	, const Timer::Channel kWriteCh			///< Timer channel for JTDI data DMA trigger
	, const Timer::Channel kReadCh			///< Timer channel for JTDO read DMA trigger
	, const uint32_t kFreq					///< Frequency of JTAG clock
	, const Scan kScan						///< Type of JTAG scan operation
	, const NumBits kNumBits				///< Number of bits to shift
>
class GeneratorSTLinkPWM
{
public:
	/// Time Base for the JCLK generation
	/// @note 8 timer cycles per JTAG clock cycle for precise timing control
	using MasterClock = Timer::InternalClock_Hz<kTim, SysClk, 8 * kFreq>;
	
	/// Master timer generating the 8-count waveform
	/// @note Single-shot mode with repetition counter for exact bit counts
	/// @note Auto-reload disabled, repetition counter controls cycle count
	using CycleTimer = Timer::Any<MasterClock, Timer::Mode::kSingleShot, 7, false, true>;

	/// Timer output channel triggering JTMS DMA transfers
	/// @note Configured for compare event at count 5 (after clock high)
	/// @note Triggers DMA to write JTMS control bits to Port B BSRR
	using TriggerTms = Timer::AnyOutputChannel<CycleTimer, kTms>;
	
	/// DMA channel for JTMS control signal updates
	/// @note Writes to Port B BSRR (bit-set/reset register)
	/// @note Memory increment, peripheral fixed address
	/// @note Medium priority for control path timing
	using DmaTms = Dma::AnyChannel
		<
		typename TriggerTms::DmaChInfo_
		, Dma::Dir::kMemToPer
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::PtrPolicy::kLongPtr
		, Dma::Prio::kMedium
		>;

	/// Timer output channel triggering JTDI DMA transfers
	/// @note Configured for compare event at count 2 (early in cycle)
	/// @note Triggers DMA to write JTDI data bits to Port A BSRR
	using TriggerWrite = Timer::AnyOutputChannel<CycleTimer, kWriteCh>;
	
	/// DMA channel for JTDI data output
	/// @note Writes to Port A BSRR (bit-set/reset register)
	/// @note Memory increment, peripheral fixed address
	/// @note Highest priority aligned with data path
	using DmaWrite = Dma::AnyChannel
		<
		typename TriggerWrite::DmaChInfo_
		, Dma::Dir::kMemToPer
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::PtrPolicy::kLongPtr
		, Dma::Prio::kHigh
		>;

	/// Timer output channel triggering JTDO DMA transfers
	/// @note Configured for compare event at count 1 (after clock rise)
	/// @note Triggers DMA to read JTDO data from Port A IDR
	using TriggerRead = Timer::AnyOutputChannel<CycleTimer, kReadCh>;
	
	/// DMA channel for JTDO data input
	/// @note Reads from Port A IDR (input data register)
	/// @note Peripheral fixed address, memory increment
	/// @note Medium priority for read completion timing
	using DmaRead = Dma::AnyChannel
		<
		typename TriggerRead::DmaChInfo_
		, Dma::Dir::kPerToMem
		, Dma::PtrPolicy::kLongPtr
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::Prio::kVeryHigh
		>;

	/// PWM output channel for automatic JTCK generation
	/// @note Configured for PWM mode 1 (active while CNT < CCR)
	/// @note Complementary output (CH1N) for ST-Link V2 hardware
	/// @note Compare value 4 gives 50% duty cycle (4 high, 4 low)
	/// @note Fast enable for rapid mode switching
	using JTCKout = Timer::AnyOutputChannel
		<
		CycleTimer
		, kTck
		, Timer::OutMode::kPWM1		// Active while CNT < CCR
		, Timer::Output::kDisabled	// Normal state (off)
		, Timer::Output::kEnabled	// Complementary output on
		, false						// No break functionality
		, true						// Fast enable for mode switching
		>;

	// Type of JTAG scan
	static constexpr Scan kScan_ = kScan;
	// Bit count
	static constexpr NumBits kNumBits_ = kNumBits;

public:
	/// Hardware initialization
	static ALWAYS_INLINE void Init()
	{
		// Repetition counter is only available on the advanced timer
		static_assert(CycleTimer::HasRepetitionCounter(), "Timer needs to be the advanced timer (TIM1)");
		static_assert(DmaTms::kChan_ != DmaWrite::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(DmaTms::kChan_ != DmaRead::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(DmaWrite::kChan_ != DmaRead::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(JTCK::kPortBase_ == JTCK_PWM::kPortBase_, "JTCK and JTCK_PWM must be on same port");
		static_assert(JTCK::kPin_ == JTCK_PWM::kPin_, "JTCK and JTCK_PWM must be same pin");
		static_assert(JTDI::kPortBase_ == JTDO::kPortBase_, "JTDI and JTDO expected on the same port");

		CycleTimer::Init();				// Timer generates time base
		DmaTms::Init();
		DmaWrite::Init();
		DmaRead::Setup();
		JTCKout::Setup();
		TriggerTms::Setup();
		TriggerWrite::Setup();
		TriggerRead::Setup();
		// Set compare values for precise timing control
		// Timer counts from 0 to 7 (8-count cycle)
		// PWM: JTCK high when CNT < CCR=4 (counts 0-3), low when CNT >= 4 (counts 4-7)
		TriggerRead::SetCompare(0);		// JTDO read after clock rise (count 6)
		TriggerWrite::SetCompare(1);	// JTDI write early in cycle (count 2)
		TriggerTms::SetCompare(2);		// JTMS update after clock high (count 5)
		JTCKout::SetCompare(4);			// PWM 50% duty: high 0-3, low 4-7
		
		// Timing diagram for 8-count cycle:
		// Count:    0   1   2   3   4   5   6   7
		//         ┌───┬───┬───┬───┬───┬───┬───┬───┐
		// PWM     │███│███│███│███│   │   │   │   │ → JTCK high (CNT < 4)
		//         ├───┼───┼───┼───┼───┼───┼───┼───┤
		// Read    │█S │   │   │   │   │   │   │   │ → JTDO @0 (data sample)
		//         ├───┼───┼───┼───┼───┼───┼───┼───┤
		// Write   │   │█W │   │   │   │   │   │   │ → JTDI @1 (data setup)
		//         ├───┼───┼───┼───┼───┼───┼───┼───┤
		// TMS     │   │   │█T │   │   │   │   │   │ → JTMS @2 (control update)
		//         └───┴───┴───┴───┴───┴───┴───┴───┘
		// JTCK    ┌───────────────┐   ┌───────────────┐
		//         │               │   │               │
		//         └───────────────┘   └───────────────┘
		//          PWM High (0-3)      PWM Low (4-7)
	}
	// Restores DMA Setup for this use (when sharing DMA with other peripherals)
	static ALWAYS_INLINE void SetupDma()
	{
		DmaTms::Setup();
		DmaWrite::Setup();
		DmaRead::Setup();
	}
	// Restores DMA channels for repurpose, when sharing DMA with other peripherals
	static ALWAYS_INLINE void ReleaseDma()
	{
		DmaTms::Disable();
		DmaWrite::Disable();
		DmaRead::Disable();
	}
	/// Prepares a buffer with a JTAG transaction
	/// JTDI idle level must be maintained to preserve CPU state when JTAG is in Run-test/IDLE
	static ALWAYS_INLINE void RenderTransaction(
		uint32_t* buffer_tdi				///< Buffer for JTDI data
		, uint32_t* buffer_tms				///< Buffer for JTMS control bits
		, const uint32_t tclk_level			///< BSRR value for the initial JTDI idle level
		, const uint32_t data_out
	)
	{
		static_assert(kScan_ != Scan::kGoIdle && kNumBits_ != NumBits::kGoIdle, "This method cannot be used to render kGoIdle");

		constexpr uint32_t tms0 = JTMS::kBitValue_ << 16;
		constexpr uint32_t tms1 = JTMS::kBitValue_;
		constexpr uint32_t tdi0 = JTDI::kBitValue_ << 16;
		constexpr uint32_t tdi1 = JTDI::kBitValue_;

		// Select DR-Scan (maintain JTDI idle level)
		*buffer_tdi++ = tclk_level;
		*buffer_tms++ = tms1;
		if (kScan_ == Scan::kIR)
		{
			// Select IR-Scan (maintain JTDI idle level)
			*buffer_tdi++ = tclk_level;
			*buffer_tms++ = tms1;
		}
		// Capture-DR / Capture-IR
		*buffer_tdi++ = tclk_level;
		*buffer_tdi++ = tclk_level;
		*buffer_tms++ = tms0;
		*buffer_tms++ = tms0;
		uint32_t mask = 0x0001U << ((uint8_t)kNumBits_ - 1);
		for (; mask > 1; mask >>= 1)
		{
			// Shift-DR / Shift-IR
			if ((data_out & mask) != 0)
				*buffer_tdi++ = tdi1;
			else
				*buffer_tdi++ = tdi0;
			*buffer_tms++ = tms0;
		}
		// Exit1-DR / Exit1-IR; The last bit
		if ((data_out & mask) != 0)
			*buffer_tdi++ = tdi1;
		else
			*buffer_tdi++ = tdi0;
		*buffer_tms++ = tms1;
		// Update-DR / Update-IR (maintain JTDI idle level)
		*buffer_tdi++ = tclk_level;
		*buffer_tms++ = tms1;
		// Run-test/IDLE (maintain JTDI idle level)
		*buffer_tdi++ = tclk_level;
		*buffer_tms++ = tms0;
	}
	static constexpr ALWAYS_INLINE uint8_t GetCount()
	{
		if (kScan_ == Scan::kGoIdle)
			return 8;
		else
			return (5 + (uint8_t)kNumBits_ + (uint8_t)kScan_);
	}
	static ALWAYS_INLINE void Start(
		uint32_t* buffer_tdo		///< JTDO data buffer
		, uint32_t* buffer_tdi		///< JTDI data buffer
		, uint32_t* buffer_tms		///< JTMS control buffer
	)
	{
		// Switch JTCK pin from GPIO to PWM mode (ensure clock is high before switching)
		uint16_t cnt = GetCount();
		CycleTimer::ClearStatus();
		CycleTimer::SetupRepetition(cnt);
		DmaTms::Start(buffer_tms, &JTMS::Io().BSRR, cnt);
		DmaWrite::Start(buffer_tdi, &JTDI::Io().BSRR, cnt);
		DmaRead::Start(&JTDO::Io().IDR, buffer_tdo, cnt);
		TriggerTms::EnableDma();
		TriggerWrite::EnableDma();
		TriggerRead::EnableDma();
		JTCK_PWM::Setup();
		CycleTimer::CounterResume();
	}
	static ALWAYS_INLINE void Wait()
	{
		// Wait for timer to complete all cycles
		CycleTimer::WaitForAutoStop();
		/* JTAG state = Run-Test/Idle */
		// Disable DMA channels to prevent residual transfers
		DmaTms::Disable();
		DmaWrite::Disable();
		DmaRead::Disable();
		// Switch JTCK pin from PWM back to GPIO mode (clock is now high from PWM idle)
		JTCK::SetHigh();
		JTCK::SetupPinMode();
	}
	static ALWAYS_INLINE uint32_t GetResult(uint32_t* buffer)
	{
		size_t pos = 3 + (uint8_t)kScan_;
		uint32_t mask = 0x0001U << ((uint8_t)kNumBits_ - 1);
		uint32_t data = 0;
		for (; mask != 0; mask >>= 1, ++pos)
		{
			if ((buffer[pos] & JTDO::kBitValue_) != 0)
				data |= mask;
		}
		return data;
	}
	// Prepare a buffer to reset state of the JTAG state machine
	static ALWAYS_INLINE void DoGoIdle(
		uint32_t* buffer_tdo		///< JTDO buffer
		, uint32_t* buffer_tdi		///< JTDI buffer
		, uint32_t* buffer_tms		///< JTMS buffer
	)
	{
		static_assert(kScan_ == Scan::kGoIdle && kNumBits_ == NumBits::kGoIdle, "This method cannot be used to render kGoIdle");

		constexpr uint32_t tms0 = JTMS::kBitValue_ << 16;
		constexpr uint32_t tms1 = JTMS::kBitValue_;

		uint32_t* tdi = buffer_tdi;
		uint32_t* tms = buffer_tms;
		// First bit is dummy, just to ensure TMS is up without a clock change
		*tdi++ = 0;	*tms++ = tms1;
		*tdi++ = 0;	*tms++ = tms1;
		*tdi++ = 0;	*tms++ = tms1;
		*tdi++ = 0;	*tms++ = tms1;
		*tdi++ = 0;	*tms++ = tms1;
		*tdi++ = 0;	*tms++ = tms1;
		*tdi++ = 0;	*tms++ = tms0;
		uint32_t tmp;
		// Change default prescaler to a slower one
		if (kScan_ == Scan::kGoIdle)
		{
			tmp = CycleTimer::GetPrescaler();
			CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		}
		Start(buffer_tdo, buffer_tdi, buffer_tms);
		Wait();
		if (kScan_ == Scan::kGoIdle)
			CycleTimer::SetPrescaler(tmp);
	}
};


} // namespace WaveJtag
