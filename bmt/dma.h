#pragma once

#include "irq.h"

#if defined(DMA1_CSELR_BASE) || defined(DMA2_CSELR_BASE)
#	define OPT_DMA_VERSION	2
#else
#	define OPT_DMA_VERSION	1
#endif

namespace Bmt
{

/// The DMA Peripheral
enum class Dma
{
	k1				///< DMA1 controller
#ifdef DMA2_BASE
	, k2			///< DMA2 controller
#endif
};

/// Channel of the DMA controller
enum class DmaCh
{
	k1			///< Channel 1 of the DMA controller
	, k2		///< Channel 2 of the DMA controller
	, k3		///< Channel 3 of the DMA controller
	, k4		///< Channel 4 of the DMA controller
	, k5		///< Channel 5 of the DMA controller
	, k6		///< Channel 6 of the DMA controller
	, k7		///< Channel 7 of the DMA controller
	, kNone		///< Indicates configuration not available
};

#if OPT_DMA_VERSION > 1
enum class DmaPerSel
{
	k0,			///< DMA request selection 0b0000
	k1,			///< DMA request selection 0b0001
	k2,			///< DMA request selection 0b0010
	k3,			///< DMA request selection 0b0011
	k4,			///< DMA request selection 0b0100
	k5,			///< DMA request selection 0b0101
	k6,			///< DMA request selection 0b0110
	k7,			///< DMA request selection 0b0111
};
#endif

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
	const Dma kDma						///< The DMA controller
	, const DmaCh kChan					///< The DMA channel
#if OPT_DMA_VERSION > 1
	, const DmaPerSel kChSel				///< The DMA channel selector
#endif
	, const DmaDirection kDir		///< Data direction for this channel
	, const DmaPointerCtrl kSrcPtr		///< Source Pointer behavior
	, const DmaPointerCtrl kDstPtr		///< Target Pointer behavior
	, const DmaPriority kPrio = kDmaMediumPrio	///< DMA transfer priority
	, const bool doInitNvic = false		///< Should be the NVIC also initialized?
>
class DmaChannel
{
public:
	/// A constant with the DMA controller instance number
	static constexpr Dma kDma_ = kDma;
	/// A constant with the DMA channel number
	static constexpr DmaCh kChan_ = kChan;
#if OPT_DMA_VERSION > 1
	/// A constant with the DMA channel number
	static constexpr DmaPerSel kChSel_ = kChSel;
#endif
	/// The address base of the DMA peripheral
	static constexpr uint32_t kDmaBase_ =
		(kDma_ == Dma::k1) ? DMA1_BASE
#ifdef DMA2_BASE
		: (kDma_ == Dma::k2) ? DMA2_BASE
#endif
		: 0;
	/// The address base of the DMA channel peripheral
	static constexpr uint32_t kChBase_ =
		(kDma_ == Dma::k1 && kChan_ == DmaCh::k1) ? DMA1_Channel1_BASE
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k2) ? DMA1_Channel2_BASE
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k3) ? DMA1_Channel3_BASE
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k4) ? DMA1_Channel4_BASE
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k5) ? DMA1_Channel5_BASE
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k6) ? DMA1_Channel6_BASE
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k7) ? DMA1_Channel7_BASE
#ifdef DMA2_BASE
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k1) ? DMA2_Channel1_BASE
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k2) ? DMA2_Channel2_BASE
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k3) ? DMA2_Channel3_BASE
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k4) ? DMA2_Channel4_BASE
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k5) ? DMA2_Channel5_BASE
#endif
		: 0;
#if OPT_DMA_VERSION > 1
	static constexpr uint32_t kChSelBase_ =
		(kDma_ == Dma::k1) ? DMA1_CSELR_BASE
#ifdef DMA2_BASE
		: (kDma_ == Dma::k2) ? DMA2_CSELR_BASE
#endif
		: 0;
#endif
	/// Transfer error Interrupt flag for the DMA channel
	static constexpr uint32_t kTeif =
		(kChan_ == DmaCh::k1) ? DMA_ISR_TEIF1
		: (kChan_ == DmaCh::k2) ? DMA_ISR_TEIF2
		: (kChan_ == DmaCh::k3) ? DMA_ISR_TEIF3
		: (kChan_ == DmaCh::k4) ? DMA_ISR_TEIF4
		: (kChan_ == DmaCh::k5) ? DMA_ISR_TEIF5
		: (kChan_ == DmaCh::k6) ? DMA_ISR_TEIF6
		: (kChan_ == DmaCh::k7) ? DMA_ISR_TEIF7
		: 0;
	/// Half transfer event Interrupt flag for the DMA channel
	static constexpr uint32_t kHtif =
		(kChan_ == DmaCh::k1) ? DMA_ISR_HTIF1
		: (kChan_ == DmaCh::k2) ? DMA_ISR_HTIF2
		: (kChan_ == DmaCh::k3) ? DMA_ISR_HTIF3
		: (kChan_ == DmaCh::k4) ? DMA_ISR_HTIF4
		: (kChan_ == DmaCh::k5) ? DMA_ISR_HTIF5
		: (kChan_ == DmaCh::k6) ? DMA_ISR_HTIF6
		: (kChan_ == DmaCh::k7) ? DMA_ISR_HTIF7
		: 0;
	/// Transfer complete Interrupt flag for the DMA channel
	static constexpr uint32_t kTcif =
		(kChan_ == DmaCh::k1) ? DMA_ISR_TCIF1
		: (kChan_ == DmaCh::k2) ? DMA_ISR_TCIF2
		: (kChan_ == DmaCh::k3) ? DMA_ISR_TCIF3
		: (kChan_ == DmaCh::k4) ? DMA_ISR_TCIF4
		: (kChan_ == DmaCh::k5) ? DMA_ISR_TCIF5
		: (kChan_ == DmaCh::k6) ? DMA_ISR_TCIF6
		: (kChan_ == DmaCh::k7) ? DMA_ISR_TCIF7
		: 0;
	/// Global interrupt flag for the DMA channel
	static constexpr uint32_t kGif =
		(kChan_ == DmaCh::k1) ? DMA_ISR_GIF1
		: (kChan_ == DmaCh::k2) ? DMA_ISR_GIF2
		: (kChan_ == DmaCh::k3) ? DMA_ISR_GIF3
		: (kChan_ == DmaCh::k4) ? DMA_ISR_GIF4
		: (kChan_ == DmaCh::k5) ? DMA_ISR_GIF5
		: (kChan_ == DmaCh::k6) ? DMA_ISR_GIF6
		: (kChan_ == DmaCh::k7) ? DMA_ISR_GIF7
		: 0;
	/// NVIC initialization flag
	static constexpr bool kDoInitNvic = doInitNvic;
	/// NVIC Interrupt flag for the DMA channel
	static constexpr IRQn_Type kNvicDmaIrqn_ =
		(kDma_ == Dma::k1 && kChan_ == DmaCh::k1) ? DMA1_Channel1_IRQn
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k2) ? DMA1_Channel2_IRQn
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k3) ? DMA1_Channel3_IRQn
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k4) ? DMA1_Channel4_IRQn
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k5) ? DMA1_Channel5_IRQn
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k6) ? DMA1_Channel6_IRQn
		: (kDma_ == Dma::k1 && kChan_ == DmaCh::k7) ? DMA1_Channel7_IRQn
#ifdef DMA2_BASE
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k1) ? DMA2_Channel1_IRQn
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k2) ? DMA2_Channel2_IRQn
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k3) ? DMA2_Channel3_IRQn
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k4) ? DMA2_Channel4_IRQn
		: (kDma_ == Dma::k2 && kChan_ == DmaCh::k5) ? DMA2_Channel5_IRQn
#endif
		: DMA1_Channel1_IRQn;
	/// The IRQ configuration template for that DMA channel
	typedef IrqTemplate<kNvicDmaIrqn_> DmaIrq;

	/// Returns root device structure
	ALWAYS_INLINE static DMA_TypeDef *GetDeviceRoot() { return (DMA_TypeDef *)kDmaBase_; }
	/// Returns device structure for the channel
	ALWAYS_INLINE static DMA_Channel_TypeDef *GetDevice() { return (DMA_Channel_TypeDef *)kChBase_; }
#if OPT_DMA_VERSION > 1
	/// Returns root device structure
	ALWAYS_INLINE static DMA_Request_TypeDef *GetDeviceSel() { return (DMA_Request_TypeDef *)kChSelBase_; }
#endif

	/// Enables the DMA controller and performs initialization
	ALWAYS_INLINE static void Init()
	{
#ifdef RCC_AHBENR_DMA1EN
		if (kDma_ == Dma::k1)
			RCC->AHBENR |= RCC_AHBENR_DMA1EN;
#endif
#ifdef RCC_AHB1ENR_DMA1EN
		if (kDma_ == Dma::k1)
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
#endif
#ifdef RCC_AHBENR_DMA2EN
		if (kDma_ == Dma::k2)
			RCC->AHBENR |= RCC_AHBENR_DMA2EN;
#endif
#ifdef RCC_AHB1ENR_DMA2EN
		if (kDma_ == Dma::k2)
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
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
#ifdef RCC_AHBENR_DMA1EN
		if (kDma_ == Dma::k1)
			RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
#endif
#ifdef RCC_AHB1ENR_DMA1EN
		if (kDma_ == Dma::k1)
			RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN;
#endif
#ifdef RCC_AHBENR_DMA2EN
		if (kDma_ == Dma::k2)
			RCC->AHBENR &= ~RCC_AHBENR_DMA2EN;
#endif
#ifdef RCC_AHB1ENR_DMA2EN
		if (kDma_ == Dma::k2)
			RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN;
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
		uint32_t tmp = kPrio << DMA_CCR_PL_Pos;
		switch (kDir)
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
		if (kDir == kDmaMemToPer || kDir == kDmaMemToPerCircular)
		{
			switch (kSrcPtr)
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
			switch (kDstPtr)
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
			switch (kSrcPtr)
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
			switch (kDstPtr)
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
#if OPT_DMA_VERSION > 1
		DMA_Request_TypeDef *sel = GetDeviceSel();
		sel->CSELR = sel->CSELR & ~(0b1111 << (uint32_t(kChan_) << 4))
			| (uint32_t(kChSel) << (uint32_t(kChan_) << 4));
#endif
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
		if (kDir == kDmaMemToPer || kDir == kDmaMemToPerCircular)
			dma->CMAR = (uint32_t)addr;
		else
			dma->CPAR = (uint32_t)addr;
	}

	/// Sets the destination pointer address
	ALWAYS_INLINE static void SetDestAddress(volatile void *addr)
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		if (kDir == kDmaMemToPer || kDir == kDmaMemToPerCircular)
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
		DMA_TypeDef *dma = GetDeviceRoot();
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
		while (!IsTransferComplete())
		{
		}
		ClearAllFlags();
	}
};


/// A template class to configure ADC with DMA transfer
template<
	const bool kCircular = true
	, const DmaPointerCtrl kDstPtr = kDmaShortPtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaAdc1Template : public DmaChannel <
	Dma::k1,
	DmaCh::k1,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k0,
#endif
	kCircular ? kDmaPerToMemCircular : kDmaPerToMem,
	kDmaShortPtrConst,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI1 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrConst
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaSpi1RxTemplate : public DmaChannel <
	Dma::k1,
	DmaCh::k2,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k1,
#endif
	kCircular ? kDmaPerToMemCircular :
	kDmaPerToMem,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI1 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrInc
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrConst
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaSpi1TxTemplate : public DmaChannel <
	Dma::k1,
	DmaCh::k3,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k1,
#endif
	kCircular ? kDmaMemToPerCircular : kDmaMemToPer,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


#ifdef SPI2_BASE
/// A template class for SPI2 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrConst
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaSpi2RxTemplate : public DmaChannel <
	Dma::k1,
	DmaCh::k4,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k1,
#endif
	kCircular ? kDmaPerToMemCircular : kDmaPerToMem,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI2 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrInc
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrConst
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaSpi2TxTemplate : public DmaChannel <
	Dma::k1,
	DmaCh::k5,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k1,
#endif
	kCircular ? kDmaMemToPerCircular : kDmaMemToPer,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };
#endif


#ifdef SPI3_BASE
/// A template class for SPI2 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrConst
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaSpi3RxTemplate : public DmaChannel <
	Dma::k2,
	DmaCh::k1,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k3,
#endif
	kCircular ? kDmaPerToMemCircular : kDmaPerToMem,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI2 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrInc
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrConst
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaSpi3TxTemplate : public DmaChannel <
	Dma::k2,
	DmaCh::k2,
#if OPT_DMA_VERSION > 1
	DmaPerSel::k3,
#endif
	kCircular ? kDmaMemToPerCircular : kDmaMemToPer,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };
#endif


/// A template class for a DMA operation triggered by Timer 1 TRG event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1TrigTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k4, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 COM (Commutation) event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1ComTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k4, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 UP event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1UpTemplate : public DmaChannel<
	Dma::k1, 
#if OPT_DMA_VERSION == 1
	DmaCh::k5,
#else
	DmaCh::k6,
	DmaPerSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH1 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1Ch1Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k2, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH2 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1Ch2Template : public DmaChannel<
	Dma::k1, 
#if OPT_DMA_VERSION == 1
	DmaCh::k4,
#else
	DmaCh::k3,
	DmaPerSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH3 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1Ch3Template : public DmaChannel<
	Dma::k1, 
#if OPT_DMA_VERSION == 1
	DmaCh::k6,
#else
	DmaCh::k7,
	DmaPerSel::k7,
#endif
	kDir, 
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH4 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim1Ch4Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k4, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


#ifdef TIM2_BASE
/// A template class for a DMA operation triggered by Timer 2 UP event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim2UpTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k2, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH1 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim2Ch1Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k5, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH2 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim2Ch2Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k7, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH3 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim2Ch3Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k1, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH4 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim2Ch4Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k7, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };
#endif	// TIM2_BASE


#ifdef TIM3_BASE
/// A template class for a DMA operation triggered by Timer 3 TRIG event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim3TrigTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k6, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };

/// A template class for a DMA operation triggered by Timer 3 UP event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim3UpTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k3, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 3 CH1 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim3Ch1Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k6, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 3 CH3 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim3Ch3Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k2, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 3 CH4 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim3Ch4Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k3, 
#if OPT_DMA_VERSION > 1
#	error Please fix this
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };
#endif	// TIM3_BASE


#ifdef TIM4_BASE
/// A template class for a DMA operation triggered by Timer 4 UP event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim4UpTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k7, 
#if OPT_DMA_VERSION > 1
#	error Please fix this
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 4 CH1 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim4Ch1Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k1, 
#if OPT_DMA_VERSION > 1
#	error Please fix this
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 4 CH3 event
template<
	const DmaDirection kDir
	, const DmaPointerCtrl kSrcPtr
	, const DmaPointerCtrl kDstPtr
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaTim4Ch3Template : public DmaChannel<
	Dma::k1, 
	DmaCh::k5, 
#if OPT_DMA_VERSION > 1
#	error Please fix this
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };
#endif	// TIM4_BASE


/// A template class for USART1 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaUsart1RxTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k5, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k2,
#endif
	kCircular ? kDmaPerToMemCircular : kDmaPerToMem,
	kDmaBytePtrConst, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for USART1 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaUsart1TxTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k4, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k2,
#endif
	kCircular ? kDmaMemToPerCircular : kDmaMemToPer,
	kSrcPtr, 
	kDmaBytePtrConst, 
	kPrio
>
{ };


#ifdef USART2_BASE
/// A template class for USART2 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaUsart2RxTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k6, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k2,
#endif
	kCircular ? kDmaPerToMemCircular : kDmaPerToMem,
	kDmaBytePtrConst, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for USART2 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaUsart2TxTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k7, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k2,
#endif
	kCircular ? kDmaMemToPerCircular : kDmaMemToPer,
	kSrcPtr, 
	kDmaBytePtrConst, 
	kPrio
>
{ };
#endif	// USART2_BASE


#ifdef USART3_BASE
/// A template class for USART3 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kDstPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaUsart3RxTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k3, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k2,
#endif
	kCircular ? kDmaPerToMemCircular : kDmaPerToMem,
	kDmaBytePtrConst, 
	kDstPtr, 
	kPrio
>
{ };

/// A template class for USART3 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const DmaPointerCtrl kSrcPtr = kDmaBytePtrInc
	, const DmaPriority kPrio = kDmaMediumPrio
>
class DmaUsart3TxTemplate : public DmaChannel<
	Dma::k1, 
	DmaCh::k2, 
#if OPT_DMA_VERSION > 1
	DmaPerSel::k2,
#endif
	kCircular ? kDmaMemToPerCircular : kDmaMemToPer,
	kSrcPtr, 
	kDmaBytePtrConst, 
	kPrio
>
{ };
#endif // USART2_BASE


}	// namespace Bmt
