/*!
\file bluepill/platform.h
\brief Definitions specific for the BluePill STM32F103 board
*/
#pragma once

/// Platform supports SPI
#define OPT_JTAG_USING_SPI	1
/// Transfer SPI bytes using DMA
#define OPT_JTAG_USING_DMA	0
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"
/// ISR handler for "GDB serial port" (provisory until USB UART is added to firmware)
#define OPT_USART_ISR "USART1_IRQHandler"

/// Option controlling SPI peripheral for JTAG communication
#if OPT_JTAG_USING_SPI
/// Tied to JCLK is TIM1:CH1 input, timer in max speed (phase shift)
typedef ExtTimeBase
<
	kTim1					///< Timer 1
	, kTI1FP1				///< Timer Input 1 (PA8)
	, 9000000UL				///< 9 Mhz (72 Mhz / 16)
	, 1						///< No prescaler for JCLK clock
	, 0						///< Input filter selection (fastest produces ~60ns delay)
	, kSlaveModeExternal	///< Enable external clock using rising edge
> ExternJClk5_;
//! Tied to JCLK is TIM4:CH1 input, timer in all other speeds
typedef ExtTimeBase
<
	kTim1					///< Timer 1
	, kTI1FP1				///< Timer Input 1 (PA8)
	, 562500UL				///< 562.5 khz (72 Mhz / 256)
	, 1						///< No prescaler for JCLK clock
	, 0						///< Input filter selection (fastest produces ~60ns delay)
	, kSlaveModeExternalNeg	///< Enable external clock using falling edge
> ExternJClk;

//! TIM1 peripheral instance
typedef TimerTemplate
<
	ExternJClk5_			///< Don't care as all time bases use same prescaler
	, kSingleShot			///< Single shot timer
	, 65535					///< Don't care, so use max value
	, false					///< No buffer as DMA will modify on the fly
> TmsShapeTimer5_;
//! TIM4 peripheral instance
typedef TimerTemplate
<
	ExternJClk				///< Don't care as all time bases use same prescaler
	, kSingleShot			///< Single shot timer
	, 65535					///< Don't care, so use max value
	, false					///< No buffer as DMA will modify on the fly
> TmsShapeTimer_;


/* SPI interface grades */
/// Timer configuration for 9 MHz communication
typedef TmsShapeTimer5_	TmsShapeTimer_5;
/// Constant for 4.5 MHz communication grade
static constexpr uint32_t JTCK_Speed_5 = 9000000UL;
/// Timer configuration for 4.5 MHz communication
typedef TmsShapeTimer_	TmsShapeTimer_4;
/// Constant for 4.5 MHz communication grade
static constexpr uint32_t JTCK_Speed_4 = 4500000UL;
/// Timer configuration for 2.25 MHz communication
typedef TmsShapeTimer_	TmsShapeTimer_3;
/// Constant for 2.25 MHz communication grade
static constexpr uint32_t JTCK_Speed_3 = 2250000UL;
/// Timer configuration for 1.125 MHz communication
typedef TmsShapeTimer_	TmsShapeTimer_2;
/// Constant for 1.125 MHz communication grade
static constexpr uint32_t JTCK_Speed_2 = 1125000UL;
/// Timer configuration for 0.563 MHz communication
typedef TmsShapeTimer_	TmsShapeTimer;
/// Constant for 0.563 MHz communication grade
static constexpr uint32_t JTCK_Speed_1 = 562500UL;

//! PA9 (TIM1:TIM1_CH2) is used as output pin
typedef TimerOutputChannel
<
	TmsShapeTimer			///< Associate timer class to the output
	, kTimCh2				///< Channel 2 is out output (PA9)
	, kTimOutLow			///< TMS level defaults to low
	, kTimOutActiveHigh		///< Active High output
	, kTimOutInactive		///< No negative output
	, false					///< No preload
	, false					///< Fast mode has no effect in timer pulse mode
> TmsShapeOutTimerChannel;

//! GPIO settings for the timer input pin
typedef TIM1_CH1_PA8_IN TmsShapeGpioIn;
//! GPIO settings for the timer output pin
typedef TIM1_CH2_PA9_OUT TmsShapeGpioOut;

#else 

/// Bitbanging pin
typedef PinUnchanged<8> TmsShapeGpioIn;

#endif

/// Dedicated pin for write JTMS
typedef GpioTemplate<PA, 9, kOutput50MHz, kPushPull, kLow> JTMS;
/// Logic state for JTMS pin initialization
typedef InputPullDownPin<PA, 9> JTMS_Init;
/// Special setting for JTMS using SPI
typedef TIM1_CH2_PA9_OUT JTMS_SPI;

/// Pin for JTCK output
typedef GpioTemplate<PA, 5, kOutput50MHz, kPushPull, kHigh> JTCK;
/// Logic state for JTCK pin initialization
typedef InputPullUpPin<PA, 5> JTCK_Init;
/// Special setting for JTCK using SPI
typedef SPI1_SCK_PA5 JTCK_SPI;

/// Pin for JTDO input (output on MCU)
typedef InputPullUpPin<PA, 6> JTDO;
/// Logic state for JTDO pin initialization
typedef InputPullUpPin<PA, 6> JTDO_Init;
/// Special setting for JTDO using SPI
typedef SPI1_MISO_PA6 JTDO_SPI;

/// Pin for JTDI output (input on MCU)
typedef GpioTemplate<PA, 7, kOutput50MHz, kPushPull, kHigh> JTDI;
/// Logic state for JTDI pin initialization
typedef InputPullUpPin<PA, 7> JTDI_Init;

/// JTDI during run/idle state produces JTCLK
typedef JTDI JTCLK;
/// Special setting for JTCLK using SPI
typedef SPI1_MOSI_PA7 JTCLK_Out_SPI;
/// Special setting for JTDI using SPI
typedef SPI1_MOSI_PA7 JTDI_SPI;

/// Pin for JRST output
typedef GpioTemplate<PA, 1, kOutput50MHz, kPushPull, kLow> JRST;
/// Logic state for JRST pin initialization
typedef InputPullUpPin<PA, 1> JRST_Init;

/// Pin for JTEST output
typedef GpioTemplate<PA, 4, kOutput50MHz, kPushPull, kLow> JTEST;
/// Logic state for JTEST pin initialization
typedef InputPullDownPin<PA, 4> JTEST_Init;

/// Pin for SBWDIO input
typedef JTDO SBWDIO_In;

/// Pin for SBWDIO output
typedef JTDI SBWDIO;

/// Pin for SBWCLK output
typedef JTCK SBWCLK;

/// Pin for Jtag Enable control
typedef GpioTemplate<PA, 10, kOutput2MHz, kPushPull, kLow> JENA_Init;
/// JENA is not accessed in a group
typedef JENA_Init JENA;

/// Pin for LED output
typedef GpioTemplate<PC, 13, kOutput50MHz, kPushPull, kHigh> RED_LED;

/// Pin for green LED
typedef GpioTemplate<PB, 9, kOutput50MHz, kPushPull, kLow> GREEN_LED;

/// PWM 3.3V target voltage
typedef GpioTemplate<PB, 8, kOutput10MHz, kPushPull, kLow> PWM_VT_0V;
/// PWM 3.3V target voltage
typedef GpioTemplate<PB, 8, kOutput10MHz, kPushPull, kHigh> PWM_VT_3V3;
/// PWM target voltage modulation
typedef TIM4_CH3_PB8_OUT PWM_VT;

/// Initial configuration for PORTA
typedef GpioPortTemplate <PA
	, PinUnused<0>		///< Vref (pending)
	, JRST_Init			///< bit bang
	, USART2_TX_PA2		///< UART2 TX --> JRXD
	, USART2_RX_PA3		///< UART2 RX -- > JTXD
	, JTEST_Init		///< bit bang
	, JTCK_Init			///< bit bang / SPI1_SCK
	, JTDO_Init			///< bit bang / SPI1_MISO
	, JTDI_Init			///< bit bang / SPI1_MOSI
	, TmsShapeGpioIn	///< TIM1 external clock input
	, JTMS_Init			///< TIM1 CH2 output / bit bang
	, JENA_Init			///< bit bang
	, PinUnused<11>		///< USB-
	, PinUnused<12>		///< USB+
	, PinUnused<13>		///< STM32 TMS/SWDIO
	, PinUnused<14>		///< STM32 TCK/SWCLK
	, PinUnused<15>		///< STM32 TDI
> PORTA;

/// Initial configuration for PORTB
typedef GpioPortTemplate <PB
	, PinUnused<0>				///< not used
	, PinUnused<1>				///< not used
	, PinUnused<2>				///< STM32 BOOT1
	, TRACESWO					///< ARM trace pin
	, PinUnused<4>				///< STM32 JNTRST
	, PinUnused<5>				///< not used
	, USART1_TX_PB6				///< GDB UART port
	, USART1_RX_PB7				///< GDB UART port
	, PWM_VT_3V3				///< Target power on
	, GREEN_LED					///< bit bang
	, PinUnused<10>				///< SPI2_SCK (reserved for SBW)
	, PinUnused<11>				///< not used
	, PinUnused<12>				///< not used
	, PinUnused<13, kFloating>	///< SPI2_CLK (reserved for SBWCLK)
	, PinUnused<14, kFloating>	///< SPI2_MISO (reserved for SBWDIO)
	, PinUnused<15, kFloating>	///< SPI2_MOSI (reserved for SBWDIO)
> PORTB;

/// Initial configuration for PORTC
typedef GpioPortTemplate <PC
	, PinUnused<0>				///< not used
	, PinUnused<1>				///< not used
	, PinUnused<2>				///< not used
	, PinUnused<3>				///< not used
	, PinUnused<4>				///< not used
	, PinUnused<5>				///< not used
	, PinUnused<6>				///< not used
	, PinUnused<7>				///< not used
	, PinUnused<8>				///< not used
	, PinUnused<9>				///< not used
	, PinUnused<10>				///< not used
	, PinUnused<11>				///< not used
	, PinUnused<12>				///< not used
	, RED_LED					///< Red LED
	, PinUnused<14>				///< not used
	, PinUnused<15>				///< not used
> PORTC;

/// Initial configuration for PORTC
typedef GpioPortTemplate <PD
	, PinUnchanged<0>	///< OSC_IN
	, PinUnchanged<1>	///< OSC_OUT
> PORTD;


/// This configuration activates JTAG bus using bit-banging
typedef GpioPortTemplate <PA
	, PinUnchanged<0>			///< state of pin unchanged
	, JRST						///< JRST pin for bit bang access
	, PinUnchanged<2>			///< UART2 state of pin unchanged
	, PinUnchanged<3>			///< UART2 state of pin unchanged
	, JTEST						///< JTEST pin for bit bang access
	, JTCK						///< JTCK pin for bit bang access
	, JTDO						///< JTDO pin for bit bang access
	, JTDI						///< JTDI pin for bit bang access
	, TmsShapeGpioIn			///< Input for TMS shape active
	, JTMS						///< JTMS pin for bit bang access
	, PinUnchanged<10>			///< JENA is always left unchanged
	, PinUnchanged<11>			///< state of pin unchanged
	, PinUnchanged<12>			///< state of pin unchanged
	, PinUnchanged<13>			///< state of pin unchanged
	, PinUnchanged<14>			///< state of pin unchanged
	, PinUnchanged<15>			///< state of pin unchanged
> JtagOn;

/// This configuration deactivates JTAG bus
typedef GpioPortTemplate <PA
	, PinUnchanged<0>			///< state of pin unchanged
	, JRST_Init					///< JRST in Hi-Z
	, PinUnchanged<2>			///< UART2 state of pin unchanged
	, PinUnchanged<3>			///< UART2 state of pin unchanged
	, JTEST_Init				///< JTEST in Hi-Z
	, JTCK_Init					///< JTCK in Hi-Z
	, JTDO_Init					///< JTDO in Hi-Z
	, JTDI_Init					///< JTDI in Hi-Z
	, TmsShapeGpioIn			///< Keep as input
	, JTMS_Init					///< JTMS in Hi-Z
	, PinUnchanged<10>			///< JENA is always left unchanged
	, PinUnchanged<11>			///< state of pin unchanged
	, PinUnchanged<12>			///< state of pin unchanged
	, PinUnchanged<13>			///< state of pin unchanged
	, PinUnchanged<14>			///< state of pin unchanged
	, PinUnchanged<15>			///< state of pin unchanged
> JtagOff;

/// This configuration activates SPI mode for JTAG, after it was activated in bit-bang mode
typedef GpioPortTemplate <PA
	, PinUnchanged<0>			///< state of pin unchanged
	, JRST						///< JRST is still used in bit bang mode
	, PinUnchanged<2>			///< UART2 state of pin unchanged
	, PinUnchanged<3>			///< UART2 state of pin unchanged
	, JTEST						///< JTEST is still used in bit bang mode
	, JTCK_SPI					///< setup JTCK pin for SPI mode
	, JTDO_SPI					///< setup JTDO pin for SPI mode
	, JTDI_SPI					///< setup JTDI pin for SPI mode
	, TmsShapeGpioIn			///< input for pulse shaper
	, JTMS_SPI					///< setup JTMS pin for SPI mode
	, PinUnchanged<10>			///< JENA is always left unchanged
	, PinUnchanged<11>			///< state of pin unchanged
	, PinUnchanged<12>			///< state of pin unchanged
	, PinUnchanged<13>			///< state of pin unchanged
	, PinUnchanged<14>			///< state of pin unchanged
	, PinUnchanged<15>			///< state of pin unchanged
> JtagSpiOn;


/// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
/// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
/// Set the clock tree
typedef SysClkTemplate<PLL, kAhbPres_1, kApbPres_2, kApbPres_1> SysClk;

#ifdef OPT_USART_ISR
/// USART1 for GDB port
typedef UsartTemplate<kUsart1, SysClk, 115200> UsartGdbSettings;
#endif

/// SPI channel for JTAG
static constexpr SpiInstance kSpiForJtag = SpiInstance::kSpi1;
#if OPT_JTAG_USING_SPI
/// Timer for JTAG TMS generation
static constexpr TimInstance kTimForTms = TimInstance::kTim1;
/// Timer channel for JTAG TMS generation
static constexpr TimChannel kTimChForTms = TimChannel::kTimCh2;
#endif
/// Timer for JTAG wave generation
static constexpr TimInstance kTimForWave = TimInstance::kTim3;
/// Timer channel for JTAG Wave generation
//static constexpr TimChannel kTimChForWave = TimChannel::kTimCh1;
/// ISR handler for "DMA Transfer Complete"
//#define OPT_JTAG_WAVE_DMA_ISR "DMA1_Channel6_IRQHandler"

typedef SysTickCounter<SysClk> TickTimer;

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

