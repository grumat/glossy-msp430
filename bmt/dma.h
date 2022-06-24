#pragma once

#include "irq.h"

/// The DMA Peripheral
enum DmaInstance
{
	kDma1			///< DMA1 controller
#ifdef DMA2_BASE
	, kDma2			///< DMA2 controller
#endif
};

/// Channel of the DMA controller
enum DmaCh
{
	kDmaCh1			///< Channel 1 of the DMA controller
	, kDmaCh2		///< Channel 2 of the DMA controller
	, kDmaCh3		///< Channel 3 of the DMA controller
	, kDmaCh4		///< Channel 4 of the DMA controller
	, kDmaCh5		///< Channel 5 of the DMA controller
	, kDmaCh6		///< Channel 6 of the DMA controller
	, kDmaCh7		///< Channel 7 of the DMA controller
};

/// Data direction of the DMA operation
enum DmaDirection
{
	kDmaMemToMem			///< Memory to memory
	, kDmaPerToMem			///< Peripheral to memory
	, kDmaPerToMemCircular	///< Peripheral to memory circular mode
	, kDmaMemToPer			///< Memory to peripheral
	, kDmaMemToPerCircular	///< Memory to peripheral circular mode
};

/// Pointer control for the source/target pointer
enum DmaPointerCtrl
{
	kDmaBytePtrConst		///< Performs a *pByte operation
	, kDmaBytePtrInc		///< Performs a *pByte++ operation
	, kDmaShortPtrConst		///< Performs a *pShort operation
	, kDmaShortPtrInc		///< Performs a *pShort++ operation
	, kDmaLongPtrConst		///< Performs a *pLong operation
	, kDmaLongPtrInc		///< Performs a *pLong++ operation
};

/// The DMA priority
enum DmaPriority
{
	kDmaLowPrio				///< Low channel priority
	, kDmaMediumPrio		///< Medium channel priority
	, kDmaHighPrio			///< High channel priority
	, kDmaVeryHighPrio		///< Very high channel priority
};

/// Template class that describes a DMA configuration
template <
	const DmaInstance DMA				///< The DMA controller
	, const DmaCh CHAN					///< The DMA channel
	, const DmaDirection DIRECTION		///< Data direction for this channel
	, const DmaPointerCtrl SRC_PTR		///< Source Pointer behavior
	, const DmaPointerCtrl DST_PTR		///< Target Pointer behavior
	, const DmaPriority PRIO = kDmaMediumPrio	///< DMA transfer priority
	, const bool doInitNvic = false		///< Should be the NVIC also initialized?
>
class DmaChannel
{
public:
	/// A constant with the DMA controller instance number
	static constexpr DmaInstance kDma_ = DMA;
	/// A constant with the DMA channel number
	static constexpr DmaCh kChan_ = CHAN;
	/// The address base of the DMA peripheral
	static constexpr uint32_t kDmaBase_ =
		(kDma_ == kDma1) ? DMA1_BASE
#ifdef DMA2_BASE
		: (kDma_ == kDma2) ? DMA2_BASE
#endif
		: 0;
	/// The address base of the DMA channel peripheral
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
	/// Transfer error Interrupt flag for the DMA channel
	static constexpr uint32_t kTeif =
		(kChan_ == kDmaCh1) ? DMA_ISR_TEIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_TEIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_TEIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_TEIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_TEIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_TEIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_TEIF7
		: 0;
	/// Half transfer event Interrupt flag for the DMA channel
	static constexpr uint32_t kHtif =
		(kChan_ == kDmaCh1) ? DMA_ISR_HTIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_HTIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_HTIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_HTIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_HTIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_HTIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_HTIF7
		: 0;
	/// Transfer complete Interrupt flag for the DMA channel
	static constexpr uint32_t kTcif =
		(kChan_ == kDmaCh1) ? DMA_ISR_TCIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_TCIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_TCIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_TCIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_TCIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_TCIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_TCIF7
		: 0;
	/// Global interrupt flag for the DMA channel
	static constexpr uint32_t kGif =
		(kChan_ == kDmaCh1) ? DMA_ISR_GIF1
		: (kChan_ == kDmaCh2) ? DMA_ISR_GIF2
		: (kChan_ == kDmaCh3) ? DMA_ISR_GIF3
		: (kChan_ == kDmaCh4) ? DMA_ISR_GIF4
		: (kChan_ == kDmaCh5) ? DMA_ISR_GIF5
		: (kChan_ == kDmaCh6) ? DMA_ISR_GIF6
		: (kChan_ == kDmaCh7) ? DMA_ISR_GIF7
		: 0;
	/// NVIC initialization flag
	static constexpr bool kDoInitNvic = doInitNvic;
	/// NVIC Interrupt flag for the DMA channel
	static constexpr IRQn_Type kNvicDmaIrqn_ =
		(kDma_ == kDma1 && kChan_ == kDmaCh1) ? DMA1_Channel1_IRQn
		: (kDma_ == kDma1 && kChan_ == kDmaCh2) ? DMA1_Channel2_IRQn
		: (kDma_ == kDma1 && kChan_ == kDmaCh3) ? DMA1_Channel3_IRQn
		: (kDma_ == kDma1 && kChan_ == kDmaCh4) ? DMA1_Channel4_IRQn
		: (kDma_ == kDma1 && kChan_ == kDmaCh5) ? DMA1_Channel5_IRQn
		: (kDma_ == kDma1 && kChan_ == kDmaCh6) ? DMA1_Channel6_IRQn
		: (kDma_ == kDma1 && kChan_ == kDmaCh7) ? DMA1_Channel7_IRQn
#ifdef DMA2_BASE
		: (kDma_ == kDma2 && kChan_ == kDmaCh1) ? DMA2_Channel1_IRQn
		: (kDma_ == kDma2 && kChan_ == kDmaCh2) ? DMA2_Channel2_IRQn
		: (kDma_ == kDma2 && kChan_ == kDmaCh3) ? DMA2_Channel3_IRQn
		: (kDma_ == kDma2 && kChan_ == kDmaCh4) ? DMA2_Channel4_IRQn
		: (kDma_ == kDma2 && kChan_ == kDmaCh5) ? DMA2_Channel5_IRQn
#endif
		: DMA1_Channel1_IRQn;
	/// The IRQ configuration template for that DMA channel
	typedef IrqTemplate<kNvicDmaIrqn_> DmaIrq;

	/// Returns root device structure
	ALWAYS_INLINE static DMA_TypeDef *GetDeviceRoot() { return (DMA_TypeDef *)kDmaBase_; }
	/// Returns device structure for the channel
	ALWAYS_INLINE static DMA_Channel_TypeDef *GetDevice() { return (DMA_Channel_TypeDef *)kChBase_; }

	/// Enables the DMA controller and performs initialization
	ALWAYS_INLINE static void Init()
	{
		if(kDma_ == kDma1)
			RCC->AHBENR |= RCC_AHBENR_DMA1EN;
#ifdef DMA2_BASE
		if (kDma_ == kDma2)
			RCC->AHBENR |= RCC_AHBENR_DMA2EN;
#endif
		// Optional NVIC initialization
		if (kDoInitNvic)
		{
			DmaIrq::ClearPending();
			DmaIrq::Enable();
		}
		Setup();
	}

	/// Stops the entire DMA controller
	ALWAYS_INLINE static void Stop()
	{
		if (kDma_ == kDma1)
			RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
#ifdef DMA2_BASE
		if (kDma_ == kDma2)
			RCC->AHBENR &= ~RCC_AHBENR_DMA2EN;
#endif
	}

	/// DMA controller initialization
	ALWAYS_INLINE static void Setup()
	{
		static_assert(kDmaBase_ != 0, "Invalid DMA instance selected");
		static_assert(kChBase_ != 0, "Invalid DMA instance selected");
		/*
		** This rather complex logic is reduced at maximum by the optimizing compiler, since 
		** everything is declared as constexpr.
		*/
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
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR = tmp;
	}

	/// Enables the DMA channel
	ALWAYS_INLINE static void Enable()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR |= DMA_CCR_EN;
	}

	/// Disables the DMA channel
	ALWAYS_INLINE static void Disable()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR &= ~DMA_CCR_EN;
	}

	/// Sets the number of transfers that will occur
	ALWAYS_INLINE static void SetTransferCount(uint16_t cnt)
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CNDTR = cnt;
	}

	/// Returns current transfer count
	ALWAYS_INLINE static uint16_t GetTransferCount()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		return dma->CNDTR;
	}

	/// Sets the source pointer address
	ALWAYS_INLINE static void SetSourceAddress(const volatile void *addr)
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		if (DIRECTION == kDmaMemToPer || DIRECTION == kDmaMemToPerCircular)
			dma->CMAR = (uint32_t)addr;
		else
			dma->CPAR = (uint32_t)addr;
	}

	/// Sets the destination pointer address
	ALWAYS_INLINE static void SetDestAddress(volatile void *addr)
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		if (DIRECTION == kDmaMemToPer || DIRECTION == kDmaMemToPerCircular)
			dma->CPAR = (uint32_t)addr;
		else
			dma->CMAR = (uint32_t)addr;
	}

	/// Starts a transfer
	ALWAYS_INLINE static void Start(const volatile void *src, volatile void *dst, uint16_t cnt)
	{
		Disable();
		SetSourceAddress(src);
		SetDestAddress(dst);
		SetTransferCount(cnt);
		Enable();
	}

	/// Enables transfer error interrupt
	ALWAYS_INLINE static void EnableTransferErrorInt()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR |= DMA_CCR_TEIE;
	}
	/// Disables transfer error interrupt
	ALWAYS_INLINE static void DisableTransferErrorInt()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR &= ~DMA_CCR_TEIE;
	}
	/// Checks if transfer error interrupt flag is signaled
	ALWAYS_INLINE static bool IsTransferError()
	{
		DMA_TypeDef * dma = GetDeviceRoot();
		return (dma->ISR & kTeif) != 0;
	}
	/// Clears the transfer error interrupt flag
	ALWAYS_INLINE static void ClearTransferErrorFlag()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		dma->IFCR |= kTeif;
	}

	/// Enables the half transfer interrupt
	ALWAYS_INLINE static void EnableHalfTransferInt()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR |= DMA_CCR_HTIE;
	}
	/// Disables the half transfer interrupt
	ALWAYS_INLINE static void DisableHalfTransferInt()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR &= ~DMA_CCR_HTIE;
	}
	/// Checks if the half transfer interrupt flag is signaled
	ALWAYS_INLINE static bool IsHalfTransfer()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		return (dma->ISR & kHtif) != 0;
	}
	/// Clears the half transfer interrupt flag
	ALWAYS_INLINE static void ClearHalfTransferFlag()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		dma->IFCR |= kHtif;
	}

	/// Enables the transfer complete interrupt
	ALWAYS_INLINE static void EnableTransferCompleteInt()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR |= DMA_CCR_TCIE;
	}
	/// Disables the transfer complete interrupt
	ALWAYS_INLINE static void DisableTransferCompleteInt()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR &= ~DMA_CCR_TCIE;
	}
	/// Checks if the transfer complete interrupt flag was signaled
	ALWAYS_INLINE static bool IsTransferComplete()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		return (dma->ISR & kTcif) != 0;
	}
	/// Clears the transfer complete interrupt flag
	ALWAYS_INLINE static void ClearTransferCompleteFlag()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		dma->IFCR |= kTcif;
	}

	/// Disables all interrupts
	ALWAYS_INLINE static void DisableAllInterrupts()
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		dma->CCR &= ~(DMA_CCR_TEIE | DMA_CCR_HTIE | DMA_CCR_TCIE);
	}

	/// Checks if global interrupt flag is signaled
	ALWAYS_INLINE static bool IsGlobalInterrupt()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		return (dma->ISR & kGif) != 0;
	}
	/// Clears the global interrupt flag
	ALWAYS_INLINE static void ClearGlobalInterruptFlag()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		dma->IFCR |= kGif;
	}
	/// Clears all interrupt flags for that channel
	ALWAYS_INLINE static void ClearAllFlags()
	{
		DMA_TypeDef *dma = GetDeviceRoot();
		dma->IFCR |= kTeif | kHtif | kTcif | kGif;
	}

	/// Waits until the transfer is complete
	ALWAYS_INLINE static void WaitTransferComplete()
	{
		while (! IsTransferComplete())
		{ }
		ClearAllFlags();
	}
};


/// A template class to configure ADC with DMA transfer
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaShortPtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaAdc1Template : public DmaChannel<kDma1, kDmaCh1, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaShortPtrConst, DST_PTR, PRIO>
{
public:
};


/// A template class for SPI1 RX transfers of bytes using DMA
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


/// A template class for SPI1 TX transfers of bytes using DMA
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


/// A template class for SPI2 RX transfers of bytes using DMA
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


/// A template class for SPI2 TX transfers of bytes using DMA
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

/// A template class for a DMA operation triggered by Timer 1 TRG event
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

/// A template class for a DMA operation triggered by Timer 1 COM (Commutation) event
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

/// A template class for a DMA operation triggered by Timer 1 UP event
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

/// A template class for a DMA operation triggered by Timer 1 CH1 event
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

/// A template class for a DMA operation triggered by Timer 1 CH2 event
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

/// A template class for a DMA operation triggered by Timer 1 CH3 event
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

/// A template class for a DMA operation triggered by Timer 1 CH4 event
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


/// A template class for a DMA operation triggered by Timer 2 UP event
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

/// A template class for a DMA operation triggered by Timer 2 CH1 event
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

/// A template class for a DMA operation triggered by Timer 2 CH2 event
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

/// A template class for a DMA operation triggered by Timer 2 CH3 event
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

/// A template class for a DMA operation triggered by Timer 2 CH4 event
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


/// A template class for a DMA operation triggered by Timer 3 TRIG event
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

/// A template class for a DMA operation triggered by Timer 3 UP event
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

/// A template class for a DMA operation triggered by Timer 3 CH1 event
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

/// A template class for a DMA operation triggered by Timer 3 CH3 event
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

/// A template class for a DMA operation triggered by Timer 3 CH4 event
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


/// A template class for a DMA operation triggered by Timer 4 UP event
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

/// A template class for a DMA operation triggered by Timer 4 CH1 event
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

/// A template class for a DMA operation triggered by Timer 4 CH3 event
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


/// A template class for USART1 RX transfers of bytes using DMA
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart1RxTemplate : public DmaChannel<kDma1, kDmaCh5, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaBytePtrConst, DST_PTR, PRIO>
{
public:
};

/// A template class for USART1 TX transfers of bytes using DMA
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart1TxTemplate : public DmaChannel<kDma1, kDmaCh4, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, kDmaBytePtrConst, PRIO>
{
public:
};

/// A template class for USART2 RX transfers of bytes using DMA
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart2RxTemplate : public DmaChannel<kDma1, kDmaCh6, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaBytePtrConst, DST_PTR, PRIO>
{
public:
};

/// A template class for USART2 TX transfers of bytes using DMA
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart2TxTemplate : public DmaChannel<kDma1, kDmaCh7, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, kDmaBytePtrConst, PRIO>
{
public:
};

/// A template class for USART3 RX transfers of bytes using DMA
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl DST_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart3RxTemplate : public DmaChannel<kDma1, kDmaCh3, CIRCULAR ? kDmaPerToMemCircular : kDmaPerToMem, kDmaBytePtrConst, DST_PTR, PRIO>
{
public:
};

/// A template class for USART3 TX transfers of bytes using DMA
template<
	const bool CIRCULAR = true
	, const DmaPointerCtrl SRC_PTR = kDmaBytePtrInc
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaUsart3TxTemplate : public DmaChannel<kDma1, kDmaCh2, CIRCULAR ? kDmaMemToPerCircular : kDmaMemToPer, SRC_PTR, kDmaBytePtrConst, PRIO>
{
public:
};

