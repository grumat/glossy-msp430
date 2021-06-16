#pragma once


enum TimInstance
{
	kTim1 = 1
	, kTim2
	, kTim3
	, kTim4
};

enum TimChannel
{
	kTimCh1
	, kTimCh2
	, kTimCh3
	, kTimCh4
};

enum Edge
{
	kRisingEdge,
	kFallingEdge,
	kBothEdges,
};

// TODO: Improve this one
template <
	const TimInstance kTimerNum
	, const TimChannel kChannelNum
	, const int kFilter = 0
	, const int kPrescaler = 0
	, const int kInputSrc = 1
	, const Edge kEdge = kRisingEdge
>
class TimerInputChannel
{
public:
	static constexpr TimInstance kTimerNum_ = kTimerNum;
	static constexpr uintptr_t kTimerBase_ =
		(kTimerNum_ == kTim1) ? TIM1_BASE
		: (kTimerNum_ == kTim2) ? TIM2_BASE
		: (kTimerNum_ == kTim3) ? TIM3_BASE
		: (kTimerNum_ == kTim4) ? TIM4_BASE
		: 0;
	static constexpr int number_ = kChannelNum - 1;
	static constexpr int filter_ = kFilter;
	static constexpr int prescaler_ = kPrescaler;
	static constexpr Edge edge_ = kEdge;

	ALWAYS_INLINE static void Enable(void)
	{
		TIM_TypeDef* timer_ = (TIM_TypeDef*)kTimerBase_;
		static_assert(kTimerBase_ != 0, "Invalid timer instance selected");
		timer_->CCR1 = 0;
		timer_->CCER &= ~(0xf << number_ * 4);
		timer_->CCER |= (1 << number_ * 4) |
			(edge_ == kFallingEdge ? 0x2 << number_ : 0) |
			(edge_ == kBothEdges ? 0xa << number_ : 0);
	}

	ALWAYS_INLINE static void Disable(void)
	{
		TIM_TypeDef* timer_ = (TIM_TypeDef*)kTimerBase_;
		timer_->CCER &= ~(1 << number_ * 4);
	}

	ALWAYS_INLINE static void SetMode(void)
	{
		TIM_TypeDef* timer_ = (TIM_TypeDef*)kTimerBase_;
		uint32_t mode = 0;

		mode = kInputSrc | prescaler_ << 2 | filter_ << 4;
		switch (kChannelNum)
		{
		case kTimCh1: timer_->CCMR1 = mode; break;
		case kTimCh2: timer_->CCMR1 = mode << 8; break;
		case kTimCh3: timer_->CCMR2 = mode; break;
		case kTimCh4: timer_->CCMR2 = mode << 8; break;
		}
	}

	ALWAYS_INLINE static void EnableIrq(void)
	{
		TIM_TypeDef* timer_ = (TIM_TypeDef*)kTimerBase_;
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
		TIM_TypeDef* timer_ = (TIM_TypeDef*)kTimerBase_;
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
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
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
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
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
		TIM_TypeDef *timer = (TIM_TypeDef *)kTimerBase_;
		switch (kTimerNum_)
		{
		case 1: return timer->CCR1;
		case 2: return timer->CCR2;
		case 3: return timer->CCR3;
		case 4: return timer->CCR4;
		}
	}
};


class UnusedTimerChannel
{
public:
	static constexpr int number_ = -1;
};


enum TimerMode
{
	kCountUp,
	kCountDown,
	kSingleShot,
	kSingleShotDown,
};


//! Template to adjust timer prescaler to register counts
template <
	const TimInstance kTimerNum
	, typename SysClk
	, const uint32_t kPrescaler = 0U	// max speed
>
class TimeBase_cnt
{
public:
	static constexpr TimInstance kTimerNum_ = kTimerNum;
	static constexpr uint32_t kFrequency_ = SysClk::kFrequency_;
	static constexpr uint32_t kClkTick = (kTimerNum_ == 1)
		? SysClk::kApb2TimerClock_
		: SysClk::kApb1TimerClock_
		;
	static constexpr uint32_t kPrescaler_ = kPrescaler;
};


//! Template to adjust timer prescaler to µs
template <
	const TimInstance kTimerNum
	, typename SysClk
	, const uint32_t kMicroSecs = 1000U
>
class TimeBase_us
{
public:
	static constexpr TimInstance kTimerNum_ = kTimerNum;
	static constexpr uint32_t kFrequency_ = SysClk::kFrequency_;
	static constexpr uint32_t kClkTick = (kTimerNum_ == 1)
		? SysClk::kApb2TimerClock_
		: SysClk::kApb1TimerClock_
		;
	static constexpr double kTimerTick_ = kMicroSecs / 1000000.0;
	static constexpr uint32_t kPrescaler_raw_ = (uint32_t)(kTimerTick_ * kClkTick + 0.5);
	static constexpr uint32_t kPrescaler_ = kPrescaler_raw_ > 0 ? kPrescaler_raw_ - 1 : 0;
};


//! Template to adjust timer prescaler to MHz
template <
	const TimInstance kTimerNum
	, typename SysClk
	, const uint32_t kMHz = 1000000
>
class TimeBase_MHz
{
public:
	static constexpr TimInstance kTimerNum_ = kTimerNum;
	static constexpr uint32_t kFrequency_ = SysClk::kFrequency_;
	static constexpr uint32_t kClkTick = (kTimerNum_ == 1)
		? SysClk::kApb2TimerClock_
		: SysClk::kApb1TimerClock_
		;
	static constexpr uint32_t kPrescaler_raw_ = (uint32_t)((kClkTick + kMHz/2) / kMHz);
	static constexpr uint32_t kPrescaler_ = kPrescaler_raw_ > 0 ? kPrescaler_raw_ - 1 : 0;
};


template <
	typename TimeBase
	, const TimerMode kTimerMode = kCountUp
	, const uint32_t kReload = 0
	, const bool kBuffered = true
>
class TimerTemplate
{
public:
	typedef uint16_t TypCnt;
	static constexpr TimInstance kTimerNum_ = TimeBase::kTimerNum_;
	static constexpr uintptr_t kTimerBase_ =
		(kTimerNum_ == kTim1) ? TIM1_BASE
		: (kTimerNum_ == kTim2) ? TIM2_BASE
		: (kTimerNum_ == kTim3) ? TIM3_BASE
		: (kTimerNum_ == kTim4) ? TIM4_BASE
		: 0;
	static constexpr uint32_t kPrescaler_ = TimeBase::kPrescaler_;
	static constexpr TimerMode kTimerMode_ = kTimerMode;
	static constexpr uint32_t kCr1Mask = TIM_CR1_CEN_Msk | TIM_CR1_UDIS_Msk | TIM_CR1_URS_Msk
		| TIM_CR1_OPM_Msk | TIM_CR1_DIR_Msk | TIM_CR1_CMS_Msk | TIM_CR1_ARPE_Msk
		| TIM_CR1_CKD_Msk
		;

	ALWAYS_INLINE static constexpr TIM_TypeDef *GetDevice() { return (TIM_TypeDef *)kTimerBase_; }

	ALWAYS_INLINE static void Init()
	{
		// Hardware does not support hardware
		static_assert(kTimerBase_ != 0, "Invalid timer instance selected");
		TIM_TypeDef* timer = (TIM_TypeDef*)kTimerBase_;
		// Enable clock
		switch (kTimerNum_)
		{
		case 1:
			RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
			RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
			RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;
			break;
		case 2:
			RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
			RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
			break;
		case 3:
			RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM3RST;
			RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM3RST;
			break;
		case 4:
			RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM4RST;
			RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM4RST;
			break;
		}
		Setup();
	}

	ALWAYS_INLINE static void Setup()
	{
		// Hardware does not support hardware
		static_assert(kTimerBase_ != 0, "Invalid timer instance selected");
		TIM_TypeDef *timer = (TIM_TypeDef *)kTimerBase_;
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
		// Compute prescaler to obtain tick count value
		constexpr uint32_t tmp2 = TimeBase::kPrescaler_;
		// Timer tick base is impossible for the hardware
		static_assert(tmp2 <= 65535, "Prescaler overflow! Timer clock is too high for the desired time base.");
		timer->PSC = tmp2;
		// reload value
		if (kReload)
			timer->ARR = kReload;
	}

	ALWAYS_INLINE static void Disable()
	{
		switch (kTimerNum_)
		{
		case 1: RCC->APB2ENR &= ~RCC_APB2ENR_TIM1EN; break;
		case 2: RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN; break;
		case 3: RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN; break;
		case 4: RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN; break;
		}
	}

	ALWAYS_INLINE static void EnableIrq(void)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		switch (kTimerNum_)
		{
		case 1: NVIC_ClearPendingIRQ(TIM1_BRK_IRQn); NVIC_EnableIRQ(TIM1_BRK_IRQn);
			NVIC_ClearPendingIRQ(TIM1_CC_IRQn); NVIC_EnableIRQ(TIM1_CC_IRQn); break;
		case 2: NVIC_ClearPendingIRQ(TIM2_IRQn); NVIC_EnableIRQ(TIM2_IRQn); break;
		case 3: NVIC_ClearPendingIRQ(TIM3_IRQn); NVIC_EnableIRQ(TIM3_IRQn); break;
		case 4: NVIC_ClearPendingIRQ(TIM4_IRQn); NVIC_EnableIRQ(TIM4_IRQn); break;
		}
	}

	ALWAYS_INLINE static void DisableIrq(void)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		switch (kTimerNum_)
		{
		case 1: NVIC_DisableIRQ(TIM1_BRK_IRQn);
			NVIC_DisableIRQ(TIM1_CC_IRQn); break;
		case 2: NVIC_DisableIRQ(TIM2_IRQn); break;
		case 3:NVIC_DisableIRQ(TIM3_IRQn); break;
		case 4:NVIC_DisableIRQ(TIM4_IRQn); break;
		}
	}

	ALWAYS_INLINE static void CounterStart(void)
	{
		TIM_TypeDef* timer = (TIM_TypeDef*)kTimerBase_;
		timer->CNT = 0;
		timer->CR1 |= TIM_CR1_CEN;
	}

	ALWAYS_INLINE static void CounterStop(void)
	{
		TIM_TypeDef* timer = (TIM_TypeDef*)kTimerBase_;
		timer->CR1 &= ~TIM_CR1_CEN;
	}

	ALWAYS_INLINE static void StartRepetition(const uint8_t rep)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		timer_->RCR = rep-1;
		timer_->EGR = 1;
		timer_->CR1 |= TIM_CR1_CEN;
	}

	ALWAYS_INLINE static void StartRepetition(const TypCnt cnt, const uint8_t rep)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		timer_->ARR = cnt;
		timer_->RCR = rep-1;
		timer_->EGR = 1;
		timer_->CR1 |= TIM_CR1_CEN;
	}

	ALWAYS_INLINE static TypCnt GetCounter() { return ((TIM_TypeDef*)kTimerBase_)->CNT; }

	ALWAYS_INLINE static TypCnt DistanceOf(TypCnt start) { return GetCounter() - start; }

// Services for One-Shot timers
public:
	//! Enable timer in single shot mode, using default tick count
	ALWAYS_INLINE static void StartShot()
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		if (kTimerMode_ == kSingleShot || kTimerMode_ == kCountUp)
			timer_->CNT = 0;
		timer_->EGR = 1;
		timer_->CR1 |= TIM_CR1_CEN;
	}

	//! Enable timer in single shot mode, specifying the total number of ticks
	ALWAYS_INLINE static void StartShot(const TypCnt ticks)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		timer_->ARR = ticks;
		if (kTimerMode_ == kSingleShot || kTimerMode_ == kCountUp)
			timer_->CNT = 0;
		timer_->EGR = 1;
		timer_->CR1 |= TIM_CR1_CEN;
	}

	//! In single shot mode timer will turn off automatically
	ALWAYS_INLINE static void WaitForAutoStop()
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		// CEN is cleared automatically in one-pulse mode
		while ((timer_->CR1 & TIM_CR1_CEN) != 0)
		{
		}
	}
};


template <
	typename TimeBase
>
class DelayTimerTemplate : public TimerTemplate<TimeBase, kSingleShotDown>
{
public:
	typedef TimerTemplate<TimeBase, kSingleShotDown> Base;
	// An rough overhead based on CPU speed for the µs tick
	static constexpr uint32_t kOverhead_ = (70 / (Base::kPrescaler_ + 1));

	ALWAYS_INLINE static void Delay(const uint16_t num)
	{
		if (num > kOverhead_)
			Delay_(num - kOverhead_);	// function call has a typical overhead of 2 µs
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
		timer_->EGR = 1;
		timer_->CR1 |= TIM_CR1_CEN;
		// CEN is cleared automatically in one-pulse mode
		Base::WaitForAutoStop();
	}
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
	typename TimType
	, const TimChannel kChannelNum
	, const TimOutMode kMode = kTimOutFrozen
	, const TimOutDrive kOut = kTimOutInactive
	, const TimOutDrive kOutN = kTimOutInactive
	, const bool kPreloadEnable = false
	, const bool kClearOnEtrf = false
>
class TimerOutputChannel
{
public:
	static constexpr TimInstance kTimerNum_ = TimType::kTimerNum_;
	static constexpr TimChannel kChannelNum_ = kChannelNum;
	static constexpr uintptr_t kTimerBase_ =
		(kTimerNum_ == kTim1) ? TIM1_BASE
		: (kTimerNum_ == kTim2) ? TIM2_BASE
		: (kTimerNum_ == kTim3) ? TIM3_BASE
		: (kTimerNum_ == kTim4) ? TIM4_BASE
		: 0;
	static constexpr uint16_t kCcmr_Mask =
		(kTimerNum_ == kTim1) ? TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1FE_Msk | TIM_CCMR1_OC1PE_Msk | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1CE_Msk
		: (kTimerNum_ == kTim2) ? TIM_CCMR1_CC2S_Msk | TIM_CCMR1_OC2FE_Msk | TIM_CCMR1_OC2PE_Msk | TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2CE_Msk
		: (kTimerNum_ == kTim3) ? TIM_CCMR2_CC3S_Msk | TIM_CCMR2_OC3FE_Msk | TIM_CCMR2_OC3PE_Msk | TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3CE_Msk
		: (kTimerNum_ == kTim4) ? TIM_CCMR2_CC4S_Msk | TIM_CCMR2_OC4FE_Msk | TIM_CCMR2_OC4PE_Msk | TIM_CCMR2_OC4M_Msk | TIM_CCMR2_OC4CE_Msk
		: 0;
	static constexpr uint16_t kCcer_Mask =
		(kTimerNum_ == kTim1) ? TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk | TIM_CCER_CC1NE_Msk | TIM_CCER_CC1NP_Msk
		: (kTimerNum_ == kTim2) ? TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk | TIM_CCER_CC2NE_Msk | TIM_CCER_CC2NP_Msk
		: (kTimerNum_ == kTim3) ? TIM_CCER_CC3E_Msk | TIM_CCER_CC3P_Msk | TIM_CCER_CC3NE_Msk | TIM_CCER_CC3NP_Msk
		: (kTimerNum_ == kTim4) ? TIM_CCER_CC4E_Msk | TIM_CCER_CC4P_Msk
		: 0;

	ALWAYS_INLINE static void Init()
	{
		TimType::Init();
		Setup();
	}

	ALWAYS_INLINE static void Setup()
	{
		TIM_TypeDef *timer = (TIM_TypeDef *)kTimerBase_;
		uint32_t tmp;
		switch (kChannelNum_)
		{
		case kTimCh1:
			tmp = (kPreloadEnable ? TIM_CCMR1_OC1PE : 0)
				| (kMode << TIM_CCMR1_OC1M_Pos)
				| (kClearOnEtrf << TIM_CCMR1_OC1CE_Pos)
				;
			timer->CCMR1 = (timer->CCMR1 & ~kCcmr_Mask) | tmp;
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
			timer->CCER = (timer->CCER & ~kCcer_Mask) | tmp;
			break;
		case kTimCh2:
			tmp = (kPreloadEnable ? TIM_CCMR1_OC2PE : 0)
				| (kMode << TIM_CCMR1_OC2M_Pos)
				| (kClearOnEtrf << TIM_CCMR1_OC2CE_Pos)
				;
			timer->CCMR1 = (timer->CCMR1 & ~kCcmr_Mask) | tmp;
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
			timer->CCER = (timer->CCER & ~kCcer_Mask) | tmp;
			break;
		case kTimCh3:
			tmp = (kPreloadEnable ? TIM_CCMR2_OC3PE : 0)
				| (kMode << TIM_CCMR2_OC3M_Pos)
				| (kClearOnEtrf << TIM_CCMR2_OC3CE_Pos)
				;
			timer->CCMR2 = (timer->CCMR2 & ~kCcmr_Mask) | tmp;
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
			timer->CCER = (timer->CCER & ~kCcer_Mask) | tmp;
			break;
		case kTimCh4:
			tmp = (kPreloadEnable ? TIM_CCMR2_OC4PE : 0)
				| (kMode << TIM_CCMR2_OC4M_Pos)
				| (kClearOnEtrf << TIM_CCMR2_OC4CE_Pos)
				;
			timer->CCMR2 = (timer->CCMR2 & ~kCcmr_Mask) | tmp;
			// Ch4 does not have OutN
			static_assert(kChannelNum_ != 4 || kOutN == kTimOutInactive, "Hardware does not support this combination");
			tmp =
				(
					(kOut == kTimOutActiveHigh) ? TIM_CCER_CC4E
					: (kOut == kTimOutActiveLow) ? TIM_CCER_CC4E | TIM_CCER_CC4P
					: 0
					)
				;
			timer->CCER = (timer->CCER & ~kCcer_Mask) | tmp;
			break;
		}
	}

	ALWAYS_INLINE static void EnableIrq(void)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
		switch (kChannelNum_)
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
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
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
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
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

	ALWAYS_INLINE static void DisableDma(void)
	{
		TIM_TypeDef *timer_ = (TIM_TypeDef *)kTimerBase_;
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
		TIM_TypeDef *timer = (TIM_TypeDef *)kTimerBase_;
		switch (kTimerNum_)
		{
		case 1: timer->CCR1 = ccr; break;
		case 2: timer->CCR2 = ccr; break;
		case 3: timer->CCR3 = ccr; break;
		case 4: timer->CCR4 = ccr; break;
		}
	}

	ALWAYS_INLINE static uint16_t GetCompare()
	{
		TIM_TypeDef *timer = (TIM_TypeDef *)kTimerBase_;
		switch (kTimerNum_)
		{
		case 1: return timer->CCR1;
		case 2: return timer->CCR2;
		case 3: return timer->CCR3;
		case 4: return timer->CCR4;
		}
	}
};

