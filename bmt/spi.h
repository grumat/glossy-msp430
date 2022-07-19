#pragma once

#include "dma.h"
#include "irq.h"


/// Instance of the SPI peripheral
enum SpiInstance
{
	kSpi1 = 0		///< SPI1 peripheral
	, kSpi2			///< SPI2 peripheral
#ifdef SPI3_BASE
	, kSpi3			///< SPI3 peripheral
#endif
};


/// SPI polarity/phase mode
enum SpiMode
{
	kSpiMode0		///< CPOL: 0, CPHA: 0
	, kSpiMode1		///< CPOL: 0, CPHA: 1
	, kSpiMode2		///< CPOL: 1, CPHA: 0
	, kSpiMode3		///< CPOL: 1, CPHA: 1
};


/// SPI operation mode
enum SpiOperation
{
	kSpiMaster			///< Master mode
	, kSpiMultiMaster	///< MultiMaster mode
	, kSpiSlaveSW		///< Slave, flow controlled by software
	, kSpiSlaveHW		///< Slave, flow controlled by hardware NSS pin
};


/// SPI bidirectional configuration
enum SpiBiDi
{
	kSpiFullDuplex		///< Normal mode (clk + 2 data lines)
	, kSpiReceiveOnly	///< 2 lines; output disable
	, kSpiHalfDuplexOut	///< Half duplex mode (clk and out lines)
	, kSpiHalfDuplexIn	///< Half duplex mode (clk and in lines)
};


/// Format of the SPI frame
enum SpiFormat
{
	kSpi8bitMsb			///< 8-bit data frame MSB first
	, kSpi16bitMsb		///< 16-bit data frame MSB first
	, kSpi8bitLsb		///< 8-bit data frame LSB first
	, kSpi16bitLsb		///< 16-bit data frame LSB first
};


/// A template class for an SPI peripheral configuration
template<
	const SpiInstance SPI_N			///< The SPI instance
	, typename CLOCK				///< Clock data type driving the system
	, const int SPEED = 1000000		///< The desired speed (effective speed is the truncated value)
	, const SpiOperation OPERATION = kSpiMaster	///< Mode of operation
	, const SpiMode MODE = kSpiMode3		///< Polarity and phase
	, const SpiFormat FORMAT = kSpi8bitMsb	///< Frame format
	, const bool USE_IRQ = true				///< Configure IRQ for the peripheral
	, const SpiBiDi DUPLEX = kSpiFullDuplex ///< SPI lines direction
>
struct SpiTemplate
{
	/// The peripheral base address as a constant
	static constexpr uintptr_t kSpiBase_ =
		(SPI_N == kSpi1) ? SPI1_BASE
		: (SPI_N == kSpi2) ? SPI2_BASE
#ifdef SPI3_BASE
		: (SPI_N == kSpi3) ? SPI3_BASE
#endif
		: 0 ;
	/// This constant refers to the ABP that owns the peripheral
	static constexpr uint32_t kInputClock_ =
		(SPI_N == kSpi1) ? CLOCK::kApb2Clock_
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
		(SPI_N == kSpi2) ? SPI2_IRQn
#ifdef SPI3_BASE
		: (SPI_N == kSpi3) ? SPI3_IRQn
#endif
		: SPI1_IRQn
		;
	/// A data-type to control IRQ settings
	typedef IrqTemplate<kNvicSpiIrqn_> SpiIrq;

	/// The DMA instance for the peripheral
	static constexpr DmaInstance DmaInstance_ = 
#ifdef SPI3_BASE
		(SPI_N == kSpi3) ? kDma2
#endif
		kDma1
		;
	/// The DMA channel Transmit instance for the peripheral
	static constexpr DmaCh DmaTxCh_ =
		(SPI_N == kSpi1) ? kDmaCh3
#ifdef SPI3_BASE
		: (SPI_N == kSpi3) ? kDmaCh2
#endif
		: kDmaCh5;
	/// The DMA channel Receive instance for the peripheral
	static constexpr DmaCh DmaRxCh_ =
		(SPI_N == kSpi1) ? kDmaCh2
#ifdef SPI3_BASE
		: (SPI_N == kSpi3) ? kDmaCh1
#endif
		: kDmaCh4;

	/// Returns peripheral register structure
	ALWAYS_INLINE static SPI_TypeDef *GetDevice() { return (SPI_TypeDef *)kSpiBase_; }

	/// Initialize hardware configuration (including RCC)
	ALWAYS_INLINE static void Init()
	{
		// Invalid SPI device selected
		static_assert(kSpiBase_ != 0, "An invalid SPI device was selected");

		SPI_TypeDef *spi = GetDevice();
		// Conditional compilation for specific peripheral
		switch (SPI_N)
		{
		case kSpi1:
			RCC->APB2RSTR |= RCC_APB2ENR_SPI1EN;
			RCC->APB2RSTR &= ~RCC_APB2ENR_SPI1EN;
			if (USE_IRQ)
			{
				SpiIrq::ClearPending();
				SpiIrq::Enable();
			}
			break;
		case kSpi2:
			RCC->APB1RSTR |= RCC_APB1ENR_SPI2EN;
			RCC->APB1RSTR &= ~RCC_APB1ENR_SPI2EN;
			if (USE_IRQ)
			{
				SpiIrq::ClearPending();
				SpiIrq::Enable();
			}
			break;
#ifdef SPI3_BASE
		case kSpi3:
			RCC->APB1RSTR |= RCC_APB1ENR_SPI3EN;
			RCC->APB1RSTR &= ~RCC_APB1ENR_SPI3EN;
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
		// Enable
		SPI_TypeDef *spi = GetDevice();
		switch (SPI_N)
		{
		case kSpi1:
			RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
			break;
		case kSpi2:
			RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
			break;
#ifdef SPI3_BASE
		case kSpi3:
			RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
			break;
#endif
		}

		uint32_t tmp = ((MODE & 0x1) ? SPI_CR1_CPHA : 0)
			| ((MODE & 0x2) ? SPI_CR1_CPOL : 0)
			| kCr1Speed_ << SPI_CR1_BR_Pos
			;
		if (FORMAT == kSpi8bitLsb || FORMAT == kSpi16bitLsb)
			tmp |= SPI_CR1_LSBFIRST;
		if (FORMAT == kSpi16bitMsb || FORMAT == kSpi16bitLsb)
			tmp |= SPI_CR1_DFF;
		// Master slave
		switch (OPERATION)
		{
		case kSpiMaster:
			tmp |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
			break;
		case kSpiMultiMaster:
			tmp |= SPI_CR1_MSTR;
			break;
		case kSpiSlaveSW:
			tmp |= SPI_CR1_SSM | SPI_CR1_SSI;
			break;
		case kSpiSlaveHW:
			break;
		}
		// TODO: Duplex mode (docs are like spaghetti in this topic!!!)
		switch (DUPLEX)
		{
		case kSpiFullDuplex:
			break;
		case kSpiReceiveOnly:
			tmp |= SPI_CR1_RXONLY;
			break;
		case kSpiHalfDuplexOut:
			tmp |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
			break;
		case kSpiHalfDuplexIn:
			tmp |= SPI_CR1_BIDIMODE;
			break;
		}
		spi->CR1 = tmp;

		tmp = 0;
		switch (OPERATION)
		{
		case kSpiMaster:
			tmp |= SPI_CR2_SSOE;
			break;
		case kSpiMultiMaster:
			break;
		case kSpiSlaveSW:
			break;
		case kSpiSlaveHW:
			break;
		}
		spi->CR2 = tmp;

		if (spi->SR & SPI_SR_MODF)
			spi->CR1 = spi->CR1;
		spi->CR1 |= SPI_CR1_SPE;
	}

	/// Enables the SPI device
	ALWAYS_INLINE static void Enable()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR1 |= SPI_CR1_SPE;
	}

	/// Disables the SPI device
	static void Disable()
	{
		SPI_TypeDef *spi = GetDevice();
		while ((spi->SR & SPI_SR_BSY) != 0)
			;
		spi->CR1 &= ~SPI_CR1_SPE;
	}

	/// Disables the SPI device, removing pending bytes
	static void DisableSafe() NO_INLINE
	{
		SPI_TypeDef *spi = GetDevice();
		if (FORMAT == kSpi8bitMsb || FORMAT == kSpi8bitLsb)
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
		SPI_TypeDef *spi = GetDevice();
		Disable();
		switch (SPI_N)
		{
		case kSpi1:
			RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
			break;
		case kSpi2:
			RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
			break;
#ifdef SPI3_BASE
		case kSpi3:
			RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;
			break;
#endif
		}
	}

	/// Activates IRQ for device
	ALWAYS_INLINE static void EnableIrq()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE;
	}

	ALWAYS_INLINE static void EnableRxIrq()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXNEIE;
	}

	ALWAYS_INLINE static void EnableRxErrIrq()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXNEIE | SPI_CR2_ERRIE;
	}

	ALWAYS_INLINE static void DisableIrq()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 &= ~(SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE);
	}

	ALWAYS_INLINE static void EnableDma()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
	}

	ALWAYS_INLINE static void EnableRxDma()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 |= SPI_CR2_RXDMAEN;
	}

	ALWAYS_INLINE static void EnableTxDma()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 |= SPI_CR2_TXDMAEN;
	}

	ALWAYS_INLINE static void DisableDma()
	{
		SPI_TypeDef *spi = GetDevice();
		spi->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	}

	ALWAYS_INLINE static bool IsBusy(void)
	{
		SPI_TypeDef *spi = GetDevice();
		return spi->SR & SPI_SR_BSY;
	}

	ALWAYS_INLINE static void WriteChar(uint8_t ch)
	{
		SPI_TypeDef *spi = GetDevice();
		while (!(spi->SR & SPI_SR_TXE))
			;
		spi->DR = ch;
	}

	ALWAYS_INLINE static void WriteWord(uint16_t w)
	{
		SPI_TypeDef *spi = GetDevice();
		while (!(spi->SR & SPI_SR_TXE))
			;
		spi->DR = w;
	}

	ALWAYS_INLINE static uint8_t ReadChar()
	{
		SPI_TypeDef *spi = GetDevice();
		while (!(spi->SR & SPI_SR_RXNE))
			;
		return spi->DR;
	}

	ALWAYS_INLINE static uint16_t ReadWord()
	{
		SPI_TypeDef *spi = GetDevice();
		while (!(spi->SR & SPI_SR_RXNE))
			;
		return spi->DR;
	}

	ALWAYS_INLINE static uint8_t ReadStatus()
	{
		SPI_TypeDef *spi = GetDevice();
		return spi->SR;
	}

	static uint8_t PutChar(uint8_t ch) NO_INLINE
	{
		WriteChar(ch);
		return ReadChar();
	}

	static uint16_t PutWord(uint16_t w) NO_INLINE
	{
		WriteWord(w);
		return ReadWord();
	}

	static void PutStream(const void *src_, void *dest_, uint32_t cnt) NO_INLINE OPTIMIZED
	{
		const uint8_t *src = (const uint8_t *)src_;
		uint8_t *dest = (uint8_t *)dest_;
		SPI_TypeDef *spi = GetDevice();
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

	//! Writes data repeatedly
	static void Repeat(uint8_t byte, uint32_t cnt) NO_INLINE OPTIMIZED
	{
		SPI_TypeDef *spi = GetDevice();
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


