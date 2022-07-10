/*!
\file bluepill/platform.h
\brief Definitions specific for the BluePill STM32F103 board
*/
#pragma once

/// Platform supports SPI (unsupported by this platform)
#define OPT_JTAG_USING_SPI	0
/// Transfer SPI bytes using DMA (unsupported by this platform)
#define OPT_JTAG_USING_DMA	0
/// ISR handler for "GDB serial port" (provisory until USB UART is added to firmware)
#define OPT_USART_ISR "USART2_IRQHandler"

/// Dedicated pin for write JTMS
typedef GpioTemplate<PA, 4, kOutput50MHz, kPushPull, kLow> JTMS;
/// Logic state for JTMS pin initialization
typedef InputPullDownPin<PA, 4> JTMS_Init;

/// Pin for JTCK output
typedef GpioTemplate<PA, 5, kOutput50MHz, kPushPull, kHigh> JTCK;
/// Logic state for JTCK pin initialization
typedef InputPullUpPin<PA, 5> JTCK_Init;

/// Pin for JTDO input (output on MCU)
typedef InputPullUpPin<PA, 6> JTDO;
/// Logic state for JTDO pin initialization
typedef InputPullUpPin<PA, 6> JTDO_Init;

/// Pin for JTDI output (input on MCU)
typedef GpioTemplate<PA, 7, kOutput50MHz, kPushPull, kHigh> JTDI;
/// Logic state for JTDI pin initialization
typedef InputPullUpPin<PA, 7> JTDI_Init;

/// JTDI during run/idle state produces JTCLK
typedef JTDI JTCLK;

/// Pin for JRST output
typedef GpioTemplate<PA, 8, kOutput50MHz, kPushPull, kLow> JRST;
/// Logic state for JRST pin initialization
typedef InputPullUpPin<PA, 8> JRST_Init;

/// Pin for JTEST output
typedef GpioTemplate<PA, 1, kOutput50MHz, kPushPull, kLow> JTEST;
/// Logic state for JTEST pin initialization
typedef InputPullDownPin<PA, 1> JTEST_Init;

/// Pin for SBWDIO input
typedef JTDO SBWDIO_In;

/// Pin for SBWDIO output
typedef JTDI SBWDIO;

/// Pin for SBWCLK output
typedef JTCK SBWCLK;

/// Pin for Jtag Enable control (not used by hardware)
typedef GpioTemplate<PB, 1, kOutput50MHz, kPushPull, kLow> JENA;

/// Pin for LED output
typedef GpioTemplate<PC, 13, kOutput50MHz, kPushPull, kHigh> RED_LED;

/// Pin for green LED
typedef GpioTemplate<PB, 0, kOutput50MHz, kPushPull, kHigh> GREEN_LED;

/// Initial configuration for PORTA
typedef GpioPortTemplate <PA
	, PinUnused<0>		///< Vref (pending)
	, JTEST_Init		///< bit bang
	, USART2_TX_PA2		///< GDB UART port / Target TXD
	, USART2_RX_PA3		///< GDB UART port / target RXD
	, JTMS_Init			///< Bit bang
	, JTCK_Init			///< bit bang / SPI1_SCK
	, JTDO_Init			///< bit bang / SPI1_MISO
	, JTDI_Init			///< bit bang / SPI1_MOSI
	, JRST_Init			///< bit bang
	, PinUnused<9>		///< not used
	, PinUnused<10>		///< not used
	, PinUnused<11>		///< USB-
	, PinUnused<12>		///< USB+
	, PinUnused<13>		///< STM32 TMS/SWDIO
	, PinUnused<14>		///< STM32 TCK/SWCLK
	, PinUnused<15>		///< STM32 TDI
> PORTA;

/// Initial configuration for PORTB
typedef GpioPortTemplate <PB
	, GREEN_LED			///< bit bang
	, JENA				///< bit bang
	, PinUnused<2>		///< STM32 BOOT1
	, TRACESWO			///< ARM trace pin
	, PinUnused<4>		///< STM32 JNTRST
	, PinUnused<5>		///< not used
	, PinUnused<6>		///< not used
	, PinUnused<7>		///< not used
	, PinUnused<8>		///< not used
	, PinUnused<9>		///< not used
	, PinUnused<10>		///< not used
	, PinUnused<11>		///< not used
	, PinUnused<12>		///< not used
	, PinUnused<13>		///< not used
	, PinUnused<14>		///< not used
	, PinUnused<15>		///< not used
> PORTB;

/// Initial configuration for PORTC
typedef GpioPortTemplate <PC
	, PinUnused<0>		///< not used
	, PinUnused<1>		///< not used
	, PinUnused<2>		///< not used
	, PinUnused<3>		///< not used
	, PinUnused<4>		///< not used
	, PinUnused<5>		///< not used
	, PinUnused<6>		///< not used
	, PinUnused<7>		///< not used
	, PinUnused<8>		///< not used
	, PinUnused<9>		///< not used
	, PinUnused<10>		///< not used
	, PinUnused<11>		///< not used
	, PinUnused<12>		///< not used
	, RED_LED			///< Red LED
	, PinUnused<14>		///< not used
	, PinUnused<15>		///< not used
> PORTC;

/// This configuration activates JTAG bus using bit-banging
typedef GpioPortTemplate <PA
	, PinUnchanged<0>	///< state of pin unchanged
	, JTEST				///< JTEST pin for bit bang access
	, PinUnchanged<2>	///< state of pin unchanged
	, PinUnchanged<3>	///< state of pin unchanged
	, JTMS				///< JTMS pin for bit bang access
	, JTCK				///< JTCK pin for bit bang access
	, JTDO				///< JTDO pin for bit bang access
	, JTDI				///< JTDI pin for bit bang access
	, JRST				///< JRST pin for bit bang access
	, PinUnchanged<9>	///< state of pin unchanged
	, PinUnchanged<10>	///< state of pin unchanged
	, PinUnchanged<11>	///< state of pin unchanged
	, PinUnchanged<12>	///< state of pin unchanged
	, PinUnchanged<13>	///< state of pin unchanged
	, PinUnchanged<14>	///< state of pin unchanged
	, PinUnchanged<15>	///< state of pin unchanged
> JtagOn;

/// This configuration deactivates JTAG bus
typedef GpioPortTemplate <PA
	, PinUnchanged<0>	///< state of pin unchanged
	, JTEST_Init		///< JTEST in Hi-Z
	, PinUnchanged<2>	///< state of pin unchanged
	, PinUnchanged<3>	///< state of pin unchanged
	, JTMS_Init			///< JTMS in Hi-Z
	, JTCK_Init			///< JTCK in Hi-Z
	, JTDO_Init			///< JTDO in Hi-Z
	, JTDI_Init			///< JTDI in Hi-Z
	, JRST_Init			///< JRST in Hi-Z
	, PinUnchanged<9>	///< state of pin unchanged
	, PinUnchanged<10>	///< state of pin unchanged
	, PinUnchanged<11>	///< state of pin unchanged
	, PinUnchanged<12>	///< state of pin unchanged
	, PinUnchanged<13>	///< state of pin unchanged
	, PinUnchanged<14>	///< state of pin unchanged
	, PinUnchanged<15>	///< state of pin unchanged
> JtagOff;


/// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
/// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
/// Set the clock tree
typedef SysClkTemplate<PLL, kAhbPres_1, kApbPres_2, kApbPres_1> SysClk;

#ifdef OPT_USART_ISR
/// USART2 for GDB port (while no USB protocolo is added)
typedef UsartTemplate<kUsart2, SysClk, 115200> UsartGdbSettings;
#endif

/// Timer for JTAG wave generation
static constexpr TimInstance kTimForJtag = TimInstance::kTim1;
/// Timer channel for JTAG wave generation
static constexpr TimChannel kTimChForJtag = TimChannel::kTimCh1;

/// Base timer for microsecond delay
typedef TimeBase_us<kTim3, SysClk, 1U> MicroDelayTimeBase;
/// Base timer for HW tick counter
typedef TimeBase_us<kTim2, SysClk, 500U> TickTimeBase;

/// Sets the red LED On
ALWAYS_INLINE void RedLedOn() { RED_LED::SetLow(); }
/// Sets the red LED Off
ALWAYS_INLINE void RedLedOff() { RED_LED::SetHigh(); }
/// Sets the red GREEN On
ALWAYS_INLINE void GreenLedOn() { GREEN_LED::SetLow(); }
/// Sets the red GREEN Off
ALWAYS_INLINE void GreenLedOff() { GREEN_LED::SetHigh(); }

/// Enables JTAG interface buffers
ALWAYS_INLINE void InterfaceOn() { JENA::SetHigh(); }
/// Disables JTAG interface buffers
ALWAYS_INLINE void InterfaceOff() { JENA::SetLow(); }

