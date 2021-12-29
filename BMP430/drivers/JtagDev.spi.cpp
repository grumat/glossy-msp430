
#include "stdproj.h"

#if JTAG_USING_SPI
#include "JtagDev.h"


typedef SpiTemplate
<
	kSpiForJtag
	, SysClk
	, ExternJClk::kFrequency_
	, kSpiMaster
	, kSpiMode3
	, kSpi8bitMsb
	, false
	, kSpiFullDuplex
> SpiJtagDevice;

typedef DmaChannel
<
	TmsShapeOutTimerChannel::DmaInstance_
	, TmsShapeOutTimerChannel::DmaCh_
	, kDmaMemToPer
	, kDmaShortPtrInc
	, kDmaShortPtrConst
	, kDmaVeryHighPrio
> TableToTimerDma;


class MuteSpiClk
{
public:
	ALWAYS_INLINE MuteSpiClk()
	{
		JTCK::SetHigh();	// SPI clk defaults to; this is used while muting JTCK_SPI
		JTCK::SetupPinMode();
	}
	ALWAYS_INLINE ~MuteSpiClk()
	{
		JTCK_SPI::SetupPinMode();
	}
};


bool JtagDev::OnOpen()
{
	mspArch_ = ChipInfoDB::kCpu;

	//TmsShapeOutTimerChannel::SetOutputMode(kTimOutLow);
	TmsShapeTimer::Init();
	// TMS uses GPIO on reset state
	TmsShapeOutTimerChannel::Setup();
	TableToTimerDma::Init();
	SpiJtagDevice::Init();
	// JUST FOR A CASUAL TEST USING LOGIC ANALYZER
#define TEST_WITH_LOGIC_ANALYZER 0
#if TEST_WITH_LOGIC_ANALYZER
#endif
	return true;
}


void JtagDev::OnClose()
{
	InterfaceOff();
	JtagOff::Enable();
	TableToTimerDma::Stop();
	SpiJtagDevice::Stop();
	TmsShapeTimer::Stop();
}


void JtagDev::OnConnectJtag()
{
	// slau320: ConnectJTAG / DrvSignals

	TmsShapeOutTimerChannel::DisableDma();
	// Drive TDI low, while pins are disabled
	SpiJtagDevice::PutChar(0);
	// Drive MCU outputs on
	JtagOn::Enable();
	// Switch to SPI
	JtagSpiOn::Enable();
	TmsShapeOutTimerChannel::EnableDma();	// default DMA active state while connected
	//TmsShapeGpioOut::Setup();
	// Enable voltage level converter
	InterfaceOn();
	// Start TEST mode
	JTEST::SetHigh();
	// Wait to settle
	StopWatch().Delay(10);
}


void JtagDev::OnReleaseJtag()
{
	// slau320: StopJtag
	JTEST::SetLow();
	// Disable Voltage level converter
	InterfaceOff();
	// Put MCU pins in 3-state
	JtagOff::Enable();
	StopWatch().Delay(10);
}


void JtagDev::OnEnterTap()
{
	unsigned int jtag_id;

	JTEST::SetLow();		//1
	StopWatch().Delay(4);	// reset TEST logic

	JRST::SetHigh();		//2

	JTEST::SetHigh();		//3
	StopWatch().Delay(20);	// activate TEST logic

	// phase 1
	JRST::SetLow();			//4
	MicroDelay::Delay(60);

	// phase 2 -> TEST pin to 0, no change on RST pin
	// for 4-wire JTAG clear Test pin
	JTEST::SetLow();		//5

	// phase 3
	MicroDelay::Delay(1);

	// phase 4 -> TEST pin to 1, no change on RST pin
	// for 4-wire JTAG
	JTEST::SetHigh();		//7
	MicroDelay::Delay(60);

	// phase 5
	JRST::SetHigh();
	StopWatch().Delay(5);
}


/*!
Reset target JTAG interface and perform fuse-HW check

This is the slau320aj sequence:
	¯¯¯¯¯¯¯| |¯| |¯| |¯| |¯| |¯| |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
TCK	       | | | | | | | | | | | |
	       |_| |_| |_| |_| |_| |_|
	         ^   ^   ^   ^   ^   ^
	¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|   |¯| |¯| |¯|
TMS                                |   | | | | | |
								   |___| |_| |_| |_____

								     |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
TDI                                  |
	_________________________________|
	                                 ^ TCLK

The sequence enters the Test-Logic-Reset, where fuse check happens
(see slau320aj @2.1.2). More pulses may happen, as it will stay locked
in the Test-Logic-Reset state.
A 10µs pause is required for the Run-Test/Idle. TDI needs to clock once
during the Run-Test/Idle state and kept high. This may draw up to 2 mA
during the fuse check.
Note that TCLK (target clock) are always generated for TDI transients 
during the Run-Test/Idle state.

Note that this sequence will be solved using SPI transfer, which may 
generate more pulses than required. Bit banging is also done for the
TMS pulses.
*/
void JtagDev::OnResetTap()
{
	//initialize it to high level
	__NOP();
	TmsShapeOutTimerChannel::DisableDma();
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutHigh);
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutToggle);	// causes a 60 ns delay after rising edge
	TmsShapeOutTimerChannel::SetCompare(6);
	TmsShapeTimer::StartShot();
	//TableToTimerDma::Start(toggles + 1, TmsShapeOutTimerChannel::GetCcrAddress(), count - 1);
	SpiJtagDevice::PutChar(0x01);	// put TDI up on Run-Test/Idle state
	TmsShapeTimer::CounterStop();

	MicroDelay::Delay(10);
#if 1
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutHigh);
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutLow);
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutHigh);
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutLow);
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutHigh);
#endif
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutLow);

	// Restore SPI stuff to default state while connected
	TmsShapeOutTimerChannel::SetOutputMode(kTimOutToggle);
	TmsShapeOutTimerChannel::EnableDma();
}


enum ScanType : uint8_t
{
	kSelectDR_Scan = 1,
	kSelectIR_Scan = 2,
};


// Local template class to handle IR/DR data shifts
/*
** This template does not work for *container_type == uint64_t* as shift operation
** are bound to register size. Not sure if this is a compiler bug or a language spec.
*/
template<
	const ScanType scan_size
	, const uint8_t payload_bitsize
	, typename arg_type
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
	// Number of clocks in the tail until update is complete
	constexpr static uint8_t kStartClocks_ = (scan_size > 1 && ExternJClk::kFrequency_ >= 5000000UL);
	// Number of clocks after select (Select DR/IR + Capture DR/IR)
	constexpr static uint8_t kClocksToShift_ = 2;
	// Number of clocks until we enter desired state (one TMS entry + sel + 2 required by state machine)
	constexpr static uint8_t kHeadClocks_ = kStartClocks_ + scan_size + kClocksToShift_;
	// Number of clocks in the tail until update is complete
	constexpr static uint8_t kTailClocks_ = 1;
	// Data should always be aligned to msb
	constexpr static uint8_t kDataShift_ = kContainerBitSize_ - kHeadClocks_ - kPayloadBitSize_;

	// This is the mask to isolate data payload bits
	constexpr static container_t kDataMask_ = (((container_t)1 << kPayloadBitSize_) - 1) << kDataShift_;

	// Number of necessary bytes to transfer everything (rounded up with +7/8)
	constexpr static container_type kStreamBytes_ = (kHeadClocks_ + kPayloadBitSize_ + kTailClocks_ + 7) / 8;

	ALWAYS_INLINE static void SetupHW()
	{
		// static buffer shall be in RAM because flash causes latencies!
		static uint16_t toggles_[] =
		{
			// TMS rise (start of state machine)
			kStartClocks_
			// TMS fall Select DR / IR
			, kStartClocks_ + scan_size
			// TMS rise signals last data bit
			, kHeadClocks_ + kPayloadBitSize_ - 1
			// After last bit an additional is required to update DR/IR register
			, kHeadClocks_ + kPayloadBitSize_ + kTailClocks_
			// End of sequence: no more requests needed
			, UINT16_MAX
		};

		TmsShapeOutTimerChannel::SetCompare(toggles_[1 - kStartClocks_]);
		TableToTimerDma::Start(&toggles_[2 - kStartClocks_], TmsShapeOutTimerChannel::GetCcrAddress(), _countof(toggles_) - 1);
		TmsShapeTimer::StartShot();

		// Special case as DMA cannot handle 1 clock widths in 9 MHz
		if (kStartClocks_ == 0)
		{
			TmsShapeOutTimerChannel::SetOutputMode(kTimOutHigh);
			TmsShapeOutTimerChannel::SetOutputMode(kTimOutToggle);
		}
	}

	//! Shifts data in and out of the JTAG bus
	ALWAYS_INLINE arg_type_t Transmit(arg_type_t data)
	{

		static_assert(kPayloadBitSize_ > 0, "no payload size specified");
		static_assert(kPayloadBitSize_ <= 8*sizeof(arg_type_t), "argument can't fit payload data");
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
		if(sizeof(w) == sizeof(uint16_t))
			w = __REV16(w);
		else if (sizeof(w) > sizeof(uint16_t))
			w = __REV(w);
#endif

		SetupHW();

		container_t r;
		SpiJtagDevice::PutStream(&w, &r, kStreamBytes_);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		// this is a little-endian machine... (Note: optimizing compiler clears unused conditions)
		if (sizeof(r) == sizeof(uint16_t))
			r = __REV16(r);
		else if (sizeof(r) > sizeof(uint16_t))
			r = __REV(r);
#endif
		// If payload fits data-type, then cast will mask bits out for us
		if(sizeof(arg_type_t)*8 == kPayloadBitSize_)
			return (arg_type_t)(r >> kDataShift_);
		else
			return (arg_type_t)((r & kDataMask_) >> kDataShift_);
		/* JTAG state = Run-Test/Idle */
	}
};


uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	SpiJtagDataShift
	<
		kSelectIR_Scan		// Select IR-Scan JTAG register
		, 8					// 8 bits data
		, uint8_t			// 8 bits data-type fits perfectly
	> jtag;
	return jtag.Transmit(instruction);
	// JTAG state = Run-Test/Idle
}


uint8_t JtagDev::OnDrShift8(uint8_t data)
{
	SpiJtagDataShift
		<
		kSelectDR_Scan		// Select IR-Scan JTAG register
		, 8					// 8 bits data
		, uint8_t			// 8 bits data-type fits perfectly
		> jtag;
	uint8_t val = jtag.Transmit(data);
	return val;
	/* JTAG state = Run-Test/Idle */
}


uint16_t JtagDev::OnDrShift16(uint16_t data)
{
	SpiJtagDataShift
		<
		kSelectDR_Scan		// Select IR-Scan JTAG register
		, 16				// 16 bits data
		, uint16_t			// 16 bits data-type fits perfectly
		> jtag;
	return jtag.Transmit(data);
	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	SpiJtagDataShift
		<
		kSelectDR_Scan		// Select IR-Scan JTAG register
		, 20				// 20 bits data
		, uint32_t			// 32 bits data-type is required
		> jtag;
	data = jtag.Transmit(data);

	return ((data << 16) + (data >> 4)) & 0x000FFFFF;
	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	typedef SpiJtagDataShift
		<
		kSelectDR_Scan		// Select IR-Scan JTAG register
		, 32				// 32 bits data
		, uint32_t			// 32 bits data-type fits perfectly
		, uint64_t
		> Payload32_t;

	constexpr static uint8_t kDataShiftHi_ = 32 - Payload32_t::kDataShift_;

	uint32_t parts[2];
	uint32_t ret[2];

	/*
	** We need to keep TDI level stable during Run-Test/Idle state otherwise
	** it would insert CPU clocks.
	*/
	bool lvl = JTDI::Get();

	uint32_t hi = data >> kDataShiftHi_;
	uint32_t lo = data << Payload32_t::kDataShift_;
	if (lvl)
	{
		hi |= ~(Payload32_t::kDataMask_ >> 32);
		lo |= ~((uint32_t)Payload32_t::kDataMask_);
	}
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	parts[0] = __REV(hi);
	parts[1] = __REV(lo);
#else
	parts[0] = hi;
	parts[1] = lo;
#endif

	Payload32_t::SetupHW();

	SpiJtagDevice::PutStream(parts, ret, Payload32_t::kStreamBytes_);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	hi = __REV(ret[0]);
	lo = __REV(ret[1]);
#else
	hi = ret[0];
	lo = ret[1];
#endif
	hi &= (Payload32_t::kDataMask_ >> 32);
	lo &= ((uint32_t)Payload32_t::kDataMask_);

	data = (hi << kDataShiftHi_) | (lo >> Payload32_t::kDataShift_);
	return data;
	/* JTAG state = Run-Test/Idle */
}


bool JtagDev::IsInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_CAPTURE);
	if ((OnDrShift16(0) & (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ)) != (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ))
		return false;
	return true;
}


void JtagDev::OnSetTclk()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0xff);
}


void JtagDev::OnClearTclk()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0x00);
}


void JtagDev::OnPulseTclk()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0xf0);
}


void JtagDev::OnPulseTclkN()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0x0f);
}


void JtagDev::OnPulseTclk(int count)
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::Repeat(0xf0, count);
}


bool JtagDev::OnInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_LOW_BYTE);
	OnDrShift8(CNTRL_SIG_READ);
	JtagDev::OnSetTclk();

	for (int i = 0; i < 10; i++)
	{
		if (IsInstrLoad())
			return true;
		JtagDev::OnPulseTclk();
	}
	return false;
}


void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
#if 0
	static const uint32_t tab[] =
	{
		tdi1
		, tdi0
	};

	// Configure timer for pulse generation every timer cycle
	FlashStrobeCtrl::SetCompare(0);
	FlashStrobeDma::SetSourceAddress(tab);
	FlashStrobeDma::SetTransferCount(_countof(tab));
	FlashStrobeDma::Enable();
	min_pulses = (min_pulses + 1) * _countof(tab);	// two borders for each cycle

	while (min_pulses != 0)
	{
		// Time repetition counter is limited to 256 pulses
		uint32_t amount = (min_pulses > 256) ? 256 : min_pulses;
		FlashStrobeTimer::WaitForAutoStop();
		FlashStrobeTimer::StartRepetition(amount);
		min_pulses -= amount;
	}
	// In single shot mode timer will disable itself
	FlashStrobeTimer::WaitForAutoStop();
	FlashStrobeDma::Disable();
#endif
}


#define OUT1RDY 0x0008
#define OUT0RDY 0x0004
#define IN1RDY  0x0002
#define IN0RDY  0x0001

#define JMB32B  0x0010
#define OUTREQ  0x0004
#define INREQ   0x0001


uint32_t JtagDev::OnReadJmbOut()
{
	uint16_t sJMBOUT0 = 0, sJMBOUT1 = 0, sJMBINCTL = 0;
	OnIrShift(IR_JMB_EXCHANGE);
	if (OnDrShift16(sJMBINCTL) & OUT1RDY)
	{
		sJMBINCTL |= JMB32B + OUTREQ;
		OnDrShift16(sJMBINCTL);
		sJMBOUT0 = OnDrShift16(0);
		sJMBOUT1 = OnDrShift16(0);
		return ((uint32_t)sJMBOUT1 << 16) + sJMBOUT0;
	}
	return 0;
}


//----------------------------------------------------------------------------
//! \brief Write a 16bit value into the JTAG mailbox system.
//! The function timeouts if the mailbox is not empty after a certain number
//! of retries.
//! \param[in] word dataX (data to be shifted into mailbox)
bool JtagDev::OnWriteJmbIn16(uint16_t dataX)
{
	uint16_t sJMBINCTL;
	uint16_t sJMBIN0;
	uint32_t Timeout = 0;
	sJMBIN0 = (uint16_t)(dataX & 0x0000FFFF);
	sJMBINCTL = INREQ;

	OnIrShift(IR_JMB_EXCHANGE);
	do
	{
		Timeout++;
		if (Timeout >= 3000)
			return false;
	}
	while (!(OnDrShift16(0x0000) & IN0RDY) && Timeout < 3000);
	if (Timeout < 3000)
	{
		OnDrShift16(sJMBINCTL);
		OnDrShift16(sJMBIN0);
	}
	return true;
}


#endif
