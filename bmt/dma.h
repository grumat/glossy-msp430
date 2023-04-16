#pragma once

#include "irq.h"

#if defined(DMA1_CSELR_BASE) || defined(DMA2_CSELR_BASE)
#	define OPT_DMA_VERSION	2
#else
#	define OPT_DMA_VERSION	1
#endif

namespace Bmt
{
namespace Dma
{

/// The DMA Peripheral
enum class Itf
{
	k1				///< DMA1 controller
#ifdef DMA2_BASE
	, k2			///< DMA2 controller
#endif
};

/// Channel of the DMA controller
enum class Chan
{
	k1			///< Channel 1 of the DMA controller
	, k2		///< Channel 2 of the DMA controller
	, k3		///< Channel 3 of the DMA controller
	, k4		///< Channel 4 of the DMA controller
	, k5		///< Channel 5 of the DMA controller
	, k6		///< Channel 6 of the DMA controller
	, k7		///< Channel 7 of the DMA controller
	, k8		///< Channel 8 of the DMA controller
	, kNone		///< Indicates configuration not available
};

#if OPT_DMA_VERSION > 1
enum class PerifSel
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
enum class Dir
{
	kMemToMem			///< Memory to memory
	, kPerToMem			///< Peripheral to memory
	, kPerToMemCircular	///< Peripheral to memory circular mode
	, kMemToPer			///< Memory to peripheral
	, kMemToPerCircular	///< Memory to peripheral circular mode
};

/// Policy used for the source/target pointer
enum class PtrPolicy
{
	kBytePtr			///< Performs a *pByte operation
	, kBytePtrInc		///< Performs a *pByte++ operation
	, kShortPtr			///< Performs a *pShort operation
	, kShortPtrInc		///< Performs a *pShort++ operation
	, kLongPtr			///< Performs a *pLong operation
	, kLongPtrInc		///< Performs a *pLong++ operation
};

/// The DMA priority
enum class Prio
{
	kLow				///< Low channel priority
	, kMedium			///< Medium channel priority
	, kHigh				///< High channel priority
	, kVeryHigh			///< Very high channel priority
};

/// Template class that describes a DMA configuration
template <
	const Itf kDma						///< The DMA controller
	, const Chan kChan					///< The DMA channel
#if OPT_DMA_VERSION > 1
	, const PerifSel kChSel				///< The DMA channel selector
#endif
	, const Dir kDir		///< Data direction for this channel
	, const PtrPolicy kSrcPtr		///< Source Pointer behavior
	, const PtrPolicy kDstPtr		///< Target Pointer behavior
	, const Prio kPrio = Prio::kMedium	///< DMA transfer priority
	, const bool doInitNvic = false		///< Should be the NVIC also initialized?
>
class AnyChannel
{
public:
	/// A constant with the DMA controller instance number
	static constexpr Itf kDma_ = kDma;
	/// A constant with the DMA channel number
	static constexpr Chan kChan_ = kChan;
#if OPT_DMA_VERSION > 1
	/// A constant with the DMA channel number
	static constexpr PerifSel kChSel_ = kChSel;
#endif
	/// The address base of the DMA peripheral
	static constexpr uint32_t kDmaBase_ =
		(kDma_ == Itf::k1) ? DMA1_BASE
#ifdef DMA2_BASE
		: (kDma_ == Itf::k2) ? DMA2_BASE
#endif
		: 0;
	/// The address base of the DMA channel peripheral
	static constexpr uint32_t kChBase_ =
		(kDma_ == Itf::k1 && kChan_ == Chan::k1) ? DMA1_Channel1_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k2) ? DMA1_Channel2_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k3) ? DMA1_Channel3_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k4) ? DMA1_Channel4_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k5) ? DMA1_Channel5_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k6) ? DMA1_Channel6_BASE
#ifdef DMA1_Channel7_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k7) ? DMA1_Channel7_BASE
#endif
#ifdef DMA1_Channel8_BASE
		: (kDma_ == Itf::k1 && kChan_ == Chan::k8) ? DMA1_Channel8_BASE
#endif
#ifdef DMA2_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k1) ? DMA2_Channel1_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k2) ? DMA2_Channel2_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k3) ? DMA2_Channel3_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k4) ? DMA2_Channel4_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k5) ? DMA2_Channel5_BASE
#endif
#ifdef DMA2_Channel6_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k6) ? DMA2_Channel6_BASE
#endif
#ifdef DMA2_Channel7_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k7) ? DMA2_Channel7_BASE
#endif
#ifdef DMA2_Channel8_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k8) ? DMA2_Channel8_BASE
#endif
		: 0;
#if OPT_DMA_VERSION > 1
	static constexpr uint32_t kChSelBase_ =
		(kDma_ == Itf::k1) ? DMA1_CSELR_BASE
#ifdef DMA2_BASE
		: (kDma_ == Itf::k2) ? DMA2_CSELR_BASE
#endif
		: 0;
#endif
	/// Transfer error Interrupt flag for the DMA channel
	static constexpr uint32_t kTeif =
		(kChan_ == Chan::k1) ? DMA_ISR_TEIF1
		: (kChan_ == Chan::k2) ? DMA_ISR_TEIF2
		: (kChan_ == Chan::k3) ? DMA_ISR_TEIF3
		: (kChan_ == Chan::k4) ? DMA_ISR_TEIF4
		: (kChan_ == Chan::k5) ? DMA_ISR_TEIF5
		: (kChan_ == Chan::k6) ? DMA_ISR_TEIF6
#ifdef DMA_ISR_TEIF7
		: (kChan_ == Chan::k7) ? DMA_ISR_TEIF7
#endif
#ifdef DMA_ISR_TEIF8
		: (kChan_ == Chan::k8) ? DMA_ISR_TEIF8
#endif
		: 0;
	/// Half transfer event Interrupt flag for the DMA channel
	static constexpr uint32_t kHtif =
		(kChan_ == Chan::k1) ? DMA_ISR_HTIF1
		: (kChan_ == Chan::k2) ? DMA_ISR_HTIF2
		: (kChan_ == Chan::k3) ? DMA_ISR_HTIF3
		: (kChan_ == Chan::k4) ? DMA_ISR_HTIF4
		: (kChan_ == Chan::k5) ? DMA_ISR_HTIF5
		: (kChan_ == Chan::k6) ? DMA_ISR_HTIF6
#ifdef DMA_ISR_HTIF7
		: (kChan_ == Chan::k7) ? DMA_ISR_HTIF7
#endif
#ifdef DMA_ISR_HTIF8
		: (kChan_ == Chan::k8) ? DMA_ISR_HTIF8
#endif
		: 0;
	/// Transfer complete Interrupt flag for the DMA channel
	static constexpr uint32_t kTcif =
		(kChan_ == Chan::k1) ? DMA_ISR_TCIF1
		: (kChan_ == Chan::k2) ? DMA_ISR_TCIF2
		: (kChan_ == Chan::k3) ? DMA_ISR_TCIF3
		: (kChan_ == Chan::k4) ? DMA_ISR_TCIF4
		: (kChan_ == Chan::k5) ? DMA_ISR_TCIF5
		: (kChan_ == Chan::k6) ? DMA_ISR_TCIF6
#ifdef DMA_ISR_TCIF7
		: (kChan_ == Chan::k7) ? DMA_ISR_TCIF7
#endif
#ifdef DMA_ISR_TCIF8
		: (kChan_ == Chan::k8) ? DMA_ISR_TCIF8
#endif
		: 0;
	/// Global interrupt flag for the DMA channel
	static constexpr uint32_t kGif =
		(kChan_ == Chan::k1) ? DMA_ISR_GIF1
		: (kChan_ == Chan::k2) ? DMA_ISR_GIF2
		: (kChan_ == Chan::k3) ? DMA_ISR_GIF3
		: (kChan_ == Chan::k4) ? DMA_ISR_GIF4
		: (kChan_ == Chan::k5) ? DMA_ISR_GIF5
		: (kChan_ == Chan::k6) ? DMA_ISR_GIF6
#ifdef DMA_ISR_GIF7
		: (kChan_ == Chan::k7) ? DMA_ISR_GIF7
#endif
#ifdef DMA_ISR_GIF8
		: (kChan_ == Chan::k8) ? DMA_ISR_GIF8
#endif
		: 0;
	/// NVIC initialization flag
	static constexpr bool kDoInitNvic = doInitNvic;
	/// NVIC Interrupt flag for the DMA channel
	static constexpr IRQn_Type kNvicDmaIrqn_ =
		(kDma_ == Itf::k1 && kChan_ == Chan::k1) ? DMA1_Channel1_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k2) ? DMA1_Channel2_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k3) ? DMA1_Channel3_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k4) ? DMA1_Channel4_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k5) ? DMA1_Channel5_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k6) ? DMA1_Channel6_IRQn
#ifdef DMA1_Channel7_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k7) ? DMA1_Channel7_IRQn
#endif
#ifdef DMA1_Channel8_IRQn
		: (kDma_ == Itf::k1 && kChan_ == Chan::k8) ? DMA1_Channel8_IRQn
#endif
#ifdef DMA2_BASE
		: (kDma_ == Itf::k2 && kChan_ == Chan::k1) ? DMA2_Channel1_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k2) ? DMA2_Channel2_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k3) ? DMA2_Channel3_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k4) ? DMA2_Channel4_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k5) ? DMA2_Channel5_IRQn
#endif
#ifdef DMA2_Channel6_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k6) ? DMA2_Channel6_IRQn
#endif
#ifdef DMA2_Channel7_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k7) ? DMA2_Channel7_IRQn
#endif
#ifdef DMA2_Channel8_IRQn
		: (kDma_ == Itf::k2 && kChan_ == Chan::k8) ? DMA2_Channel8_IRQn
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
		if (kDma_ == Itf::k1)
			RCC->AHBENR |= RCC_AHBENR_DMA1EN;
#endif
#ifdef RCC_AHB1ENR_DMA1EN
		if (kDma_ == Itf::k1)
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
#endif
#ifdef RCC_AHBENR_DMA2EN
		if (kDma_ == Itf::k2)
			RCC->AHBENR |= RCC_AHBENR_DMA2EN;
#endif
#ifdef RCC_AHB1ENR_DMA2EN
		if (kDma_ == Itf::k2)
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
		if (kDma_ == Itf::k1)
			RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
#endif
#ifdef RCC_AHB1ENR_DMA1EN
		if (kDma_ == Itf::k1)
			RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN;
#endif
#ifdef RCC_AHBENR_DMA2EN
		if (kDma_ == Itf::k2)
			RCC->AHBENR &= ~RCC_AHBENR_DMA2EN;
#endif
#ifdef RCC_AHB1ENR_DMA2EN
		if (kDma_ == Itf::k2)
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
		uint32_t tmp = uint32_t(kPrio) << DMA_CCR_PL_Pos;
		switch (kDir)
		{
		case Dir::kMemToMem:
			tmp |= DMA_CCR_MEM2MEM;
			break;
		case Dir::kPerToMem:
			break;
		case Dir::kPerToMemCircular:
			tmp |= DMA_CCR_CIRC;
			break;
		case Dir::kMemToPer:
			tmp |= DMA_CCR_DIR;
			break;
		case Dir::kMemToPerCircular:
			tmp |= DMA_CCR_DIR | DMA_CCR_CIRC;
			break;
		}
		if (kDir == Dir::kMemToPer || kDir == Dir::kMemToPerCircular)
		{
			switch (kSrcPtr)
			{
			case PtrPolicy::kBytePtr:
				break;
			case PtrPolicy::kBytePtrInc:
				tmp |= DMA_CCR_MINC;
				break;
			case PtrPolicy::kShortPtr:
				tmp |= DMA_CCR_MSIZE_0;
				break;
			case PtrPolicy::kShortPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_0;
				break;
			case PtrPolicy::kLongPtr:
				tmp |= DMA_CCR_MSIZE_1;
				break;
			case PtrPolicy::kLongPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_1;
				break;
			}
			switch (kDstPtr)
			{
			case PtrPolicy::kBytePtr:
				break;
			case PtrPolicy::kBytePtrInc:
				tmp |= DMA_CCR_PINC;
				break;
			case PtrPolicy::kShortPtr:
				tmp |= DMA_CCR_PSIZE_0;
				break;
			case PtrPolicy::kShortPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_0;
				break;
			case PtrPolicy::kLongPtr:
				tmp |= DMA_CCR_PSIZE_1;
				break;
			case PtrPolicy::kLongPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_1;
				break;
			}
		}
		else
		{
			switch (kSrcPtr)
			{
			case PtrPolicy::kBytePtr:
				break;
			case PtrPolicy::kBytePtrInc:
				tmp |= DMA_CCR_PINC;
				break;
			case PtrPolicy::kShortPtr:
				tmp |= DMA_CCR_PSIZE_0;
				break;
			case PtrPolicy::kShortPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_0;
				break;
			case PtrPolicy::kLongPtr:
				tmp |= DMA_CCR_PSIZE_1;
				break;
			case PtrPolicy::kLongPtrInc:
				tmp |= DMA_CCR_PINC | DMA_CCR_PSIZE_1;
				break;
			}
			switch (kDstPtr)
			{
			case PtrPolicy::kBytePtr:
				break;
			case PtrPolicy::kBytePtrInc:
				tmp |= DMA_CCR_MINC;
				break;
			case PtrPolicy::kShortPtr:
				tmp |= DMA_CCR_MSIZE_0;
				break;
			case PtrPolicy::kShortPtrInc:
				tmp |= DMA_CCR_MINC | DMA_CCR_MSIZE_0;
				break;
			case PtrPolicy::kLongPtr:
				tmp |= DMA_CCR_MSIZE_1;
				break;
			case PtrPolicy::kLongPtrInc:
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
		if (kDir == Dir::kMemToPer || kDir == Dir::kMemToPerCircular)
			dma->CMAR = (uint32_t)addr;
		else
			dma->CPAR = (uint32_t)addr;
	}

	/// Sets the destination pointer address
	ALWAYS_INLINE static void SetDestAddress(volatile void *addr)
	{
		DMA_Channel_TypeDef *dma = GetDevice();
		if (kDir == Dir::kMemToPer || kDir == Dir::kMemToPerCircular)
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
	, const PtrPolicy kDstPtr = PtrPolicy::kShortPtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyAdc1 : public AnyChannel <
	Itf::k1,
	Chan::k1,
#if OPT_DMA_VERSION > 1
	PerifSel::k0,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	PtrPolicy::kShortPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI1 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtr
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnySpi1Rx : public AnyChannel <
	Itf::k1,
	Chan::k2,
#if OPT_DMA_VERSION > 1
	PerifSel::k1,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI1 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtrInc
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtr
	, const Prio kPrio = Prio::kMedium
>
class AnySpi1Tx : public AnyChannel <
	Itf::k1,
	Chan::k3,
#if OPT_DMA_VERSION > 1
	PerifSel::k1,
#endif
	kCircular ? Dir::kMemToPerCircular : Dir::kMemToPer,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


#ifdef SPI2_BASE
/// A template class for SPI2 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtr
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnySpi2Rx : public AnyChannel <
	Itf::k1,
	Chan::k4,
#if OPT_DMA_VERSION > 1
	PerifSel::k1,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI2 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtrInc
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtr
	, const Prio kPrio = Prio::kMedium
>
class AnySpi2Tx : public AnyChannel <
	Itf::k1,
	Chan::k5,
#if OPT_DMA_VERSION > 1
	PerifSel::k1,
#endif
	kCircular ? Dir::kMemToPerCircular : Dir::kMemToPer,
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
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtr
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnySpi3Rx : public AnyChannel <
	Itf::k2,
	Chan::k1,
#if OPT_DMA_VERSION > 1
	PerifSel::k3,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };


/// A template class for SPI2 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtrInc
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtr
	, const Prio kPrio = Prio::kMedium
>
class AnySpi3Tx : public AnyChannel <
	Itf::k2,
	Chan::k2,
#if OPT_DMA_VERSION > 1
	PerifSel::k3,
#endif
	kCircular ? Dir::kMemToPerCircular : Dir::kMemToPer,
	kSrcPtr,
	kDstPtr,
	kPrio
>
{ };
#endif


/// A template class for a DMA operation triggered by Timer 1 TRG event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Trig : public AnyChannel<
	Itf::k1, 
	Chan::k4, 
#if OPT_DMA_VERSION > 1
	PerifSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 COM (Commutation) event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Com : public AnyChannel<
	Itf::k1, 
	Chan::k4, 
#if OPT_DMA_VERSION > 1
	PerifSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 UP event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Up : public AnyChannel<
	Itf::k1, 
#if OPT_DMA_VERSION == 1
	Chan::k5,
#else
	Chan::k6,
	PerifSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH1 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Ch1 : public AnyChannel<
	Itf::k1, 
	Chan::k2, 
#if OPT_DMA_VERSION > 1
	PerifSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH2 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Ch2 : public AnyChannel<
	Itf::k1, 
#if OPT_DMA_VERSION == 1
	Chan::k4,
#else
	Chan::k3,
	PerifSel::k7,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH3 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Ch3 : public AnyChannel<
	Itf::k1, 
#if OPT_DMA_VERSION == 1
	Chan::k6,
#else
	Chan::k7,
	PerifSel::k7,
#endif
	kDir, 
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 1 CH4 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim1Ch4 : public AnyChannel<
	Itf::k1, 
	Chan::k4, 
#if OPT_DMA_VERSION > 1
	PerifSel::k7,
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
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim2Up : public AnyChannel<
	Itf::k1, 
	Chan::k2, 
#if OPT_DMA_VERSION > 1
	PerifSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH1 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim2Ch1 : public AnyChannel<
	Itf::k1, 
	Chan::k5, 
#if OPT_DMA_VERSION > 1
	PerifSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH2 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim2Ch2 : public AnyChannel<
	Itf::k1, 
	Chan::k7, 
#if OPT_DMA_VERSION > 1
	PerifSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH3 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim2Ch3 : public AnyChannel<
	Itf::k1, 
	Chan::k1, 
#if OPT_DMA_VERSION > 1
	PerifSel::k4,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 2 CH4 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim2Ch4 : public AnyChannel<
	Itf::k1, 
	Chan::k7, 
#if OPT_DMA_VERSION > 1
	PerifSel::k4,
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
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim3Trig : public AnyChannel<
	Itf::k1, 
	Chan::k6, 
#if OPT_DMA_VERSION > 1
	PerifSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };

/// A template class for a DMA operation triggered by Timer 3 UP event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim3Up : public AnyChannel<
	Itf::k1, 
	Chan::k3, 
#if OPT_DMA_VERSION > 1
	PerifSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 3 CH1 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim3Ch1 : public AnyChannel<
	Itf::k1, 
	Chan::k6, 
#if OPT_DMA_VERSION > 1
	PerifSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 3 CH3 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim3Ch3 : public AnyChannel<
	Itf::k1, 
	Chan::k2, 
#if OPT_DMA_VERSION > 1
	PerifSel::k5,
#endif
	kDir,
	kSrcPtr, 
	kDstPtr, 
	kPrio
>
{ };


/// A template class for a DMA operation triggered by Timer 3 CH4 event
template<
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim3Ch4 : public AnyChannel<
	Itf::k1, 
	Chan::k3, 
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
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim4Up : public AnyChannel<
	Itf::k1, 
	Chan::k7, 
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
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim4Ch1 : public AnyChannel<
	Itf::k1, 
	Chan::k1, 
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
	const Dir kDir
	, const PtrPolicy kSrcPtr
	, const PtrPolicy kDstPtr
	, const Prio kPrio = Prio::kMedium
>
class AnyTim4Ch3 : public AnyChannel<
	Itf::k1, 
	Chan::k5, 
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
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyUsart1Rx : public AnyChannel<
	Itf::k1, 
	Chan::k5, 
#if OPT_DMA_VERSION > 1
	PerifSel::k2,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	PtrPolicy::kBytePtr,
	kDstPtr, 
	kPrio
>
{ };


/// A template class for USART1 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyUsart1Tx : public AnyChannel<
	Itf::k1, 
	Chan::k4, 
#if OPT_DMA_VERSION > 1
	PerifSel::k2,
#endif
	kCircular ? Dir::kMemToPerCircular : Dir::kMemToPer,
	kSrcPtr, 
	PtrPolicy::kBytePtr,
	kPrio
>
{ };


#ifdef USART2_BASE
/// A template class for USART2 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyUsart2Rx : public AnyChannel<
	Itf::k1, 
	Chan::k6, 
#if OPT_DMA_VERSION > 1
	PerifSel::k2,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	PtrPolicy::kBytePtr,
	kDstPtr, 
	kPrio
>
{ };


/// A template class for USART2 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyUsart2Tx : public AnyChannel<
	Itf::k1, 
	Chan::k7, 
#if OPT_DMA_VERSION > 1
	PerifSel::k2,
#endif
	kCircular ? Dir::kMemToPerCircular : Dir::kMemToPer,
	kSrcPtr, 
	PtrPolicy::kBytePtr,
	kPrio
>
{ };
#endif	// USART2_BASE


#ifdef USART3_BASE
/// A template class for USART3 RX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kDstPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyUsart3Rx : public AnyChannel<
	Itf::k1, 
	Chan::k3, 
#if OPT_DMA_VERSION > 1
	PerifSel::k2,
#endif
	kCircular ? Dir::kPerToMemCircular : Dir::kPerToMem,
	PtrPolicy::kBytePtr,
	kDstPtr, 
	kPrio
>
{ };

/// A template class for USART3 TX transfers of bytes using DMA
template<
	const bool kCircular = true
	, const PtrPolicy kSrcPtr = PtrPolicy::kBytePtrInc
	, const Prio kPrio = Prio::kMedium
>
class AnyUsart3Tx : public AnyChannel<
	Itf::k1, 
	Chan::k2, 
#if OPT_DMA_VERSION > 1
	PerifSel::k2,
#endif
	kCircular ? Dir::kMemToPerCircular : Dir::kMemToPer,
	kSrcPtr, 
	PtrPolicy::kBytePtr,
	kPrio
>
{ };
#endif // USART2_BASE


}	// namespace Dma
}	// namespace Bmt
