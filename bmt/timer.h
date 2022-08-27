#pragma once

#include "dma.h"


/// Enumeration for the timer instance
enum TimInstance
{
	kTim1 = 1	///< Timer 1
	, kTim2		///< Timer 2
	, kTim3		///< Timer 3
	, kTim4		///< Timer 4
};

/// The timer channel
enum TimChannel
{
	kTimCh1		///< Timer Channel 1
	, kTimCh2	///< Timer Channel 2
	, kTimCh3	///< Timer Channel 3
	, kTimCh4	///< Timer Channel 4
};

/// Capture block input control
enum CaptureEdge
{
	kRisingEdge,
	kFallingEdge,
};

/// Timer count mode
enum TimerMode
{
	kCountUp,
	kCountDown,
	kSingleShot,
	kSingleShotDown,
};

/// External clock source selection
enum ExtClockSource
{
	kETR,			///< ETR signal after being prescaled, synchronized then filtered
	kETRN,			///< ETR signal after being prescaled, synchronized then filtered
	kTI1F_EdgeDet,	///< The TI1FD signal which sensitive to both signal edges
	kTI1FP1Clk,		///< The TI1FP1 input signal that are the synchronized, filtered TI1
	kTI1FP1ClkN,	///< The negative TI1FP1 input signal
	kTI2FP2Clk,		///< The TI2FP2 input signal that are the synchronized, filtered TI2
	kTI2FP2ClkN,	///< The negative TI2FP2 input signal
};

/// Master timer mode
enum MasterTimerMode
{
	kUpdate,		///< Sends trigger to slave on every update event
	kEnable,		///< Sends trigger to slave when master is enabled
	kComparePulse,	///< Positive pulse on CC1IF
	kCompare1,		///< OC1REF used as trigger output
	kCompare2,		///< OC1REF used as trigger output
	kCompare3,		///< OC1REF used as trigger output
	kCompare4,		///< OC1REF used as trigger output
};


/// Slave timer mode
enum SlaveTimerMode
{
	kMasterIsClock,	///< Triggers from master are used to generate clock
	kResetCnt,		///< Trigger from master clears the slave counter
	kGatedMode,		///< High triggers from master enables/gates the slave counter clock
	kStartMode,		///< Counter start when master triggers. Only start is controlled
};

/// Timer input for capturing
enum InputCapture
{
	kTI1FP1,	///< TI1FP1 clock input
	kTI1FP2,	///< TI1FP2 clock input
	kTI2FP1,	///< TI2FP1 clock input
	kTI2FP2,	///< TI2FP2 clock input
	kTI3FP3,	///< TI3FP3 clock input
	kTI3FP4,	///< TI3FP4 clock input
	kTI4FP3,	///< TI4FP3 clock input
	kTI4FP4,	///< TI4FP4 clock input
	kTRC,		///< TRC clock input
};

enum TimOutMode
{
	kTimOutFrozen = 0
	, kTimOutSet
	, kTimOutReset
	, kTimOutToggle
	, kTimOutLow
	, kTimOutHigh
	, kTimOutPwm1
	, kTimOutPwm2
};

enum TimOutDrive
{
	kTimOutInactive
	, kTimOutActiveHigh
	, kTimOutActiveLow
};


template <
	const TimInstance kTimerNum
>
class AnyTimer_
{
public:
	static constexpr TimInstance kTimerNum_ = kTimerNum;
	static constexpr uintptr_t kTimerBase_ =
		(kTimerNum_ == kTim1) ? TIM1_BASE
		: (kTimerNum_ == kTim2) ? TIM2_BASE
		: (kTimerNum_ == kTim3) ? TIM3_BASE
		: (kTimerNum_ == kTim4) ? TIM4_BASE
		: 0;
	static constexpr DmaInstance DmaInstance_ = kDma1;
	// Timer update DMA channel
	static constexpr DmaCh DmaCh_ = 
		(kTimerNum_ == kTim1) ? kDmaCh5
		: (kTimerNum_ == kTim2) ? kDmaCh2
		: (kTimerNum_ == kTim3) ? kDmaCh3
		: (kTimerNum_ == kTim4) ? kDmaCh7
		: kDmaChNone;
	// Timer trigger DMA channel
	static constexpr DmaCh DmaChTrigger_ = 
		(kTimerNum_ == kTim1) ? kDmaCh4
		: (kTimerNum_ == kTim3) ? kDmaCh6
		: kDmaChNone;
	/// Timer having BDTR register
	static constexpr bool kHasBdtr = (kTimerNum_ == kTim1);

	ALWAYS_INLINE static TIM_TypeDef *GetDevice()
	{
		static_assert(kTimerBase_ != 0, "Invalid timer instance selected");
		return (TIM_TypeDef *)kTimerBase_;
	}

	ALWAYS_INLINE static void EnableTriggerDma(void)
	{
		if (DmaCh_ != kDmaChNone)
		{
			TIM_TypeDef* timer_ = GetDevice();
			timer_->DIER |= TIM_DIER_TDE;
		}
		else
		{
			// MCU does not support this DMA channel
			McuCore::Abort();
		}
	}

	ALWAYS_INLINE static void DisableTriggerDma(void)
	{
		if (DmaCh_ != kDmaChNone)
		{
			TIM_TypeDef* timer_ = GetDevice();
			timer_->DIER &= ~TIM_DIER_TDE;
		}
		else
		{
			// MCU does not support this DMA channel
			McuCore::Abort();
		}
	}

	ALWAYS_INLINE static void EnableUpdateDma(void)
	{
		if (DmaCh_ != kDmaChNone)
		{
			TIM_TypeDef* timer_ = GetDevice();
			timer_->DIER |= TIM_DIER_UDE;
		}
		else
		{
			// MCU does not support this DMA channel
			McuCore::Abort();
		}
	}

	ALWAYS_INLINE static void DisableUpdateDma(void)
	{
		if (DmaCh_ != kDmaChNone)
		{
			TIM_TypeDef* timer_ = GetDevice();
			timer_->DIER &= ~TIM_DIER_UDE;
		}
		else
		{
			// MCU does not support this DMA channel
			McuCore::Abort();
		}
	}
};


//! Template to adjust timer prescaler to register counts
template <
	const TimInstance kTimerNum
	, typename SysClk
	, const uint32_t kPrescaler = 0U	// max speed
>
class InternalClock : public AnyTimer_<kTimerNum>
{
public:
	typedef AnyTimer_<kTimerNum> BASE;
	static constexpr uint32_t kFrequency_ = SysClk::kFrequency_;
	static constexpr uint32_t kClkTick = (BASE::kTimerNum_ == kTim1)
		? SysClk::kApb2TimerClock_
		: SysClk::kApb1TimerClock_
		;
	static constexpr uint32_t kPrescaler_ = kPrescaler;
};


//! Template to adjust timer prescaler to us
template <
	const TimInstance kTimerNum
	, typename SysClk
	, const uint32_t kMicroSecs = 1000U
>
class InternalClock_us : public AnyTimer_<kTimerNum>
{
public:
	typedef AnyTimer_<kTimerNum> BASE;
	static constexpr uint32_t kClkTick = (BASE::kTimerNum_ == kTim1)
		? SysClk::kApb2TimerClock_
		: SysClk::kApb1TimerClock_
		;
	static constexpr double kTimerTick_ = kMicroSecs / 1000000.0;
	static constexpr uint32_t kPrescaler_raw_ = (uint32_t)(kTimerTick_ * kClkTick + 0.5);
	static constexpr uint32_t kPrescaler_ = kPrescaler_raw_ > 0 ? kPrescaler_raw_ - 1 : 0;

	ALWAYS_INLINE static void Setup() {}
};


//! Template to adjust timer prescaler to Hz
template <
	const TimInstance kTimerNum
	, typename SysClk
	, const uint32_t kMHz = 1000000
>
class InternalClock_Hz : public AnyTimer_<kTimerNum>
{
public:
	typedef AnyTimer_<kTimerNum> BASE;
	static constexpr uint32_t kFrequency_ = SysClk::kFrequency_;
	static constexpr uint32_t kClkTick = (BASE::kTimerNum_ == kTim1)
		? SysClk::kApb2TimerClock_
		: SysClk::kApb1TimerClock_
		;
	static constexpr uint32_t kPrescaler_raw_ = (uint32_t)((kClkTick + kMHz/2) / kMHz);
	static constexpr uint32_t kPrescaler_ = kPrescaler_raw_ > 0 ? kPrescaler_raw_ - 1 : 0;

	ALWAYS_INLINE static void Setup() {}
};


template <
	const TimInstance kTimerNum
	, const ExtClockSource kExtIn
	, const uint32_t kFreq = 1000000	// this value has no effect, but helps interacting with template
	, const uint32_t kPrescaler = 1
	, const uint32_t kFilter = 0		// a value between 0 and 15 (see docs)
>
class ExternalClock : public AnyTimer_<kTimerNum>
{
public:
	typedef AnyTimer_<kTimerNum> BASE;
	static constexpr ExtClockSource kExtIn_ = kExtIn;
	static constexpr uint32_t kFrequency_ = kFreq;
	static constexpr uint32_t kPrescaler_ = 0;
	static constexpr uint32_t kInputPrescaler_ = kPrescaler;
	static constexpr bool kUsesInput1 = (kExtIn == kTI1F_EdgeDet || kExtIn == kTI1FP1Clk);
	static constexpr bool kUsesInput2 = (kExtIn == kTI2FP2Clk);
	static constexpr uint16_t kSmcr_Mask = TIM_SMCR_MSM_Msk;
	static constexpr uint16_t kCcmr_Mask =
		(kUsesInput1) ? TIM_CCMR1_CC1S_Msk | TIM_CCMR1_IC1PSC_Msk | TIM_CCMR1_IC1F_Msk
		: (kUsesInput2) ? TIM_CCMR1_CC2S_Msk | TIM_CCMR1_IC2PSC_Msk | TIM_CCMR1_IC2F_Msk
		: 0;
	static constexpr uint16_t kCcer_Mask =
		(kUsesInput1) ? TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk | TIM_CCER_CC1NE_Msk | TIM_CCER_CC1NP_Msk
		: (kUsesInput2) ? TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk | TIM_CCER_CC2NE_Msk | TIM_CCER_CC2NP_Msk
		: 0;

	ALWAYS_INLINE static void Setup()
	{
		// Validate prescaler
		static_assert(kInputPrescaler_ == 1 || kInputPrescaler_ == 2 || kInputPrescaler_ == 4 || kInputPrescaler_ == 8, "Invalid prescaler value. Possible values are 1,2,4 or 8.");
		// Validate filter
		static_assert(kFilter < 16, "Invalid ETRP filter value. Only values between 0 and 15 are allowed.");

		TIM_TypeDef* timer = BASE::GetDevice();
		// Use a local variable so the code optimizer condenses all logic to a single constant
		uint32_t tmp = 0;
		// Apply pulse polarity
		if (kExtIn_ == kETRN)
			tmp |= TIM_SMCR_ETP;

		switch (kExtIn_)
		{
		case kETR:
		case kETRN:
			// Apply mode 2 bit
			tmp |= TIM_SMCR_ECE;
			switch (kInputPrescaler_)
			{
			case 2:
				tmp |= TIM_SMCR_ETPS_0;
				break;
			case 4:
				tmp |= TIM_SMCR_ETPS_1;
				break;
			case 8:
				tmp |= TIM_SMCR_ETPS_0 | TIM_SMCR_ETPS_1;
				break;
			default:
				break;
			}
			tmp |= (kFilter << TIM_SMCR_ETF_Pos);
			tmp |= TIM_SMCR_TS_0 | TIM_SMCR_TS_1 | TIM_SMCR_TS_2;
			break;
		case kTI1F_EdgeDet:
			tmp |= TIM_SMCR_TS_2;
			break;
		case kTI1FP1Clk:
			tmp |= TIM_SMCR_TS_0 | TIM_SMCR_TS_2;
			break;
		case kTI2FP2Clk:
			tmp |= TIM_SMCR_TS_1 | TIM_SMCR_TS_2;
			break;
		default:
			break;
		}
		// Apply slave mode
		tmp |= TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0;
		timer->SMCR = (timer->SMCR & kSmcr_Mask) | tmp;

		// Setup CCMR1 register
		if (kUsesInput1 || kUsesInput2)
		{
			tmp = 0;
			switch (kInputPrescaler_)
			{
			case 2:
				tmp |= kUsesInput1 ? TIM_CCMR1_IC1PSC_0 : TIM_CCMR1_IC2PSC_0;
				break;
			case 4:
				tmp |= kUsesInput1 ? TIM_CCMR1_IC1PSC_1 : TIM_CCMR1_IC2PSC_1;
				break;
			case 8:
				tmp |= kUsesInput1 ? TIM_CCMR1_IC1PSC_0 | TIM_CCMR1_IC1PSC_1
					: TIM_CCMR1_IC2PSC_0 | TIM_CCMR1_IC2PSC_1;
				break;
			default:
				break;
			}
			// Always use this setting as input
			tmp |= kUsesInput1 ? TIM_CCMR1_CC1S_0 : TIM_CCMR1_CC2S_1;
			timer->CCMR1 = (timer->CCMR1 & ~kCcmr_Mask) | tmp;

			// Setup CCER register
			tmp = 0;
			if (kExtIn_ == kTI1FP1ClkN)
				tmp |= TIM_CCER_CC1P;
			else if(kExtIn_ == kTI2FP2ClkN)
				tmp |= TIM_CCER_CC2P;
			timer->CCER = (timer->CCER & ~kCcer_Mask) | tmp;
		}
	}
};


/*!
Template that establishes a Master/Slave relation between two timers.
Following parameters are configurable:
- The master timer that will source events
- The slave timer that will be triggered by master events
- The source of event on the master that will be directed to the slave timer
- The effect of the master trigger on the slave

Examples:
	// TIM1 is master and updates on it will be used as clock for TIM2
	typedef MasterSlaveTimers<kTim1, kTim2> Tim1PrescalerToTim2;
	Tim1PrescalerToTim2::Setup();
	// Use Output Compare of Timer 1 to enable Timer 2
	// Note: TIM1 clock and Compare register needs to be setup
	MasterSlaveTimers<kTim1, kTim2, kCompare1, kGatedMode>::Setup();
*/
template <
	const TimInstance kMasterTimer						///< Master timer
	, const TimInstance kSlaveTimer						///< Slave timer
	, const MasterTimerMode kMasterMode = kUpdate		///< Master timer mode
	, const SlaveTimerMode kSlaveMode = kMasterIsClock	///< Master used as clock source
	, const uint32_t kPrescaler = 0						///< Optional prescaler
>
class MasterSlaveTimers : public AnyTimer_<kSlaveTimer>
{
public:
	typedef AnyTimer_<kSlaveTimer> BASE;
	typedef AnyTimer_<kMasterTimer> MASTER;
	static constexpr TimInstance kMasterTimer_ = kMasterTimer;
	static constexpr uint32_t kPrescaler_ = kPrescaler;
	static constexpr uint32_t kTS_ =
		// TIM1
#if 0
		kSlaveTimer == kTim1 && kMasterTimer == kTim5 ? 0 :
#endif
		kSlaveTimer == kTim1 && kMasterTimer == kTim2 ? TIM_SMCR_TS_0 :
		kSlaveTimer == kTim1 && kMasterTimer == kTim3 ? TIM_SMCR_TS_1 :
		kSlaveTimer == kTim1 && kMasterTimer == kTim4 ? TIM_SMCR_TS_0 | TIM_SMCR_TS_1 :
		// TIM2
		kSlaveTimer == kTim2 && kMasterTimer == kTim1 ? 0 :
#if 0
		kSlaveTimer == kTim2 && kMasterTimer == kTim8 ? TIM_SMCR_TS_0 :
#endif
		kSlaveTimer == kTim2 && kMasterTimer == kTim3 ? TIM_SMCR_TS_1 :
		kSlaveTimer == kTim2 && kMasterTimer == kTim4 ? TIM_SMCR_TS_0 | TIM_SMCR_TS_1 :
		// TIM3
		kSlaveTimer == kTim3 && kMasterTimer == kTim1 ? 0 :
		kSlaveTimer == kTim3 && kMasterTimer == kTim2 ? TIM_SMCR_TS_0 :
#if 0
		kSlaveTimer == kTim3 && kMasterTimer == kTim5 ? TIM_SMCR_TS_1 :
#endif
		kSlaveTimer == kTim3 && kMasterTimer == kTim4 ? TIM_SMCR_TS_0 | TIM_SMCR_TS_1 :
		// TIM4
		kSlaveTimer == kTim4 && kMasterTimer == kTim1 ? 0 :
		kSlaveTimer == kTim4 && kMasterTimer == kTim2 ? TIM_SMCR_TS_0 :
		kSlaveTimer == kTim4 && kMasterTimer == kTim3 ? TIM_SMCR_TS_1 :
#if 0
		kSlaveTimer == kTim4 && kMasterTimer == kTim8 ? TIM_SMCR_TS_0 | TIM_SMCR_TS_1 :
		// TIM5
		kSlaveTimer == kTim5 && kMasterTimer == kTim2 ? 0 :
		kSlaveTimer == kTim5 && kMasterTimer == kTim3 ? TIM_SMCR_TS_0 :
		kSlaveTimer == kTim5 && kMasterTimer == kTim4 ? TIM_SMCR_TS_1 :
		kSlaveTimer == kTim5 && kMasterTimer == kTim8 ? TIM_SMCR_TS_0 | TIM_SMCR_TS_1 :
		// TIM8
		kSlaveTimer == kTim8 && kMasterTimer == kTim1 ? 0 :
		kSlaveTimer == kTim8 && kMasterTimer == kTim2 ? TIM_SMCR_TS_0 :
		kSlaveTimer == kTim8 && kMasterTimer == kTim4 ? TIM_SMCR_TS_1 :
		kSlaveTimer == kTim8 && kMasterTimer == kTim5 ? TIM_SMCR_TS_0 | TIM_SMCR_TS_1 :
#endif
		TIM_SMCR_TS_2;
	static constexpr uint32_t kMMS_ =
		kMasterMode == kEnable ? TIM_CR2_MMS_0 :
		kMasterMode == kComparePulse ? TIM_CR2_MMS_1 | TIM_CR2_MMS_0 :
		kMasterMode == kCompare1 ? TIM_CR2_MMS_2 :
		kMasterMode == kCompare2 ? TIM_CR2_MMS_2 | TIM_CR2_MMS_0 :
		kMasterMode == kCompare3 ? TIM_CR2_MMS_2 | TIM_CR2_MMS_1 :
		kMasterMode == kCompare4 ? TIM_CR2_MMS_2 | TIM_CR2_MMS_1 | TIM_CR2_MMS_0 :
		TIM_CR2_MMS_1;
	static constexpr uint32_t kSMS_ =
		kSlaveMode == kResetCnt ? TIM_SMCR_SMS_2 :
		kSlaveMode == kGatedMode ? TIM_SMCR_SMS_2 | TIM_SMCR_SMS_0 :
		kSlaveMode == kStartMode ? TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 :
		TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_2;

	ALWAYS_INLINE static void Setup()
	{
		// Validate prescaler
		static_assert(kTS_ < TIM_SMCR_TS_2, "Master/Slave combination not supported by hardware");

		BASE::GetDevice()->SMCR = kTS_ | kSMS_;
		// Master timer trigger generation
		TIM_TypeDef* master = MASTER::GetDevice();
		master->SMCR |= TIM_SMCR_MSM;
		// Master timer mode
		master->CR2 = (master->CR2 & ~TIM_CR2_MMS_Msk) | kMMS_;
	}
};


/*!
** @brief Core methods to access a timer unit
** This class is used as core to setup a timer.
**
** @tparam TimeBase: A timer base used for the timer (a InternalClock_us, 
**		InternalClock_Hz or ExtTimeBase declaration)
** @tparam kTimerMode: See TimerMode
** @tparam kReload: A value used for the auto-reload register (ARR)
** @tparam kBuffered: If auto-reload register should have a buffer (ARPE bit)
*/
template <
	typename TimeBase
	, const TimerMode kTimerMode = kCountUp
	, const uint32_t kReload = 0
	, const bool kBuffered = true
>
class TimerTemplate : public AnyTimer_<TimeBase::kTimerNum_>
{
public:
	typedef AnyTimer_<TimeBase::kTimerNum_> BASE;
	typedef uint16_t TypCnt;
	static constexpr uint32_t kPrescaler_ = TimeBase::kPrescaler_;
	static constexpr TimerMode kTimerMode_ = kTimerMode;
	static constexpr uint32_t kCr1Mask = TIM_CR1_CEN_Msk | TIM_CR1_UDIS_Msk | TIM_CR1_URS_Msk
		| TIM_CR1_OPM_Msk | TIM_CR1_DIR_Msk | TIM_CR1_CMS_Msk | TIM_CR1_ARPE_Msk
		| TIM_CR1_CKD_Msk
		;

	ALWAYS_INLINE static void Init()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		// Enable clock
		switch (BASE::kTimerNum_)
		{
		case kTim1:
			RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
			RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
			RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;
			break;
		case kTim2:
			RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
			RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
			break;
		case kTim3:
			RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM3RST;
			RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM3RST;
			break;
		case kTim4:
			RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM4RST;
			RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM4RST;
			break;
		}
		TimeBase::Setup();
		Setup();
	}

	//! Performs timer setup
	ALWAYS_INLINE static void Setup()
	{
		TIM_TypeDef *timer = BASE::GetDevice();
		// Compute CR1 register
		uint32_t tmp = kBuffered ? TIM_CR1_ARPE : 0;
		if (kTimerMode_ == kCountDown)
		{
			tmp |= TIM_CR1_DIR;
		}
		else if (kTimerMode_ == kSingleShot)
		{
			tmp |= TIM_CR1_OPM;
		}
		else if (kTimerMode_ == kSingleShotDown)
		{
			tmp |= TIM_CR1_DIR | TIM_CR1_OPM;
		}
		timer->CR1 = (timer->CR1 & ~kCr1Mask) | tmp;
		// Initialize BDTR
		if (BASE::kHasBdtr)
			timer->BDTR = 0;	// unsupported by this template
		// Compute prescaler to obtain tick count value
		constexpr uint32_t tmp2 = TimeBase::kPrescaler_;
		// Timer tick base is impossible for the hardware
		static_assert(tmp2 <= 65535, "Prescaler overflow! Timer clock is too high for the desired time base.");
		timer->PSC = tmp2;
		// reload value
		if (kReload)
		{
			timer->ARR = kReload;
			timer->EGR = TIM_EGR_UG;	// Generate an update event to reload the Prescaler
		}
	}

	//! Disables timer peripheral on the APB register.
	ALWAYS_INLINE static void Stop()
	{
		switch (BASE::kTimerNum_)
		{
		case kTim1: RCC->APB2ENR &= ~RCC_APB2ENR_TIM1EN; break;
		case kTim2: RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN; break;
		case kTim3: RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN; break;
		case kTim4: RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN; break;
		}
	}

	//! Enables interrupt masks
	ALWAYS_INLINE static void EnableIrq(void)
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		switch (BASE::kTimerNum_)
		{
		case kTim1: NVIC_ClearPendingIRQ(TIM1_BRK_IRQn); NVIC_EnableIRQ(TIM1_BRK_IRQn);
			NVIC_ClearPendingIRQ(TIM1_CC_IRQn); NVIC_EnableIRQ(TIM1_CC_IRQn); break;
		case kTim2: NVIC_ClearPendingIRQ(TIM2_IRQn); NVIC_EnableIRQ(TIM2_IRQn); break;
		case kTim3: NVIC_ClearPendingIRQ(TIM3_IRQn); NVIC_EnableIRQ(TIM3_IRQn); break;
		case kTim4: NVIC_ClearPendingIRQ(TIM4_IRQn); NVIC_EnableIRQ(TIM4_IRQn); break;
		}
	}

	//! Disables interrupts
	ALWAYS_INLINE static void DisableIrq(void)
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		switch (BASE::kTimerNum_)
		{
		case kTim1: NVIC_DisableIRQ(TIM1_BRK_IRQn);
			NVIC_DisableIRQ(TIM1_CC_IRQn); break;
		case kTim2: NVIC_DisableIRQ(TIM2_IRQn); break;
		case kTim3:NVIC_DisableIRQ(TIM3_IRQn); break;
		case kTim4:NVIC_DisableIRQ(TIM4_IRQn); break;
		}
	}

	/// Enable "update" DMA
	ALWAYS_INLINE static void EnableUpdateDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		timer_->DIER |= TIM_DIER_UDE;
		// Main Timer Interrupt settings controlled by timer device
	}
	/// Disable "update" DMA
	ALWAYS_INLINE static void DisableUpdateDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		timer_->DIER &= ~TIM_DIER_UDE_Msk;
		// Main Timer Interrupt settings controlled by timer device
	}

	/// Enable "trigger" DMA
	ALWAYS_INLINE static void EnableTriggerDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		timer_->DIER |= TIM_DIER_TDE;
		// Main Timer Interrupt settings controlled by timer device
	}
	/// Disable "trigger" DMA
	ALWAYS_INLINE static void DisableTriggerDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		timer_->DIER &= ~TIM_DIER_TDE_Msk;
		// Main Timer Interrupt settings controlled by timer device
	}

	//! Starts the counting
	ALWAYS_INLINE static void CounterStart(void)
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		timer->CNT = 0;
		timer->CR1 |= TIM_CR1_CEN;
	}

	//! Stops timer
	ALWAYS_INLINE static void CounterStop(void)
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		timer->CR1 &= ~TIM_CR1_CEN;
	}

	ALWAYS_INLINE static void StartRepetition(const uint8_t rep)
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		timer_->RCR = rep-1;
		timer_->EGR = TIM_EGR_UG;		// UG Event
		timer_->CR1 |= TIM_CR1_CEN;
	}

	ALWAYS_INLINE static void StartRepetition(const TypCnt cnt, const uint8_t rep)
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		timer_->ARR = cnt;
		timer_->RCR = rep-1;
		timer_->EGR = TIM_EGR_UG;		// UG Event
		timer_->CR1 |= TIM_CR1_CEN;
	}

	ALWAYS_INLINE static TypCnt GetCounter() { return (BASE::GetDevice())->CNT; }

	ALWAYS_INLINE static TypCnt DistanceOf(TypCnt start) { return GetCounter() - start; }

// Services for One-Shot timers
public:
	//! Enable timer in single shot mode, using default tick count
	ALWAYS_INLINE static void StartShot()
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		if (kTimerMode_ == kSingleShot || kTimerMode_ == kCountUp)
			timer_->CNT = 0;
		timer_->EGR = TIM_EGR_UG;
		timer_->CR1 |= TIM_CR1_CEN;
	}

	//! Enable timer in single shot mode, specifying the total number of ticks
	ALWAYS_INLINE static void StartShot(const TypCnt ticks)
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		timer_->ARR = ticks;
		if (kTimerMode_ == kSingleShot || kTimerMode_ == kCountUp)
			timer_->CNT = 0;
		timer_->EGR = TIM_EGR_UG;
		timer_->CR1 |= TIM_CR1_CEN;
	}

	//! In single shot mode timer will turn off automatically
	ALWAYS_INLINE static void WaitForAutoStop()
	{
		TIM_TypeDef *timer_ = BASE::GetDevice();
		// CEN is cleared automatically in one-pulse mode
		while ((timer_->CR1 & TIM_CR1_CEN) != 0)
		{
		}
	}

	//! Checks if timer is enabled
	ALWAYS_INLINE static bool IsTimerEnabled()
	{
		return ((BASE::GetDevice()->CR1 & TIM_CR1_CEN) != 0);
	}
};


/*!
 * @brief Class to setup a delay timer
 * @tparam TimeBase 
*/
template <
	typename TimeBase
>
class DelayTimerTemplate : public TimerTemplate<TimeBase, kSingleShotDown>
{
public:
	typedef TimerTemplate<TimeBase, kSingleShotDown> Base;
	// An rough overhead based on CPU speed for the us tick
	static constexpr uint32_t kOverhead_ = (70 / (Base::kPrescaler_ + 1));

	ALWAYS_INLINE static void Delay(const uint16_t num)
	{
		if (num > kOverhead_)
			Delay_(num - kOverhead_);	// function call has a typical overhead of 2 us
		else
			Delay_(1);
	}
	ALWAYS_INLINE static bool HasEllapsed()
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)Base::kTimerBase_;
		return ((timer_->CR1 & TIM_CR1_CEN) == 0);
	}


protected:
	// NO_INLINE ensure a function call and a stable overhead
	static void Delay_(const uint16_t num) NO_INLINE
	{
		TIM_TypeDef* timer_ = (TIM_TypeDef*)Base::kTimerBase_;
		timer_->ARR = num;
		timer_->EGR = TIM_EGR_UG;
		timer_->CR1 |= TIM_CR1_CEN;
		// CEN is cleared automatically in one-pulse mode
		Base::WaitForAutoStop();
	}
};


template <
	const TimInstance kTimerNum
	, const TimChannel kChannelNum
>
class AnyTimerChannel_ : public AnyTimer_<kTimerNum>
{
public:
	typedef AnyTimer_<kTimerNum> BASE;
	static constexpr TimChannel kChannelNum_ = kChannelNum;
	static constexpr DmaInstance DmaInstance_ = kDma1;
	static constexpr DmaCh DmaCh_
		= BASE::kTimerNum_ == kTim1 && kChannelNum_ == kTimCh1 ? kDmaCh2
		: BASE::kTimerNum_ == kTim1 && kChannelNum_ == kTimCh2 ? kDmaCh3
		: BASE::kTimerNum_ == kTim1 && kChannelNum_ == kTimCh3 ? kDmaCh6
		: BASE::kTimerNum_ == kTim1 && kChannelNum_ == kTimCh4 ? kDmaCh4
		: BASE::kTimerNum_ == kTim2 && kChannelNum_ == kTimCh1 ? kDmaCh5
		: BASE::kTimerNum_ == kTim2 && kChannelNum_ == kTimCh2 ? kDmaCh7
		: BASE::kTimerNum_ == kTim2 && kChannelNum_ == kTimCh3 ? kDmaCh1
		: BASE::kTimerNum_ == kTim2 && kChannelNum_ == kTimCh4 ? kDmaCh7
		: BASE::kTimerNum_ == kTim3 && kChannelNum_ == kTimCh1 ? kDmaCh6
		: BASE::kTimerNum_ == kTim3 && kChannelNum_ == kTimCh3 ? kDmaCh2
		: BASE::kTimerNum_ == kTim3 && kChannelNum_ == kTimCh4 ? kDmaCh3
		: BASE::kTimerNum_ == kTim4 && kChannelNum_ == kTimCh1 ? kDmaCh1
		: BASE::kTimerNum_ == kTim4 && kChannelNum_ == kTimCh2 ? kDmaCh4
		: BASE::kTimerNum_ == kTim4 && kChannelNum_ == kTimCh3 ? kDmaCh5
		: kDmaChNone;	// default; Not all combinations are possible

	ALWAYS_INLINE static volatile void* GetCcrAddress()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1: return &timer->CCR1;
		case kTimCh2: return &timer->CCR2;
		case kTimCh3: return &timer->CCR3;
		case kTimCh4: return &timer->CCR4;
		}
		return 0;
	}

	ALWAYS_INLINE static void EnableIrq(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (BASE::kChannelNum_)
		{
		case kTimCh1:
			timer_->DIER |= TIM_DIER_CC1IE;
			break;
		case kTimCh2:
			timer_->DIER |= TIM_DIER_CC2IE;
			break;
		case kTimCh3:
			timer_->DIER |= TIM_DIER_CC3IE;
			break;
		case kTimCh4:
			timer_->DIER |= TIM_DIER_CC4IE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static void DisableIrq(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1:
			timer_->DIER &= ~TIM_DIER_CC1IE;
			break;
		case kTimCh2:
			timer_->DIER &= ~TIM_DIER_CC2IE;
			break;
		case kTimCh3:
			timer_->DIER &= ~TIM_DIER_CC3IE;
			break;
		case kTimCh4:
			timer_->DIER &= ~TIM_DIER_CC4IE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static void EnableDma(void)
	{
		if (DmaCh_ != kDmaChNone)
		{
			TIM_TypeDef* timer_ = BASE::GetDevice();
			switch (kChannelNum_)
			{
			case kTimCh1:
				timer_->DIER |= TIM_DIER_CC1DE;
				break;
			case kTimCh2:
				timer_->DIER |= TIM_DIER_CC2DE;
				break;
			case kTimCh3:
				timer_->DIER |= TIM_DIER_CC3DE;
				break;
			case kTimCh4:
				timer_->DIER |= TIM_DIER_CC4DE;
				break;
			}
			// Main Timer Interrupt settings controlled by timer device
		}
		else
		{
			// MCU does not support this DMA channel
			McuCore::Abort();
		}
	}

	ALWAYS_INLINE static void DisableDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1:
			timer_->DIER &= ~TIM_DIER_CC1DE;
			break;
		case kTimCh2:
			timer_->DIER &= ~TIM_DIER_CC2DE;
			break;
		case kTimCh3:
			timer_->DIER &= ~TIM_DIER_CC3DE;
			break;
		case kTimCh4:
			timer_->DIER &= ~TIM_DIER_CC4DE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static void SetCompare(uint16_t ccr)
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1: timer->CCR1 = ccr; break;
		case kTimCh2: timer->CCR2 = ccr; break;
		case kTimCh3: timer->CCR3 = ccr; break;
		case kTimCh4: timer->CCR4 = ccr; break;
		}
	}

	ALWAYS_INLINE static uint16_t GetCapture()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1: return timer->CCR1;
		case kTimCh2: return timer->CCR2;
		case kTimCh3: return timer->CCR3;
		case kTimCh4: return timer->CCR4;
		}
	}
	//! This bit is set by hardware on a capture. It is cleared by reading the CCRx register.
	ALWAYS_INLINE static bool HasCaptured()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1: return (timer->SR & TIM_SR_CC1IF) != 0;
		case kTimCh2: return (timer->SR & TIM_SR_CC2IF) != 0;
		case kTimCh3: return (timer->SR & TIM_SR_CC3IF) != 0;
		case kTimCh4: return (timer->SR & TIM_SR_CC4IF) != 0;
		}
	}
	//! This flag is set by hardware when the counter matches the compare value. Flag is also cleared here.
	ALWAYS_INLINE static bool HasCompared()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		uint16_t flag = 0;
		switch (kChannelNum_)
		{
		case kTimCh1: flag = TIM_SR_CC1IF; break;
		case kTimCh2: flag = TIM_SR_CC2IF; break;
		case kTimCh3: flag = TIM_SR_CC3IF; break;
		case kTimCh4: flag = TIM_SR_CC4IF; break;
		}
		uint16_t old = timer->SR;
		timer->SR = old & ~flag;
		return (old & flag) != 0;
	}
	//! The counter value has been captured in CCRx register while CC1IF flag was already set. Flag is also cleared here.
	ALWAYS_INLINE static bool HasOverCaptured()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		uint16_t flag = 0;
		switch (kChannelNum_)
		{
		case kTimCh1: flag = TIM_SR_CC1OF; break;
		case kTimCh2: flag = TIM_SR_CC2OF; break;
		case kTimCh3: flag = TIM_SR_CC3OF; break;
		case kTimCh4: flag = TIM_SR_CC4OF; break;
		}
		uint16_t old = timer->SR;
		timer->SR = old & ~flag;
		return (old & flag) != 0;
	}
	//! Software generated capture event
	ALWAYS_INLINE static void GenerateCaptureEvent()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1: timer->EGR = TIM_EGR_CC1G; break;
		case kTimCh2: timer->EGR = TIM_EGR_CC2G; break;
		case kTimCh3: timer->EGR = TIM_EGR_CC3G; break;
		case kTimCh4: timer->EGR = TIM_EGR_CC4G; break;
		}
	}
	//! Software generated compare event
	ALWAYS_INLINE static void GenerateCompareEvent()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (kChannelNum_)
		{
		case kTimCh1: timer->EGR = TIM_EGR_CC1G; break;
		case kTimCh2: timer->EGR = TIM_EGR_CC2G; break;
		case kTimCh3: timer->EGR = TIM_EGR_CC3G; break;
		case kTimCh4: timer->EGR = TIM_EGR_CC4G; break;
		}
	}
};


// TODO: Improve this one
template <
	const TimInstance kTimerNum
	, const TimChannel kChannelNum
	, const InputCapture kInputSrc
	, const CaptureEdge kEdge = kRisingEdge
	, const int kFilter = 0
	, const int kPrescaler = 0
>
class TimerInputChannel : public AnyTimerChannel_<kTimerNum, kChannelNum>
{
public:
	typedef AnyTimerChannel_<kTimerNum, kChannelNum> BASE;
	static constexpr int kNumber_ = (int)kChannelNum;		///< Timer channel number
	static constexpr InputCapture kInputSrc_ = kInputSrc;	///< Selectable Input Source
	static constexpr int kShift4_ = 4 * kNumber_;			///< Bit shift for CCER register
	static constexpr int kShift8_ = 8 * (kNumber_ & 1);		///< Bit shift for CCMRx register
	static constexpr int kFilter_ = kFilter;				///< Input filter
	static constexpr int kPrescaler_ = kPrescaler;			///< Clock Prescaler
	static constexpr CaptureEdge kEdge_ = kEdge;			///< Clock edge
	static constexpr uint32_t kCCxS =
		kInputSrc_ == kTRC ? TIM_CCMR1_CC1S_1 | TIM_CCMR1_CC1S_0 :
		kChannelNum == kTimCh1 && kInputSrc_ == kTI1FP1 ? TIM_CCMR1_CC1S_0 :
		kChannelNum == kTimCh1 && kInputSrc_ == kTI1FP2 ? TIM_CCMR1_CC1S_1 :
		kChannelNum == kTimCh2 && kInputSrc_ == kTI2FP1 ? TIM_CCMR1_CC2S_0 :
		kChannelNum == kTimCh2 && kInputSrc_ == kTI2FP2 ? TIM_CCMR1_CC2S_1 :
		kChannelNum == kTimCh3 && kInputSrc_ == kTI3FP3 ? TIM_CCMR2_CC3S_0 :
		kChannelNum == kTimCh3 && kInputSrc_ == kTI3FP4 ? TIM_CCMR2_CC3S_1 :
		kChannelNum == kTimCh4 && kInputSrc_ == kTI4FP3 ? TIM_CCMR2_CC4S_0 :
		kChannelNum == kTimCh4 && kInputSrc_ == kTI4FP4 ? TIM_CCMR2_CC4S_1 :
		0;
	static constexpr uint32_t kICxPSC =
		kPrescaler_ == 2 ? (TIM_CCMR1_IC1PSC_0) << kShift8_ :
		kPrescaler_ == 4 ? (TIM_CCMR1_IC1PSC_1) << kShift8_ :
		kPrescaler_ == 8 ? (TIM_CCMR1_IC1PSC_1 | TIM_CCMR1_IC1PSC_0) << kShift8_ :
		0;
	static constexpr uint32_t kICxF = kFilter_ << (TIM_CCMR1_IC1F_Pos + kShift8_);
	static constexpr uint32_t kCCx =
		kEdge_ == kFallingEdge ? (TIM_CCER_CC1P) << kShift4_ : 0;

	/// Enables input channel
	ALWAYS_INLINE static void Setup(void)
	{
		static_assert(kCCxS != 0, "Selected channel (P2) does not support given channel input (P3)");
		static_assert(kPrescaler_ == 0 || kPrescaler_ == 2 || kPrescaler_ == 4 || kPrescaler_ == 8, "Unsupported prescaler value");
		static_assert(kFilter_ < 16, "Filter parameter must be a value between 0 and 15");

		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum)
		{
		case kTimCh1:
			timer_->CCR1 = 0;
			timer_->CCMR1 = kICxF | kICxPSC | kCCxS;
			break;
		case kTimCh2:
			timer_->CCR2 = 0;
			timer_->CCMR1 = kICxF | kICxPSC | kCCxS;
			break;
		case kTimCh3:
			timer_->CCR3 = 0;
			timer_->CCMR2 = kICxF | kICxPSC | kCCxS;
			break;
		case kTimCh4:
			timer_->CCR4 = 0;
			timer_->CCMR2 = kICxF | kICxPSC | kCCxS;
			break;
		}
		uint32_t tmp = timer_->CCER & ~(0xf << kShift4_);
		tmp |= kCCx;
		timer_->CCER = tmp;
	}

	/// Enables capture register
	ALWAYS_INLINE static void Enable(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		timer_->CCER |= (TIM_CCER_CC1E << kShift4_);
	}

	/// Disables capture register
	ALWAYS_INLINE static void Disable(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		timer_->CCER &= ~(TIM_CCER_CC1E << kShift4_);
	}

	ALWAYS_INLINE static void EnableIrq(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum)
		{
		case kTimCh1:
			timer_->DIER |= TIM_DIER_CC1IE;
			break;
		case kTimCh2:
			timer_->DIER |= TIM_DIER_CC2IE;
			break;
		case kTimCh3:
			timer_->DIER |= TIM_DIER_CC3IE;
			break;
		case kTimCh4:
			timer_->DIER |= TIM_DIER_CC4IE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static void DisableIrq(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum)
		{
		case kTimCh1:
			timer_->DIER &= ~TIM_DIER_CC1IE;
			break;
		case kTimCh2:
			timer_->DIER &= ~TIM_DIER_CC2IE;
			break;
		case kTimCh3:
			timer_->DIER &= ~TIM_DIER_CC3IE;
			break;
		case kTimCh4:
			timer_->DIER &= ~TIM_DIER_CC4IE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static void EnableDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum)
		{
		case kTimCh1:
			timer_->DIER |= TIM_DIER_CC1DE;
			break;
		case kTimCh2:
			timer_->DIER |= TIM_DIER_CC2DE;
			break;
		case kTimCh3:
			timer_->DIER |= TIM_DIER_CC3DE;
			break;
		case kTimCh4:
			timer_->DIER |= TIM_DIER_CC4DE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static void DisableDma(void)
	{
		TIM_TypeDef* timer_ = BASE::GetDevice();
		switch (kChannelNum)
		{
		case kTimCh1:
			timer_->DIER &= ~TIM_DIER_CC1DE;
			break;
		case kTimCh2:
			timer_->DIER &= ~TIM_DIER_CC2DE;
			break;
		case kTimCh3:
			timer_->DIER &= ~TIM_DIER_CC3DE;
			break;
		case kTimCh4:
			timer_->DIER &= ~TIM_DIER_CC4DE;
			break;
		}
		// Main Timer Interrupt settings controlled by timer device
	}

	ALWAYS_INLINE static uint16_t GetCapture()
	{
		TIM_TypeDef* timer = BASE::GetDevice();
		switch (BASE::kChannelNum_)
		{
		case kTimCh1: return timer->CCR1;
		case kTimCh2: return timer->CCR2;
		case kTimCh3: return timer->CCR3;
		case kTimCh4: return timer->CCR4;
		}
	}
};


class UnusedTimerChannel
{
public:
	static constexpr int kNumber_ = -1;
};


/// Configures a timer channel as compare mode (output)
template <
	typename TimType
	, const TimChannel kChannelNum
	, const TimOutMode kMode = kTimOutFrozen
	, const TimOutDrive kOut = kTimOutInactive
	, const TimOutDrive kOutN = kTimOutInactive
	, const bool kPreloadEnable = false
	, const bool kFastEnable = false
	, const bool kClearOnEtrf = false
>
class TimerOutputChannel : public AnyTimerChannel_<TimType::kTimerNum_, kChannelNum>
{
public:
	typedef AnyTimerChannel_<TimType::kTimerNum_, kChannelNum> BASE;
	static constexpr uint16_t kCcmr_Mask =
		(BASE::kChannelNum_ == kTimCh1) ? TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1FE_Msk | TIM_CCMR1_OC1PE_Msk | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1CE_Msk
		: (BASE::kChannelNum_ == kTimCh2) ? TIM_CCMR1_CC2S_Msk | TIM_CCMR1_OC2FE_Msk | TIM_CCMR1_OC2PE_Msk | TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2CE_Msk
		: (BASE::kChannelNum_ == kTimCh3) ? TIM_CCMR2_CC3S_Msk | TIM_CCMR2_OC3FE_Msk | TIM_CCMR2_OC3PE_Msk | TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3CE_Msk
		: (BASE::kChannelNum_ == kTimCh4) ? TIM_CCMR2_CC4S_Msk | TIM_CCMR2_OC4FE_Msk | TIM_CCMR2_OC4PE_Msk | TIM_CCMR2_OC4M_Msk | TIM_CCMR2_OC4CE_Msk
		: 0;
	static constexpr uint16_t kCcer_Mask =
		(BASE::kChannelNum_ == kTimCh1) ? TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk | TIM_CCER_CC1NE_Msk | TIM_CCER_CC1NP_Msk
		: (BASE::kChannelNum_ == kTimCh2) ? TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk | TIM_CCER_CC2NE_Msk | TIM_CCER_CC2NP_Msk
		: (BASE::kChannelNum_ == kTimCh3) ? TIM_CCER_CC3E_Msk | TIM_CCER_CC3P_Msk | TIM_CCER_CC3NE_Msk | TIM_CCER_CC3NP_Msk
		: (BASE::kChannelNum_ == kTimCh4) ? TIM_CCER_CC4E_Msk | TIM_CCER_CC4P_Msk
		: 0;
	static constexpr uint16_t kCCxE =
		(BASE::kChannelNum_ == kTimCh1) ? TIM_CCER_CC1E
		: (BASE::kChannelNum_ == kTimCh2) ? TIM_CCER_CC2E
		: (BASE::kChannelNum_ == kTimCh3) ? TIM_CCER_CC3E
		: (BASE::kChannelNum_ == kTimCh4) ? TIM_CCER_CC4E
		: 0;

	ALWAYS_INLINE static void Init()
	{
		TimType::Init();
		Setup();
	}

	ALWAYS_INLINE static void Setup()
	{
		TIM_TypeDef *timer = BASE::GetDevice();
		uint32_t tmpccer = timer->CCER;
		tmpccer &= ~(kCCxE);
		timer->CCER = tmpccer;	// disable output
		uint32_t tmpccmr;
		uint32_t tmp = 0;
		switch (BASE::kChannelNum_)
		{
		case kTimCh1:
			tmpccmr = timer->CCMR1;
			tmp = (kPreloadEnable ? TIM_CCMR1_OC1PE : 0)
				| (kMode << TIM_CCMR1_OC1M_Pos)
				| (kFastEnable << TIM_CCMR1_OC1FE_Pos)
				| (kClearOnEtrf << TIM_CCMR1_OC1CE_Pos)
				;
			tmpccmr = (tmpccmr & ~kCcmr_Mask) | tmp;
			tmp =
				(
					(kOut == kTimOutActiveHigh) ? TIM_CCER_CC1E
					: (kOut == kTimOutActiveLow) ? TIM_CCER_CC1E | TIM_CCER_CC1P
					: 0
					) |
				(
					(kOutN == kTimOutActiveHigh) ? TIM_CCER_CC1NE
					: (kOutN == kTimOutActiveLow) ? TIM_CCER_CC1NE | TIM_CCER_CC1NP
					: 0
					)
				;
			tmpccer = (tmpccer & ~kCcer_Mask) | tmp;
			break;
		case kTimCh2:
			tmpccmr = timer->CCMR1;
			tmp = (kPreloadEnable ? TIM_CCMR1_OC2PE : 0)
				| (kMode << TIM_CCMR1_OC2M_Pos)
				| (kFastEnable << TIM_CCMR1_OC2FE_Pos)
				| (kClearOnEtrf << TIM_CCMR1_OC2CE_Pos)
				;
			tmpccmr = (tmpccmr & ~kCcmr_Mask) | tmp;
			tmp =
				(
					(kOut == kTimOutActiveHigh) ? TIM_CCER_CC2E
					: (kOut == kTimOutActiveLow) ? TIM_CCER_CC2E | TIM_CCER_CC2P
					: 0
					) |
				(
					(kOutN == kTimOutActiveHigh) ? TIM_CCER_CC2NE
					: (kOutN == kTimOutActiveLow) ? TIM_CCER_CC2NE | TIM_CCER_CC2NP
					: 0
					)
				;
			tmpccer = (tmpccer & ~kCcer_Mask) | tmp;
			break;
		case kTimCh3:
			tmpccmr = timer->CCMR2;
			tmp = (kPreloadEnable ? TIM_CCMR2_OC3PE : 0)
				| (kMode << TIM_CCMR2_OC3M_Pos)
				| (kFastEnable << TIM_CCMR2_OC3FE_Pos)
				| (kClearOnEtrf << TIM_CCMR2_OC3CE_Pos)
				;
			tmpccmr = (tmpccmr & ~kCcmr_Mask) | tmp;
			tmp =
				(
					(kOut == kTimOutActiveHigh) ? TIM_CCER_CC3E
					: (kOut == kTimOutActiveLow) ? TIM_CCER_CC3E | TIM_CCER_CC3P
					: 0
					) |
				(
					(kOutN == kTimOutActiveHigh) ? TIM_CCER_CC3NE
					: (kOutN == kTimOutActiveLow) ? TIM_CCER_CC3NE | TIM_CCER_CC3NP
					: 0
					)
				;
			tmpccer = (tmpccer & ~kCcer_Mask) | tmp;
			break;
		case kTimCh4:
			tmpccmr = timer->CCMR2;
			tmp = (kPreloadEnable ? TIM_CCMR2_OC4PE : 0)
				| (kMode << TIM_CCMR2_OC4M_Pos)
				| (kFastEnable << TIM_CCMR2_OC4FE_Pos)
				| (kClearOnEtrf << TIM_CCMR2_OC4CE_Pos)
				;
			tmpccmr = (tmpccmr & ~kCcmr_Mask) | tmp;
			// Ch4 does not have OutN
			static_assert(BASE::kChannelNum_ != 4 || kOutN == kTimOutInactive, "Hardware does not support this combination");
			tmp =
				(
					(kOut == kTimOutActiveHigh) ? TIM_CCER_CC4E
					: (kOut == kTimOutActiveLow) ? TIM_CCER_CC4E | TIM_CCER_CC4P
					: 0
					)
				;
			tmpccer = (tmpccer & ~kCcer_Mask) | tmp;
			break;
		}
		// TIM1 has extra features
		if (BASE::kHasBdtr)
		{
			static constexpr uint16_t kCr2Bits =
				(BASE::kChannelNum_ == kTimCh1) ? TIM_CR2_OIS1 | TIM_CR2_OIS1N
				: (BASE::kChannelNum_ == kTimCh2) ? TIM_CR2_OIS2 | TIM_CR2_OIS2N
				: (BASE::kChannelNum_ == kTimCh3) ? TIM_CR2_OIS3 | TIM_CR2_OIS3N
				: (BASE::kChannelNum_ == kTimCh4) ? TIM_CR2_OIS4
				: 0;
			uint32_t tmpcr2 = timer->CR2;
			tmpcr2 &= ~kCr2Bits;			// use default (feature not implemented)
			timer->CR2 = tmpcr2;
		}
		if((BASE::kChannelNum_ == kTimCh1) || (BASE::kChannelNum_ == kTimCh2))
			timer->CCMR1 = tmpccmr;
		else
			timer->CCMR2 = tmpccmr;
		// Necessary to activate output (no break feature is activated)
		if (BASE::kHasBdtr)
			timer->BDTR = TIM_BDTR_MOE;
		timer->CCER = tmpccer;
	}

	ALWAYS_INLINE static void SetOutputMode(TimOutMode mode)
	{
		TIM_TypeDef *timer = BASE::GetDevice();
		switch (BASE::kChannelNum_)
		{
		case kTimCh1:
			timer->CCMR1 = (timer->CCMR1 & ~TIM_CCMR1_OC1M_Msk) | (mode << TIM_CCMR1_OC1M_Pos);
			break;
		case kTimCh2:
			timer->CCMR1 = (timer->CCMR1 & ~TIM_CCMR1_OC2M_Msk) | (mode << TIM_CCMR1_OC2M_Pos);
			break;
		case kTimCh3:
			timer->CCMR2 = (timer->CCMR2 & ~TIM_CCMR2_OC3M_Msk) | (mode << TIM_CCMR2_OC3M_Pos);
			break;
		case kTimCh4:
			timer->CCMR2 = (timer->CCMR2 & ~TIM_CCMR2_OC4M_Msk) | (mode << TIM_CCMR2_OC4M_Pos);
			break;
		}
	}
};

