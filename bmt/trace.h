#pragma once

#include "clocks.h"


// General trace output
void trace_s_(int channel, const char* msg);


enum SwoProtocol
{
	kManchester = 1
	, kAsynchronous = 2
};


template <const int8_t channel>
class SwoChannel
{
public:
	//! The channel number
	static constexpr int8_t kChannel_ = channel;
	static constexpr bool kEnabled_ = channel >= 0;
	//! The bitmap for the channel
	static constexpr uint32_t kChannelBit_ = kEnabled_ ? (1 << kChannel_) : 0;

	static inline uint32_t word_;
	static inline uint32_t shift_;

	// returns true if tracing is enabled for this channel
	ALWAYS_INLINE static bool IsTracing()
	{
		static_assert(kChannel_ >= -1 && kChannel_ < 32, "Invalid channel number.");
		// Const constraint
		if (kEnabled_ == false)
			return false;
		return ((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL)	// globally enabled?
			&& ((ITM->TER & kChannelBit_) != 0UL);		// channel enabled?
	}
	// Initializes or resets the object state
	ALWAYS_INLINE static void Init() { word_ = 0; shift_ = 0; }
	// Sends a char to the Trace unit
	static void PutChar(char ch) NO_INLINE
	{
		// Skip if not tracing
		if(IsTracing())
		{
			word_ |= (uint32_t)ch << shift_;
			shift_ += 8;
			if (shift_ == 32)
			{
				while (ITM->PORT[kChannel_].u32 == 0UL)
					__NOP();
				ITM->PORT[kChannel_].u32 = word_;
				Init();
			}
		}
	}
	// Sends a string to the trace unit
	static void PutS(const char* msg) NO_INLINE
	{
		if(IsTracing())
		{
			while (*msg)
				PutChar(*msg++);
		}
	}
	// Flushes pending chars
	static void Flush() NO_INLINE
	{
		if (IsTracing())
		{
			// Pending chars?
			if (shift_ > 0)
			{
				while (ITM->PORT[kChannel_].u32 == 0UL)
					__NOP();
				ITM->PORT[kChannel_].u32 = word_;
				Init();
			}
		}
	}
};


// A dummy SWO channel (disabled)
typedef SwoChannel<-1> SwoDummyChannel;


template<
	typename SysClk
	, const SwoProtocol proto
	, const uint32_t bitrate
	, typename Ch0
	, typename Ch1 = SwoDummyChannel
	, typename Ch2 = SwoDummyChannel
	, typename Ch3 = SwoDummyChannel
	, typename Ch4 = SwoDummyChannel
	, typename Ch5 = SwoDummyChannel
	, typename Ch6 = SwoDummyChannel
	, typename Ch7 = SwoDummyChannel
	, typename Ch8 = SwoDummyChannel
	, typename Ch9 = SwoDummyChannel
	, typename Ch10 = SwoDummyChannel
	, typename Ch11 = SwoDummyChannel
	, typename Ch12 = SwoDummyChannel
	, typename Ch13 = SwoDummyChannel
	, typename Ch14 = SwoDummyChannel
	, typename Ch15 = SwoDummyChannel
	, typename Ch16 = SwoDummyChannel
	, typename Ch17 = SwoDummyChannel
	, typename Ch18 = SwoDummyChannel
	, typename Ch19 = SwoDummyChannel
	, typename Ch20 = SwoDummyChannel
	, typename Ch21 = SwoDummyChannel
	, typename Ch22 = SwoDummyChannel
	, typename Ch23 = SwoDummyChannel
	, typename Ch24 = SwoDummyChannel
	, typename Ch25 = SwoDummyChannel
	, typename Ch26 = SwoDummyChannel
	, typename Ch27 = SwoDummyChannel
	, typename Ch28 = SwoDummyChannel
	, typename Ch29 = SwoDummyChannel
	, typename Ch30 = SwoDummyChannel
	, typename Ch31 = SwoDummyChannel
>
class SwoTraceSetup
{
public:
	static constexpr SwoProtocol kProto_ = proto;
	static constexpr uint32_t kChannelMask_ = 
		Ch0::kChannelBit_ | Ch1::kChannelBit_
		| Ch2::kChannelBit_ | Ch3::kChannelBit_
		| Ch4::kChannelBit_ | Ch5::kChannelBit_
		| Ch6::kChannelBit_ | Ch7::kChannelBit_
		| Ch8::kChannelBit_ | Ch9::kChannelBit_
		| Ch10::kChannelBit_ | Ch11::kChannelBit_
		| Ch12::kChannelBit_ | Ch13::kChannelBit_
		| Ch14::kChannelBit_ | Ch15::kChannelBit_
		| Ch16::kChannelBit_ | Ch17::kChannelBit_
		| Ch18::kChannelBit_ | Ch19::kChannelBit_
		| Ch20::kChannelBit_ | Ch21::kChannelBit_
		| Ch22::kChannelBit_ | Ch23::kChannelBit_
		| Ch24::kChannelBit_ | Ch25::kChannelBit_
		| Ch26::kChannelBit_ | Ch27::kChannelBit_
		| Ch28::kChannelBit_ | Ch29::kChannelBit_
		| Ch30::kChannelBit_ | Ch31::kChannelBit_
		;

	ALWAYS_INLINE static void Init()
	{
		// At least one channel needs to be enabled
		if(kChannelMask_ != 0);
		{
			CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;

			TPI->CSPSR = 1;					// protocol width = 1 bit
			TPI->SPPR = kProto_;			// 1 = Manchester, 2 = Asynchronous
			// Bit rate depends on protocol type
			if (kProto_ == kManchester)
				TPI->ACPR = SysClk::kFrequency_ / (2 * bitrate) - 1;
			else
				TPI->ACPR = SysClk::kFrequency_ / bitrate - 1;
			TPI->FFCR = 0;					// turn off formatter, discard ETM output

			ITM->LAR = 0xC5ACCE55;			// unlock access to ITM registers
			ITM->TCR = ITM_TCR_SWOENA_Msk | ITM_TCR_ITMENA_Msk;
			ITM->TPR = 0;					// privileged access is off
			ITM->TER = kChannelMask_;		// enable stimulus channel(s)

			//RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // enable AFIO access
			//AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;  // disable JTAG to release TRACESWO
			DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN; // enable IO trace pins
		}
	}

	ALWAYS_INLINE static bool IsTracing()
	{
		return (ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL;
	}
};

