
#include "stdproj.h"

#if OPT_INCLUDE_JTAG_DTRIG_

#include "JtagDev.h"
#include "util/TimDmaWave.h"
#include "util/DtrigJtag.h"

using namespace Bmt::Dma;
using namespace Bmt::Timer;
using namespace JtagFrame;
using namespace WaveJtag;


/// CNT preset values that align TIM1's first TMS DMA event with the correct SPI bit edge.
/// Tune these per speed grade with a logic analyzer; default 0 is a safe starting point.
static constexpr uint16_t kDtrigCntOffset_1 = 1; ///< 0.5625 MHz
static constexpr uint16_t kDtrigCntOffset_2 = 7; ///< 1.125 MHz
static constexpr uint16_t kDtrigCntOffset_3 = 6; ///< 2.25 MHz
static constexpr uint16_t kDtrigCntOffset_4 = 6; ///< 4.5 MHz
static constexpr uint16_t kDtrigCntOffset_5 = 6; ///< 9 MHz


// ── SPI device templates (one per speed grade) ───────────────────────────────

template<const uint32_t SPEED>
struct SpiJtagDevType
	: Spi::SpiTemplate
		<
		kSpiForJtag
		, SysClk
		, SPEED
		, Spi::Role::kMaster
		, Spi::ClkPol::kMode3	// CPOL=1 CPHA=1: SCK idles high, data sampled on falling edge
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
	static_assert(BASE::kInputClock_ / 8 >= SPEED, "Clock speed out of range");
};

typedef SpiJtagDevType<JTCK_Speed_1> SpiJtagDev_1;
typedef SpiJtagDevType<JTCK_Speed_2> SpiJtagDev_2t;
typedef SpiJtagDevType<JTCK_Speed_3> SpiJtagDev_3t;
typedef SpiJtagDevType<JTCK_Speed_4> SpiJtagDev_4t;
typedef SpiJtagDevType<JTCK_Speed_5> SpiJtagDev_5t;


// ── DtrigJtag type aliases ────────────────────────────────────────────────────

// GoIdle uses grade-1 SPI (slow, safe for TAP reset from any state)
using DtrigGoIdle = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1, JTCK_Speed_1, Scan::kGoIdle, NumBits::kGoIdle>;

// All scan types use grade-1 template for Start/Wait/GetResult — the actual SPI
// baud rate is configured at open time via the appropriate DtrigInit_N::Init().
using DtrigIr8  = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1, JTCK_Speed_1, Scan::kIR, NumBits::k8>;
using DtrigDr8  = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1, JTCK_Speed_1, Scan::kDR, NumBits::k8>;
using DtrigDr16 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1, JTCK_Speed_1, Scan::kDR, NumBits::k16>;
using DtrigDr20 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1, JTCK_Speed_1, Scan::kDR, NumBits::k20>;
using DtrigDr32 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1, JTCK_Speed_1, Scan::kDR, NumBits::k32>;

// Per-grade Init-only instantiations: each sets TIM1 PSC and SPI BAUD for its grade
using DtrigInit_1 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_1,  JTCK_Speed_1, Scan::kDR, NumBits::k8>;
using DtrigInit_2 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_2t, JTCK_Speed_2, Scan::kDR, NumBits::k8>;
using DtrigInit_3 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_3t, JTCK_Speed_3, Scan::kDR, NumBits::k8>;
using DtrigInit_4 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_4t, JTCK_Speed_4, Scan::kDR, NumBits::k8>;
using DtrigInit_5 = DtrigJtag<SysClk, kWaveJtagTimer, kWaveJtagTms,
	SpiJtagDev_5t, JTCK_Speed_5, Scan::kDR, NumBits::k8>;

static_assert(DtrigDr32::kTotalClocks + 1 < JtagDev::kAuxBufSize_,
	"TMS buffer (reusing read_buf_) is too small for a 32-bit DR scan");

// SPI1_TX → DMA1_CH3, SPI1_RX → DMA1_CH2, TIM1_CH3 → DMA1_CH6: verify no conflicts
static_assert(DtrigDr8::DmaTms::kChan_ != DtrigDr8::SpiTxDma::kChan_,
	"TMS DMA conflicts with SPI TX DMA - choose a different kWaveJtagTms");
static_assert(DtrigDr8::DmaTms::kChan_ != DtrigDr8::SpiRxDma::kChan_,
	"TMS DMA conflicts with SPI RX DMA - choose a different kWaveJtagTms");


// ── JtclkWaveGen (TIM2+TIM3 for MSP430 flash clock) ─────────────────────────
// TIM3_UP → DMA1_CH3 (= SPI1 TX), TIM2_UP → DMA1_CH2 (= SPI1 RX).
// Both SPI1 DMA channels must be released before calling JtclkWaveGen::RunEx().

typedef TimDmaWav<
	SysClk
	, kTimDmaWavBeat		// TIM2
	, kTimForJtclkCnt		// TIM3
	, kTimDmaWavFreq
	, 2						// two edges per pulse (set + reset)
	> JtclkWaveGen;


// ── Static storage ────────────────────────────────────────────────────────────

// Active-grade CNT offset, updated by each OnOpen()
static uint16_t s_cnt_offset = kDtrigCntOffset_1;

// BSRR table for JtclkWaveGen (JTCLK = JTDI = PA7 in GPIO mode)
static const uint32_t s_bsrr_table[] =
{
	JTCLK::kBitValue_ << 16,	// reset JTCLK (low)
	JTCLK::kBitValue_,			// set JTCLK (high)
};


// ── Hardware mode state machine ───────────────────────────────────────────────
//
// The dtrig driver multiplexes PA5/PA6/PA7 between two configurations:
//
//   kSpi:  PA5→SPI1_SCK (JTCK), PA6→SPI1_MISO (JTDO), PA7→SPI1_MOSI (JTDI/JTCLK)
//   kGpio: PA5→JTCK GPIO out,   PA6→JTDO GPIO in,      PA7→JTDI GPIO out
//
// JTMS (PB14) is always GPIO and is not managed here.
// JRST (PB0) and JTEST (PB1) are always GPIO and are not managed here.
//
// TDI no-transition rule: PA7 (JTDI/JTCLK) must not glitch during a mode switch,
// because a level change on TDI while the TAP is in Run-Test/Idle is interpreted as
// a JTCLK edge by the MSP430 CPU.  The helpers below enforce this:
//
//   SPI → GPIO: PutChar(0xFF) settles MOSI=PA7 HIGH; JtagGpioOn sets PA7 output HIGH.
//   GPIO → SPI: JTDI::SetHigh() ensures ODR=1; JtagSpiOn switches to AF; PutChar(0xFF)
//               drives MOSI HIGH on the first bit and also settles SCK idle-HIGH.
//               The 8 resulting JTCK pulses are safe because TMS=0 keeps the TAP in RTI.

enum class HwMode : uint8_t { kOff, kGpio, kSpi };
static HwMode s_hw_mode = HwMode::kOff;

/// Switch PA5/6/7 to SPI AF mode.  Idempotent: no-op if already in SPI mode.
static ALWAYS_INLINE void AcquireSpiMode()
{
	if (s_hw_mode != HwMode::kSpi)
	{
		JTDI::Set(JTCLK::Get());   // ensure ODR is JTCLK
		JtagSpiOn::SetupPinMode();	// PA5→SCK, PA6→MISO, PA7→MOSI (SPI1 AF)
		s_hw_mode = HwMode::kSpi;
	}
}

/// Switch PA5/6/7 to GPIO mode.  Idempotent: no-op if already in GPIO mode.
static ALWAYS_INLINE void AcquireGpioMode()
{
	if (s_hw_mode != HwMode::kGpio)
	{
		JTDI::Set(JTCLK::Get());	// ensure ODR is JTCLK
		JtagGpioOn::SetupPinMode(); // PA5→JTCK, PA6→JTDO in, PA7→JTDI
		s_hw_mode = HwMode::kGpio;
	}
}

class MuteSpiClk
{
  public:
	ALWAYS_INLINE MuteSpiClk()
	{
		JTCK::SetHigh(); // SPI clk defaults; this is used while muting JTCK_SPI
		JTCK::SetupPinMode();
	}
	ALWAYS_INLINE ~MuteSpiClk()
	{
		JTCK_SPI::SetupPinMode();
	}
};


// ── Constructors ──────────────────────────────────────────────────────────────

JtagDev::JtagDev()   {}
JtagDev_2::JtagDev_2() {}
JtagDev_3::JtagDev_3() {}
JtagDev_4::JtagDev_4() {}
JtagDev_5::JtagDev_5() {}


// ── JtagDev virtual method implementations ────────────────────────────────────

bool JtagDev::OnAnticipateTms() const { return false; }


bool JtagDev::OnOpen()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
			  \__/
	*/
	s_hw_mode = HwMode::kOff;
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, s_bsrr_table, _countof(s_bsrr_table));
	DtrigDr32::ReleaseDma();  // JtclkWaveGen may leave DMA1_CH2/CH3 enabled; clear EN before SPI init
	s_cnt_offset = kDtrigCntOffset_1;
	DtrigInit_1::Init();

	// JUST FOR A CASUAL TEST USING LOGIC ANALYZER
#define TEST_WITH_LOGIC_ANALYZER 1
#if TEST_WITH_LOGIC_ANALYZER
	WATCHPOINT();
	OnConnectJtag();
	OnEnterTap();
	OnResetTap();

	OnIrShift(IR_CNTRL_SIG_RELEASE); // 0xA8
	//OnFlashTclk(6);

	// Hardware buffers in tri-state...
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
	s_hw_mode = HwMode::kOff;
	while (true)
		__WFI();

#endif // TEST_WITH_LOGIC_ANALYZER

	return true;
}

bool JtagDev_2::OnOpen()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
			  \__/
	*/
	s_hw_mode = HwMode::kOff;
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, s_bsrr_table, _countof(s_bsrr_table));
	DtrigDr32::ReleaseDma();
	s_cnt_offset = kDtrigCntOffset_2;
	DtrigInit_2::Init();
	return true;
}

bool JtagDev_3::OnOpen()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
			  \__/
	*/
	s_hw_mode = HwMode::kOff;
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, s_bsrr_table, _countof(s_bsrr_table));
	DtrigDr32::ReleaseDma();
	s_cnt_offset = kDtrigCntOffset_3;
	DtrigInit_3::Init();
	return true;
}

bool JtagDev_4::OnOpen()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
			  \__/
	*/
	s_hw_mode = HwMode::kOff;
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, s_bsrr_table, _countof(s_bsrr_table));
	DtrigDr32::ReleaseDma();
	s_cnt_offset = kDtrigCntOffset_4;
	DtrigInit_4::Init();
	return true;
}

bool JtagDev_5::OnOpen()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
		      \__/
	*/
	s_hw_mode = HwMode::kOff;
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, s_bsrr_table, _countof(s_bsrr_table));
	DtrigDr32::ReleaseDma();
	s_cnt_offset = kDtrigCntOffset_5;
	DtrigInit_5::Init();
	return true;
}


void JtagDev::OnClose()
{
	JtagOff::SetupPinMode();
	SpiJtagDev_1::Stop();
	s_hw_mode = HwMode::kOff;
	// Hardware buffers in tri-state...
	SetBusState(BusState::off);
}


/*!
Switch PA5/PA6/PA7 to SPI AF mode and assert JTAG bus active.

AcquireSpiMode() handles pin reconfiguration and TDI settling regardless of the
previous mode (kOff, kGpio, or already kSpi).
*/
void JtagDev::OnConnectJtag()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
	                  \_________/
	*/

	// slau320: ConnectJTAG / DrvSignals
	// This puts the MCU in reset state
	// This puts the MCU in reset state
	JtagOn::Setup();
	s_hw_mode = HwMode::kGpio;
	// Hardware buffers driving sbw lines (JTAG comes after bus is acquire)
	SetBusState(BusState::sbw);
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnReleaseJtag()
{
	// Ensure MOSI settles high before releasing the bus (slau320: StopJtag)
	AcquireSpiMode();
	{
		MuteSpiClk scope;
		SpiJtagDev_1::PutChar(0xFF);
	}
	JTEST::SetLow();
	// Hardware buffers in tri state
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
	s_hw_mode = HwMode::kOff;
	StopWatch().Delay<Msec(10)>();
}


/*!
Enter TAP via TEST/RST reset sequence (slau320 4.2.1).

Only touches JRST (PB0) and JTEST (PB1).  PA5/PA6/PA7 and PB14 are not involved,
so no hardware mode change is needed here.
*/
void JtagDev::OnEnterTap()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
									 \______/
	*/

	JRST::SetLow();
	JTEST::SetLow();
	StopWatch().Delay<Msec(2)>();

	JRST::SetHigh();
	__NOP();
	JTEST::SetHigh();
	StopWatch().Delay<Msec(20)>();

	JRST::SetLow();
	StopWatch().Delay<Usec(40)>();

	JTEST::SetLow();
	StopWatch().Delay<Usec(1)>();

	JTEST::SetHigh();
	StopWatch().Delay<Msec(40)>();

	JRST::SetHigh();
	// Hardware buffers driving JTAG lines
	SetBusState(BusState::jtag);
	StopWatch().Delay<Msec(5)>();
}


/*!
Reset the JTAG TAP and perform fuse-HW check.

Uses DtrigGoIdle (SPI + TIM1) to clock 6× TMS=1 + 1× TMS=0 for a guaranteed
Test-Logic-Reset from any JTAG state, then bit-bangs the fuse-check TMS pulses.
JTMS (PB14) is always GPIO; no mode switch is needed for the fuse-check sequence.
*/
void JtagDev::OnResetTap()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
												 \______/
	*/

	WATCHPOINT();
	AcquireSpiMode();
	JTMS::SetHigh();

	DtrigGoIdle::DoGoIdle((uint8_t *)tx_buf_.GetCurrent(), aux_buf_.GetCurrent(), s_cnt_offset);

	// Fuse-check TMS pulses (slau320)
	JTMS::SetHigh();
	__NOP();
	JTMS::SetLow();
	StopWatch().Delay<Usec(5)>();
	JTMS::SetHigh();
	__NOP();
	JTMS::SetLow();
	StopWatch().Delay<Usec(5)>();
	JTMS::SetHigh();
	__NOP();
	JTMS::SetLow();
}


uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	AcquireSpiMode();
	using R = DtrigIr8;
	uint8_t *tx = tx_buf_.GetNext();
	uint32_t *aux = aux_buf_.GetNext();
	R::RenderTransaction(tx, aux, JTCLK::IsHigh(), instruction);
	// Make sure last frame was sent
	R::Wait();
	tx_buf_.Step();
	rx_buf_.Step();
	aux_buf_.Step();
	uint8_t *rx = rx_buf_.GetCurrent();
	R::Start(tx, rx, aux, s_cnt_offset);
	// TODO: return void and run async
	R::Wait();
	return static_cast<uint8_t>(R::GetResult(rx));
}


uint8_t JtagDev::OnDrShift8(uint8_t data)
{
	AcquireSpiMode();
	using R = DtrigDr8;
	uint8_t *tx = tx_buf_.GetNext();
	uint32_t *aux = aux_buf_.GetNext();
	R::RenderTransaction(tx, aux, JTCLK::IsHigh(), data);
	// Make sure last frame was sent
	R::Wait();
	tx_buf_.Step();
	rx_buf_.Step();
	aux_buf_.Step();
	uint8_t *rx = rx_buf_.GetCurrent();
	R::Start(tx, rx, aux, s_cnt_offset);
	// TODO: return void and run async
	R::Wait();
	return static_cast<uint8_t>(R::GetResult(rx));
}


uint16_t JtagDev::OnDrShift16(uint16_t data)
{
	AcquireSpiMode();
	using R = DtrigDr16;
	uint8_t *tx = tx_buf_.GetNext();
	uint32_t *aux = aux_buf_.GetNext();
	R::RenderTransaction(tx, aux, JTCLK::IsHigh(), data);
	// Make sure last frame was sent
	R::Wait();
	tx_buf_.Step();
	rx_buf_.Step();
	aux_buf_.Step();
	uint8_t *rx = rx_buf_.GetCurrent();
	R::Start(tx, rx, aux, s_cnt_offset);
	// TODO: return void and run async
	R::Wait();
	return static_cast<uint16_t>(R::GetResult(rx));
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	AcquireSpiMode();
	using R = DtrigDr20;
	uint8_t *tx = tx_buf_.GetNext();
	uint32_t *aux = aux_buf_.GetNext();
	R::RenderTransaction(tx, aux, JTCLK::IsHigh(), data);
	// Make sure last frame was sent
	R::Wait();
	tx_buf_.Step();
	rx_buf_.Step();
	aux_buf_.Step();
	uint8_t *rx = rx_buf_.GetCurrent();
	R::Start(tx, rx, aux, s_cnt_offset);
	// TODO: return void and run async
	R::Wait();
	data = R::GetResult(rx);
	return ((data << 16) + (data >> 4)) & 0x000FFFFF;
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	AcquireSpiMode();
	using R = DtrigDr32;
	uint8_t *tx = tx_buf_.GetNext();
	uint32_t *aux = aux_buf_.GetNext();
	R::RenderTransaction(tx, aux, JTCLK::IsHigh(), data);
	// Make sure last frame was sent
	R::Wait();
	tx_buf_.Step();
	rx_buf_.Step();
	aux_buf_.Step();
	uint8_t *rx = rx_buf_.GetCurrent();
	R::Start(tx, rx, aux, s_cnt_offset);
	R::Wait();
	// TODO: return void and run async
	R::Wait();
	return R::GetResult(rx);
}


/*!
Set JTCLK (= JTDI = SPI MOSI) high.

In dtrig mode, SPI SCK (PA5 = JTCK) cannot be gated independently from the SPI
peripheral, so sending a byte causes 8 JTCK pulses.  This is harmless when the JTAG
TAP is in Run-Test/Idle (TMS=0 after the last scan's DMA write); each JTCK pulse
clocks RTI → RTI.  The MSB-first 0xFF byte leaves MOSI (= JTCLK) HIGH after the
last bit.
*/
void JtagDev::OnSetTclk()
{
	AcquireSpiMode();
	MuteSpiClk mute_clk;
	SpiJtagDev_1::PutChar(0xff);
}


void JtagDev::OnClearTclk()
{
	AcquireSpiMode();
	MuteSpiClk mute_clk;
	SpiJtagDev_1::PutChar(0x00);
}


/*!
One TCLK rising transition (low → high).

0xF0 = 0b11110000: MOSI high for first 4 bits then low for last 4.
The falling edge of JTCK (bit 4) with MOSI=0 followed by MOSI=1 at bit 5 is the
rising edge of JTCLK seen by the MSP430.  MOSI ends HIGH after the byte.
*/
void JtagDev::OnPulseTclk()
{
	AcquireSpiMode();
	MuteSpiClk mute_clk;
	SpiJtagDev_1::PutChar(0xf0);
}


/*!
One TCLK falling transition (high → low → high).

0x0F = 0b00001111: MOSI low for first 4 bits, high for last 4.
MOSI ends HIGH after the byte.
*/
void JtagDev::OnPulseTclkN()
{
	AcquireSpiMode();
	MuteSpiClk mute_clk;
	SpiJtagDev_1::PutChar(0x0f);
}


void JtagDev::OnPulseTclk(int count)
{
	AcquireSpiMode();
	SpiJtagDev_1::Repeat(0xF0, count);
}


/*!
Generate high-frequency JTCLK pulses for MSP430 flash operations (~450 kHz).

JtclkWaveGen (TIM2+TIM3+DMA) needs DMA1_CH2 (TIM2_UP) and DMA1_CH3 (TIM3_UP),
which are also used by SPI1_RX and SPI1_TX respectively.  The sequence is:

  1. AcquireGpioMode()  — settle MOSI high, switch PA5/6/7 to GPIO (releases SPI AF)
  2. ReleaseDma()       — free DMA1_CH2 and DMA1_CH3 for JtclkWaveGen
  3. JtclkWaveGen       — drives PA7 (JTCLK) via BSRR DMA at ~450 kHz
  4. SetupDma()         — re-arm DMA channels before SPI reclaim
  5. AcquireSpiMode()   — switch PA5/6/7 back to SPI AF, send 0xFF to settle
*/
void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
	AcquireGpioMode();
	DtrigDr32::ReleaseDma();
	JtclkWaveGen::RunEx(min_pulses);
	JTCLK::SetHigh();          // PA7 idles HIGH after wave stops
	DtrigDr32::SetupDma();
	AcquireSpiMode();
}

#endif // OPT_INCLUDE_JTAG_DTRIG_
