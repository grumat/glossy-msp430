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
		// Enable clock
		volatile SPI_TypeDef*spi = GetDevice();
		volatile uint32_t delay;	// inserts delay instructions
		switch (SPI_N)
		{
		case kSpi1:
			RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
			delay = RCC->APB2ENR & RCC_APB2ENR_SPI1EN;
			break;
		case kSpi2:
			RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
			delay = RCC->APB2ENR & RCC_APB1ENR_SPI2EN;
			break;
#ifdef SPI3_BASE
		case kSpi3:
			RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
			delay = RCC->APB2ENR & RCC_APB1ENR_SPI3EN;
			break;
#endif
		}
		// Clock Polarity, phase, speed
		uint32_t tmp = ((MODE & 0x1) ? SPI_CR1_CPHA : 0)
			| ((MODE & 0x2) ? SPI_CR1_CPOL : 0)
			| kCr1Speed_ << SPI_CR1_BR_Pos
			;
		// Frame format
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

		// Register CR2
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
		volatile SPI_TypeDef*spi = GetDevice();
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

