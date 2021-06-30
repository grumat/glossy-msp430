/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009-2012 Daniel Beer
 * Copyright (C) 2012 Peter Bägel
 *
 * ppdev/ppi abstraction inspired by uisp src/DARPA.C
 *   originally written by Sergey Larin;
 *   corrected by Denis Chertykov, Uros Platise and Marek Michalkiewicz.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdproj.h"

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
#define SetTMS()			JTMS::SetLow()
#define ClrTCLK()			JTCLK::SetLow()
#define SetTCLK()			JTCLK::SetLow()
#define ClrTCK()			JTCK::SetLow()
#define SetTCK()			JTCK::SetLow()
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

typedef TimeBase_MHz<kTimForJtag, SysClk, 8 * 400000> FlashStrobeTiming;	// ~400kHz
typedef TimerTemplate<FlashStrobeTiming, kSingleShot, 1> FlashStrobeTimer;
typedef TimerOutputChannel<FlashStrobeTimer, kTimCh1> FlashStrobeCtrl;
typedef DmaForJtagWave<kDmaMemToPerCircular, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> FlashStrobeDma;
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
typedef DmaChannel<kDma1, kDmaCh2, kDmaMemToMem, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> PulseModDma;
void DoInit()
{
	PulseModDma::Init();
	PulseModDma::SetSourceAddress(tab);
	PulseModDma::SetDestAddress(&(JTDI::GetPortBase()->BSRR));
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
		JTDI::GetPortBase()->BSRR = tab[i];
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
typedef TimeBase_MHz<kTimForJtag, SysClk, 6000000> PulseModTimeBase;
typedef TimerTemplate<PulseModTimeBase, kSingleShot, 1> PulseMod;
typedef TimerOutputChannel<PulseMod, kTimCh1> PulseModOut;
typedef DmaForJtagWave<kDmaMemToPerCircular, kDmaLongPtrInc, kDmaLongPtrConst, kDmaHighPrio> PulseModDma;
void DoInit()
{
	PulseMod::Init();
	PulseModOut::Init();
	PulseModOut::SetCompare(0);
	PulseModDma::Init();
	PulseModDma::Start(tab, &(JTDI::GetPortBase()->BSRR), _countof(tab));
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
	volatile GPIO_TypeDef *port = JTCK::GetPortBase();
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
	TableToGpioDma::Start(tab, &(JTDI::GetPortBase()->BSRR), samps);
}

//! Runs a bit-bang from a table (this ensures clocks < 10 MHz)
ALWAYS_INLINE static void WaitBitBangDma()
{
	TableToGpioDma::WaitTransferComplete();
	TableToGpioDma::Disable();
}


bool JtagDev::OnOpen()
{
	mspArch_ = ChipInfoDB::kCpu;
	JtagOn::Enable();
	InterfaceOn();
	// Initialize DMA timer (do not add multiple for shared timer channel!)
	FlashStrobeCtrl::Init();
	// Timer should trigger the DMA, when running
	FlashStrobeCtrl::EnableDma();
	// Initialize DMA (do not add multiple for shared DMA channel!)
	FlashStrobeDma::Init();
	FlashStrobeDma::SetDestAddress(&(JTDI::GetPortBase()->BSRR));
#if 0
	jtag_tck_clr(p);
	//jtag_tclk_clr(p);
	__NOP();
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
	assert(false);
#endif
	return true;
}


void JtagDev::OnClose()
{
	InterfaceOff();
	JtagOff::Enable();
}


void JtagDev::OnConnect()
{
	//JENABLE::SetHigh();
	JTEST::SetHigh();
}


void JtagDev::OnRelease()
{
	JTEST::SetLow();
	//JENABLE::SetLow();
}


/*!
Reset target JTAG interface and perform fuse-HW check
*/
void JtagDev::ResetTap()
{
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
		//MicroDelay::Delay(10);
		jtag_tck_clr(p);
		//MicroDelay::Delay(10);
		jtag_tck_set(p);
	}

	/* Set JTAG state machine to Run-Test/IDLE */
	jtag_tms_clr(p);
	jtag_tck_clr(p);
	MicroDelay::Delay(10);
	jtag_tck_set(p);
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
	MilliDelay::Delay(4);

	jtag_rst_set(p);
	jtag_tst_clr(p);
	MilliDelay::Delay(5);
	jtag_tst_set(p);
	MilliDelay::Delay(5);
	jtag_rst_clr(p);
	jtag_tst_clr(p);
	MilliDelay::Delay(5);
	jtag_tst_set(p);

	p->f->jtdev_connect(p);
	jtag_rst_set(p);
	MilliDelay::Delay(5);
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
		MicroDelay::Delay(5);
		jtag_tst_set(p);
		MicroDelay::Delay(5);
		jtag_rst_clr(p);
		MicroDelay::Delay(5);
		jtag_tst_clr(p);		// Enter JTAG 4w
		MicroDelay::Delay(5);
		jtag_tst_set(p);
		MicroDelay::Delay(2);
		jtag_rst_set(p);
		MicroDelay::Delay(100);
#if 0
		else
		{
			__NOP();
			jtag_tst_clr(p);			//1
			MicroDelay::Delay(4000);

			jtag_rst_set(p);			//2
			jtag_tst_set(p);			//3
			MicroDelay::Delay(20000);

			jtag_rst_clr(p);			//4
			MicroDelay::Delay(60);

			// for 4-wire JTAG clear Test pin Test(0)
			jtag_tst_clr(p);			//5
			MicroDelay::Delay(1);

			// for 4-wire JTAG - Test (1)
			jtag_tst_set(p);
			MicroDelay::Delay(60);

			// phase 5 Reset(1)
			jtag_rst_set(p);
			MicroDelay::Delay(500);
		}
#endif
	}
#endif
	ResetTap();
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
static uint32_t jtag_shift(uint8_t num_bits, uint32_t data_out)
{
	uint32_t data_in;
	uint32_t mask;

	volatile GPIO_TypeDef *port = JTMS::GetPortBase();
	bool tclk_save = JTCLK::Get();

	data_in = 0;
	mask = 0x0001U << (num_bits - 1);
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
	return jtag_shift(8, instruction);

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
	return jtag_shift(8, data);

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
	return jtag_shift(16, data);

	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	EntryDr_();

	//data = ((data & 0xFFFF) << 4) | (data >> 16);

	/* JTAG state = Shift-DR, Shift in TDI (20-bit) */
	data = jtag_shift(20, data);

	/* JTAG state = Run-Test/Idle */
	data = ((data & 0xFFFF0) >> 4) | (data & 0x0f << 16);
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
	volatile GPIO_TypeDef *port = JTMS::GetPortBase();
	port->BSRR = tclk1;
	port->BSRR = tclk0_s;
}


void JtagDev::OnPulseTclk(int count)
{
	volatile GPIO_TypeDef *port = JTMS::GetPortBase();
	for(int i = 0 ; i < count; ++i)
	{
		port->BSRR = tclk1;
		port->BSRR = tclk0_s;
	}
}


void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
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

	while(min_pulses != 0)
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
}


void JtagDev::OnPulseTclkN()
{
	volatile GPIO_TypeDef *port = JTMS::GetPortBase();
	port->BSRR = tclk0;
	port->BSRR = tclk1_s;
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

