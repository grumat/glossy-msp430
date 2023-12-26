#pragma once

#ifndef STDPROJ_H__INCLUDED__
#	include "../stdproj.h"
#endif

#include "TmsAutoShaper.h"


/// Which of the JTAG register we want to read/write
enum ScanType : uint8_t
{
	kSelectDR_Scan = 1,
	kSelectIR_Scan = 2,
};

/// Clock compensation for very fast streams
/*!
Use this with care! In general if the SPI clock of 1/8 of the system clock
you have to anticipate clock transitions to compensate internal timer delays.
For an SPI clock of 1/16 or below, the kNormalSpeed should be ok. I have not 
tested, but I bet that a clock of SPI/4 will be impossible to implement.
*/
enum PhaseOpts : uint8_t
{
	kNormalSpeed,
	kAnticipateClock,	// use early clock transition (high speed)
};


/*!
These is the table of the JTAG state bit durations, for a complete cycle starting
from idle, transfer and back to idle.
Please check JTAG documentation to understand each state label.
JTAG allows to burst more data into the channel without switching back to the idle
state, for improved transfer performance. This is not supported by tis implementation 
and also not covered here.
*/
template <const uint8_t kPayloadSize>
struct JtagStates
{
	/*
	This is a typical state sequence of the JTAG state machine
	*/
	
	// Number of clocks required to enter Select-DR-Scan state
	constexpr static uint8_t kSelectDrScan_ = 1;
	// Number of clocks required to enter Capture-DR state
	constexpr static uint8_t kCaptureDr_ = 2;
	// Number of clocks required to enter Capture-DR state
	constexpr static uint8_t kShiftDr_ = kPayloadSize - 1;
	// Number of clocks required to enter Exit1-DR state
	constexpr static uint8_t kExit1Dr_ = 1;
	// Number of clocks required to enter Update-DR state
	constexpr static uint8_t kUpdateDr_ = 1;
	
	// Number of clocks required to enter Select-IR-Scan state
	constexpr static uint8_t kSelectIrScan_ = 2;
	// Number of clocks required to enter Capture-IR state
	constexpr static uint8_t kCaptureIr_ = 2;
	// Number of clocks required to enter Capture-IR state
	constexpr static uint8_t kShiftIr_ = kPayloadSize - 1;
	// Number of clocks required to enter Exit1-IR state
	constexpr static uint8_t kExit1Ir_ = 1;
	// Number of clocks required to enter Update-IR state
	constexpr static uint8_t kUpdateIr_ = 1;
	
	// Number of clocks required to enter Run-Test/Idle state
	constexpr static uint8_t kIdle_ = 1;
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
	/// Autonomous TMS generation
	typedef TmsAutoShaper<
		SysClk
		, kJtmsShapeTimer
		, kTmsOutChannel
		, 0
		> TmsGen;

public:
	// Container is a POD data that MCU can optimize and fit all stuff
	typedef arg_type arg_type_t;
	// Container is a POD data that MCU can optimize and fit all stuff
	typedef container_type container_t;
	// The JTAG logic to be used
	typedef JtagStates<payload_bitsize> JtagBits;

// General attributes
public:
	// Total container bit-size
	constexpr static uint8_t kContainerBitSize_ = sizeof(container_t) * 8;
	// Data payload bit-size
	constexpr static uint8_t kPayloadBitSize_ = payload_bitsize;

	// Clocks per bit (both edges triggers a clock)
	constexpr static uint8_t kClocksPerBit_ = 2;
	// On the first edge, CNT = 1
	constexpr static uint8_t kCounterStart_ = 1;
	// Period on the ARR register is 0-based, so always subtract 1
	constexpr static uint8_t kPeriodOffset_ = 1;
	
	// Amount of clock cycles to anticipate (for high speeds)
	constexpr static uint8_t kClockAnticipation_ = (phase != kNormalSpeed);
	// End pulse needs to be always anticipated (capture compare is 0-based)
	constexpr static uint8_t kP1Anticipation_ = 1;
	// End pulse needs to be always anticipated (capture compare is 0-based)
	constexpr static uint8_t kP2Anticipation_ = kP1Anticipation_;

	// Clocks per bit (both edges triggers a clock)
	constexpr static uint8_t kDummyBits_ = 1;


// JTAG clock counts, according to mode
public:
	// Number of bits to enter IR or DR mode
	constexpr static uint8_t kSelectScan =
		scan_size == kSelectDR_Scan
		? JtagBits::kSelectDrScan_
		: JtagBits::kSelectIrScan_
		;
	// Bits added before the payload
	constexpr static uint8_t kCapture_ =
		scan_size == kSelectDR_Scan
		? JtagBits::kCaptureDr_
		: JtagBits::kCaptureIr_
		;
	// Number of bits for the Shift-IR/DR cycle
	constexpr static uint8_t kShift_ =
		scan_size == kSelectDR_Scan
		? JtagBits::kShiftDr_
		: JtagBits::kShiftIr_
		;
	// Number of bits for the Exit1-IR/DR cycle
	constexpr static uint8_t kExit1_ =
		scan_size == kSelectDR_Scan
		? JtagBits::kExit1Dr_
		: JtagBits::kExit1Ir_
		;
	// Number of bits for the Update-IR/DR cycle
	constexpr static uint8_t kUpdate_ =
		scan_size == kSelectDR_Scan
		? JtagBits::kUpdateDr_
		: JtagBits::kUpdateIr_
		;
	// Amount of bits necessary for a complete cycle
	constexpr static uint8_t kNecessaryBits =
		kSelectScan
		+ kCapture_
		+ kShift_
		+ kExit1_
		+ kUpdate_
		+ JtagBits::kIdle_
		;

// Attributes for First TMS pulse (both edges generates a clock)
public:
	// Number of clocks for start block (1st edge, CNT = 1)
	constexpr static uint8_t kStart1stPulse_ =
		kDummyBits_ > 0
		? kDummyBits_ * kClocksPerBit_ + kCounterStart_
		: kCounterStart_
		;
	// Number of clocks for start block (both edges generates a clock)
	constexpr static uint8_t kEnd1stPulse_ = 
		(kDummyBits_ + kSelectScan) * kClocksPerBit_ + kCounterStart_ - kPeriodOffset_;

// Attributes for Second TMS pulse (considering that timer counter has restarted)
public:
	// Position of last data bit (now CNT = 0!!!)
	constexpr static uint8_t kLastDataBit = (kCapture_ + kShift_);
	// Number of clock for last bit
	constexpr static uint8_t kStart2ndPulse_ = kLastDataBit * kClocksPerBit_;

	// Number of bits until transfer mode exits
	constexpr static uint8_t kExitBits_ = kExit1_ + kUpdate_;
	// Number of bits until transfer mode exits (subtracted -1, because ARR requires to)
	constexpr static uint8_t kEnd2ndPulse_ = (kLastDataBit + kExitBits_) * kClocksPerBit_ - 1;

// Attributes for the SPI data composition
public:
	// Number of clocks until we enter desired state (one TMS entry + sel + 2 required by state machine)
	constexpr static uint8_t kHeadClocks_ = kDummyBits_ + kSelectScan + kCapture_;
	// Data should always be aligned to msb
	constexpr static uint8_t kDataShift_ = kContainerBitSize_ - kHeadClocks_ - kPayloadBitSize_;

	// This is the mask to isolate data payload bits
	constexpr static container_t kDataMask_ = (((container_t)1 << kPayloadBitSize_) - 1) << kDataShift_;

	// Number of necessary bytes to transfer everything (rounded up with +7/8)
	constexpr static container_type kStreamBytes_ = (kDummyBits_ + kNecessaryBits + 7) / 8;

public:
	/// Setups PWM channel to produce two TMS pulses
	constexpr ALWAYS_INLINE static void SetupHW()
	{
		static_assert(kPayloadBitSize_ <= kContainerBitSize_, "Too much data for given container");
		
		static_assert(kEnd1stPulse_ >= kStart1stPulse_, "Inconsistent 1st pulse width");
		static_assert(kStart2ndPulse_ > 0, "Start of 2nd pulse is inconsistent");
		static_assert(kEnd2ndPulse_ > kStart2ndPulse_, "Inconsistent 2nd pulse width");

		/*
		** Note: With the use of a logic analyzer I checked that "Clock anticipation"
		** is not required for the last pulse. Both ways works, but without it
		** the best signal is better...
		*/
		TmsGen::Start(
			kStart1stPulse_ - kClockAnticipation_
			, kEnd1stPulse_ - kClockAnticipation_
			, kStart2ndPulse_ - kClockAnticipation_
			, kEnd2ndPulse_ /*- kClockAnticipation*/
		);
	}

	//! Shifts data in and out of the JTAG bus
	ALWAYS_INLINE static void Transmit(arg_type_t data)
	{
		static_assert(kPayloadBitSize_ > 0, "no payload size specified");
		static_assert(kPayloadBitSize_ <= 8 * sizeof(arg_type_t), "argument can't fit payload data");
		static_assert(kStreamBytes_ <= kContainerBitSize_, "container can't fit all necessary bits");

		/*
		** We need to keep TDI level stable during Run-Test/Idle state otherwise
		** it would insert CPU clocks.
		*/
		bool lvl = JTDI::Get();

		if (kContainerBitSize_ <= 32)
		{
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
			TmsGen::Stop();
		}
		else
		{
			constexpr static uint8_t kDataShiftHi_ = 32 - kDataShift_;
	
			uint32_t hi = data >> kDataShiftHi_;
			uint32_t lo = data << kDataShift_;
			if (lvl)
			{
				hi |= ~(kDataMask_ >> 32);
				lo |= ~((uint32_t)kDataMask_);
			}
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			JtagDev::tx_buf_.dwords[0] = __REV(hi);
			JtagDev::tx_buf_.dwords[1] = __REV(lo);
#else
			JtagDev::tx_buf_.dwords[0] = hi;
			JtagDev::tx_buf_.dwords[1] = lo;
#endif

			SetupHW();

			SendStream::SendStream(kStreamBytes_);
			TmsGen::Stop();

		}
		/* JTAG state = Run-Test/Idle */
	}
	
	//! You can save a few cycles by calling this only when necessary
	ALWAYS_INLINE static arg_type_t Decode()
	{
		if (kContainerBitSize_ <= 32)
		{			
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
		}
		else
		{			
			constexpr static uint8_t kDataShiftHi_ = 32 - kDataShift_;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			uint32_t hi = __REV(JtagDev::rx_buf_.dwords[0]);
			uint32_t lo = __REV(JtagDev::rx_buf_.dwords[1]);
#else
			uint32_t hi = JtagDev::rx_buf_.dwords[0];
			uint32_t lo = JtagDev::rx_buf_.dwords[1];
#endif
			hi &= (kDataMask_ >> 32);
			lo &= ((uint32_t)kDataMask_);

			arg_type_t data = (hi << kDataShiftHi_) | (lo >> kDataShift_);
			return data;
		}
	}
};

