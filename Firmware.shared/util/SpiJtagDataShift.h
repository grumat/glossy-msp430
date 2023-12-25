#pragma once

#ifndef STDPROJ_H__INCLUDED__
#	include "../stdproj.h"
#endif

#include "TmsAutoShaper.h"


/// Autonomous TMS generation
typedef TmsAutoShaper<
	SysClk
	, kJtmsShapeTimer
	, kJtmsTimerClk
	, kTmsOutChannel
	, JTCK_Speed_1
	, JTCK_Speed_2
	, JTCK_Speed_3
	, JTCK_Speed_4
	, JTCK_Speed_5
> TmsGen;


enum ScanType : uint8_t
{
	kSelectDR_Scan = 1,
	kSelectIR_Scan = 2,
};

enum PhaseOpts : uint8_t
{
	kNormalSpeed,
	kInvertPhase,	// clock edge inverted so half phase earlier
};


// Local template class to handle IR/DR data shifts
/*
** This template does not work for *container_type == uint64_t* as shift operation
** are bound to register size. Not sure if this is a compiler bug or a language spec.
*/
template<
	typename SendStream
	, const ScanType scan_size
	, const uint8_t payload_bitsize
	, typename arg_type
	, const PhaseOpts phase = kNormalSpeed
	, typename container_type = uint32_t
>
class SpiJtagDataShift
{
public:
	// Container is a POD data that MCU can optimize and fit all stuff
	typedef arg_type arg_type_t;
	// Container is a POD data that MCU can optimize and fit all stuff
	typedef container_type container_t;

	// Total container bit-size
	constexpr static uint8_t kContainerBitSize_ = sizeof(container_t) * 8;
	// Data payload bit-size
	constexpr static uint8_t kPayloadBitSize_ = payload_bitsize;
	// Number of clocks in the head until update is complete
	constexpr static uint8_t kStartClocks_ = (scan_size > 1);
	//
	constexpr static uint8_t kAddPhase = (phase == kNormalSpeed);
	// Number of clocks after select (Select DR/IR + Capture DR/IR)
	constexpr static uint8_t kClocksToShift_ = 2;
	// Number of clocks until we enter desired state (one TMS entry + sel + 2 required by state machine)
	constexpr static uint8_t kHeadClocks_ = kStartClocks_ + scan_size + kClocksToShift_;
	// Number of clocks in the tail until update is complete
	constexpr static uint8_t kTailClocks_ = 1;
	// Number of clocks in the tail to reset TMS state
	constexpr static uint8_t kResetClocks_ = 1;
	// Data should always be aligned to msb
	constexpr static uint8_t kDataShift_ = kContainerBitSize_ - kHeadClocks_ - kPayloadBitSize_;

	// This is the mask to isolate data payload bits
	constexpr static container_t kDataMask_ = (((container_t)1 << kPayloadBitSize_) - 1) << kDataShift_;

	// Number of necessary bytes to transfer everything (rounded up with +7/8)
	constexpr static container_type kStreamBytes_ = (kHeadClocks_ + kPayloadBitSize_ + kTailClocks_ + kResetClocks_ + 7) / 8;

	ALWAYS_INLINE static void SetupHW()
	{
		// static buffer shall be in RAM because flash causes latencies!
		static uint16_t toggles_[] =
		{
			// TMS rise (start of state machine)
			kStartClocks_ + kAddPhase
			// TMS fall Select DR / IR
			, kStartClocks_ + kAddPhase + scan_size
			// TMS rise signals last data bit
			, kHeadClocks_ + kAddPhase + kPayloadBitSize_ - 1
			// After last bit an additional is required to update DR/IR register
			, kHeadClocks_ + kAddPhase + kPayloadBitSize_ + kTailClocks_
			// End of sequence: no more requests needed
			, UINT16_MAX
		};

		TmsGen::StartDma(&toggles_[2 - kStartClocks_], _countof(toggles_) - 1);
		TmsGen::Start(toggles_[1 - kStartClocks_]);

		// Special case as DMA cannot handle 1 clock widths in 9 MHz
		if (kStartClocks_ == 0)
			TmsGen::Set();
	}

	//! Shifts data in and out of the JTAG bus
	ALWAYS_INLINE arg_type_t Transmit(arg_type_t data)
	{
		static_assert(kPayloadBitSize_ > 0, "no payload size specified");
		static_assert(kPayloadBitSize_ <= 8 * sizeof(arg_type_t), "argument can't fit payload data");
		static_assert(kStreamBytes_ <= kContainerBitSize_, "container can't fit all necessary bits");

		/*
		** We need to keep TDI level stable during Run-Test/Idle state otherwise
		** it would insert CPU clocks.
		*/
		bool lvl = JTDI::Get();

		// Move bits inside container aligned to msb
		container_t w = (data << kDataShift_);
		// Current TDI level is copied to all unused bits
		if (lvl)
			w |= ~kDataMask_;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		// this is a little-endian machine... (Note: optimizing compiler clears unused conditions)
		if (sizeof(w) == sizeof(uint16_t))
			w = __REV16(w);
		else if (sizeof(w) > sizeof(uint16_t))
			w = __REV(w);
#endif

		SetupHW();

		(container_t&)JtagDev::tx_buf_ = w;
		SendStream::SendStream(kStreamBytes_);
		container_t r = (container_t&)JtagDev::rx_buf_;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		// this is a little-endian machine... (Note: optimizing compiler clears unused conditions)
		if (sizeof(r) == sizeof(uint16_t))
			r = __REV16(r);
		else if (sizeof(r) > sizeof(uint16_t))
			r = __REV(r);
#endif
		// If payload fits data-type, then cast will mask bits out for us
		if (sizeof(arg_type_t) * 8 == kPayloadBitSize_)
			return (arg_type_t)(r >> kDataShift_);
		else
			return (arg_type_t)((r & kDataMask_) >> kDataShift_);
		/* JTAG state = Run-Test/Idle */
	}
};

