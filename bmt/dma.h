#pragma once

enum DmaInstance
{
	kDma1
#ifdef DMA2_BASE
	, kDma2
#endif
};

enum DmaCh
{
	kDmaCh1
	, kDmaCh2
	, kDmaCh3
	, kDmaCh4
	, kDmaCh5
	, kDmaCh6
	, kDmaCh7
};


enum DmaDirection
{
	kDmaMemToMem			// Memory to memory
	, kDmaPerToMem			// Peripheral to memory
	, kDmaPerToMemCircular	// Peripheral to memory circular mode
	, kDmaMemToPer			// Memory to peripheral
	, kDmaMemToPerCircular	// Memory to peripheral circular mode
};

enum DmaPointerCtrl
{
	kDmaBytePtrConst		// *pBbyte
	, kDmaBytePtrInc		// *pByte++
	, kDmaShortPtrConst		// *pShort
	, kDmaShortPtrInc		// *pShort++
	, kDmaLongPtrConst		// *pLong
	, kDmaLongPtrInc		// *pLong++
};

enum DmaPriority
{
	kDmaLowPrio
	, kDmaMediumPrio
	, kDmaHighPrio
	, kDmaVeryHighPrio
};

template <
	const DmaInstance DMA
	, const DmaCh CHAN
	, const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaChannel
{
public:
	static constexpr DmaInstance kDma_ = DMA;
	static constexpr DmaCh kChan_ = CHAN;
	static constexpr uint32_t kDmaBase_ =
		(kDma_ == kDma1) ? DMA1_BASE
#ifdef DMA2_BASE
		: (kDma_ == kDma2) ? DMA2_BASE
#endif
		: 0;
	static constexpr uint32_t kChBase_ =
		(kDma_ == kDma1 && kChan_ == kDmaCh1) ? DMA1_Channel1_BASE
		: (kDma_ == kDma1 && kChan_ == kDmaCh2) ? DMA1_Channel2_BASE
		: (kDma_ == kDma1 && kChan_ == kDmaCh3) ? DMA1_Channel3_BASE
		: (kDma_ == kDma1 && kChan_ == kDmaCh4) ? DMA1_Channel4_BASE
		: (kDma_ == kDma1 && kChan_ == kDmaCh5) ? DMA1_Channel5_BASE
		: (kDma_ == kDma1 && kChan_ == kDmaCh6) ? DMA1_Channel6_BASE
		: (kDma_ == kDma1 && kChan_ == kDmaCh7) ? DMA1_Channel7_BASE
#ifdef DMA2_BASE
		: (kDma_ == kDma2 && kChan_ == kDmaCh1) ? DMA2_Channel1_BASE
		: (kDma_ == kDma2 && kChan_ == kDmaCh2) ? DMA2_Channel2_BASE
		: (kDma_ == kDma2 && kChan_ == kDmaCh3) ? DMA2_Channel3_BASE
		: (kDma_ == kDma2 && kChan_ == kDmaCh4) ? DMA2_Channel4_BASE
		: (kDma_ == kDma2 && kChan_ == kDmaCh5) ? DMA2_Channel5_BASE
#endif
		: 0;
	static constexpr uint32_t kTeif =
		(kChan_ == kDmaCh1) ? DMA_ISR_TEIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_TEIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_TEIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_TEIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_TEIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_TEIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_TEIF7
		: 0;
	static constexpr uint32_t kHtif =
		(kChan_ == kDmaCh1) ? DMA_ISR_HTIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_HTIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_HTIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_HTIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_HTIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_HTIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_HTIF7
		: 0;
	static constexpr uint32_t kTcif =
		(kChan_ == kDmaCh1) ? DMA_ISR_TCIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_TCIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_TCIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_TCIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_TCIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_TCIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_TCIF7
		: 0;
	static constexpr uint32_t kGif =
		(kChan_ == kDmaCh1) ? DMA_ISR_GIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_GIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_GIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_GIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_GIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_GIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_GIF7
		: 0;

	ALWAYS_INLINE static void Init()
	{
		if(kDma_ == kDma1)
			RCC->AHBENR |= RCC_AHBENR_DMA1EN;
#ifdef DMA2_BASE
		if (kDma_ == kDma2)
			RCC->AHBENR |= RCC_AHBENR_DMA2EN;
#endif
		Setup();
	}


	ALWAYS_INLINE static void Setup()
	{
		static_assert(kDmaBase_ != 0, "Invalid DMA instance selected");
		static_assert(kChBase_ != 0, "Invalid DMA instance selected");
		uint32_t tmp = PRIO << DMA_CCR_PL_Pos;
		switch (DIRECTION)
		{
		case kDmaMemToMem:
			tmp |= DMA_CCR_MEM2MEM;
			break;
		case kDmaPerToMem:
			break;
		case kDmaPerToMemCircular:
			tmp |= DMA_CCR_CIRC;
			break;
		case kDmaMemToPer:
			tmp |= DMA_CCR_DIR;
			break;
		case kDmaMemToPerCircular:
			tmp |= DMA_CCR_DIR | DMA_CCR_CIRC;
			break;
		}
		if (DIRECTION == kDmaMemToPer || DIRECTION == kDmaMemToPerCircular)
		{
			switch (SRC_PTR)
			{
			case kDmaBytePtrConst:
				break;
			case kDmaBytePtrInc:
				tmp |= DMA_CCR_MINC;
				break;
			case kDmaShortPtrConst:
				tmp |= DMA_CCR_MSIZE_0;
				break;
			case kDmaShortPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_0;
				break;
			case kDmaLongPtrConst:
				tmp |= DMA_CCR_MSIZE_1;
				break;
			case kDmaLongPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_1;
				break;
			}
			switch (DST_PTR)
			{
			case kDmaBytePtrConst:
				break;
			case kDmaBytePtrInc:
				tmp |= DMA_CCR_PINC;
				break;
			case kDmaShortPtrConst:
				tmp |= DMA_CCR_PSIZE_0;
				break;
			case kDmaShortPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_0;
				break;
			case kDmaLongPtrConst:
				tmp |= DMA_CCR_PSIZE_1;
				break;
			case kDmaLongPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_1;
				break;
			}
		}
		else
		{
			switch (SRC_PTR)
			{
			case kDmaBytePtrConst:
				break;
			case kDmaBytePtrInc:
				tmp |= DMA_CCR_PINC;
				break;
			case kDmaShortPtrConst:
				tmp |= DMA_CCR_PSIZE_0;
				break;
			case kDmaShortPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_0;
				break;
			case kDmaLongPtrConst:
				tmp |= DMA_CCR_PSIZE_1;
				break;
			case kDmaLongPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_1;
				break;
			}
			switch (DST_PTR)
			{
			case kDmaBytePtrConst:
				break;
			case kDmaBytePtrInc:
				tmp |= DMA_CCR_MINC;
				break;
			case kDmaShortPtrConst:
				tmp |= DMA_CCR_MSIZE_0;
				break;
			case kDmaShortPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_0;
				break;
			case kDmaLongPtrConst:
				tmp |= DMA_CCR_MSIZE_1;
				break;
			case kDmaLongPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_1;
				break;
			}
		}
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR = tmp;
	}

	// Enables the DMA channel
	ALWAYS_INLINE static void Enable()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR |= DMA_CCR_EN;
	}

	// Enables the DMA channel
	ALWAYS_INLINE static void Disable()
	{
		Setup();
		//DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		//dma->CCR &= ~DMA_CCR_EN;
	}

	ALWAYS_INLINE static void SetTransferCount(uint16_t cnt)
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CNDTR = cnt;
	}
	// Returns current transfer count
	ALWAYS_INLINE static uint16_t GetTransferCount()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		return dma->CNDTR;
	}

	ALWAYS_INLINE static void SetSourceAddress(const volatile void *addr)
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		if (DIRECTION == kDmaMemToPer || DIRECTION == kDmaMemToPerCircular)
			dma->CMAR = (uint32_t)addr;
		else
			dma->CPAR = (uint32_t)addr;
	}
	ALWAYS_INLINE static void SetDestAddress(volatile void *addr)
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		if (DIRECTION == kDmaMemToPer || DIRECTION == kDmaMemToPerCircular)
			dma->CPAR = (uint32_t)addr;
		else
			dma->CMAR = (uint32_t)addr;
	}
	ALWAYS_INLINE static void Start(const volatile void *src, volatile void *dst, uint16_t cnt)
	{
		Disable();
		SetSourceAddress(src);
		SetDestAddress(dst);
		SetTransferCount(cnt);
		Enable();
	}

	ALWAYS_INLINE static void EnableTransferErrorInt()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR |= DMA_CCR_TEIE;
	}
	ALWAYS_INLINE static void DisableTransferErrorInt()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR &= ~DMA_CCR_TEIE;
	}
	ALWAYS_INLINE static bool IsTransferError()
	{
		DMA_TypeDef * dma = (DMA_TypeDef *)kDmaBase_;
		return (dma->ISR & kTeif) != 0;
	}
	ALWAYS_INLINE static void ClearTransferErrorFlag()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		dma->IFCR |= kTeif;
	}


	ALWAYS_INLINE static void EnableHalfTransferInt()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR |= DMA_CCR_HTIE;
	}
	ALWAYS_INLINE static void DisableHalfTransferInt()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR &= ~DMA_CCR_HTIE;
	}
	ALWAYS_INLINE static bool IsHalfTransfer()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		return (dma->ISR & kHtif) != 0;
	}
	ALWAYS_INLINE static void ClearHalfTransferFlag()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		dma->IFCR |= kHtif;
	}

	ALWAYS_INLINE static void EnableTransferCompleteInt()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR |= DMA_CCR_TCIE;
	}
	ALWAYS_INLINE static void DisableTransferCompleteInt()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR &= ~DMA_CCR_TCIE;
	}
	ALWAYS_INLINE static bool IsTransferComplete()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		return (dma->ISR & kTcif) != 0;
	}
	ALWAYS_INLINE static void ClearTransferCompleteFlag()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		dma->IFCR |= kTcif;
	}

	ALWAYS_INLINE static void DisableAllInterrupts()
	{
		DMA_Channel_TypeDef *dma = (DMA_Channel_TypeDef *)kChBase_;
		dma->CCR &= ~(DMA_CCR_TEIE | DMA_CCR_HTIE | DMA_CCR_TCIE);
	}

	ALWAYS_INLINE static bool IsGlobalInterrupt()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		return (dma->ISR & kGif) != 0;
	}

	ALWAYS_INLINE static void ClearGlobalInterruptFlag()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		dma->IFCR |= kGif;
	}

	ALWAYS_INLINE static void ClearAllFlags()
	{
		DMA_TypeDef *dma = (DMA_TypeDef *)kDmaBase_;
		dma->IFCR |= kTeif | kHtif | kTcif | kGif;
	}

	ALWAYS_INLINE static void WaitTransferComplete()
	{
		while (! IsTransferComplete())
		{ }
		ClearAllFlags();
	}
};


template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaShortPtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaAdc1Template : public DmaChannel<kDma1, kDmaCh1, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaShortPtrConst, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrConst
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaSpi1RxTemplate : public DmaChannel<kDma1, kDmaCh2, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrConst
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaSpi1TxTemplate : public DmaChannel<kDma1, kDmaCh3, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrConst
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaSpi2RxTemplate : public DmaChannel<kDma1, kDmaCh4, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrConst
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaSpi2TxTemplate : public DmaChannel<kDma1, kDmaCh5, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1TrigTemplate : public DmaChannel<kDma1, kDmaCh4, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1ComTemplate : public DmaChannel<kDma1, kDmaCh4, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1UpTemplate : public DmaChannel<kDma1, kDmaCh5, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1Ch1Template : public DmaChannel<kDma1, kDmaCh2, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1Ch2Template : public DmaChannel<kDma1, kDmaCh4, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1Ch3Template : public DmaChannel<kDma1, kDmaCh6, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim1Ch4Template : public DmaChannel<kDma1, kDmaCh4, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};


template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim2UpTemplate : public DmaChannel<kDma1, kDmaCh2, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim2Ch1Template : public DmaChannel<kDma1, kDmaCh5, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim2Ch2Template : public DmaChannel<kDma1, kDmaCh7, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim2Ch3Template : public DmaChannel<kDma1, kDmaCh1, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim2Ch4Template : public DmaChannel<kDma1, kDmaCh7, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};


template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim3TrigTemplate : public DmaChannel<kDma1, kDmaCh6, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim3UpTemplate : public DmaChannel<kDma1, kDmaCh3, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim3Ch1Template : public DmaChannel<kDma1, kDmaCh6, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim3Ch3Template : public DmaChannel<kDma1, kDmaCh2, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim3Ch4Template : public DmaChannel<kDma1, kDmaCh3, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};


template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim4UpTemplate : public DmaChannel<kDma1, kDmaCh7, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim4Ch1Template : public DmaChannel<kDma1, kDmaCh1, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaTim4Ch3Template : public DmaChannel<kDma1, kDmaCh5, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};


template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart1RxTemplate : public DmaChannel<kDma1, kDmaCh5, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaBytePtrConst, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart1TxTemplate : public DmaChannel<kDma1, kDmaCh4, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, kDmaBytePtrConst, PRIO>
{
public:
};


template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart2RxTemplate : public DmaChannel<kDma1, kDmaCh6, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaBytePtrConst, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart2TxTemplate : public DmaChannel<kDma1, kDmaCh7, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, kDmaBytePtrConst, PRIO>
{
public:
};


template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart3RxTemplate : public DmaChannel<kDma1, kDmaCh3, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaBytePtrConst, DST_PTR, PRIO>
{
public:
};

template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart3TxTemplate : public DmaChannel<kDma1, kDmaCh2, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, kDmaBytePtrConst, PRIO>
{
public:
};

