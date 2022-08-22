#pragma once

#include "fifo.h"
#include "mcu-system.h"
#include "critical_section.h"
#include "tasks.h"
#include "irq.h"


enum UsartInstance
{
	kUsart1 = 0	///< USART 1 peripheral
	, kUsart2	///< USART 2 peripheral
	, kUsart3	///< USART 3 peripheral
#ifdef RCC_APB1ENR_UART4EN
	, kUsart4	///< USART 4 peripheral
	, kUsart5	///< USART 5 peripheral
#endif
};

/// Parity options
enum ParitySel
{
	kParityNone		///< No parity bit
	, kParityEven	///< Even parity bit
	, kParityOdd,	///< Odd parity bit
};

/// Stop bits options
enum StopBits
{
	kStop0_5		///< a half stop bit
	, kStop1		///< one stop bit
	, kStop1_5		///< 1.5 stop bits
	, kStop2		///< 2 stop bits
};


/// Defines static settings to setup an UART peripheral
template<
	const UsartInstance uart_n				///< UART number
	, typename Clock						///< Clock source
	, const int baud						///< Baud rate
	, const int wordlen = 8					///< Bit size
	, const ParitySel parity = kParityNone	///< Selected parity
	, const StopBits stopbits = kStop1		///< Selected stop bits
>
class UsartTemplate
{
public:
	/// The UART instance
	static constexpr UsartInstance kUsartInstance_ = uart_n;
	/// The base address of the UART hardware
	static constexpr uintptr_t kUsartBase_ = 
		(kUsartInstance_ == kUsart1) ? USART1_BASE
		: (kUsartInstance_ == kUsart2) ? USART2_BASE
		: (kUsartInstance_ == kUsart3) ? USART3_BASE
#ifdef USART4_BASE
		: (kUsartInstance_ == kUsart4) ? USART4_BASE
		: (kUsartInstance_ == kUsart5) ? USART5_BASE
#endif
		: 0;
	/// Clock register for the particular hardware
	static constexpr uint32_t kRccUsartFlag_ =
		(kUsartInstance_ == kUsart1) ? RCC_APB2ENR_USART1EN
		: (kUsartInstance_ == kUsart2) ? RCC_APB1ENR_USART2EN
		: (kUsartInstance_ == kUsart3) ? RCC_APB1ENR_USART3EN
#ifdef USART4_BASE
		: (kUsartInstance_ == kUsart4) ? RCC_APB1ENR_USART4EN
		: (kUsartInstance_ == kUsart5) ? RCC_APB1ENR_USART5EN
#endif
		: 0;
	/// Interruppt handler for the particular hardware
	static constexpr IRQn_Type kNvicUsartIrqn_ =
		(kUsartInstance_ == kUsart1) ? USART1_IRQn
		: (kUsartInstance_ == kUsart2) ? USART2_IRQn
		: (kUsartInstance_ == kUsart3) ? USART3_IRQn
#ifdef USART4_BASE
		: (kUsartInstance_ == kUsart4) ? USART4_IRQn
		: (kUsartInstance_ == kUsart5) ? USART5_IRQn
#endif
		: USART1_IRQn;
	/// Baud rate
	static constexpr int kBaud_ = baud;
	/// Bit size for the port
	static constexpr int kWordLen_ = wordlen;
	/// Parity for the port
	static constexpr ParitySel kParity_ = parity;
	/// Stop bits for the port
	static constexpr StopBits kStopBits_ = wordlen == 7	// 7-bit frames needs emulation
		? (stopbits == kStop2 || kUsartInstance_ > kUsart3)
			? kStop1
			: kStop0_5
		: stopbits;

	/// IRQ for that port
	typedef IrqTemplate<kNvicUsartIrqn_> UartIrq;
	/// A scoped critical section class data type
	typedef CriticalSectionIrq<UartIrq> IrqLock;

	/// Access to the hardware IO data structure
	ALWAYS_INLINE static volatile USART_TypeDef &Io()	{ return *(volatile USART_TypeDef*)kUsartBase_; }

	/// Initialize the object
	ALWAYS_INLINE static void Init(void)
	{
		static_assert(kUsartBase_ != 0, "Invalid USART device");
		// Size of word is 7, 8 or 9
		static_assert(kWordLen_ >= 7 && kWordLen_ <= 9, "Hardware does not support the desired bit size.");
		// USART4 & USART5 does not support 0.5 or 1.5 stop bits
		static_assert(kUsartInstance_ <= kUsart3 || (kStopBits_ != kStop0_5 && kStopBits_ != kStop1_5, "Hardware does not support the selected combination"));
		
		volatile USART_TypeDef &uart = Io();
		// INFO: Only one of these branches are generated, since kUsartInstance_ is constexpr
		if (kUsartInstance_ == kUsart1)
		{
			// Enable device
			RCC->APB2ENR |= kRccUsartFlag_;
			// Reset device
			RCC->APB2RSTR |= kRccUsartFlag_;
			RCC->APB2RSTR &= ~kRccUsartFlag_;
			// Baud rate depends on peripheral clock
			uart.BRR = Clock::kApb2Clock_ / kBaud_;
		}
		else
		{
			// Enable device
			RCC->APB1ENR |= kRccUsartFlag_;
			// Reset device
			RCC->APB1RSTR |= kRccUsartFlag_;
			RCC->APB1RSTR &= ~kRccUsartFlag_;
			// Baud rate depends on peripheral clock
			uart.BRR = Clock::kApb1Clock_ / kBaud_;
		}
		// Parity selection
		uint32_t tmp = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
		if (kParity_ != kParityNone)
			tmp |= USART_CR1_PCE;
		if(kParity_ == kParityOdd)
			tmp |= USART_CR1_PS;
		// Word length
		if (kWordLen_ == 9)
			tmp |= USART_CR1_M;
		uart.CR1 = tmp;
		// Stop bit selection
		tmp = 0;
		if (kStopBits_ == kStop0_5)
			tmp = USART_CR2_STOP_0;
		else if (kStopBits_ == kStop1_5)
			tmp = USART_CR2_STOP_1 | USART_CR2_STOP_0;
		else if (kStopBits_ == kStop2)
			tmp = USART_CR2_STOP_1;
		uart.CR2 = tmp;
		// Enable interrupts
		UartIrq::ClearPending();
		UartIrq::Enable();
		// Enable the IRQ
		EnableRxIrq();
	}

	/// Enable the peripheral by activating clock
	ALWAYS_INLINE static void Enable(void)
	{
		(kUsartInstance_ == kUsart1 ? RCC->APB2ENR : RCC->APB1ENR) |= kRccUsartFlag_;
		volatile uint32_t delay = (kUsartInstance_ == kUsart1 ? RCC->APB2ENR : RCC->APB1ENR) & kRccUsartFlag_;
	}

	/// Turns clock off, disabling the peripheral
	ALWAYS_INLINE static void Disable(void)
	{
		(kUsartInstance_ == kUsart1 ? RCC->APB2ENR : RCC->APB1ENR) &= ~kRccUsartFlag_;
	}

	/// Enables the TX interrupt
	ALWAYS_INLINE static void EnableTxIrq(void)
	{
		Io().CR1 |= USART_CR1_TXEIE;
	}

	/// Disables the TX interrupt
	ALWAYS_INLINE static void DisableTxIrq(void)
	{
		Io().CR1 &= ~USART_CR1_TXEIE;
	}

	/// Returns the transmit complete flag
	ALWAYS_INLINE static bool TxComplete(void)
	{
		return Io().SR & USART_SR_TC;
	}

	/// Enables the RX interrupt
	ALWAYS_INLINE static void EnableRxIrq(void)
	{
		Io().CR1 |= USART_CR1_RXNEIE;
	}

	/// Disables the RX interrupt
	ALWAYS_INLINE static void DisableRxIrq(void)
	{
		Io().CR1 &= ~USART_CR1_RXNEIE;
	}

	/// Clear RX interrupt flag, reenabling the input
	ALWAYS_INLINE static void ClearRxIrq(void)
	{
		Io().SR &= ~USART_SR_RXNE;
	}
	
	/// Returs error flags
	ALWAYS_INLINE static uint32_t GetCommStatus()
	{
		return Io().SR;
	}
	
	ALWAYS_INLINE static uint16_t GetByte(void)
	{
		switch (kWordLen_)
		{
		case 7:
			// 7-bit frames needs emulation
			return ((uint8_t &)Io().DR & 0x7f);
		case 8:
			// Read register as 8 bit
			return (uint8_t &)Io().DR;
		case 9:
			// Read a 16-bit register
			return Io().DR;
		}
	}

	/// Puts data to the peripheral
	ALWAYS_INLINE static void PutByte(uint16_t ch)
	{
		switch (kWordLen_)
		{
		case 7:
			// 7-bit frames needs emulation
			Io().DR = (uint8_t)(ch | 0x80);
			break;
		case 8:
			Io().DR = (uint8_t)ch;
			break;
		case 9:
			Io().DR = ch;
			break;
		}
	}
};


//! A dummy class to handle special events
class DummyEvent
{
public:
	ALWAYS_INLINE DummyEvent(uintptr_t data = 0) {}
};


/// A FIFO implementation for the UART
template<
	typename UsartHwInstance
	, const int buf_in = 64
	, const int buf_out = 64
	, typename OnPutBufferFull = DummyEvent
>
class UartFifo
{
public:
	/// Datatype for the hardware instance
	typedef UsartHwInstance HwInstance;
	typedef typename UsartHwInstance::UartIrq UsartIrqLock;
	/// Input buffer with specified size
	static inline Fifo<buf_in> m_BufIn;
	/// Output buffer with specified size
	static inline Fifo<buf_out> m_BufOut;

// Interface to the interrupt driver
public:
	/// Initialize object
	ALWAYS_INLINE void Init(void)
	{
		m_BufIn.Reset();
		m_BufOut.Reset();
	}

	/// Part of interrupt handler to transmit a char
	ALWAYS_INLINE void OnXmitChar()
	{
		// Take the char
		int ch = m_BufOut.Get();
		// Disable transmit interrupt if no more chars available
		if (ch < 0)
			UsartHwInstance::DisableTxIrq();
		else
			UsartHwInstance::PutByte((uint8_t)ch);	// put char into hw reg
	}

	/// Part of the interrupt handler to receive a char
	ALWAYS_INLINE bool OnCharReceived()
	{
		// Take char from HW reg and enqueue
		if (!m_BufIn.Put((char)UsartHwInstance::GetByte()))
		{
			// Buffer is full (char is lost!)
			OnPutBufferFull((uintptr_t)this);
		}
		// Rearm hardware for next byte
		//UsartHwInstance::ClearRxIrq();
		return true;
	}
	/// No handling for CTS signal	
	ALWAYS_INLINE bool OnCts() { return true; }
	/// No handling for Break signal	
	ALWAYS_INLINE bool OnBreak() { return true; }
	/// No handling for Parity Error	
	ALWAYS_INLINE bool OnParityError() { return true; }
	/// No handling for Framing error	
	ALWAYS_INLINE bool OnFramingError() { return true; }
	/// No handling for Overrun error	
	ALWAYS_INLINE bool OnOverrun() { return true; }
	/// No handling for Idle signal	
	ALWAYS_INLINE bool OnIdle() { return true; }
	/// No handling for Noise error	
	ALWAYS_INLINE bool OnNoise() { return true; }

public:
	/// Puts a char into the output FIFO, while xmit interrupt pull them out
	void PutChar(char data) NO_INLINE
	{
		// Not enough space?
		while (m_BufOut.IsFull())
		{
			// Ensures that TX interrupt is working
			HwInstance::EnableTxIrq();
			// Wait until interrupt routine wakes us again
			McuCore::Sleep();
		}
		// Locks NVIC flag
		CriticalSectionIrq<UsartIrqLock> lock;
		// Enqueue byte
		m_BufOut.Put(data);
		// Make sure peripheral interrupt flag is active again (see OnXmitChar())s
		HwInstance::EnableTxIrq();
		// At exit IRQ interrupt will be released and byte will be serviced
	}

	//! Removes the current pending char from the input queue
	int GetChar() NO_INLINE
	{
		// Temporarily freeze UART interrupts
		CriticalSectionIrq<UsartIrqLock> lock;
		// Take char or -1 if empty
		return m_BufIn.Get();
	}

	//! Bytes waiting to be read
	ALWAYS_INLINE int GetInCount() const
	{
		CriticalSectionIrq<UsartIrqLock> lock;
		return m_BufIn.GetCount();
	}

	//! Space left in transmit queue
	ALWAYS_INLINE int GetOutFree() const
	{
		CriticalSectionIrq<UsartIrqLock> lock;
		return m_BufOut.GetCount();
	}
};


//! A simple model of an UART Buffer implementation. Use it as basis for an extension
/*!
The example declares a MCU running at 72 MHz clock using the PLL and USART1 running 
at 115200 bauds:

\code{.cpp}
// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
// Set the clock tree
typedef SysClkTemplate<PLL, 1, 2, 1> SysClk;
// USART1 for GDB port
typedef UsartTemplate<kUsart1, SysClk, 115200> MyUsartSettings;
// Dual FIFO buffers for the USART1
typedef UartFifo<MyUsartSettings, 64, 64> MyUsartWithBuffers;
// A driver model using interrupts
typedef UsartIntDriverModel<MyUsartWithBuffers> UsartDriver;
// A singleton exists on the implementation file as UART instance
extern UsartDriver g_UartSingleton;
\endcode
*/
template<
	class UsartIntEvents	// takes an UartFifo<...> instance
>
class UsartIntDriverModel
{
public:	
	//! The hardware instance
	typedef typename UsartIntEvents::HwInstance HwInstance;
	//! UART buffer and interrupt events handler
	UsartIntEvents events_;

	//! Initialize instance
	ALWAYS_INLINE void Init()
	{
		events_.Init();
		HwInstance::Init();
	}

	/// Puts a char into the queue
	ALWAYS_INLINE void PutChar(char data) { events_.PutChar(data); }
	/// Space left on the output queue
	ALWAYS_INLINE size_t  GetOutFree() { return events_.GetOutFree(); }
	/// Takes a byte from the input queue
	ALWAYS_INLINE int GetChar() { return events_.GetChar(); }
	/// Bytes available on the input queue
	ALWAYS_INLINE size_t GetInCount() { return events_.GetInCount(); }

	/// String put into UART device
	ALWAYS_INLINE void PutS(const char* data)
	{
		while (*data)
			PutChar(*data++);
	}

	/// Buffer array to put into UART device
	ALWAYS_INLINE void PutBuf(const char* data, uint32_t cnt)
	{
		for (; cnt != 0; --cnt)
			PutChar(*data++);
	}

	/// Handles IRQ interrupts
	ALWAYS_INLINE void HandleIrq()
	{
		uint32_t status = HwInstance::GetCommStatus();
		bool xmit = true;
		// Handle events (unused events are optimized out by the compiler)
		if((status & USART_SR_PE))
			xmit &= events_.OnParityError();
		if ((status & USART_SR_FE))
			xmit &= events_.OnFramingError();
		if ((status & USART_SR_NE))
			xmit &= events_.OnNoise();
		if ((status & USART_SR_ORE))
			xmit &= events_.OnOverrun();
		if ((status & USART_SR_IDLE))
			xmit &= events_.OnIdle();
		if ((status & USART_SR_LBD))
			xmit &= events_.OnBreak();
		if ((status & USART_SR_CTS))
			xmit &= events_.OnCts();
		// Receive chars
		if(status & USART_SR_RXNE)
			xmit &= events_.OnCharReceived();
		// Transmit bytes (if allowed by upper logic)
		if(xmit && (status & USART_SR_TXE))
			events_.OnXmitChar();
	}
};

