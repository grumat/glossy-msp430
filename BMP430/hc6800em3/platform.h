#pragma once


//! Dedicated pin for read SBWTDI
typedef InputPullUpPin<PB, 9> SBWTDI;
typedef SBWTDI	SBWTDI_Init;
//! Dedicated pin for write TMS
typedef GpioTemplate<PB, 10, kOutput50MHz, kPushPull, kHigh> JTMS;
typedef InputPullUpPin<PB, 10> JTMS_Init;

typedef JTMS SBWTDO;	// shared between 2w and 4w modes

//! Pin for TCK output
typedef GpioTemplate<PB, 8, kOutput50MHz, kPushPull, kHigh> JTCK;
typedef InputPullUpPin<PB, 8> JTCK_Init;

//! Pin for TDI output (input on MCU)
typedef GpioTemplate<PB, 11, kOutput50MHz, kPushPull, kHigh> JTDI;
typedef InputPullUpPin<PB, 11> JTDI_Init;
typedef JTDI	JTCLK;

//! Pin for TDO input (output on MCU)
typedef InputPullUpPin<PB, 12> JTDO;
typedef JTDO	JTDO_Init;

//! Pin for RST output
typedef GpioTemplate<PB, 13, kOutput50MHz, kOpenDrain, kHigh> JRST;
typedef InputPullUpPin<PB, 13> JRST_Init;

//! Pin for TEST output
typedef GpioTemplate<PB, 14, kOutput50MHz, kPushPull, kLow> JTEST;
typedef InputPullDownPin<PB, 14> JTEST_Init;

//! Pin for TCLK output (this pin was removed!!!)
//typedef PinUnused<1> JTCLK;

//! Pin for LED output
typedef GpioTemplate<PB, 15, kOutput50MHz, kPushPull, kLow> RED_LED;

typedef GpioPortTemplate <PA
	, PinUnused<0>
	, PinUnused<1>
	, PinUnused<2>
	, PinUnused<3>
	, PinUnused<4>
	, PinUnused<5>
	, PinUnused<6>
	, PinUnused<7>
	, PinUnused<8>
	, PinUnused<9>
	, PinUnused<10>
	, PinUnused<11>
	, PinUnused<12>
	, PinUnused<13>
	, PinUnused<14>
	, PinUnused<15>
> PORTA;

// This group is used only for initialization of the GPIOB
typedef GpioPortTemplate <PB
	, PinUnused<0>
	, PinUnused<1>
	, PinUnused<2>
	, PinUnused<3>
	, PinUnused<4>
	, PinUnused<5>
	, PinUnused<6>
	, PinUnused<7>
	, JTCK_Init
	, SBWTDI_Init
	, JTMS_Init
	, JTDI_Init
	, JTDO_Init
	, JRST_Init
	, JTEST_Init
	, RED_LED
> PORTB;

// This group activates JTAG bus
typedef GpioPortTemplate <PB
	, PinUnused<0>
	, PinUnused<1>
	, PinUnused<2>
	, PinUnused<3>
	, PinUnused<4>
	, PinUnused<5>
	, PinUnused<6>
	, PinUnused<7>
	, JTCK
	, SBWTDI_Init
	, JTMS
	, JTDI
	, JTDO
	, JRST
	, JTEST_Init
	, PinUnused<15>
> JtagOn;

// This group deactivates JTAG bus
typedef GpioPortTemplate <PB
	, PinUnused<0>
	, PinUnused<1>
	, PinUnused<2>
	, PinUnused<3>
	, PinUnused<4>
	, PinUnused<5>
	, PinUnused<6>
	, PinUnused<7>
	, JTCK_Init
	, PinUnused<9>
	, JTMS_Init
	, JTDI_Init
	, JTDO_Init
	, JRST_Init
	, PinUnused<14>
	, PinUnused<15>
> JtagOff;


// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;

typedef PllTemplate<HSE, 48000000UL> PLL;

// 8 MHz is enough for this example
typedef SysClkTemplate<PLL> SysClk;

extern UsartTemplate<kUsart1, SysClk, 115200> gUartGdb;
