
#include "stdproj.h"

#if ! OPT_JTAG_USING_SPI

#include "JtagDev.h"


 // JTMS and JTCK shall be on the same port, for performance reason
static_assert(JTMS::kPortBase_ == JTCK::kPortBase_, "Same port required for performance reason");
// JTMS and JTDI shall be on the same port, for performance reason
static_assert(JTMS::kPortBase_ == JTDI::kPortBase_, "Same port required for performance reason");

#if 1
#define jtag_tms_set(p)		JTMS::SetHigh()
#define jtag_tms_clr(p)		JTMS::SetLow()
#define jtag_tck_set(p)		JTCK::SetHigh()
#define jtag_tck_clr(p)		JTCK::SetLow()
#define jtag_tdi_set(p)		JTDI::SetHigh()
#define jtag_tdi_clr(p)		JTDI::SetLow()
#define jtag_tclk_set(p)	JTCLK::SetHigh()
#define jtag_tclk_clr(p)	JTCLK::SetLow()
#define jtag_rst_set(p)		JRST::SetHigh()
#define jtag_rst_clr(p)		JRST::SetLow()
#define jtag_tst_set(p)		JTEST::SetHigh()
#define jtag_tst_clr(p)		JTEST::SetLow()

#define jtag_tclk_get(p)	(JTCLK::Get() != 0)
#define jtag_tdo_get(p)		(JTDO::Get() != 0)

#define ClrTMS()			JTMS::SetLow()
#define SetTMS()			JTMS::SetHigh()
#define ClrTCLK()			JTCLK::SetLow()
#define SetTCLK()			JTCLK::SetHigh()
#define ClrTCK()			JTCK::SetLow()
#define SetTCK()			JTCK::SetHigh()
#define ClrRST()			JRST::SetLow()
#define SetRST()			JRST::SetHigh()
#define ClrTST()			JTEST::SetLow()
#define SetTST()			JTEST::SetHigh()
#endif


 // Maps set/reset bit on port BSRR (Port bit set/reset register)
static constexpr uint32_t tms0 = JTMS::kBitValue_ << 16;
static constexpr uint32_t tms1 = JTMS::kBitValue_;
static constexpr uint32_t tdi0 = JTDI::kBitValue_ << 16;
static constexpr uint32_t tdi1 = JTDI::kBitValue_;
static constexpr uint32_t tck0 = JTCK::kBitValue_ << 16;
static constexpr uint32_t tck1 = JTCK::kBitValue_;
static constexpr uint32_t tclk0 = JTCLK::kBitValue_ << 16;
static constexpr uint32_t tclk1 = JTCLK::kBitValue_;
static constexpr uint32_t tms0tck0 = tms0 | tck0;
static constexpr uint32_t tms0tck1 = tms0 | tck1;
static constexpr uint32_t tms1tck0 = tms1 | tck0;
static constexpr uint32_t tms1tck1 = tms1 | tck1;


/// Time Base for the JTCLK generation
typedef InternalClock_Hz<kTimForJtclkCnt, SysClk, 4 * 470000> JtclkTiming; // MSP430 max freq is 476kHz
/// Time base is managed by prescaler, so use just one step
typedef TimerTemplate<JtclkTiming, TimerMode::kUpCounter, 1> JtclkTimer;

/// A DMA channel for JTCLK clock generation
template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaForJtagWave : public DmaChannel
	<JtclkTimer::DmaInstance_
	, JtclkTimer::DmaCh_
	, DIRECTION
	, SRC_PTR
	, DST_PTR
	, PRIO>
{
public:
};

/// Wave generation needs a circular DMA
typedef DmaForJtagWave<kDmaMemToPerCircular, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> JtclkDmaCh;
/// Single series of DMA transfers
typedef DmaForJtagWave<kDmaMemToMem, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> TableToGpioDma;


#if 0
// Example of DMA transfer (~2.3 MHz for Flash table; ~3.7MHz for RAM table)
static const uint32_t tab[] =
{
	tdi1
	, tdi0 | tck1
	, tdi1
	, tdi0 | tck0
	, tdi1
	, tdi0 | tck1
	, tdi1
	, tdi0 | tck0
	, tdi1
	, tdi0 | tck1
};
typedef DmaChannel<Dma::k1, kDmaCh2, kDmaMemToMem, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> PulseModDma;
void DoInit()
{
	PulseModDma::Init();
	PulseModDma::SetSourceAddress(tab);
	PulseModDma::SetDestAddress(&(JTDI::Io()->BSRR));
}
void DoRun()
{
	PulseModDma::SetTransferCount(_countof(tab));
	PulseModDma::Enable();
	PulseModDma::WaitTransferComplete();
	PulseModDma::Disable();
}
// -O0: 800 kHz; -Og: 837 kHz; -O1: 1.6 MHz
void DoRunBitBang()
{
	for (int i = 0; i < _countof(tab); ++i)
	{
		JTDI::Io()->BSRR = tab[i];
	}
}
#endif
#if 0
static const uint32_t tab[] =
{
	tdi1
	, tdi0 | tck1
	, tdi1
	, tdi0 | tck0
	, tdi1
};
// Example of timer controlled DMA transfer (up to 1.8 MHz)
typedef InternalClock_Hz<kTimForJtag, SysClk, 6000000> PulseModTimeBase;
typedef TimerTemplate<PulseModTimeBase, kSingleShot, 1> PulseMod;
typedef TimerOutputChannel<PulseMod, TimChannel::k1> PulseModOut;
typedef DmaForJtagWave<kDmaMemToPerCircular, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> PulseModDma;
void DoInit()
{
	PulseMod::Init();
	PulseModOut::Init();
	PulseModOut::SetCompare(0);
	PulseModDma::Init();
	PulseModDma::Start(tab, &(JTDI::Io()->BSRR), _countof(tab));
	PulseModOut::EnableDma();
}
void DoRun()
{
	PulseMod::StartRepetition(40);
	PulseMod::WaitForAutoStop();
}
#endif


//static void RunBitBangPoll(const uint32_t *tab, const uint32_t samps) NO_INLINE;
ALWAYS_INLINE static void RunBitBangDma(const uint32_t *tab, const uint32_t samps);

#if 0
//! Runs a bit-bang from a table (this ensures clocks < 10 MHz)
static void RunBitBangPoll(const uint32_t *tab, const uint32_t samps)
{
	volatile GPIO_TypeDef *port = JTCK::Io();
	for (uint32_t i = 0; i < samps; ++i)
		port->BSRR = tab[i];
}
#endif

//! Runs a bit-bang from a table
/*!
** This is a method to control speed of bit banging. If you write bit banging
** using the optimized compiler, constants and pointers are loaded into registers
** and Performance in some pulses will be above 12 MHz or even more, which is far
** above the 10 MHz range of MSP430 MCU. It may work with the newer faster parts 
** but it is not recommended. Using the DMA and reading from a table in Flash, 
** happens two accesses to the bus, one for read and other for write, which 
** results speeds below 4 MHz, and results very nice shaped pulses.
*/
static void RunBitBangDma(const uint32_t *tab, const uint32_t samps)
{
	TableToGpioDma::Setup();
	TableToGpioDma::Start(tab, &(JTDI::Io()->BSRR), samps);
}

//! Runs a bit-bang from a table (this ensures clocks < 10 MHz)
ALWAYS_INLINE static void WaitBitBangDma()
{
	TableToGpioDma::WaitTransferComplete();
	TableToGpioDma::Disable();
}


bool JtagDev::OnOpen()
{
	// Initialize DMA timer (do not add multiple for shared timer channel!)
	JtclkTimer::Init();
	// Timer should trigger the DMA, every count
	JtclkTimer::EnableTriggerDma();
	// Initialize DMA (do not add multiple for shared DMA channel!)
	JtclkDmaCh::Init();
	JtclkDmaCh::SetDestAddress(&(JTDI::Io()->BSRR));

	// JUST FOR A CASUAL TEST USING LOGIC ANALYZER
#define TEST_WITH_LOGIC_ANALYZER 0
#if TEST_WITH_LOGIC_ANALYZER
	WATCHPOINT();
	JtagOn::Enable();
	InterfaceOn();
	jtag_tck_clr(p);
	//jtag_tclk_clr(p);
	__NOP();
	//jtag_tclk_set(p);
	//jtag_tms_clr(p);
	jtag_tck_set(p);

	for(int i = 0; i < 20; ++i)
		__NOP();
	OnDrShift8(IR_CNTRL_SIG_RELEASE);
	OnDrShift16(0x1234);
	OnDrShift20(0x12345);
	for (int i = 0; i < 100; ++i)
		__NOP();
	InterfaceOff();
	JtagOff::Enable();
	assert(false);
#endif
	return true;
}


void JtagDev::OnClose()
{
	InterfaceOff();
	JtagOff::Enable();
}


void JtagDev::OnConnectJtag()
{
	// slau320: ConnectJTAG / DrvSignals
	JtagOn::Enable();
	InterfaceOn();
	//JENABLE::SetHigh();
	JTEST::SetHigh();
	StopWatch().Delay<10>();
}


void JtagDev::OnReleaseJtag()
{
	// slau320: StopJtag
	JTEST::SetLow();
	InterfaceOff();
	JtagOff::Enable();
	//JENABLE::SetLow();
	StopWatch().Delay<10>();
}


void JtagDev::OnEnterTap()
{
	unsigned int jtag_id;

#if 0
	jtag_rst_clr(p);
	p->f->jtdev_power_on(p);
#if 0
	jtag_tdi_set(p);
	jtag_tms_set(p);
	jtag_tck_set(p);
	jtag_tclk_set(p);
#endif
	/*
			________             ____
	RST  __|        |___________|
				  _____    __________
	TEST ________|     |__|
	*/
	StopWatch().DelayUS<4>();

	jtag_rst_set(p);
	jtag_tst_clr(p);
	StopWatch().DelayUS<5>();
	jtag_tst_set(p);
	StopWatch().DelayUS<5>();
	jtag_rst_clr(p);
	jtag_tst_clr(p);
	StopWatch().DelayUS<5>();
	jtag_tst_set(p);

	p->f->jtdev_connect(p);
	jtag_rst_set(p);
	StopWatch().DelayUS<5>();
#elif 1			// slau320
	ClrTST();		//1
	StopWatch().Delay<4>();		// reset TEST logic

	SetRST();		//2

	SetTST();		//3
	StopWatch().Delay<20>();	// activate TEST logic

	// phase 1
	ClrRST();		//4
	StopWatch().DelayUS<60>();

	// phase 2 -> TEST pin to 0, no change on RST pin
	// for 4-wire JTAG clear Test pin
	ClrTST();		//5

	// phase 3
	StopWatch().DelayUS<1>();

	// phase 4 -> TEST pin to 1, no change on RST pin
	// for 4-wire JTAG
	SetTST();		//7
	StopWatch().DelayUS<60>();

	// phase 5
	SetRST();
	StopWatch().Delay<5>();
#else
	/*-------------RstLow_JTAG----------------
				________           __________
	Test ______|        |_________|
							  _______________
	Rst_____________________|
	----------------------------------------*/
	CriticalSection lock;
	{
		jtag_tst_clr(p);
		StopWatch().DelayUS<5>();
		jtag_tst_set(p);
		StopWatch().DelayUS<5>();
		jtag_rst_clr(p);
		StopWatch().DelayUS<5>();
		jtag_tst_clr(p);		// Enter JTAG 4w
		StopWatch().DelayUS<2>();
		jtag_tst_set(p);
		StopWatch().DelayUS<5>();
		jtag_rst_set(p);
		StopWatch().DelayUS<100>();
#if 0
	else
	{
		WATCHPOINT();
		jtag_tst_clr(p);			//1
		StopWatch().Delay<4>();

		jtag_rst_set(p);			//2
		jtag_tst_set(p);			//3
		StopWatch().Delay<20>();

		jtag_rst_clr(p);			//4
		StopWatch().Delay<60>();

		// for 4-wire JTAG clear Test pin Test(0)
		jtag_tst_clr(p);			//5
		StopWatch().DelayUS<1>();

		// for 4-wire JTAG - Test (1)
		jtag_tst_set(p);
		StopWatch().DelayUS<60>();

		// phase 5 Reset(1)
		jtag_rst_set(p);
		StopWatch().DelayUS<500>();
		}
#endif
	}
#endif
}


/*!
Reset target JTAG interface and perform fuse-HW check
*/
void JtagDev::OnResetTap()
{
	WATCHPOINT();
	jtag_tms_set(p);
	jtag_tck_set(p);

#if 0
	/* Perform fuse check */
	jtag_tms_clr(p);
	jtag_tms_set(p);
	jtag_tms_clr(p);
	jtag_tms_set(p);
#endif

	/* Reset JTAG state machine */
	for (int loop_counter = 6; loop_counter > 0; loop_counter--)
	{
		//StopWatch().DelayUS<10>();
		jtag_tck_clr(p);
		//StopWatch().DelayUS<10>();
		jtag_tck_set(p);
	}

	/* Set JTAG state machine to Run-Test/IDLE */
	jtag_tms_clr(p);
	jtag_tck_clr(p);
	StopWatch().DelayUS<10>();

	jtag_tms_set(p);
	jtag_tms_clr(p);
	StopWatch().DelayUS<5>();
	jtag_tms_set(p);
	jtag_tms_clr(p);
	StopWatch().DelayUS<5>();
	jtag_tms_set(p);
	jtag_tms_clr(p);

	jtag_tck_set(p);
}

// Slow speed constants for better shaped waves
static volatile const uint32_t tck0_s = tck0;
static volatile const uint32_t tck1_s = tck1;
static volatile const uint32_t tclk0_s = tclk0;
static volatile const uint32_t tclk1_s = tclk1;
static volatile const uint32_t tms0_s = tms0;
static volatile const uint32_t tms0tck0_s = tms0tck0;

/*!
Shift a value into TDI (MSB first) and simultaneously shift out a value from TDO (MSB first)

\param num_bits: number of bits to shift
\param data_out: data to be shifted out
\return: scanned TDO value
*/
static uint32_t JtagShift(uint8_t num_bits, uint32_t data_out)
{
	volatile GPIO_TypeDef *port = JTDI::Io();
	bool tclk_save = JTCLK::Get();

	uint32_t data_in = 0;
	uint32_t mask = 0x0001U << (num_bits - 1);
	WaitBitBangDma();
	while ( true )
	{
		uint32_t cmd = tck0;
		if ((data_out & mask) != 0)
			cmd |= tdi1;
		else
			cmd |= tdi0;
		if (mask == 1)
		{
			cmd |= tms1;
			port->BSRR = cmd;
			break;
		}
		port->BSRR = cmd;
		port->BSRR = tck0_s;	// just to create a larger pulse shape
		port->BSRR = tck1_s;
		if (JTDO::Get() != 0)
			data_in |= mask;
		mask >>= 1;
	}
	__NOP();	// required to make the pulse width at least 50 ns
	port->BSRR = tck1;
	data_in |= (JTDO::Get() != 0);


	/*!
	This function sets the target JTAG state machine
	back into the Run-Test/Idle state after a shift access
	*/
	/*
	** Write sequence
	** TCK:  ___|"""|___|"""
	** TMS:  """""""|_______
	*/
	port->BSRR = tclk_save ? tck0 | tclk1 : tck0| tclk0;
	port->BSRR = tck1_s;
	port->BSRR = tms0tck0_s;
	port->BSRR = tck0_s;	// added for better wave shape
	port->BSRR = tck1_s;

	/* JTAG state = Run-Test/Idle */

	return data_in;
}


// Table on RAM for 4 MHz performance
static uint32_t entry_ir[] =
{
	tms1tck0	// Run-Test/Idle
	, tms1tck1
	, tms1tck0	// Select DR-Scan
	, tms1tck1
	, tms0tck0	// Select IR-Scan
	, tms0tck1
	, tms0tck0	// Capture-IR
	, tms0tck1
};


ALWAYS_INLINE static void EntryIr_()
{
	GPIO_TypeDef *port = (GPIO_TypeDef *)JTMS::kPortBase_;

	/*
	** Write sequence
	** TCK:  ¯|___|¯¯¯|___|¯¯¯|___|¯¯¯|___|¯¯¯
	** TMS:  _|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|_______________
	*/

	RunBitBangDma(entry_ir, _countof(entry_ir));
}


/*!
Shifts a new instruction into the JTAG instruction register through TDI
MSB first, with interchanged MSB/LSB, to use the shifting function

\param instruction: 8 bit instruction
\return: scanned TDO value
*/
uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	EntryIr_();

	/* JTAG state = Shift-IR, Shift in TDI (8-bit) */
	uint8_t res = JtagShift(8, instruction);
	return res;

	/* JTAG state = Run-Test/Idle */
}


// Table on RAM for 4 MHz performance
static uint32_t entry_dr[] =
{
	tms1tck0	// Run-Test/Idle
	, tms1tck1
	, tms0tck0	// Select DR-Scan
	, tms0tck1
	, tms0tck0	// Capture-DR
	, tms0tck1
};

ALWAYS_INLINE static void EntryDr_()
{
	GPIO_TypeDef *port = (GPIO_TypeDef *)JTMS::kPortBase_;

	/*
	** Write sequence
	** TCK:  "|___|"""|___|"""|___|"""
	** TMS:  _|"""""""|_______________
	*/

	RunBitBangDma(entry_dr, _countof(entry_dr));
}


/*!
Shifts a given 8-bit byte into the JTAG data register through TDI.

\param data  : 8 bit data
\return: scanned TDO value
*/
uint8_t JtagDev::OnDrShift8(uint8_t data)
{
	EntryDr_();

	/* JTAG state = Shift-DR, Shift in TDI (16-bit) */
	return JtagShift(8, data);

	/* JTAG state = Run-Test/Idle */
}


/*!
Shifts a given 16-bit word into the JTAG data register through TDI.

\param data  : 16 bit data
\return: scanned TDO value
*/
uint16_t JtagDev::OnDrShift16(uint16_t data)
{
	EntryDr_();

	/* JTAG state = Shift-DR, Shift in TDI (16-bit) */
	return JtagShift(16, data);

	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	EntryDr_();

	//data = ((data & 0xFFFF) << 4) | (data >> 16);

	/* JTAG state = Shift-DR, Shift in TDI (20-bit) */
	data = JtagShift(20, data);

	/* JTAG state = Run-Test/Idle */
	data = ((data << 16) + (data >> 4)) & 0x000FFFFF;
	return data;
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	EntryDr_();

	//data = ((data & 0xFFFF) << 4) | (data >> 16);

	/* JTAG state = Shift-DR, Shift in TDI (20-bit) */
	data = JtagShift(32, data);

	/* JTAG state = Run-Test/Idle */
	return data;
}


bool JtagDev::IsInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_CAPTURE);
	if ((OnDrShift16(0) & (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ)) != (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ))
		return false;
	return true;
}


bool JtagDev::OnInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_LOW_BYTE);
	OnDrShift8(CNTRL_SIG_READ);
	jtag_tclk_set(p);

	for (int i = 0; i < 10; i++)
	{
		if (IsInstrLoad())
			return true;
		jtag_tclk_set(p);
		jtag_tclk_clr(p);
	}
	return false;
}


void JtagDev::OnSetTclk()
{
	jtag_tclk_set(p);
}


void JtagDev::OnClearTclk()
{
	jtag_tclk_clr(p);
}


void JtagDev::OnPulseTclk()
{
	volatile GPIO_TypeDef *port = JTMS::Io();
	port->BSRR = tclk1;
	port->BSRR = tclk0_s;
}


void JtagDev::OnPulseTclk(int count)
{
	volatile GPIO_TypeDef *port = JTMS::Io();
	for(int i = 0 ; i < count; ++i)
	{
		port->BSRR = tclk1;
		port->BSRR = tclk0_s;
	}
}


void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
	static const uint32_t bsrr_table[] =
	{
		JTCLK::kBitValue_			// set bit
		, JTCLK::kBitValue_ << 16	// reset bit
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
		, JTCLK::kBitValue_
		, JTCLK::kBitValue_ << 16
	};

	// Configure timer for pulse generation every timer cycle
	JtclkDmaCh::SetSourceAddress(bsrr_table);
	JtclkDmaCh::SetTransferCount(_countof(bsrr_table));
	JtclkDmaCh::Enable();
	// Table has 8 cycles; round up for the next 8 cycle count
	if (min_pulses & 0x00000003)
		min_pulses += 8;
	min_pulses = min_pulses >> 3;

	// Max timer value
	JtclkTimer::EnableUpdateDma();
	JtclkTimer::StartShot();
	uint16_t last = _countof(bsrr_table);
	// Repeat until no more pulses are required
	while (min_pulses)
	{
		uint16_t curr = JtclkDmaCh::GetTransferCount();
		// Timer is circular and every time hardware wraps around we decrement counter
		if (curr > last)
			--min_pulses;
		last = curr;
	}
	// Freeze timer and DMA
	JtclkTimer::CounterStop();
	JtclkTimer::DisableUpdateDma();
	JtclkDmaCh::Disable();
}


void JtagDev::OnPulseTclkN()
{
	volatile GPIO_TypeDef *port = JTMS::Io();
	port->BSRR = tclk0;
	port->BSRR = tclk1_s;
}


void JtagDev::OnTclk(DataClk tclk)
{
	volatile GPIO_TypeDef *port = JTMS::Io();
	switch (tclk)
	{
	case kdTclk0:
		JTCLK::SetLow();
		break;
	case kdTclk1:
		JTCLK::SetHigh();
		break;
	case kdTclk2P:
		port->BSRR = tclk1;
		port->BSRR = tclk0_s;
		// FALL THROUGH
	case kdTclkP:
		port->BSRR = tclk1;
		port->BSRR = tclk0_s;
		break;
	case kdTclk2N:
		port->BSRR = tclk0;
		port->BSRR = tclk1_s;
		// FALL THROUGH
	case kdTclkN:
		port->BSRR = tclk0;
		port->BSRR = tclk1_s;
		break;
	default:
		break;
	}
}


uint16_t JtagDev::OnData16(DataClk clk0, uint16_t data, DataClk clk1)
{
	OnIrShift(IR_DATA_16BIT);
	
	OnTclk(clk0);
	data = OnDrShift16(data);
	OnTclk(clk1);
	return data;
}


#if 0
void JtagDev::OnClockThroughPsa()
{
	/* Clock through the PSA */
	if (mspArch_ == ChipInfoDB::kCpuXv2)
	{
		static const uint32_t tab[] =
		{
			tclk0
			, tck0 | tms1
			, tck1			// Select DR scan
			, tck0 | tms0
			, tck1			// Capture DR
			, tck0
			, tck1			// Shift DR
			, tck0 | tms1
			, tck1			// Exit DR
			, tck0

			, tck1			// Set JTAG FSM back into Run-Test/Idle
			, tck0 | tms0
			, tck1

			, tclk1
		};
		RunBitBangDma(tab, _countof(tab));
		WaitBitBangDma();
	}
	else
	{
		static const uint32_t tab[] =
		{
			tclk1
			, tck0 | tms1
			, tck1
			, tck0 | tms0
			, tck1
			, tck0
			, tck1
			, tck0 | tms1
			, tck1
			, tck0
			, tck1
			, tck0 | tms0
			, tck1
			, tclk0
		};
		RunBitBangDma(tab, _countof(tab));
		WaitBitBangDma();
	}
}
#endif

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
	constexpr uint16_t sJMBINCTL = INREQ;
	const uint16_t sJMBIN0 = dataX;
	const SysTickUnits duration = TickTimer::M2T<25>::kTicks;

	StopWatch stopwatch;

	OnIrShift(IR_JMB_EXCHANGE);
	do
	{
		// Timeout
		if (stopwatch.GetEllapsedTicks() > duration)
		{
#if DEBUG
			McuCore::Abort();
#endif // DEBUG
			return false;
		}
	} while (!(OnDrShift16(0x0000) & IN0RDY));
	OnDrShift16(sJMBINCTL);
	OnDrShift16(sJMBIN0);
	return true;
}

#endif // JTAG_USING_SPI
