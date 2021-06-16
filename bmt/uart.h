#pragma once

#include "fifo.h"
#include "mcu-system.h"
#include "critical_section.h"
#include "tasks.h"


enum UsartInstance
{
	kUsart1 = 0
	, kUsart2
	, kUsart3
#ifdef RCC_APB1ENR_UART4EN
	, kUsart4
	, kUsart5
#endif
};

enum ParitySel
{
	kParityNone
	, kParityEven
	, kParityOdd,
};

enum StopBits
{
	kStop0_5
	, kStop1
	, kStop1_5
	, kStop2
};


//! Defines static settings to setup an UART peripheral
template<
	const UsartInstance uart_n
	, typename Clock
	, const int baud
	, const int wordlen = 8
	, const ParitySel parity = kParityNone
	, const StopBits stopbits = kStop1
>
class UsartTemplate
{
public:
	static constexpr UsartInstance kUsartInstance_ = uart_n;
	static constexpr uintptr_t kUsartBase_ = 
		(kUsartInstance_ == kUsart1) ? USART1_BASE
		: (kUsartInstance_ == kUsart2) ? USART2_BASE
		: (kUsartInstance_ == kUsart3) ? USART3_BASE
#ifdef USART4_BASE
		: (kUsartInstance_ == kUsart4) ? USART4_BASE
		: (kUsartInstance_ == kUsart5) ? USART5_BASE
#endif
		: 0;
	static constexpr uint32_t kRccUsartFlag_ =
		(kUsartInstance_ == kUsart1) ? RCC_APB2ENR_USART1EN
		: (kUsartInstance_ == kUsart2) ? RCC_APB1ENR_USART2EN
		: (kUsartInstance_ == kUsart3) ? RCC_APB1ENR_USART3EN
#ifdef USART4_BASE
		: (kUsartInstance_ == kUsart4) ? RCC_APB1ENR_USART4EN
		: (kUsartInstance_ == kUsart5) ? RCC_APB1ENR_USART5EN
#endif
		: 0;
	static constexpr IRQn_Type kNvicUsartIrqn_ =
		(kUsartInstance_ == kUsart1) ? USART1_IRQn
		: (kUsartInstance_ == kUsart2) ? USART2_IRQn
		: (kUsartInstance_ == kUsart3) ? USART3_IRQn
#ifdef USART4_BASE
		: (kUsartInstance_ == kUsart4) ? USART4_IRQn
		: (kUsartInstance_ == kUsart5) ? USART5_IRQn
#endif
		: USART1_IRQn;
	static constexpr int kBaud_ = baud;
	static constexpr int kWordLen_ = wordlen;
	static constexpr ParitySel kParity_ = parity;
	static constexpr StopBits kStopBits_ = wordlen == 7	// 7-bit frames needs emulation
		? (stopbits == kStop2 || kUsartInstance_ > kUsart3)
			? kStop1
			: kStop0_5
		: stopbits;

	typedef IrqTemplate<kNvicUsartIrqn_> UartIrq;
	typedef CriticalSectionIrq<UartIrq> IrqLock;
	
	ALWAYS_INLINE static USART_TypeDef &Io()	{ return *(USART_TypeDef*)kUsartBase_; }

	ALWAYS_INLINE static void Init(void)
	{
		static_assert(kUsartBase_ != 0, "Invalid USART device");
		// Size of word is 7, 8 or 9
		static_assert(kWordLen_ >= 7 && kWordLen_ <= 9, "Hardware does not support the desired bit size.");
		// USART4 & USART5 does not support 0.5 or 1.5 stop bits
		static_assert(kUsartInstance_ <= kUsart3 || (kStopBits_ != kStop0_5 && kStopBits_ != kStop1_5, "Hardware does not support the selected combination"));
		
		USART_TypeDef &uart = Io();
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
		NVIC_EnableIRQ(kNvicUsartIrqn_);
		NVIC_ClearPendingIRQ(kNvicUsartIrqn_);
		EnableRxIrq();
	}

	ALWAYS_INLINE static void Enable(void)
	{
		(kUsartInstance_ == kUsart1 ? RCC->APB2ENR : RCC->APB1ENR) |= kRccUsartFlag_;
	}

	ALWAYS_INLINE static void Disable(void)
	{
		(kUsartInstance_ == kUsart1 ? RCC->APB2ENR : RCC->APB1ENR) &= ~kRccUsartFlag_;
	}

	ALWAYS_INLINE static void EnableTxIrq(void)
	{
		Io().CR1 |= USART_CR1_TXEIE;
	}

	ALWAYS_INLINE static void DisableTxIrq(void)
	{
		Io().CR1 &= ~USART_CR1_TXEIE;
	}

	ALWAYS_INLINE static bool TxComplete(void)
	{
		return Io().SR & USART_SR_TC;
	}

	ALWAYS_INLINE static void EnableRxIrq(void)
	{
		Io().CR1 |= USART_CR1_RXNEIE;
	}

	ALWAYS_INLINE static void DisableRxIrq(void)
	{
		Io().CR1 &= ~USART_CR1_RXNEIE;
	}

	ALWAYS_INLINE static void ClearRxIrq(void)
	{
		Io().SR &= ~USART_SR_RXNE;
	}
	
	ALWAYS_INLINE static uint32_t GetCommStatus()
	{
		return Io().SR;
	}
	
	ALWAYS_INLINE static uint16_t GetByte(void)
	{
		// 7-bit frames needs emulation
		if (kWordLen_ != 7)
			return (uint16_t)Io().DR;
		else
		switch (kWordLen_)
		{
		case 7:
			// 7-bit frames needs emulation
			return ((uint8_t &)Io().DR & 0x7f);
		case 8:
			return (uint8_t &)Io().DR;
		case 9:
			return Io().DR;
		}
	}
	
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


template<
	typename UsartHwInstance
	, const int buf_in = 64
	, const int buf_out = 64
	, typename OnPutBufferFull = DummyEvent
>
class UartFifo
{
public:
	typedef UsartHwInstance HwInstance;
	static inline Fifo<buf_in> m_BufIn;
	static inline Fifo<buf_out> m_BufOut;

// Interface to the interrupt driver
public:
	ALWAYS_INLINE void Init(void)
	{
		m_BufIn.Reset();
		m_BufOut.Reset();
	}
	
	ALWAYS_INLINE void OnXmitChar()
	{
		int ch = m_BufOut.Get();
		if (ch < 0)
			UsartHwInstance::DisableTxIrq();
		else
			UsartHwInstance::PutByte((uint8_t)ch);
	}
	
	ALWAYS_INLINE bool OnCharReceived()
	{
		if (!m_BufIn.Put((char)UsartHwInstance::GetByte()))
		{
			// Buffer is full
			OnPutBufferFull();
		}
		UsartHwInstance::ClearRxIrq();
		return true;
	}
	
	ALWAYS_INLINE bool OnCts() { return true; }
			
	ALWAYS_INLINE bool OnBreak() { return true; }
			
	ALWAYS_INLINE bool OnParityError() { return true; }
			
	ALWAYS_INLINE bool OnFramingError() { return true; }
			
	ALWAYS_INLINE bool OnOverrun() { return true; }
			
	ALWAYS_INLINE bool OnIdle() { return true; }
			
	ALWAYS_INLINE bool OnNoise() { return true; }

public:
	void PutChar(char data) NO_INLINE
	{
		if (m_BufOut.IsFull())
		{
			for (;;)
			{
				HwInstance::EnableTxIrq();
				if (!m_BufOut.IsFull())
					break;
				McuCore::Sleep();
			}
		}
		typename HwInstance::IrqLock lock;
		HwInstance::EnableTxIrq();
		m_BufOut.Put(data);
	}

	int GetChar() NO_INLINE
	{
		typename HwInstance::IrqLock lock;
		return m_BufIn.Get();
	}

	ALWAYS_INLINE int GetInCount() const
	{
		typename HwInstance::IrqLock lock;
		return m_BufIn.GetCount();
	}

	ALWAYS_INLINE int GetOutFree() const
	{
		typename HwInstance::IrqLock lock;
		return m_BufOut.GetCount();
	}
};


//! A simple model of an UART Buffer implementation. Use it as basis for an extension
template<
	class UsartIntEvents
>
class UsartIntDriverModel
{
public:	
	typedef typename UsartIntEvents::HwInstance HwInstance;
	UsartIntEvents events_;
	
	ALWAYS_INLINE void Init()
	{
		events_.Init();
		HwInstance::Init();
	}
	
	ALWAYS_INLINE void PutChar(char data) { events_.PutChar(data); }
	ALWAYS_INLINE size_t  GetOutFree() { return events_.GetOutFree(); }
	ALWAYS_INLINE int GetChar() { return events_.GetChar(); }
	ALWAYS_INLINE size_t GetInCount() { return events_.GetInCount(); }

	ALWAYS_INLINE void PutS(const char* data)
	{
		while (*data)
			PutChar(*data++);
	}

	ALWAYS_INLINE void PutBuf(const char* data, uint32_t cnt)
	{
		for (; cnt != 0; --cnt)
			PutChar(*data++);
	}

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

