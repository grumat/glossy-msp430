#pragma once

namespace Bmt
{
namespace Gpio
{


//////////////////////////////////////////////////////////////////////
// ADC12
//////////////////////////////////////////////////////////////////////
/// A default configuration for ADC12 IN0 on PA0
typedef AnyAnalog<Port::PA, 0>	ADC12_IN0;
/// A default configuration for ADC12 IN1 on PA1
typedef AnyAnalog<Port::PA, 1>	ADC12_IN1;
/// A default configuration for ADC12 IN2 on PA2
typedef AnyAnalog<Port::PA, 2>	ADC12_IN2;
/// A default configuration for ADC12 IN3 on PA3
typedef AnyAnalog<Port::PA, 3>	ADC12_IN3;
/// A default configuration for ADC12 IN4 on PA4
typedef AnyAnalog<Port::PA, 4>	ADC12_IN4;
/// A default configuration for ADC12 IN5 on PA5
typedef AnyAnalog<Port::PA, 5>	ADC12_IN5;
/// A default configuration for ADC12 IN6 on PA6
typedef AnyAnalog<Port::PA, 6>	ADC12_IN6;
/// A default configuration for ADC12 IN7 on PA7
typedef AnyAnalog<Port::PA, 7>	ADC12_IN7;
/// A default configuration for ADC12 IN8 on PB0
typedef AnyAnalog<Port::PB, 0>	ADC12_IN8;
/// A default configuration for ADC12 IN9 on PB1
typedef AnyAnalog<Port::PB, 1>	ADC12_IN9;
/// A default configuration for ADC12 IN10 on PC0
typedef AnyAnalog<Port::PC, 0>	ADC12_IN10;
/// A default configuration for ADC12 IN11 on PC1
typedef AnyAnalog<Port::PC, 1>	ADC12_IN11;
/// A default configuration for ADC12 IN12 on PC2
typedef AnyAnalog<Port::PC, 2>	ADC12_IN12;
/// A default configuration for ADC12 IN13 on PC3
typedef AnyAnalog<Port::PC, 3>	ADC12_IN13;
/// A default configuration for ADC12 IN14 on PC4
typedef AnyAnalog<Port::PC, 4>	ADC12_IN14;
/// A default configuration for ADC12 IN15 on PC5
typedef AnyAnalog<Port::PC, 5>	ADC12_IN15;


//////////////////////////////////////////////////////////////////////
// CAN - Configuration 1 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PA11 pin
typedef AnyInPu<Port::PA, 11, AfCan_PA11_12>		CAN_RX_PA11;
/// A default configuration for CAN/TX on PA12 pin
typedef AnyAltOut<Port::PA, 12, AfCan_PA11_12>	CAN_TX_PA12;

//////////////////////////////////////////////////////////////////////
// CAN - Configuration 2 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PB8 pin
typedef AnyInPu<Port::PB, 8, AfCan_PB8_9>			CAN_RX_PB8;
/// A default configuration for CAN/TX on PB9 pin
typedef AnyAltOut<Port::PB, 9, AfCan_PB8_9>		CAN_TX_PB9;

//////////////////////////////////////////////////////////////////////
// CAN - Configuration 3 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PD0 pin
typedef AnyInPu<Port::PD, 0, AfCan_PD0_1>			CAN_RX_PD0;
/// A default configuration for CAN/TX on PD1 pin
typedef AnyAltOut<Port::PD, 1, AfCan_PD0_1>		CAN_TX_PD1;

//////////////////////////////////////////////////////////////////////
// GPIO vs Oscillator
//////////////////////////////////////////////////////////////////////
/// A default configuration to map OSC_IN to PD0 (input)
template<const PuPd kPuPd = PuPd::kFloating>
struct PD0_IN : AnyIn<Port::PD, 0, kPuPd, Af_PD01_GPIO> {};
/// A default configuration to map OSC_IN to PD0 (output)
template<const Level kLevel = Level::kLow, const Speed kSpeed = Speed::kFast, const Mode kMode = Mode::kOutput>
struct PD0_OUT : AnyOut< Port::PD, 0, kSpeed, kLevel, kMode, Af_PD01_GPIO> {};
/// A default configuration to map OSC_OUT to PD1 (input)
template<const PuPd kPuPd = PuPd::kFloating>
struct PD1_IN : AnyIn<Port::PD, 1, kPuPd, Af_PD01_GPIO> {};
/// A default configuration to map OSC_OUT to PD1 (output)
template<const Level kLevel = Level::kLow, const Speed kSpeed = Speed::kFast, const Mode kMode = Mode::kOutput>
struct PD1_OUT : AnyOut< Port::PD, 0, kSpeed, kLevel, kMode, Af_PD01_GPIO> {};

//////////////////////////////////////////////////////////////////////
// I2C1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C1 SCL on PB6 pin
typedef AnyAltOutOD<Port::PB, 6, AfI2C1_PB6_7>	I2C1_SCL_PB6;
/// A default configuration to map I2C1 SDA on PB7 pin
typedef AnyAltOutOD<Port::PB, 7, AfI2C1_PB6_7>	I2C1_SDA_PB7;
// I2C1 - Configuration 2
/// A default configuration to map I2C1 SCL on PB8 pin
typedef AnyAltOutOD<Port::PB, 8, AfI2C1_PB8_9>	I2C1_SCL_PB8;
/// A default configuration to map I2C1 SDA on PB9 pin
typedef AnyAltOutOD<Port::PB, 9, AfI2C1_PB8_9>	I2C1_SDA_PB9;
// I2C1 - Configuration 1 & 2
/// A default configuration to map I2C1 SMBAI on PB5 pin
typedef AnyAltOutOD<Port::PB, 5, AfNoRemap>		I2C1_SMBAI_PB5;

//////////////////////////////////////////////////////////////////////
// I2C2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C2 SCL on PB10 pin
typedef AnyAltOutOD<Port::PB, 10, AfNoRemap>		I2C2_SCL_PB10;
/// A default configuration to map I2C2 SDA on PB11 pin
typedef AnyAltOutOD<Port::PB, 11, AfNoRemap>		I2C2_SDA_PB11;
/// A default configuration to map I2C2 SMBAI on PB12 pin
typedef AnyAltOutOD<Port::PB, 12, AfNoRemap>		I2C2_SMBAI_PB12;

//////////////////////////////////////////////////////////////////////
// SPI1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI1 NSS on PA4 pin (master)
typedef AnyAltOut<Port::PA, 4, AfSpi1_PA4_5_6_7>		SPI1_NSS_PA4;
/// A default configuration to map SPI1 NSS on PA4 pin (slave)
typedef Floating<Port::PA, 4, AfSpi1_PA4_5_6_7>		SPI1_NSS_PA4_SLAVE;
/// A default configuration to map SPI1 SCK on PA5 pin (master)
typedef AnyAltOut<Port::PA, 5, AfSpi1_PA4_5_6_7>		SPI1_SCK_PA5;
/// A default configuration to map SPI1 SCK on PA5 pin (slave)
typedef Floating<Port::PA, 5, AfSpi1_PA4_5_6_7>		SPI1_SCK_PA5_SLAVE;
/// A default configuration to map SPI1 MISO on PA6 pin (master)
typedef Floating<Port::PA, 6, AfSpi1_PA4_5_6_7>		SPI1_MISO_PA6;
/// A default configuration to map SPI1 MISO on PA6 pin (slave)
typedef AnyAltOut<Port::PA, 6, AfSpi1_PA4_5_6_7>		SPI1_MISO_PA6_SLAVE;
/// A default configuration to map SPI1 MOSI on PA7 pin (master)
typedef AnyAltOut<Port::PA, 7, AfSpi1_PA4_5_6_7>		SPI1_MOSI_PA7;
/// A default configuration to map SPI1 MOSI on PA7 pin (slave)
typedef Floating<Port::PA, 7, AfSpi1_PA4_5_6_7>		SPI1_MOSI_PA7_SLAVE;
// SPI1 - Configuration 2
/// A default configuration to map SPI1 NSS on PA15 pin (master)
typedef AnyAltOut<Port::PA, 15, AfSpi1_PA15_PB3_4_5>	SPI1_NSS_PA15;
/// A default configuration to map SPI1 NSS on PA15 pin (slave)
typedef Floating<Port::PA, 15, AfSpi1_PA15_PB3_4_5>	SPI1_NSS_PA15_SLAVE;
/// A default configuration to map SPI1 SCK on PB3 pin (master)
typedef AnyAltOut<Port::PB, 3, AfSpi1_PA15_PB3_4_5>	SPI1_SCK_PB3;
/// A default configuration to map SPI1 SCK on PB3 pin (slave)
typedef Floating<Port::PB, 3, AfSpi1_PA15_PB3_4_5>	SPI1_SCK_PB3_SLAVE;
/// A default configuration to map SPI1 MISO on PB4 pin (master)
typedef Floating<Port::PB, 4, AfSpi1_PA15_PB3_4_5>	SPI1_MISO_PB4;
/// A default configuration to map SPI1 MISO on PB4 pin (slave)
typedef AnyAltOut<Port::PB, 4, AfSpi1_PA15_PB3_4_5>	SPI1_MISO_PB4_SLAVE;
/// A default configuration to map SPI1 MOSI on PB5 pin (master)
typedef AnyAltOut<Port::PB, 5, AfSpi1_PA15_PB3_4_5>	SPI1_MOSI_PB5;
/// A default configuration to map SPI1 MOSI on PB5 pin (slave)
typedef Floating<Port::PB, 5, AfSpi1_PA15_PB3_4_5>	SPI1_MOSI_PB5_SLAVE;

//////////////////////////////////////////////////////////////////////
// SPI2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI2 NSS on PB12 pin (master)
typedef AnyAltOut<Port::PB, 12, AfNoRemap>			SPI2_NSS_PB12;
/// A default configuration to map SPI2 NSS on PB12 pin (slave)
typedef Floating<Port::PB, 12, AfNoRemap>				SPI2_NSS_PB12_SLAVE;
/// A default configuration to map SPI2 SCK on PB13 pin (master)
typedef AnyAltOut<Port::PB, 13, AfNoRemap>			SPI2_SCK_PB13;
/// A default configuration to map SPI2 SCK on PB13 pin (slave)
typedef Floating<Port::PB, 13, AfNoRemap>				SPI2_SCK_PB13_SLAVE;
/// A default configuration to map SPI2 MISO on PB14 pin (master)
typedef Floating<Port::PB, 14, AfNoRemap>				SPI2_MISO_PB14;
/// A default configuration to map SPI2 MISO on PB14 pin (slave)
typedef AnyAltOut<Port::PB, 14, AfNoRemap>			SPI2_MISO_PB14_SLAVE;
/// A default configuration to map SPI2 MOSI on PB15 pin (master)
typedef AnyAltOut<Port::PB, 15, AfNoRemap>			SPI2_MOSI_PB15;
/// A default configuration to map SPI2 MOSI on PB15 pin (slave)
typedef Floating<Port::PB, 15, AfNoRemap>				SPIs_MOSI_PB15_SLAVE;


//////////////////////////////////////////////////////////////////////
// TIM1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 ETR on PA12 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_ETR_PA12 : AnyIn<Port::PA, 12, kPuPd, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 ETR on PA12 pin (input)
typedef TIM1_ETR_PA12<>												TIM1_ETR_PA12_IN;

/// A generic configuration to map TIM1 CH1 on PA8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1_PA8 : AnyPin<Port::PA, 8, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH1 on PA8 pin (output)
typedef TIM1_CH1_PA8<Mode::kAlternate, Speed::kFast>				TIM1_CH1_PA8_OUT;
/// A default configuration to map TIM1 CH1 on PA8 pin (input)
typedef TIM1_CH1_PA8<Mode::kInput, Speed::kInput>					TIM1_CH1_PA8_IN;

/// A generic configuration to map TIM1 CH2 on PA9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2_PA9 : AnyPin<Port::PA, 9, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH2 on PA9 pin (output)
typedef TIM1_CH2_PA9<Mode::kAlternate, Speed::kFast>				TIM1_CH2_PA9_OUT;
/// A default configuration to map TIM1 CH2 on PA9 pin (input)
typedef TIM1_CH2_PA9<Mode::kInput, Speed::kInput>					TIM1_CH2_PA9_IN;

/// A generic configuration to map TIM1 CH3 on PA10 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3_PA10 : AnyPin<Port::PA, 10, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH3 on PA10 pin (output)
typedef TIM1_CH3_PA10<Mode::kAlternate, Speed::kFast>				TIM1_CH3_PA10_OUT;
/// A default configuration to map TIM1 CH3 on PA10 pin (input)
typedef TIM1_CH3_PA10<Mode::kInput, Speed::kInput>					TIM1_CH3_PA10_IN;

/// A generic configuration to map TIM1 CH4 on PA11 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH4_PA11 : AnyPin<Port::PA, 11, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH4 on PA11 pin (output)
typedef TIM1_CH4_PA11<Mode::kAlternate, Speed::kFast>				TIM1_CH4_PA11_OUT;
/// A default configuration to map TIM1 CH4 on PA11 pin (input)
typedef TIM1_CH4_PA11<Mode::kInput, Speed::kInput>					TIM1_CH4_PA11_IN;

/// A generic configuration to map TIM1 BKIN on PB12 pin
template<const PuPd kPuPd>
struct TIM1_BKIN_PB12 : AnyIn<Port::PB, 12, kPuPd, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 BKIN on PB12 pin (input)
typedef TIM1_BKIN_PB12<PuPd::kFloating>								TIM1_BKIN_PB12_IN;

/// A generic configuration to map TIM1 CH1N on PB13 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1N_PB13 : AnyAltOut<Port::PB, 13, AfTim1_PA12_8_9_10_11_PB12_13_14_15, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH1N on PB13 pin (output)
typedef TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>				TIM1_CH1N_PB13_OUT;

/// A generic configuration to map TIM1 CH2N on PB14 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2N_PB14 : AnyAltOut<Port::PB, 14, AfTim1_PA12_8_9_10_11_PB12_13_14_15, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH2N on PB14 pin (output)
typedef TIM1_CH2N_PB14<Mode::kAlternate, Speed::kFast>				TIM1_CH2N_PB14_OUT;

/// A generic configuration to map TIM1 CH3N on PB15 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3N_PB15 : AnyAltOut<Port::PB, 15, AfTim1_PA12_8_9_10_11_PB12_13_14_15, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH3N on PB15 pin (output)
typedef TIM1_CH3N_PB15<Mode::kAlternate, Speed::kFast>				TIM1_CH3N_PB15_OUT;

//////////////////////////////////////////////////////////////////////
// TIM1 - Configuration 2 (partial remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 ETR on PA12 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_ETR_PA12_P : AnyIn<Port::PA, 12, kPuPd, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 ETR on PA12 pin (input)
typedef TIM1_ETR_PA12_P<PuPd::kFloating>							TIM1_ETR_PA12_IN_CFG2;

/// A generic configuration to map TIM1 CH1 on PA8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1_PA8_P : AnyPin<Port::PA, 8, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH1 on PA8 pin (output)
typedef TIM1_CH1_PA8_P<Mode::kAlternate, Speed::kFast>				TIM1_CH1_PA8_OUT_CFG2;
/// A default configuration to map TIM1 CH1 on PA8 pin (input)
typedef TIM1_CH1_PA8_P<Mode::kInput, Speed::kInput>					TIM1_CH1_PA8_IN_CFG2;

/// A generic configuration to map TIM1 CH2 on PA9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2_PA9_P : AnyPin<Port::PA, 9, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH2 on PA9 pin (output)
typedef TIM1_CH2_PA9_P<Mode::kAlternate, Speed::kFast>				TIM1_CH2_PA9_OUT_CFG2;
/// A default configuration to map TIM1 CH2 on PA9 pin (input)
typedef TIM1_CH2_PA9_P<Mode::kInput, Speed::kInput>					TIM1_CH2_PA9_IN_CFG2;

/// A generic configuration to map TIM1 CH3 on PA10 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3_PA10_P : AnyPin<Port::PA, 10, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH3 on PA10 pin (output)
typedef TIM1_CH3_PA10_P<Mode::kAlternate, Speed::kFast>				TIM1_CH3_PA10_OUT_CFG2;
/// A default configuration to map TIM1 CH3 on PA10 pin (input)
typedef TIM1_CH3_PA10_P<Mode::kInput, Speed::kInput>				TIM1_CH3_PA10_IN_CFG2;

/// A generic configuration to map TIM1 CH4 on PA11 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH4_PA11_P : AnyPin<Port::PA, 11, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH4 on PA11 pin (output)
typedef TIM1_CH4_PA11_P<Mode::kAlternate, Speed::kFast>				TIM1_CH4_PA11_OUT_CFG2;
/// A default configuration to map TIM1 CH4 on PA11 pin (input)
typedef TIM1_CH4_PA11_P<Mode::kInput, Speed::kInput>				TIM1_CH4_PA11_IN_CFG2;

/// A generic configuration to map TIM1 BKIN on PA6 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_BKIN_PA6_P : AnyIn<Port::PA, 6, kPuPd, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 BKIN on PA6 pin (input)
typedef TIM1_BKIN_PA6_P<PuPd::kFloating>							TIM1_BKIN_PA6_IN_CFG2;

/// A generic configuration to map TIM1 CH1N on PA7 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1N_PA7_P : AnyAltOut<Port::PA, 7, AfTim1_PA12_8_9_10_11_6_7_PB0_1, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH1N on PA7 pin (output)
typedef TIM1_CH1N_PA7_P<Mode::kAlternate, Speed::kFast>				TIM1_CH1N_PA7_OUT_CFG2;

/// A generic configuration to map TIM1 CH2N on PB0 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2N_PB0_P : AnyAltOut<Port::PB, 0, AfTim1_PA12_8_9_10_11_6_7_PB0_1, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH2N on PB0 pin (output)
typedef TIM1_CH2N_PB0_P<Mode::kAlternate, Speed::kFast>				TIM1_CH2N_PB0_OUT_CFG2;

/// A generic configuration to map TIM1 CH3N on PB1 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3N_PB1_P : AnyAltOut<Port::PB, 1, AfTim1_PA12_8_9_10_11_6_7_PB0_1, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH3N on PB1 pin (output)
typedef TIM1_CH3N_PB1_P<Mode::kAlternate, Speed::kFast>				TIM1_CH3N_PB1_OUT_CFG2;

//////////////////////////////////////////////////////////////////////
// TIM1 - Configuration 3
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 ETR on PE7 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_ETR_PE7 : AnyIn<Port::PE, 7, kPuPd, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 ETR on PE7 pin (input)
typedef TIM1_ETR_PE7<PuPd::kFloating>								TIM1_ETR_PE7_IN_CFG3;

/// A generic configuration to map TIM1 CH1 on PE9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1_PE9 : AnyPin<Port::PE, 9, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH1 on PE9 pin (output)
typedef TIM1_CH1_PE9<Mode::kAlternate, Speed::kFast>				TIM1_CH1_PE9_OUT_CFG3;
/// A default configuration to map TIM1 CH1 on PE9 pin (input)
typedef TIM1_CH1_PE9<Mode::kInput, Speed::kInput>					TIM1_CH1_PE9_IN_CFG3;

/// A generic configuration to map TIM1 CH2 on PE11 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2_PE11 : AnyPin<Port::PE, 11, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH2 on PE11 pin (output)
typedef TIM1_CH2_PE11<Mode::kAlternate, Speed::kFast>				TIM1_CH2_PE11_OUT_CFG3;
/// A default configuration to map TIM1 CH2 on PE11 pin (input)
typedef TIM1_CH2_PE11<Mode::kInput, Speed::kInput>					TIM1_CH2_PE11_IN_CFG3;

/// A generic configuration to map TIM1 CH3 on PE13 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3_PE13 : AnyPin<Port::PE, 13, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH3 on PE13 pin (output)
typedef TIM1_CH3_PE13<Mode::kAlternate, Speed::kFast>				TIM1_CH3_PE13_OUT_CFG3;
/// A default configuration to map TIM1 CH3 on PE13 pin (input)
typedef TIM1_CH3_PE13<Mode::kInput, Speed::kInput>					TIM1_CH3_PE13_IN_CFG3;

/// A generic configuration to map TIM1 CH4 on PE14 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH4_PE14 : AnyPin<Port::PE, 14, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH4 on PE14 pin (output)
typedef TIM1_CH4_PE14<Mode::kAlternate, Speed::kFast>				TIM1_CH4_PE14_OUT_CFG3;
/// A default configuration to map TIM1 CH4 on PE14 pin (input)
typedef TIM1_CH4_PE14<Mode::kInput, Speed::kInput>					TIM1_CH4_PE14_IN_CFG3;

/// A generic configuration to map TIM1 BKIN on PE15 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_BKIN_PE15 : AnyIn<Port::PE, 15, kPuPd, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 BKIN on PE15 pin (input)
typedef TIM1_BKIN_PE15<PuPd::kFloating>								TIM1_BKIN_PE15_IN_CFG3;

/// A generic configuration to map TIM1 CH1N on PE8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1N_PE8 : AnyAltOut<Port::PE, 8, AfTim1_PE7_9_11_13_14_15_8_10_12, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH1N on PE8 pin (output)
typedef TIM1_CH1N_PE8<Mode::kAlternate, Speed::kFast>				TIM1_CH1N_PE8_OUT_CFG3;

/// A generic configuration to map TIM1 CH2N on PE10 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2N_PE10 : AnyAltOut<Port::PE, 10, AfTim1_PE7_9_11_13_14_15_8_10_12, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH2N on PE10 pin (output)
typedef TIM1_CH2N_PE10<Mode::kAlternate, Speed::kFast>				TIM1_CH2N_PE10_OUT_CFG3;

/// A generic configuration to map TIM1 CH3N on PE12 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3N_PE12 : AnyAltOut<Port::PE, 12, AfTim1_PE7_9_11_13_14_15_8_10_12, kSpeed, kLevel, kPuPd, kMode> {};
/// A default configuration to map TIM1 CH3N on PE12 pin (output)
typedef TIM1_CH3N_PE12<Mode::kAlternate, Speed::kFast>				TIM1_CH3N_PE12_OUT_CFG3;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 1 (no remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 CH1 on PA0 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA0 : AnyPin<Port::PA, 0, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH1 on PA0 pin (output)
typedef TIM2_CH1_PA0<Mode::kAlternate, Speed::kFast>				TIM2_CH1_PA0_OUT;
/// A default configuration to map TIM1 CH1 on PA0 pin (input)
typedef TIM2_CH1_PA0<Mode::kInput, Speed::kInput>					TIM2_CH1_PA0_IN;
/// A default configuration to map TIM1 CH1 on PA0 pin (input)
typedef TIM2_CH1_PA0<Mode::kInput, Speed::kInput>					TIM2_ETR_PA0_IN;

/// A generic configuration to map TIM1 CH2 on PA1 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PA1 : AnyPin<Port::PA, 1, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH2 on PA1 pin (output)
typedef TIM2_CH2_PA1<Mode::kAlternate, Speed::kFast>				TIM2_CH2_PA1_OUT;
/// A default configuration to map TIM1 CH2 on PA1 pin (input)
typedef TIM2_CH2_PA1<Mode::kInput, Speed::kInput>					TIM2_CH2_PA1_IN;

/// A generic configuration to map TIM1 CH3 on PA2 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PA2 : AnyPin<Port::PA, 2, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH3 on PA2 pin (output)
typedef TIM2_CH3_PA2<Mode::kAlternate, Speed::kFast>				TIM2_CH3_PA2_OUT;
/// A default configuration to map TIM1 CH3 on PA2 pin (input)
typedef TIM2_CH3_PA2<Mode::kInput, Speed::kInput>					TIM2_CH3_PA2_IN;

/// A generic configuration to map TIM1 CH4 on PA3 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PA3 : AnyPin<Port::PA, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH4 on PA3 pin (output)
typedef TIM2_CH4_PA3<Mode::kAlternate, Speed::kFast>				TIM2_CH4_PA3_OUT;
/// A default configuration to map TIM1 CH4 on PA3 pin (input)
typedef TIM2_CH4_PA3<Mode::kInput, Speed::kInput>					TIM2_CH4_PA3_IN;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 2 (partial remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM2 CH1 on PA15 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA15_P : AnyPin<Port::PA, 15, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH1 on PA15 pin (partial remap/config 2 - output)
typedef TIM2_CH1_PA15_P<Mode::kAlternate, Speed::kFast>				TIM2_CH1_PA15_OUT_CFG2;
/// A default configuration to map TIM2 CH1 on PA15 pin (partial remap/config 2 - input)
typedef TIM2_CH1_PA15_P<Mode::kInput, Speed::kInput>				TIM2_CH1_PA15_IN_CFG2;
/// A default configuration to map TIM2 ETR on PA15 pin (partial remap/config 2 - input)
typedef TIM2_CH1_PA15_P<Mode::kInput, Speed::kInput>				TIM2_ETR_PA15_IN_CFG2;

/// A generic configuration to map TIM2 CH2 on PB3 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PB3_P : AnyPin<Port::PB, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH2 on PB3 pin (partial remap/config 2 - output)
typedef TIM2_CH2_PB3_P<Mode::kAlternate, Speed::kFast>				TIM2_CH2_PB3_OUT_CFG2;
/// A default configuration to map TIM2 CH2 on PB3 pin (partial remap/config 2 - input)
typedef TIM2_CH2_PB3_P<Mode::kInput, Speed::kInput>					TIM2_CH2_PB3_IN_CFG2;

/// A generic configuration to map TIM2 CH3 on PA2 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PA2_P : AnyPin<Port::PA, 2, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH3 on PA2 pin (partial remap/config 2 - output)
typedef TIM2_CH3_PA2_P<Mode::kAlternate, Speed::kFast>				TIM2_CH3_PA2_OUT_CFG2;
/// A default configuration to map TIM2 CH3 on PA2 pin (partial remap/config 2 - input)
typedef TIM2_CH3_PA2_P<Mode::kInput, Speed::kInput>					TIM2_CH3_PA2_IN_CFG2;

/// A generic configuration to map TIM2 CH4 on PA3 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PA3_P : AnyPin<Port::PA, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH4 on PA3 pin (partial remap/config 2 - output)
typedef TIM2_CH4_PA3_P<Mode::kAlternate, Speed::kFast>				TIM2_CH4_PA3_OUT_CFG2;
/// A default configuration to map TIM2 CH4 on PA3 pin (partial remap/config 2 - input)
typedef TIM2_CH4_PA3_P<Mode::kInput, Speed::kInput>					TIM2_CH4_PA3_IN_CFG2;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 3 (partial remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM2 CH1 on PA0 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA0_P : AnyPin<Port::PA, 0, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH1 on PA0 pin (partial remap/config 3 - output)
typedef TIM2_CH1_PA0_P<Mode::kAlternate, Speed::kFast>				TIM2_CH1_PA0_OUT_CFG3;
/// A default configuration to map TIM2 CH1 on PA0 pin (partial remap/config 3 - input)
typedef TIM2_CH1_PA0_P<Mode::kInput, Speed::kInput>					TIM2_CH1_PA0_IN_CFG3;
/// A default configuration to map TIM2 ETR on PA0 pin (partial remap/config 3 - input)
typedef TIM2_CH1_PA0_P<Mode::kInput, Speed::kInput>					TIM2_ETR_PA0_IN_CFG3;

/// A generic configuration to map TIM2 CH2 on PA1 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PA1_P : AnyPin<Port::PA, 1, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH2 on PA1 pin (partial remap/config 3 - output)
typedef TIM2_CH2_PA1_P<Mode::kAlternate, Speed::kFast>				TIM2_CH2_PA1_OUT_CFG3;
/// A default configuration to map TIM2 CH2 on PA1 pin (partial remap/config 3 - input)
typedef TIM2_CH2_PA1_P<Mode::kInput, Speed::kInput>					TIM2_CH2_PA1_IN_CFG3;

/// A generic configuration to map TIM2 CH3 on PB10 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PB10_P : AnyPin<Port::PB, 10, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH3 on PA10 pin (partial remap/config 3 - output)
typedef TIM2_CH3_PB10_P<Mode::kAlternate, Speed::kFast>				TIM2_CH3_PB10_OUT_CFG3;
/// A default configuration to map TIM2 CH3 on PA10 pin (partial remap/config 3 - input)
typedef TIM2_CH3_PB10_P<Mode::kInput, Speed::kInput>				TIM2_CH3_PB10_IN_CFG3;

/// A generic configuration to map TIM2 CH3 on PB11 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PB11_P : AnyPin<Port::PB, 11, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH3 on PB11 pin (partial remap/config 3 - output)
typedef TIM2_CH4_PB11_P<Mode::kAlternate, Speed::kFast>				TIM2_CH4_PB11_OUT_CFG3;
/// A default configuration to map TIM2 CH3 on PB11 pin (partial remap/config 3 - input)
typedef TIM2_CH4_PB11_P<Mode::kInput, Speed::kInput>				TIM2_CH4_PB11_IN_CFG3;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 4 (full remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM2 CH1 on PA15 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA15 : AnyPin<Port::PA, 15, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH1 on PA15 pin (full remap/config 4 - output)
typedef TIM2_CH1_PA15<Mode::kAlternate, Speed::kFast>			TIM2_CH1_PA15_OUT_CFG4;
/// A default configuration to map TIM2 CH1 on PA15 pin (full remap/config 4 - input)
typedef TIM2_CH1_PA15<Mode::kInput, Speed::kInput>				TIM2_CH1_PA15_IN_CFG4;
/// A default configuration to map TIM2 ETR on PA15 pin (full remap/config 4 - input)
typedef TIM2_CH1_PA15<Mode::kInput, Speed::kInput>				TIM2_ETR_PA15_IN_CFG4;

/// A generic configuration to map TIM2 CH2 on PB3 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PB3 : AnyPin<Port::PB, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH2 on PB3 pin (full remap/config 4 - output)
typedef TIM2_CH2_PB3<Mode::kAlternate, Speed::kFast>			TIM2_CH2_PB3_OUT_CFG4;
/// A default configuration to map TIM2 CH2 on PB3 pin (full remap/config 4 - input)
typedef TIM2_CH2_PB3<Mode::kInput, Speed::kInput>				TIM2_CH2_PB3_IN_CFG4;

/// A generic configuration to map TIM2 CH3 on PB10 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PB10 : AnyPin<Port::PB, 10, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH3 on PB10 pin (full remap/config 4 - output)
typedef TIM2_CH3_PB10<Mode::kAlternate, Speed::kFast>			TIM2_CH3_PB10_OUT_CFG4;
/// A default configuration to map TIM2 CH3 on PB10 pin (full remap/config 4 - input)
typedef TIM2_CH3_PB10<Mode::kInput, Speed::kInput>				TIM2_CH3_PB10_IN_CFG4;

/// A generic configuration to map TIM2 CH4 on PB11 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PB11 : AnyPin<Port::PB, 11, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH4 on PB11 pin (full remap/config 4 - output)
typedef TIM2_CH4_PB11<Mode::kAlternate, Speed::kFast>			TIM2_CH4_PB11_OUT_CFG4;
/// A default configuration to map TIM2 CH4 on PB11 pin (full remap/config 4 - input)
typedef TIM2_CH4_PB11<Mode::kInput, Speed::kInput>				TIM2_CH4_PB11_IN_CFG4;

//////////////////////////////////////////////////////////////////////
// TIM3 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM3 CH1 on PA6 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH1_PA6 : AnyPin<Port::PA, 6, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH1 on PA6 pin (output)
typedef TIM3_CH1_PA6<Mode::kAlternate, Speed::kFast>			TIM3_CH1_PA6_OUT;
/// A default configuration to map TIM3 CH1 on PA6 pin (input)
typedef TIM3_CH1_PA6<Mode::kInput, Speed::kInput>				TIM3_CH1_PA6_IN;

/// A generic configuration to map TIM3 CH1 on PA7 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH2_PA7 : AnyPin<Port::PA, 7, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH2 on PA7 pin (output)
typedef TIM3_CH2_PA7<Mode::kAlternate, Speed::kFast>			TIM3_CH2_PA7_OUT;
/// A default configuration to map TIM3 CH2 on PA7 pin (input)
typedef TIM3_CH2_PA7<Mode::kInput, Speed::kInput>				TIM3_CH2_PA7_IN;

/// A generic configuration to map TIM3 CH3 on PB0 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH3_PB0 : AnyPin<Port::PB, 0, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH3 on PB0 pin (input)
typedef TIM3_CH3_PB0<Mode::kAlternate, Speed::kFast>			TIM3_CH3_PB0_OUT;
/// A default configuration to map TIM3 CH3 on PB0 pin (input)
typedef TIM3_CH3_PB0<Mode::kInput, Speed::kInput>				TIM3_CH3_PB0_IN;

/// A generic configuration to map TIM3 CH4 on PB1 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH4_PB1 : AnyPin<Port::PB, 1, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH4 on PB1 pin (input)
typedef TIM3_CH4_PB1<Mode::kAlternate, Speed::kFast>			TIM3_CH4_PB1_OUT;
/// A default configuration to map TIM3 CH4 on PB1 pin (input)
typedef TIM3_CH4_PB1<Mode::kInput, Speed::kInput>				TIM3_CH4_PB1_IN;

// TIM3 - Configuration 2 (partial remap)
/// A generic configuration to map TIM3 CH1 on PB4 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH1_PB4_P : AnyPin<Port::PB, 4, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH1 on PB4 pin (partial remap/config 2 - output)
typedef TIM3_CH1_PB4_P<Mode::kAlternate, Speed::kFast>			TIM3_CH1_PB4_OUT_CFG2;
/// A default configuration to map TIM3 CH1 on PB4 pin (partial remap/config 2 - input)
typedef TIM3_CH1_PB4_P<Mode::kInput, Speed::kInput>				TIM3_CH1_PB4_IN_CFG2;

/// A generic configuration to map TIM3 CH2 on PB5 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH2_PB5_P : AnyPin<Port::PB, 5, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH2 on PB5 pin (partial remap/config 2 - output)
typedef TIM3_CH2_PB5_P<Mode::kAlternate, Speed::kFast>			TIM3_CH2_PB5_OUT_CFG2;
/// A default configuration to map TIM3 CH2 on PB5 pin (partial remap/config 2 - input)
typedef TIM3_CH2_PB5_P<Mode::kInput, Speed::kInput>				TIM3_CH2_PB5_IN_CFG2;

/// A generic configuration to map TIM3 CH3 on PB0 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH3_PB0_P : AnyPin<Port::PB, 0, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH3 on PB0 pin (partial remap/config 2 - output)
typedef TIM3_CH3_PB0_P<Mode::kAlternate, Speed::kFast>			TIM3_CH3_PB0_OUT_CFG2;
/// A default configuration to map TIM3 CH3 on PB0 pin (partial remap/config 2 - input)
typedef TIM3_CH3_PB0_P<Mode::kInput, Speed::kInput>				TIM3_CH3_PB0_IN_CFG2;

/// A generic configuration to map TIM3 CH4 on PB1 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH4_PB1_P : AnyPin<Port::PB, 1, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH4 on PB1 pin (partial remap/config 2 - output)
typedef TIM3_CH4_PB1_P<Mode::kAlternate, Speed::kFast>			TIM3_CH4_PB1_OUT_CFG2;
/// A default configuration to map TIM3 CH4 on PB1 pin (partial remap/config 2 - input)
typedef TIM3_CH4_PB1_P<Mode::kInput, Speed::kInput>				TIM3_CH4_PB1_IN_CFG2;

//////////////////////////////////////////////////////////////////////
// TIM3 - Configuration 3 (full remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM3 CH1 on PC6 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH1_PC6 : AnyPin<Port::PC, 6, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH1 on PC6 pin (full remap/config 3 - output)
typedef TIM3_CH1_PC6<Mode::kAlternate, Speed::kFast>			TIM3_CH1_PC6_OUT_CFG3;
/// A default configuration to map TIM3 CH1 on PC6 pin (full remap/config 3 - input)
typedef TIM3_CH1_PC6<Mode::kInput, Speed::kInput>				TIM3_CH1_PC6_IN_CFG3;

/// A generic configuration to map TIM3 CH2 on PC7 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH2_PC7 : AnyPin<Port::PC, 7, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH2 on PC7 pin (full remap/config 3 - output)
typedef TIM3_CH2_PC7<Mode::kAlternate, Speed::kFast>			TIM3_CH2_PC7_OUT_CFG3;
/// A default configuration to map TIM3 CH2 on PC7 pin (full remap/config 3 - input)
typedef TIM3_CH2_PC7<Mode::kInput, Speed::kInput>				TIM3_CH2_PC7_IN_CFG3;

/// A generic configuration to map TIM3 CH3 on PC8 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH3_PC8 : AnyPin<Port::PC, 8, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH3 on PC8 pin (full remap/config 3 - output)
typedef TIM3_CH3_PC8<Mode::kAlternate, Speed::kFast>			TIM3_CH3_PC8_OUT_CFG3;
/// A default configuration to map TIM3 CH3 on PC8 pin (full remap/config 3 - input)
typedef TIM3_CH3_PC8<Mode::kInput, Speed::kInput>				TIM3_CH3_PC8_IN_CFG3;

/// A generic configuration to map TIM3 CH4 on PC9 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH4_PC9 : AnyPin<Port::PC, 9, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH4 on PC9 pin (full remap/config 3 - output)
typedef TIM3_CH4_PC9<Mode::kAlternate, Speed::kFast>			TIM3_CH4_PC9_OUT_CFG3;
/// A default configuration to map TIM3 CH4 on PC9 pin (full remap/config 3 - input)
typedef TIM3_CH4_PC9<Mode::kInput, Speed::kInput>				TIM3_CH4_PC9_IN_CFG3;

//////////////////////////////////////////////////////////////////////
// TIM4 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM4 CH1 on PB6 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH1_PB6 : AnyPin<Port::PB, 6, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH1 on PB6 pin (output)
typedef TIM4_CH1_PB6<Mode::kAlternate, Speed::kFast>			TIM4_CH1_PB6_OUT;
/// A default configuration to map TIM4 CH1 on PB6 pin (input)
typedef TIM4_CH1_PB6<Mode::kInput, Speed::kInput>				TIM4_CH1_PB6_IN;

/// A generic configuration to map TIM4 CH2 on PB7 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH2_PB7 : AnyPin<Port::PB, 7, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH2 on PB7 pin (output)
typedef TIM4_CH2_PB7<Mode::kAlternate, Speed::kFast>			TIM4_CH2_PB7_OUT;
/// A default configuration to map TIM4 CH2 on PB7 pin (input)
typedef TIM4_CH2_PB7<Mode::kInput, Speed::kInput>				TIM4_CH2_PB7_IN;

/// A generic configuration to map TIM4 CH3 on PB8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH3_PB8 : AnyPin<Port::PB, 8, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH3 on PB8 pin (output)
typedef TIM4_CH3_PB8<Mode::kAlternate, Speed::kFast>			TIM4_CH3_PB8_OUT;
/// A default configuration to map TIM4 CH3 on PB8 pin (input)
typedef TIM4_CH3_PB8<Mode::kInput, Speed::kInput>				TIM4_CH3_PB8_IN;

/// A generic configuration to map TIM4 CH4 on PB9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH4_PB9 : AnyPin<Port::PB, 9, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH4 on PB9 pin (output)
typedef TIM4_CH4_PB9<Mode::kAlternate, Speed::kFast>			TIM4_CH4_PB9_OUT;
/// A default configuration to map TIM4 CH4 on PB9 pin (input)
typedef TIM4_CH4_PB9<Mode::kInput, Speed::kInput>				TIM4_CH4_PB9_IN;

//////////////////////////////////////////////////////////////////////
// TIM4 - Configuration 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM4 CH1 on PD12 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH1_PD12 : AnyPin<Port::PD, 12, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH1 on PD12 pin (output)
typedef TIM4_CH1_PD12<Mode::kAlternate, Speed::kFast>			TIM4_CH1_PD12_OUT;
/// A default configuration to map TIM4 CH1 on PD12 pin (input)
typedef TIM4_CH1_PD12<Mode::kInput, Speed::kInput>				TIM4_CH1_PD12_IN;

/// A generic configuration to map TIM4 CH2 on PD13 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH2_PD13 : AnyPin<Port::PD, 13, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH2 on PD13 pin (output)
typedef TIM4_CH2_PD13<Mode::kAlternate, Speed::kFast>			TIM4_CH2_PD13_OUT;
/// A default configuration to map TIM4 CH2 on PD13 pin (input)
typedef TIM4_CH2_PD13<Mode::kInput, Speed::kInput>				TIM4_CH2_PD13_IN;

/// A generic configuration to map TIM4 CH3 on PD14 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH3_PD14 : AnyPin<Port::PD, 14, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH3 on PD14 pin (output)
typedef TIM4_CH3_PD14<Mode::kAlternate, Speed::kFast>			TIM4_CH3_PD14_OUT;
/// A default configuration to map TIM4 CH3 on PD14 pin (input)
typedef TIM4_CH3_PD14<Mode::kInput, Speed::kInput>				TIM4_CH3_PD14_IN;

/// A generic configuration to map TIM4 CH4 on PD15 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH4_PD15 : AnyPin<Port::PD, 15, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH4 on PD15 pin (output)
typedef TIM4_CH4_PD15<Mode::kAlternate, Speed::kFast>			TIM4_CH4_PD15_OUT;
/// A default configuration to map TIM4 CH4 on PD15 pin (input)
typedef TIM4_CH4_PD15<Mode::kInput, Speed::kInput>				TIM4_CH4_PD15_IN;


//////////////////////////////////////////////////////////////////////
// USART1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 TX on PA9 pin
typedef AnyAltOut<Port::PA, 9, AfUsart1_PA9_10>			USART1_TX_PA9;
/// A generic configuration to map USART1 RX on PA10 pin
typedef AnyInPu<Port::PA, 10, AfUsart1_PA9_10>			USART1_RX_PA10;

//////////////////////////////////////////////////////////////////////
// USART1 - Configuration 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 TX on PB6 pin (config 2)
typedef AnyAltOut<Port::PB, 6, AfUsart1_PB6_7>		USART1_TX_PB6;
/// A generic configuration to map USART1 RX on PB7 pin (config 2)
typedef AnyInPu<Port::PB, 7, AfUsart1_PB6_7>			USART1_RX_PB7;

//////////////////////////////////////////////////////////////////////
// USART1 - Configuration 1 & 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 CK on PA8 pin (config 1 & 2)
typedef AnyAltOut<Port::PA, 8, AfNoRemap>				USART1_CK_PA8;
/// A generic configuration to map USART1 CTS on PA8 pin (config 1 & 2)
typedef AnyInPu<Port::PA, 11, AfNoRemap>				USART1_CTS_PA11;
/// A generic configuration to map USART1 RTS on PA8 pin (config 1 & 2)
typedef AnyAltOut<Port::PA, 12, AfNoRemap>			USART1_RTS_PA12;

//////////////////////////////////////////////////////////////////////
// USART2 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART2 CTS on PA0 pin (config 1)
typedef AnyInPu<Port::PA, 0, AfUsart2_PA0_1_2_3_4>	USART2_CTS_PA0;
/// A generic configuration to map USART2 RTS on PA1 pin (config 1)
typedef AnyAltOut<Port::PA, 1, AfUsart2_PA0_1_2_3_4>	USART2_RTS_PA1;
/// A generic configuration to map USART2 TX on PA2 pin (config 1)
typedef AnyAltOut<Port::PA, 2, AfUsart2_PA0_1_2_3_4>	USART2_TX_PA2;
/// A generic configuration to map USART2 RX on PA3 pin (config 1)
typedef AnyInPu<Port::PA, 3, AfUsart2_PA0_1_2_3_4>	USART2_RX_PA3;
/// A generic configuration to map USART2 CK on PA4 pin (config 1)
typedef AnyAltOut<Port::PA, 4, AfUsart2_PA0_1_2_3_4>	USART2_CK_PA4;

//////////////////////////////////////////////////////////////////////
// USART2 - Configuration 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART2 CTS on PD3 pin (config 2)
typedef AnyInPu<Port::PD, 3, AfUsart2_PD3_4_5_6_7>	USART2_CTS_PD3;
/// A generic configuration to map USART2 RTS on PD4 pin (config 2)
typedef AnyAltOut<Port::PD, 4, AfUsart2_PD3_4_5_6_7>	USART2_RTS_PD4;
/// A generic configuration to map USART2 TX on PD5 pin (config 2)
typedef AnyAltOut<Port::PD, 5, AfUsart2_PD3_4_5_6_7>	USART2_TX_PD5;
/// A generic configuration to map USART2 RX on PD6 pin (config 2)
typedef AnyInPu<Port::PD, 6, AfUsart2_PD3_4_5_6_7>	USART2_RX_PD6;
/// A generic configuration to map USART2 CK on PD7 pin (config 2)
typedef AnyAltOut<Port::PD, 7, AfUsart2_PD3_4_5_6_7>	USART2_CK_PD7;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 TX on PB10 pin (config 2)
typedef AnyAltOut<Port::PB, 10, AfUsart3_PB10_11_12_13_14>	USART3_TX_PB10;
/// A generic configuration to map USART3 RX on PB11 pin (config 2)
typedef AnyInPu<Port::PB, 11, AfUsart3_PB10_11_12_13_14> 		USART3_RX_PB11;
/// A generic configuration to map USART3 CK on PB12 pin (config 2)
typedef AnyAltOut<Port::PB, 12, AfUsart3_PB10_11_12_13_14>	USART3_CK_PB12;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 1 & 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 CTS on PB13 pin (config 1 & 2)
typedef AnyInPu<Port::PB, 13, AfUsart3_PB10_11_12_13_14> 		USART3_CTS_PB13;
/// A generic configuration to map USART3 RTS on PB14 pin (config 1 & 2)
typedef AnyAltOut<Port::PB, 14, AfUsart3_PB10_11_12_13_14>	USART3_RTS_PB14;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 2
//////////////////////////////////////////////////////////////////////
typedef AnyAltOut<Port::PC, 10, AfUsart3_PC10_11_12_PB13_14>	USART3_TX_PC10;
typedef AnyInPu<Port::PC, 11, AfUsart3_PC10_11_12_PB13_14> 	USART3_RX_PC11;
typedef AnyAltOut<Port::PC, 12, AfUsart3_PC10_11_12_PB13_14>	USART3_CK_PC12;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 3
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 TX on PD8 pin (config 3)
typedef AnyAltOut<Port::PD, 8, AfUsart3_PD8_9_10_11_12>	USART3_TX_PD8;
/// A generic configuration to map USART3 RX on PD9 pin (config 3)
typedef AnyInPu<Port::PD, 9, AfUsart3_PD8_9_10_11_12> 	USART3_RX_PD9;
/// A generic configuration to map USART3 CK on PD10 pin (config 3)
typedef AnyAltOut<Port::PD, 10, AfUsart3_PD8_9_10_11_12>	USART3_CK_PD10;
/// A generic configuration to map USART3 CTS on PD11 pin (config 3)
typedef AnyInPu<Port::PD, 11, AfUsart3_PD8_9_10_11_12> 	USART3_CTS_PD11;
/// A generic configuration to map USART3 RTS on PD12 pin (config 3)
typedef AnyAltOut<Port::PD, 12, AfUsart3_PD8_9_10_11_12>	USART3_RTS_PD12;

//////////////////////////////////////////////////////////////////////
// USB
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USB DM on PA11 pin
typedef Floating<Port::PA, 11, AfNoRemap>		USB_DM_PA11;
/// A generic configuration to map USB DP on PA12 pin
typedef Floating<Port::PA, 12, AfNoRemap>		USB_DP_PA12;

//////////////////////////////////////////////////////////////////////
// SWO
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TRACESWO on PB3 pin
typedef AnyAltOut<Port::PB, 3, AfNoRemap>	TRACESWO;


}	// namespace Gpio
}	// namespace Bmt
