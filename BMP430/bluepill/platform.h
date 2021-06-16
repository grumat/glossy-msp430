#pragma once


//! Dedicated pin for write TMS
typedef GpioTemplate<PA, 4, kOutput50MHz, kPushPull, kLow> JTMS;
typedef InputPullDownPin<PA, 4> JTMS_Init;

//! Pin for TCK output
typedef GpioTemplate<PA, 5, kOutput50MHz, kPushPull, kHigh> JTCK;
typedef InputPullUpPin<PA, 5> JTCK_Init;
//! Special setting for JTCK using SPI
typedef SPI1_SCK_PA5 JTCK_SPI;

//! Pin for TDO input (output on MCU)
typedef FloatingPin<PA, 6> JTDO;
typedef InputPullUpPin<PA, 6> JTDO_Init;
//! Special setting for JTDO using SPI
typedef SPI1_MISO_PA6 JTDO_SPI;

//! Pin for TDI output (input on MCU)
typedef GpioTemplate<PA, 7, kOutput50MHz, kPushPull, kHigh> JTDI;
typedef InputPullUpPin<PA, 7> JTDI_Init;
typedef JTDI	JTCLK;
//! Special setting for JTCLK using SPI
typedef SPI1_MOSI_PA7 JTCLK_Out_SPI;
//! Special setting for JTDI using SPI
typedef SPI1_MOSI_PA7 JTDI_SPI;

//! Pin for RST output
typedef GpioTemplate<PA, 8, kOutput50MHz, kOpenDrain, kHigh> JRST;
typedef InputPullUpPin<PA, 8> JRST_Init;

//! Pin for TEST output
typedef GpioTemplate<PA, 1, kOutput50MHz, kPushPull, kLow> JTEST;
typedef InputPullDownPin<PA, 1> JTEST_Init;

//! Pin for SBWDIO input
typedef GpioTemplate<PB, 4, kOutput50MHz, kOpenDrain, kHigh> SBWDIO_In;
typedef InputPullUpPin<PB, 4> SBWDIO_In_Init;

//! Pin for SBWDIO output
typedef GpioTemplate<PB, 5, kOutput50MHz, kOpenDrain, kHigh> SBWDIO;
typedef InputPullUpPin<PB, 5> SBWDIO_Init;

//! Pin for SBWCLK output
typedef JTCK	SBWCLK;

//! Pin for LED output
typedef GpioTemplate<PC, 13, kOutput50MHz, kPushPull, kHigh> RED_LED;

//! No green LED available
typedef PinUnused<14> GREEN_LED;

typedef GpioPortTemplate <PA
	, PinUnused<0>
	, JTEST_Init		// bit bang
	, USART2_TX_PA2		// GDB UART port
	, USART2_RX_PA3		// GDB UART port
	, JTMS_Init			// bit bang
	, JTCK_Init			// bit bang / SPI1_SCK
	, JTDO_Init			// bit bang / SPI1_MISO
	, JTDI_Init			// bit bang / SPI1_MOSI
	, JRST_Init
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
	, TRACESWO			// ARM trace pin
	, SBWDIO_In_Init	// SPI or bit bang (tied to SBW connector)
	, SBWDIO_Init		// SPI or bit bang (with resistor to SBWDIO_In)
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
> PORTB;

typedef GpioPortTemplate <PC
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
	, RED_LED
	, PinUnused<14>
	, PinUnused<15>
> PORTC;

// This group activates JTAG bus
typedef GpioPortTemplate <PA
	, PinUnchanged<0>
	, JTEST
	, PinUnchanged<2>
	, PinUnchanged<3>
	, JTMS
	, JTCK
	, JTDO
	, JTDI
	, JRST
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, PinUnchanged<13>
	, PinUnchanged<14>
	, PinUnchanged<15>
> JtagOn;

// This group deactivates JTAG bus
typedef GpioPortTemplate <PA
	, PinUnchanged<0>
	, JTEST_Init
	, PinUnchanged<2>
	, PinUnchanged<3>
	, JTMS_Init
	, JTCK_Init
	, JTDO_Init
	, JTDI_Init
	, JRST_Init
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, PinUnchanged<13>
	, PinUnchanged<14>
	, PinUnchanged<15>
> JtagOff;

// This group activates SPI mode for JTAG
typedef GpioPortTemplate <PA
	, PinUnchanged<0>
	, PinUnchanged<1>
	, PinUnchanged<2>
	, PinUnchanged<3>
	, PinUnchanged<4>
	, JTCK_SPI
	, JTDO_SPI
	, JTDI_SPI
	, PinUnchanged<9>
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, PinUnchanged<13>
	, PinUnchanged<14>
	, PinUnchanged<15>
> JtagSpiOn;


// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
// Set the clock tree
typedef SysClkTemplate<PLL, 1, 4, 1> SysClk;

// USART1 for GDB port
typedef UsartTemplate<kUsart2, SysClk, 115200> UsartGdbSettings;

// SPI channel for JTAG
static constexpr SpiInstance kSpiForJtag = SpiInstance::kSpi1;
// Timer for JTAG wave generation
static constexpr TimInstance kTimForJtag = TimInstance::kTim1;
static constexpr TimChannel kTimChForJtag = TimChannel::kTimCh1;
static constexpr DmaInstance kDmaForJtag = DmaInstance::kDma1;
static constexpr DmaCh kDmaChForJtag = DmaCh::kDmaCh2;

ALWAYS_INLINE void RedLedOn() { RED_LED::SetLow(); }
ALWAYS_INLINE void RedLedOff() { RED_LED::SetHigh(); }
ALWAYS_INLINE void RedGreenOn() { GREEN_LED::SetLow(); }
ALWAYS_INLINE void RedGreenOff() { GREEN_LED::SetHigh(); }
