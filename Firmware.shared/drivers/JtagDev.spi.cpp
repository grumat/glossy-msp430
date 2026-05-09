// =============================================================================
// DEPRECATED — being phased out in favour of OPT_JTAG_IMPL_DTRIG.
//
// The TMS pulse shaper here clocks TIM1 from SCK (`TmsAutoShaper` with
// `kTI1F_ED`), which pegs TMS edge resolution to half a SCK period. By the
// parity of the DR/IR header lengths, at least one TMS edge of every shift
// always lands on a TCK rising edge — the same edge the JTAG target uses to
// sample TMS. Result: setup/hold violations on the target and ambiguous
// captures on a 100 MS/s logic analyzer.
//
// DTRIG runs TIM1 from SYSCLK with slave-mode reset by SCK, giving ~14 ns TMS
// edge resolution and proper mid-period placement. Rationale, geometric
// proof, and migration plan in:
//
//     .claude/docs/drivers/SPI_VARIANT_DEPRECATION.md
//
// Until the bluepill DtrigJtag template is generalised to drive TMS from a
// regular CH (PA10 = TIM1_CH3) instead of only CHN (PB14 = TIM1_CH2N on
// STLinkV2), this file remains the bluepill default.
// =============================================================================

#include "stdproj.h"

#if OPT_INCLUDE_JTAG_SPI_

using namespace Bmt::Timer;
using namespace Bmt::Dma;

#include "JtagDev.h"
#include "util/WaveSet.h"
#include "util/TimDmaWave.h"
#include "util/SpiJtagDataShift.h"


// A template for all different SPI configuration. Same device but different BAUD rate.
template<
	const uint32_t SPEED
>
struct SpiJtagDevType 
	: Spi::SpiTemplate
		<
		kSpiForJtag
		, SysClk
		, SPEED
		, Spi::Role::kMaster
		, Spi::ClkPol::kMode3
		, Spi::Bits::k8
		, Spi::Props::kDefault
		, Spi::BiDi::kFullDuplex
		>
{
	typedef Spi::SpiTemplate<
		kSpiForJtag
		, SysClk
		, SPEED
		, Spi::Role::kMaster
		, Spi::ClkPol::kMode3
		, Spi::Bits::k8
		, Spi::Props::kDefault
		, Spi::BiDi::kFullDuplex
	> BASE;
	static_assert(BASE::kInputClock_/8 >= SPEED, "Clock");
};


// Forward declaration for templates to work
struct DmaMode_;

// A data-type to transmit 8-bits to the IR register
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectIR_Scan	// Select IR-Scan JTAG register
	, 8					// 8 bits data
	, uint8_t			// 8 bits data-type fits perfectly
> XMitIr8Type;
// A data-type to transmit 8-bits to the DR register
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 8					// 8 bits data
	, uint8_t			// 8 bits data-type fits perfectly
> XMitDr8Type;
// A data-type to transmit 16-bits to the DR register
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 16				// 16 bits data
	, uint16_t			// 16 bits data-type fits perfectly
> XMitDr16Type;
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 20				// 20 bits data
	, uint32_t			// 32 bits data-type is required
> XMitDr20Type;
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 32				// 32 bits data
	, uint32_t			// 32 bits data-type fits perfectly
	, uint64_t
> XMitDr32Type;

// Lower speed grade SPI connection (or all default operation)
typedef SpiJtagDevType<JTCK_Speed_1> SpiJtagDevice;


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
typedef AnyChannel
<
	SpiJtagDevice::DmaInstance_
	, SpiJtagDevice::DmaTxCh_
	, Dir::kMemToPer
	, PtrPolicy::kBytePtrInc
	, PtrPolicy::kBytePtr
	, Prio::kMedium
> SpiTxDma;

typedef AnyChannel
<
	SpiJtagDevice::DmaInstance_
	, SpiJtagDevice::DmaRxCh_
	, Dir::kPerToMem
	, PtrPolicy::kBytePtr
	, PtrPolicy::kBytePtrInc
	, Prio::kHigh
	, true
> SpiRxDma;

// DMA channels are shared in STM32. Check conflicts.
static_assert(GenericJtagType::TmsGen::TmsOutCh_::DmaCh_ != SpiTxDma::kChan_, "DMA Channels are conflicting. This hardware setup is not compatible when JTAG_USING_DMA==1.");
static_assert(GenericJtagType::TmsGen::TmsOutCh_::DmaCh_ != SpiRxDma::kChan_, "DMA Channels are conflicting. This hardware setup is not compatible when JTAG_USING_DMA==1.");
// Just in case...
static_assert(SpiTxDma::kChan_ != SpiRxDma::kChan_, "The timer does not have independent DMA channels. This hardware setup is not compatible when JTAG_USING_DMA==1.");

struct DmaMode_
{
	static ALWAYS_INLINE void OnOpen()
	{
		SpiTxDma::Init();
		SpiTxDma::SetDestAddress(&SpiJtagDevice::GetDevice()->DR);
		// Note: SPI path never calls buf_.Step(), so the "current" half is fixed at init.
		// TODO: re-arm DMA addresses per frame when SPI is upgraded to true ping-pong
		// (matches the dtrig async model: render N+1 while DMA is shifting N).
		SpiTxDma::SetSourceAddress(JtagDev::buf_.GetCurrent1());
		SpiRxDma::Init();
		SpiRxDma::SetSourceAddress(&SpiJtagDevice::GetDevice()->DR);
		SpiRxDma::SetDestAddress(JtagDev::buf_.GetCurrent2());
	}
	static ALWAYS_INLINE void OnSpiInit()
	{
		SpiJtagDevice::Disable();
		SpiJtagDevice::EnableDma();
		SpiJtagDevice::Enable();
	}
	static ALWAYS_INLINE void OnClose()
	{
		SpiTxDma::Stop();
		SpiRxDma::Stop();
	}
	/// Kick the SPI-RX-then-TX DMA pair without waiting. Returns immediately;
	/// the caller is expected to do unrelated work (e.g. render the next frame
	/// into the ping-pong's "Next" half) and then call Wait() to block on
	/// completion. This is the async-friendly half of the legacy SendStream().
	static OPTIMIZED void Start(uint16_t cnt)
	{
		SpiRxDma::SetTransferCount(cnt);
		SpiRxDma::Enable();
		SpiTxDma::SetTransferCount(cnt);
		SpiTxDma::Enable();		// this will start the transmission
	}
	/// Poll until the RX DMA has consumed all bytes; clears the TC flag and
	/// disables both channels so the next Start() is on a clean slate.
	/// Replaces the former WFI/IRQ wake-up — there is no global wake event,
	/// the CPU just busy-waits, leaving room for a future split where the
	/// caller does useful work between Start() and Wait().
	static OPTIMIZED void Wait()
	{
		SpiRxDma::WaitTransferComplete();
		SpiTxDma::Disable();
		SpiRxDma::Disable();
	}
	/// Synchronous shortcut: kick + wait, used by the current Transmit()
	/// contract. The future async pipeline will call Start()/Wait()
	/// directly so a frame can be rendered while DMA is still shifting.
	static ALWAYS_INLINE void SendStream(uint16_t cnt)
	{
		Start(cnt);
		Wait();
	}
};

#else	// JTAG_USING_DMA

struct DmaMode_
{
	static ALWAYS_INLINE void OnOpen() {}
	static ALWAYS_INLINE void OnSpiInit() {}
	static ALWAYS_INLINE void OnClose() {}
	static ALWAYS_INLINE void SendStream(uint16_t cnt)
	{
		SpiJtagDevice::PutStream(JtagDev::buf_.GetCurrent1(), JtagDev::buf_.GetCurrent2(), cnt);
	}
};

#endif	// JTAG_USING_DMA

/*
** JTCLK generation. The TIM_DMA and TIM_DMA_2 implementations share the same
** call sites (JtclkWaveGen::Init/SetTarget/RunEx); they differ only in which
** template instantiation backs them, so the selector fires once here and the
** rest of the file just talks to JtclkWaveGen.
**
** USE_JTCLK_TIMDMA — internal shorthand for "either TIM_DMA flavour is active"
** so the body of OnOpen / OnFlashTclk doesn't have to repeat the OR.
*/
#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA \
 || OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA_2
#  define USE_JTCLK_TIMDMA 1
#else
#  define USE_JTCLK_TIMDMA 0
#endif

#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA
typedef TimDmaWav<
	SysClk
	, kTimDmaWavBeat
	, kTimForJtclkCnt
	, kTimDmaWavFreq
	, 2						// each pulse has two borders
	> JtclkWaveGen;
#elif OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA_2
typedef TimDmaWav2<
	SysClk
	, kTimDmaWavBeat
	, kTimForJtclkCnt
	, kTimChForJtclkCnt
	, kTimDmaWavFreq
	, 2						// each pulse has two borders
	> JtclkWaveGen;
#endif

class MuteSpiClk
{
public:
	ALWAYS_INLINE MuteSpiClk()
	{
		JTCK::SetHigh();	// SPI clk defaults; this is used while muting JTCK_SPI
		JTCK::SetupPinMode();
	}
	ALWAYS_INLINE ~MuteSpiClk()
	{
		JTCK_SPI::SetupPinMode();
	}
};


// Module-static flag: when the active bus speed needs early SPI-clock transitions
// to compensate for internal timer delays (formerly the JtagDevVhc derived class).
// Set by OnConnectJtag() based on BusSpeed; the singleton lifecycle makes the
// global state safe.
static bool s_anticipate_clock = false;


JtagDev::JtagDev()
{
}


bool JtagDev::OnOpen()
{
#if USE_JTCLK_TIMDMA
	/*
	** This table has up/down bits for the GPIOx_BSRR register that will be sourced
	** into the DMA to generate a 470 kHz frequency for the Flash memory. This clock is
	** required during flash erase and writes for the Gen1 and Gen2 MSP430 devices.
	** The table consists of 8 pulses, so polling will always have a chance to track DMA
	** state also when CPU is moderately interrupted (a complete table scan will require
	** about 16.9 us)
	** IMPORTANT: For all MSP430 JTCLK is shared with JTDI. The interface redirects pulses
	** in the JTDI line when JTAG state machine is in "Run-Test/Idle" state.
	*/
	static const uint32_t bsrr_table[] =
	{
		JTCLK::kBitValue_ << 16, // reset bit
		JTCLK::kBitValue_,		 // set bit,
	};
#endif

	XMitDr8Type::TmsGen::Config::Setup();
	// TMS uses GPIO on reset state
	XMitDr8Type::TmsGen::InitOutput();
#if USE_JTCLK_TIMDMA
	WATCHPOINT();
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, bsrr_table, _countof(bsrr_table));
#endif
	DmaMode_::OnOpen();
	SpiJtagDevice::Setup();
	DmaMode_::OnSpiInit();

#if OPT_TEST_WITH_LOGIC_ANALYZER
	DoLogicAnalyzerTest();
#endif
	return true;
}


void JtagDev::OnClose()
{
	// Hardware buffers in tri-state
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
	XMitDr8Type::TmsGen::Close();
	DmaMode_::OnClose();
	SpiJtagDevice::Stop();
}


void JtagDev::OnConnectJtag(BusSpeed speed)
{
	speed_ = speed;
	// At the highest bus speed the SPI clock is fast enough that the TMS
	// pulse-shaper must anticipate clock transitions to compensate the
	// internal timer delay; below that, normal-speed timing is used.
	s_anticipate_clock = (speed == BusSpeed::kFastest);
	// slau320: ConnectJTAG / DrvSignals

	// BLOCK: Auto TMS output temporary suspension
	{
		// Suspend TMS output as we want to clock the char without changing TMS
		XMitDr8Type::TmsGen::SuspendAutoOutput suspend;
		// Drive TDI high, while pins are disabled
		{
			SpiJtagDevice::PutChar(0xFF);
		}
		// Drive MCU outputs on
		// Switch to SPI
		JtagSpiOn::Setup();
	}
	//TmsShapeGpioOut::Setup();
	// Hardware buffers driving sbw lines (JTAG comes after bus is acquire)
	SetBusState(BusState::sbw);
	// Wait to settle
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnReleaseJtag()
{
	{
		MuteSpiClk scope;
		SpiJtagDevice::PutChar(0xFF);
	}
	// slau320: StopJtag
	JTEST::SetLow();
	JTCK::SetHigh();
	// Hardware buffers in tri state
	SetBusState(BusState::off);
	// Put MCU pins in 3-state
	JtagOff::SetupPinMode();
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnEnterTap()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
									 \______/
			________             ____
	RST  __|        |___________|
				  _____    __________
	TEST ________|     |__|
	*/

	JRST::SetLow();
	JTEST::SetLow();		//1
	StopWatch().Delay<Msec(4)>(); // reset TEST logic

	JRST::SetHigh();		//2
	JTEST::SetHigh();		//3
	StopWatch().Delay<Msec(20)>(); // activate TEST logic

	// phase 1
	JRST::SetLow();			//4
	StopWatch().Delay<Usec(50)>();

	// phase 2 -> TEST pin to 0, no change on RST pin
	// for 4-wire JTAG clear Test pin
	JTEST::SetLow();		//5

	// phase 3
	StopWatch().Delay<Usec(1)>();

	// phase 4 -> TEST pin to 1, no change on RST pin
	// for 4-wire JTAG
	JTEST::SetHigh();		//7
	StopWatch().Delay<Msec(40)>();

	// phase 5
	JRST::SetHigh();
	// Hardware buffers driving JTAG lines
	SetBusState(BusState::jtag);
	StopWatch().Delay<Msec(5)>();
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
	WATCHPOINT();
	//initialize it to high level
	XMitDr8Type::TmsGen::SuspendAutoOutput suspend;
	XMitDr8Type::TmsGen::Set();
	XMitDr8Type::TmsGen::ConfigForResetTap(s_anticipate_clock);
	SpiJtagDevice::PutChar(0xFF); // Keep TDI up
	XMitDr8Type::TmsGen::Stop();

	StopWatch().Delay<Usec(10)>();

	XMitDr8Type::TmsGen::Pulse(false);
	StopWatch().Delay<Usec(5)>();
	XMitDr8Type::TmsGen::Pulse(false);
	StopWatch().Delay<Usec(5)>();
	XMitDr8Type::TmsGen::Pulse(true);	// Restore toggle mode
}


uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	XMitIr8Type::Transmit(instruction, s_anticipate_clock);
	return XMitIr8Type::Decode();
	// JTAG state = Run-Test/Idle
}


uint8_t JtagDev::OnDrShift8(uint8_t data)
{
	XMitDr8Type::Transmit(data, s_anticipate_clock);
	return XMitDr8Type::Decode();
	/* JTAG state = Run-Test/Idle */
}


uint16_t JtagDev::OnDrShift16(uint16_t data)
{
	XMitDr16Type::Transmit(data, s_anticipate_clock);
	return XMitDr16Type::Decode();
	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	XMitDr20Type::Transmit(data, s_anticipate_clock);
	data = XMitDr20Type::Decode();

	return ((data << 16) + (data >> 4)) & 0x000FFFFF;
	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	XMitDr32Type::Transmit(data, s_anticipate_clock);
	return XMitDr32Type::Decode();
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


void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_SPI

	// Mute JCLK
	MuteSpiClk mute;
	// Sets the SPI to the speed required for JTCLK generation
	SpiJtmsWave::Disable();
	Spi::RawSpiSpeed oldspeed = SpiJtmsWave::SetupSpeed();
	SpiJtmsWave::Enable();
	// Send pulses up to the minimal required
	for (uint32_t pulses = 0; pulses < min_pulses; pulses += kNumPeriods)
		SpiJtmsWave::PutStream(g_JtmsWave, _countof(g_JtmsWave));
	// Restore SPI to previous speed
	SpiJtmsWave::DisableSafe();
	SpiJtmsWave::RestoreSpeed(oldspeed);
	SpiJtmsWave::Enable();

#elif USE_JTCLK_TIMDMA

	// Enable GPIO mode for TDI pin	
	JTCLK::SetupPinMode();
	// Run and wait
	JtclkWaveGen::RunEx(min_pulses);
	// Let SPI take control again
	JTCLK_SPI::SetupPinMode();
	// Regardless of state where it stopped, keep GPIO always high
	JTCLK::SetHigh();

#endif // OPT_JTAG_TCLK_IMPLEMENTATION
}

#endif // OPT_INCLUDE_JTAG_SPI_
