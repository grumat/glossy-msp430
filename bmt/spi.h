#pragma once

#include "dma.h"
#include "irq.h"

namespace Bmt
{

/// Instance of the SPI peripheral
enum class Spi
{
	k1 = 0			///< SPI1 peripheral
#ifdef SPI2_BASE
	, k2			///< SPI2 peripheral
#endif
#ifdef SPI3_BASE
	, k3			///< SPI3 peripheral
#endif
};


/// SPI polarity/phase of signals
enum class SpiSignals
{
	kMode0			///< CPOL: 0, CPHA: 0
	, kMode1		///< CPOL: 0, CPHA: 1
	, kMode2		///< CPOL: 1, CPHA: 0
	, kMode3		///< CPOL: 1, CPHA: 1
};


/// SPI operation mode
enum class SpiRole
{
	kMaster			///< Master mode
	, kMultiMaster	///< MultiMaster mode
	, kSlaveSW		///< Slave, flow controlled by software
	, kSlaveHW		///< Slave, flow controlled by hardware NSS pin
};


/// SPI bidirectional configuration
enum class SpiBiDi
{
	kFullDuplex			///< Normal mode (clk + 2 data lines)
	, kReceiveOnly		///< 2 lines; output disable
	, kHalfDuplexOut	///< Half duplex mode (clk and out lines)
	, kHalfDuplexIn		///< Half duplex mode (clk and in lines)
};


/// Format of the SPI frame
enum class SpiFormat
{
	k8bitMsb		///< 8-bit data frame MSB first
	, k16bitMsb		///< 16-bit data frame MSB first
	, k8bitLsb		///< 8-bit data frame LSB first
	, k16bitLsb		///< 16-bit data frame LSB first
};

enum class RawSpiSpeed : uint32_t;

/// A template class for an SPI peripheral configuration
template<
	const Spi SPI_N			///< The SPI instance
	, typename CLOCK				///< Clock data type driving the system
	, const int SPEED = 1000000		///< The desired speed (effective speed is the truncated value)
	, const SpiRole OPERATION = SpiRole::kMaster	///< Mode of operation
	, const SpiSignals mode_ = SpiSignals::kMode3		///< Polarity and phase
	, const SpiFormat FORMAT = SpiFormat::k8bitMsb	///< Frame format
	, const bool USE_IRQ = true				///< Configure IRQ for the peripheral
	, const SpiBiDi DUPLEX = SpiBiDi::kFullDuplex ///< SPI lines direction
>
struct SpiTemplate
{
	/// The peripheral base address as a constant
	static constexpr uintptr_t kSpiBase_ =
		(SPI_N == Spi::k1) ? SPI1_BASE
#ifdef SPI2_BASE
		: (SPI_N == Spi::k2) ? SPI2_BASE
#endif
#ifdef SPI3_BASE
		: (SPI_N == Spi::k3) ? SPI3_BASE
#endif
		: 0 ;
	/// This constant refers to the ABP that owns the peripheral
	static constexpr uint32_t kInputClock_ =
		(SPI_N == Spi::k1) ? CLOCK::kApb2Clock_
		: CLOCK::kApb1Clock_;
	/// The constant representing the effective speed of the peripheral
	static constexpr uint32_t kFrequency_ =
		(SPEED >= kInputClock_ / 2) ? kInputClock_ / 2
		: (SPEED >= kInputClock_ / 4) ? kInputClock_ / 4
		: (SPEED >= kInputClock_ / 8) ? kInputClock_ / 8
		: (SPEED >= kInputClock_ / 16) ? kInputClock_ / 16
		: (SPEED >= kInputClock_ / 32) ? kInputClock_ / 32
		: (SPEED >= kInputClock_ / 64) ? kInputClock_ / 64
		: (SPEED >= kInputClock_ / 128) ? kInputClock_ / 128
		: kInputClock_ / 256;
	/// The constant to be applied to the CR1 register
	static constexpr uint32_t kCr1Speed_ =
		(SPEED >= kInputClock_ / 2) ? 0
		: (SPEED >= kInputClock_ / 4) ? 1
		: (SPEED >= kInputClock_ / 8) ? 2
		: (SPEED >= kInputClock_ / 16) ? 3
		: (SPEED >= kInputClock_ / 32) ? 4
		: (SPEED >= kInputClock_ / 64) ? 5
		: (SPEED >= kInputClock_ / 128) ? 6
		: 7;
	/// The constant with the IRQ type for this particular peripheral
	static constexpr IRQn_Type kNvicSpiIrqn_ =
		(SPI_N == Spi::k1) ? SPI1_IRQn :
#ifdef SPI2_BASE
		(SPI_N == Spi::k2) ? SPI2_IRQn :
#endif
#ifdef SPI3_BASE
		(SPI_N == Spi::k3) ? SPI3_IRQn :
#endif
		SPI1_IRQn
		;
	/// A data-type to control IRQ settings
	typedef IrqTemplate<kNvicSpiIrqn_> SpiIrq;

	/// The DMA instance for the peripheral
	static constexpr Dma DmaInstance_ = 
		(SPI_N == Spi::k1) ? Dma::k1 :
#ifdef SPI2_BASE
		(SPI_N == Spi::k2) ? Dma::k1 :
#endif
#ifdef SPI3_BASE
		(SPI_N == Spi::k3) ? Dma::k2 :
#endif
		Dma::k1
		;
	/// The DMA channel Transmit instance for the peripheral
	static constexpr DmaCh DmaTxCh_ =
		(SPI_N == Spi::k1) ? DmaCh::k3 :
#ifdef SPI2_BASE
		(SPI_N == Spi::k2) ? DmaCh::k5 :
#endif
#ifdef SPI3_BASE
		(SPI_N == Spi::k3) ? DmaCh::k2 :
#endif
		DmaCh::k5;
	/// The DMA channel Receive instance for the peripheral
	static constexpr DmaCh DmaRxCh_ =
		(SPI_N == Spi::k1) ? DmaCh::k2 :
#ifdef SPI2_BASE
		(SPI_N == Spi::k2) ? DmaCh::k4 :
#endif
#ifdef SPI3_BASE
		(SPI_N == Spi::k3) ? DmaCh::k1 :
#endif
		DmaCh::k4;

	/// Returns peripheral register structure
	ALWAYS_INLINE static volatile SPI_TypeDef * GetDevice() { return (volatile SPI_TypeDef *)kSpiBase_; }

	/// Initialize hardware configuration (including RCC)
	ALWAYS_INLINE static void Init()
	{
		// Invalid SPI device selected
		static_assert(kSpiBase_ != 0, "An invalid SPI device was selected");

		volatile SPI_TypeDef *spi = GetDevice();
		// Conditional compilation for specific peripheral
		switch (SPI_N)
		{
		case Spi::k1:
			RCC->APB2RSTR |= RCC_APB2ENR_SPI1EN;
			RCC->APB2RSTR &= ~RCC_APB2ENR_SPI1EN;
			if (USE_IRQ)
			{
				SpiIrq::ClearPending();
				SpiIrq::Enable();
			}
			break;
#ifdef SPI2_BASE
		case Spi::k2:
			RCC->APB1RSTR |= RCC_APB1ENR_SPI2EN;
			RCC->APB1RSTR &= ~RCC_APB1ENR_SPI2EN;
			if (USE_IRQ)
			{
				SpiIrq::ClearPending();
				SpiIrq::Enable();
			}
			break;
#endif
#if defined(RCC_APB1ENR_SPI3EN)
		case Spi::k3:
			RCC->APB1RSTR |= RCC_APB1ENR_SPI3EN;
			RCC->APB1RSTR &= ~RCC_APB1ENR_SPI3EN;
			if (USE_IRQ)
			{
				NVIC_EnableIRQ(SPI3_IRQn);
				NVIC_ClearPendingIRQ(SPI3_IRQn);
			}
			break;
#elif defined(RCC_APB1ENR1_SPI3EN)
		case Spi::k3:
			RCC->APB1RSTR1 |= RCC_APB1ENR1_SPI3EN;
			RCC->APB1RSTR1 &= ~RCC_APB1ENR1_SPI3EN;
			if (USE_IRQ)
			{
				NVIC_EnableIRQ(SPI3_IRQn);
				NVIC_ClearPendingIRQ(SPI3_IRQn);
			}
			break;
#endif
		}
		Setup();
	}

	/// Setup the SPI device
	ALWAYS_INLINE static void Setup()
	{
		// Enable clock
		volatile SPI_TypeDef*spi = GetDevice();
		volatile uint32_t delay;	// inserts delay instructions
		switch (SPI_N)
		{
		case Spi::k1:
			RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
			delay = RCC->APB2ENR & RCC_APB2ENR_SPI1EN;
			break;
#ifdef SPI2_BASE
		case Spi::k2:
			RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
			delay = RCC->APB2ENR & RCC_APB1ENR_SPI2EN;
			break;
#endif
#if defined(RCC_APB1ENR_SPI3EN)
		case Spi::k3:
			RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
			delay = RCC->APB1ENR & RCC_APB1ENR_SPI3EN;
			break;
#elif defined (RCC_APB1ENR1_SPI3EN)
		case Spi::k3:
			RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN;
			delay = RCC->APB1ENR1 & RCC_APB1ENR1_SPI3EN;
			break;
#endif
		}
		// Clock Polarity, phase, speed
		uint32_t tmp = ((uint32_t(mode_) & 0x1) ? SPI_CR1_CPHA : 0)
			| ((uint32_t(mode_) & 0x2) ? SPI_CR1_CPOL : 0)
			| kCr1Speed_ << SPI_CR1_BR_Pos
			;
		// Frame format
		if (FORMAT == SpiFormat::k8bitLsb || FORMAT == SpiFormat::k16bitLsb)
			tmp |= SPI_CR1_LSBFIRST;
#ifdef SPI_CR1_DFF
		if (FORMAT == SpiFormat::k16bitMsb || FORMAT == SpiFormat::k16bitLsb)
			tmp |= SPI_CR1_DFF;
#endif
		// Master slave
		switch (OPERATION)
		{
		case SpiRole::kMaster:
			tmp |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
			break;
		case SpiRole::kMultiMaster:
			tmp |= SPI_CR1_MSTR;
			break;
		case SpiRole::kSlaveSW:
			tmp |= SPI_CR1_SSM | SPI_CR1_SSI;
			break;
		case SpiRole::kSlaveHW:
			break;
		}
		// TODO: Duplex mode (docs are like spaghetti in this topic!!!)
		switch (DUPLEX)
		{
		case SpiBiDi::kFullDuplex:
			break;
		case SpiBiDi::kReceiveOnly:
			tmp |= SPI_CR1_RXONLY;
			break;
		case SpiBiDi::kHalfDuplexOut:
			tmp |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
			break;
		case SpiBiDi::kHalfDuplexIn:
			tmp |= SPI_CR1_BIDIMODE;
			break;
		}
		spi->CR1 = tmp;

		// Register CR2
		tmp = 0;
		switch (OPERATION)
		{
		case SpiRole::kMaster:
			tmp |= SPI_CR2_SSOE;
			break;
		case SpiRole::kMultiMaster:
			break;
		case SpiRole::kSlaveSW:
			break;
		case SpiRole::kSlaveHW:
			break;
		}
#ifdef SPI_CR2_DS
		if (FORMAT == SpiFormat::k16bitMsb || FORMAT == SpiFormat::k16bitLsb)
			tmp |= SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_DS_3;
		else
			tmp |= SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2;
#endif
		spi->CR2 = tmp;

		/*
		** Use the following software sequence to clear the MODF bit:
		** 1. Make a read or write access to the SPI_SR register while the MODF bit is set.
		** 2. Then write to the SPI_CR1 register.
		*/
		if (spi->SR & SPI_SR_MODF)
			spi->CR1 = spi->CR1;
		// Enable device
		spi->CR1 |= SPI_CR1_SPE;
	}

	//! Efficiently changes the speed
	ALWAYS_INLINE static RawSpiSpeed SetupSpeed()
	{
		volatile SPI_TypeDef* spi = GetDevice();
		// Clock Polarity, phase, speed
		uint32_t tmp = ((uint32_t(mode_) & 0x1) ? SPI_CR1_CPHA : 0)
			| ((uint32_t(mode_) & 0x2) ? SPI_CR1_CPOL : 0)
			| kCr1Speed_ << SPI_CR1_BR_Pos
			;
		// Frame format
		if (FORMAT == SpiFormat::k8bitLsb || FORMAT == SpiFormat::k16bitLsb)
			tmp |= SPI_CR1_LSBFIRST;
#ifdef SPI_CR1_DFF
		if (FORMAT == SpiFormat::k16bitMsb || FORMAT == SpiFormat::k16bitLsb)
			tmp |= SPI_CR1_DFF;
#endif
		// Master slave
		switch (OPERATION)
		{
		case SpiRole::kMaster:
			tmp |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
			break;
		case SpiRole::kMultiMaster:
			tmp |= SPI_CR1_MSTR;
			break;
		case SpiRole::kSlaveSW:
			tmp |= SPI_CR1_SSM | SPI_CR1_SSI;
			break;
		case SpiRole::kSlaveHW:
			break;
		}
		// TODO: Duplex mode (docs are like spaghetti in this topic!!!)
		switch (DUPLEX)
		{
		case SpiBiDi::kFullDuplex:
			break;
		case SpiBiDi::kReceiveOnly:
			tmp |= SPI_CR1_RXONLY;
			break;
		case SpiBiDi::kHalfDuplexOut:
			tmp |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
			break;
		case SpiBiDi::kHalfDuplexIn:
			tmp |= SPI_CR1_BIDIMODE;
			break;
		}
		uint32_t old = spi->CR1;
		spi->CR1 = tmp;
		return (RawSpiSpeed)old;
	}

	ALWAYS_INLINE static void RestoreSpeed(RawSpiSpeed raw)
	{
		GetDevice()->CR1 = (uint32_t)raw;
	}

	/// Enables the SPI device
	ALWAYS_INLINE static void Enable()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR1 |= SPI_CR1_SPE;
	}

	/// Disables the SPI device
	static void Disable()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		while ((spi->SR & SPI_SR_BSY) != 0)
			;
		spi->CR1 &= ~SPI_CR1_SPE;
	}

	/// Disables the SPI device, removing pending bytes
	static void DisableSafe() NO_INLINE
	{
		volatile SPI_TypeDef*spi = GetDevice();
		if (FORMAT == SpiFormat::k8bitMsb || FORMAT == SpiFormat::k8bitLsb)
			ReadChar();
		else
			ReadWord();
		while ((spi->SR & SPI_SR_TXE) == 0)
			;
		while ((spi->SR & SPI_SR_BSY) != 0)
			;
		spi->CR1 &= ~SPI_CR1_SPE;
	}

	/// Stops the device, turning clock off
	ALWAYS_INLINE static void Stop()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		Disable();
		switch (SPI_N)
		{
		case Spi::k1:
			RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
			break;
#ifdef SPI2_BASE
		case Spi::k2:
			RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
			break;
#endif
#if defined(RCC_APB1ENR_SPI3EN)
		case Spi::k3:
			RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;
			break;
#elif defined(RCC_APB1ENR1_SPI3EN)
		case Spi::k3:
			RCC->APB1ENR1 &= ~RCC_APB1ENR1_SPI3EN;
			break;
#endif
		}
	}

	/// Activates all IRQs for device
	ALWAYS_INLINE static void EnableIrq()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE;
	}

	/// Activates RXNE IRQ for device
	ALWAYS_INLINE static void EnableRxIrq()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXNEIE;
	}

	/// Activates RXNE+ERR IRQs for device
	ALWAYS_INLINE static void EnableRxErrIrq()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXNEIE | SPI_CR2_ERRIE;
	}

	/// Disables all IRQs from device
	ALWAYS_INLINE static void DisableIrq()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 &= ~(SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE);
	}

	/// Enables both receive and transmit DMA
	ALWAYS_INLINE static void EnableDma()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
	}

	/// Enables RX DMA
	ALWAYS_INLINE static void EnableRxDma()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXDMAEN;
	}

	/// Enables TX DMA
	ALWAYS_INLINE static void EnableTxDma()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 |= SPI_CR2_TXDMAEN;
	}

	/// Disables all DMA
	ALWAYS_INLINE static void DisableDma()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		spi->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	}

	/// Checks if busy bit is active
	ALWAYS_INLINE static bool IsBusy(void)
	{
		volatile SPI_TypeDef*spi = GetDevice();
		return spi->SR & SPI_SR_BSY;
	}

	/// Writes a byte to the device
	ALWAYS_INLINE static void WriteChar(uint8_t ch)
	{
		volatile SPI_TypeDef*spi = GetDevice();
		while (!(spi->SR & SPI_SR_TXE))
			;
		spi->DR = ch;
	}

	/// Writes a half-word to the device
	ALWAYS_INLINE static void WriteWord(uint16_t w)
	{
		volatile SPI_TypeDef*spi = GetDevice();
		while (!(spi->SR & SPI_SR_TXE))
			;
		spi->DR = w;
	}

	/// Reads a byte from the device
	ALWAYS_INLINE static uint8_t ReadChar()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		while (!(spi->SR & SPI_SR_RXNE))
			;
		return spi->DR;
	}

	/// Writes a half-word from the device
	ALWAYS_INLINE static uint16_t ReadWord()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		while (!(spi->SR & SPI_SR_RXNE))
			;
		return spi->DR;
	}

	/// Reads status register
	ALWAYS_INLINE static uint8_t ReadStatus()
	{
		volatile SPI_TypeDef*spi = GetDevice();
		return spi->SR;
	}

	/// Sends and receives a byte on the SPI bus
	static uint8_t PutChar(uint8_t ch) NO_INLINE
	{
		WriteChar(ch);
		return ReadChar();
	}

	/// Sends and receive a half-word on the SPI bus
	static uint16_t PutWord(uint16_t w) NO_INLINE
	{
		WriteWord(w);
		return ReadWord();
	}

	/// Sends and receive a data stream on the SPI bus
	static void PutStream(const void* src_, uint32_t cnt) NO_INLINE OPTIMIZED
	{
		const uint8_t* src = (const uint8_t*)src_;
		volatile SPI_TypeDef* spi = GetDevice();
		while (cnt)
		{
			if (cnt != 0 && (spi->SR & SPI_SR_TXE))
			{
				spi->DR = *src++;
				--cnt;
			}
			if (spi->SR & SPI_SR_RXNE)
			{
				uint8_t tmp = spi->DR;
			}
		}
	}

	/// Sends and receive a data stream on the SPI bus
	static void PutStream(const void *src_, void *dest_, uint32_t cnt) NO_INLINE OPTIMIZED
	{
		const uint8_t *src = (const uint8_t *)src_;
		uint8_t *dest = (uint8_t *)dest_;
		volatile SPI_TypeDef*spi = GetDevice();
		uint32_t cnt2 = cnt;
		while (cnt2)
		{
			if (cnt != 0 && (spi->SR & SPI_SR_TXE))
			{
				spi->DR = *src++;
				--cnt;
			}
			if (spi->SR & SPI_SR_RXNE)
			{
				*dest++ = spi->DR;
				--cnt2;
			}
		}
	}

	//! Writes a constant byte repeatedly. Input data is ignored
	static void Repeat(uint8_t byte, uint32_t cnt) NO_INLINE OPTIMIZED
	{
		volatile SPI_TypeDef*spi = GetDevice();
		uint32_t cnt2 = cnt;
		while (cnt2)
		{
			if (cnt != 0 && (spi->SR & SPI_SR_TXE))
			{
				spi->DR = byte;
				--cnt;
			}
			if (spi->SR & SPI_SR_RXNE)
			{
				uint8_t unused = spi->DR;
				--cnt2;
			}
		}
	}
};


}	// namespace Bmt
