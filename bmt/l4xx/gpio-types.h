#pragma once

namespace Bmt
{
namespace Gpio
{


//////////////////////////////////////////////////////////////////////
// ADC12
//////////////////////////////////////////////////////////////////////
/// A default configuration for ADC12 IN5 on PA0
typedef AnyAnalog<Port::PA, 0> ADC12_IN5_PA0;
/// A default configuration for ADC12 IN6 on PA1
typedef AnyAnalog<Port::PA, 1> ADC12_IN6_PA1;
/// A default configuration for ADC12 IN7 on PA2
typedef AnyAnalog<Port::PA, 2> ADC12_IN7_PA2;
/// A default configuration for ADC12 IN8 on PA3
typedef AnyAnalog<Port::PA, 3> ADC12_IN8_PA3;
/// A default configuration for ADC12 IN9 on PA4
typedef AnyAnalog<Port::PA, 4> ADC12_IN9_PA4;
/// A default configuration for ADC12 IN10 on PA5
typedef AnyAnalog<Port::PA, 5> ADC12_IN10_PA5;
/// A default configuration for ADC12 IN11 on PA6
typedef AnyAnalog<Port::PA, 6> ADC12_IN11_PA6;
/// A default configuration for ADC12 IN12 on PA7
typedef AnyAnalog<Port::PA, 7> ADC12_IN12_PA7;
/// A default configuration for ADC12 IN15 on PB0
typedef AnyAnalog<Port::PB, 0> ADC12_IN15_PB0;
/// A default configuration for ADC12 IN16 on PB1
typedef AnyAnalog<Port::PB, 1> ADC12_IN16_PB1;

//////////////////////////////////////////////////////////////////////
// SYS
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map MCO on PA8 pin
typedef AnyAltOut<Port::PA, 8, AfSYS_MCO_PA8>		SYS_MCO;

//////////////////////////////////////////////////////////////////////
// JTAG / SWD
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map JTMS on PA13 pin
typedef AnyAltOut<Port::PA, 13, AfSYS_JTMS_PA13>		SYS_JTMS;
/// A generic configuration to map JTCK on PA14 pin
typedef AnyAltOut<Port::PA, 14, AfSYS_JTCK_PA14>		SYS_JTCK;
/// A generic configuration to map JTDI on PA15 pin
typedef AnyAltOut<Port::PA, 15, AfSYS_JTDI_PA15>		SYS_JTDI;
/// A generic configuration to map JTDO on PB3 pin
typedef AnyAltOut<Port::PB, 3, AfSYS_JTDO_PB3>		SYS_JTDO;
/// A generic configuration to map NJTRST on PB4 pin
typedef AnyAltOut<Port::PB, 4, AfSYS_NJTRST_PB4>		SYS_NJTRST;
/// A generic configuration to map SWDIO on PA13 pin
typedef AnyAltOut<Port::PA, 13, AfSYS_SWDIO_PA13>		SYS_SWDIO;
/// A generic configuration to map SWCLK on PA14 pin
typedef AnyAltOut<Port::PA, 14, AfSYS_SWCLK_PA14>		SYS_SWCLK;
/// A generic configuration to map TRACESWO on PB3 pin
typedef AnyAltOut<Port::PB, 3, AfSYS_TRACESWO_PB3>	SYS_TRACESWO;

//////////////////////////////////////////////////////////////////////
// CAN - Configuration 1 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PA11 pin
typedef Floating<Port::PA, 11, AfCAN1_RX_PA11>		CAN_RX_PA11;
/// A default configuration for CAN/TX on PA12 pin
typedef AnyAltOut<Port::PA, 12, AfCAN1_TX_PA12>		CAN_TX_PA12;

//////////////////////////////////////////////////////////////////////
// I2C1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C1 SCL on PA9 pin
typedef AnyAltOutOD<Port::PA, 9, AfI2C1_SCL_PA9>		I2C1_SCL_PA9;
/// A default configuration to map I2C1 SCL on PA9 pin
typedef AnyAltOutOD<Port::PB, 6, AfI2C1_SCL_PB6>		I2C1_SCL_PB6;
/// A default configuration to map I2C1 SDA on PA10 pin
typedef AnyAltOutOD<Port::PA, 10, AfI2C1_SDA_PA10>	I2C1_SDA_PA10;
/// A default configuration to map I2C1 SDA on PB7 pin
typedef AnyAltOutOD<Port::PB, 7, AfI2C1_SDA_PB7>		I2C1_SDA_PB7;
/// A default configuration to map I2C1 SMBA on PA1 pin
typedef AnyAltOutOD<Port::PA, 1, AfI2C1_SMBA_PA1>		I2C1_SMBA_PA1;
/// A default configuration to map I2C1 SMBA on PA14 pin
typedef AnyAltOutOD<Port::PA, 14, AfI2C1_SMBA_PA14>	I2C1_SMBA_PA14;
/// A default configuration to map I2C1 SMBA on PA14 pin
typedef AnyAltOutOD<Port::PB, 5, AfI2C1_SMBA_PB5>		I2C1_SMBA_PB5;

//////////////////////////////////////////////////////////////////////
// I2C3
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C3 SCL on PA7 pin
typedef AnyAltOutOD<Port::PA, 7, AfI2C3_SCL_PA7>		I2C3_SCL_PA7;
/// A default configuration to map I2C3 SDA on PB4 pin
typedef AnyAltOutOD<Port::PB, 4, AfI2C3_SDA_PB4>		I2C2_SDA_PB4;

//////////////////////////////////////////////////////////////////////
// SPI1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI1 NSS on PA4 pin (master)
typedef AnyAltOut<Port::PA, 4, AfSPI1NSS_PA4>		SPI1_NSS_PA4;
/// A default configuration to map SPI1 NSS on PA4 pin (slave)
typedef Floating<Port::PA, 4, AfSPI1NSS_PA4>		SPI1_NSS_PA4_SLAVE;
/// A default configuration to map SPI1 NSS on PA15 pin (master)
typedef AnyAltOut<Port::PA, 15, AfSPI1NSS_PA15>	SPI1_NSS_PA15;
/// A default configuration to map SPI1 NSS on PA15 pin (slave)
typedef Floating<Port::PA, 15, AfSPI1NSS_PA15>	SPI1_NSS_PA15_SLAVE;
/// A default configuration to map SPI1 NSS on PB0 pin (master)
typedef AnyAltOut<Port::PB, 0, AfSPI1NSS_PB0>		SPI1_NSS_PB0;
/// A default configuration to map SPI1 NSS on PB0 pin (slave)
typedef Floating<Port::PB, 0, AfSPI1NSS_PB0>		SPI1_NSS_PB0_SLAVE;
/// A default configuration to map SPI1 SCK on PA1 pin (master)
typedef AnyAltOut<Port::PA, 1, AfSPI1SCK_PA1>		SPI1_SCK_PA1;
/// A default configuration to map SPI1 SCK on PA1 pin (slave)
typedef Floating<Port::PA, 1, AfSPI1SCK_PA1>		SPI1_SCK_PA1_SLAVE;
/// A default configuration to map SPI1 SCK on PA5 pin (master)
typedef AnyAltOut<Port::PA, 5, AfSPI1SCK_PA5>		SPI1_SCK_PA5;
/// A default configuration to map SPI1 SCK on PA5 pin (slave)
typedef Floating<Port::PA, 5, AfSPI1SCK_PA5>		SPI1_SCK_PA5_SLAVE;
/// A default configuration to map SPI1 SCK on PB3 pin (master)
typedef AnyAltOut<Port::PB, 3, AfSPI1SCK_PB3>		SPI1_SCK_PB3;
/// A default configuration to map SPI1 SCK on PB3 pin (slave)
typedef Floating<Port::PB, 3, AfSPI1SCK_PB3>		SPI1_SCK_PB3_SLAVE;
/// A default configuration to map SPI1 MISO on PA6 pin (master)
typedef Floating<Port::PA, 6, AfSPI1MISO_PA6>		SPI1_MISO_PA6;
/// A default configuration to map SPI1 MISO on PA6 pin (slave)
typedef AnyAltOut<Port::PA, 6, AfSPI1MISO_PA6>	SPI1_MISO_PA6_SLAVE;
/// A default configuration to map SPI1 MISO on PA11 pin (master)
typedef Floating<Port::PA, 11, AfSPI1MISO_PA11>	SPI1_MISO_PA11;
/// A default configuration to map SPI1 MISO on PA11 pin (slave)
typedef AnyAltOut<Port::PA, 11, AfSPI1MISO_PA11>	SPI1_MISO_PA11_SLAVE;
/// A default configuration to map SPI1 MISO on PB4 pin (master)
typedef Floating<Port::PB, 4, AfSPI1MISO_PB4>		SPI1_MISO_PB4;
/// A default configuration to map SPI1 MISO on PB4 pin (slave)
typedef AnyAltOut<Port::PB, 4, AfSPI1MISO_PB4>	SPI1_MISO_PB4_SLAVE;
/// A default configuration to map SPI1 MOSI on PA7 pin (master)
typedef AnyAltOut<Port::PA, 7, AfSPI1MOSI_PA7>	SPI1_MOSI_PA7;
/// A default configuration to map SPI1 MOSI on PA7 pin (slave)
typedef Floating<Port::PA, 7, AfSPI1MOSI_PA7>		SPI1_MOSI_PA7_SLAVE;
/// A default configuration to map SPI1 MOSI on PA12 pin (master)
typedef AnyAltOut<Port::PA, 12, AfSPI1MOSI_PA12>	SPI1_MOSI_PA12;
/// A default configuration to map SPI1 MOSI on PA12 pin (slave)
typedef Floating<Port::PA, 12, AfSPI1MOSI_PA12>	SPI1_MOSI_PA12_SLAVE;
/// A default configuration to map SPI1 MOSI on PB5 pin (master)
typedef AnyAltOut<Port::PB, 5, AfSPI1MOSI_PB5>	SPI1_MOSI_PB5;
/// A default configuration to map SPI1 MOSI on PB5 pin (slave)
typedef Floating<Port::PB, 5, AfSPI1MOSI_PB5>		SPI1_MOSI_PB5_SLAVE;

//////////////////////////////////////////////////////////////////////
// SPI3
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI3 NSS on PA4 pin (master)
typedef AnyAltOut<Port::PA, 4, AfSPI3NSS_PA4>		SPI3_NSS_PA4;
/// A default configuration to map SPI3 NSS on PA4 pin (slave)
typedef Floating<Port::PA, 4, AfSPI3NSS_PA4>		SPI3_NSS_PA4_SLAVE;
/// A default configuration to map SPI3 NSS on PA15 pin (master)
typedef AnyAltOut<Port::PA, 15, AfSPI3NSS_PA15>	SPI3_NSS_PA15;
/// A default configuration to map SPI3 NSS on PA15 pin (slave)
typedef Floating<Port::PA, 15, AfSPI3NSS_PA15>	SPI3_NSS_PA15_SLAVE;
/// A default configuration to map SPI3 SCK on PB3 pin (master)
typedef AnyAltOut<Port::PB, 3, AfSPI3SCK_PB3>		SPI3_SCK_PB3;
/// A default configuration to map SPI3 SCK on PB3 pin (slave)
typedef Floating<Port::PB, 3, AfSPI3SCK_PB3>		SPI3_SCK_PB3_SLAVE;
/// A default configuration to map SPI3 MISO on PB4 pin (master)
typedef Floating<Port::PB, 4, AfSPI3MISO_PB4>		SPI3_MISO_PB4;
/// A default configuration to map SPI3 MISO on PB4 pin (slave)
typedef AnyAltOut<Port::PB, 4, AfSPI3MISO_PB4>	SPI3_MISO_PB4_SLAVE;
/// A default configuration to map SPI3 MOSI on PB5 pin (master)
typedef AnyAltOut<Port::PB, 5, AfSPI3MOSI_PB5>	SPI3_MOSI_PB5;
/// A default configuration to map SPI3 MOSI on PB5 pin (slave)
typedef Floating<Port::PB, 5, AfSPI3MOSI_PB5>		SPI3_MOSI_PB5_SLAVE;


//////////////////////////////////////////////////////////////////////
// TIM1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM1 ETR on PA12 pin (input)
typedef Floating<Port::PA, 12, AfTIM1_ETR_PA12>	TIM1_ETR_PA12_IN;
/// A default configuration to map TIM1 BKIN on PA6 pin (input)
typedef Floating<Port::PA, 6, AfTIM1_BKIN_PA6>	TIM1_BKIN_PA6_IN;
/// A default configuration to map TIM1 BKIN2 on PA11 pin (input)
typedef Floating<Port::PA, 11, AfTIM1_BKIN2_PA11>	TIM1_BKIN2_PA11_IN;

/// A default configuration to map TIM1 CH1 on PA8 pin (output)
typedef AnyAltOut<Port::PA, 8, AfTIM1_CH1_PA8>	TIM1_CH1_PA8_OUT;
/// A default configuration to map TIM1 CH1 on PA8 pin (input)
typedef Floating<Port::PA, 8, AfTIM1_CH1_PA8>		TIM1_CH1_PA8_IN;
/// A default configuration to map TIM1 CH1N on PA7 pin (output)
typedef AnyAltOut<Port::PA, 7, AfTIM1_CH1N_PA7>	TIM1_CH1N_PA7_OUT;

/// A default configuration to map TIM1 CH2 on PA9 pin (output)
typedef AnyAltOut<Port::PA, 9, AfTIM1_CH2_PA9>	TIM1_CH2_PA9_OUT;
/// A default configuration to map TIM1 CH2 on PA9 pin (input)
typedef Floating<Port::PA, 9, AfTIM1_CH2_PA9>		TIM1_CH2_PA9_IN;
/// A default configuration to map TIM1 CH2N on PB0 pin (output)
typedef AnyAltOut<Port::PB, 0, AfTIM1_CH2N_PB0>	TIM1_CH2N_PB0_OUT;

/// A default configuration to map TIM1 CH3 on PA10 pin (output)
typedef AnyAltOut<Port::PA, 10, AfTIM1_CH3_PA10>	TIM1_CH3_PA10_OUT;
/// A default configuration to map TIM1 CH3 on PA10 pin (input)
typedef Floating<Port::PA, 10, AfTIM1_CH3_PA10>	TIM1_CH3_PA10_IN;
/// A default configuration to map TIM1 CH3N on PB1 pin (output)
typedef AnyAltOut<Port::PB, 1, AfTIM1_CH3N_PB1>	TIM1_CH3N_PB1_OUT;

/// A default configuration to map TIM1 CH4 on PA11 pin (output)
typedef AnyAltOut<Port::PA, 11, AfTIM1_CH4_PA11>	TIM1_CH4_PA11_OUT;
/// A default configuration to map TIM1 CH4 on PA11 pin (input)
typedef Floating<Port::PA, 11, AfTIM1_CH4_PA11>	TIM1_CH4_PA11_IN;

//////////////////////////////////////////////////////////////////////
// TIM2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM2 ETR on PA0 pin (input)
typedef Floating<Port::PA, 0, AfTIM2_ETR_PA0>		TIM2_ETR_PA0_IN;
/// A default configuration to map TIM2 ETR on PA5 pin (input)
typedef Floating<Port::PA, 5, AfTIM2_ETR_PA5>		TIM2_ETR_PA5_IN;
/// A default configuration to map TIM2 ETR on PA15 pin (input)
typedef Floating<Port::PA, 15, AfTIM2_ETR_PA15>	TIM2_ETR_PA15_IN;

/// A default configuration to map TIM2 CH1 on PA0 pin (output)
typedef AnyAltOut<Port::PA, 0, AfTIM2_CH1_PA0>	TIM2_CH1_PA0_OUT;
/// A default configuration to map TIM2 CH1 on PA0 pin (input)
typedef Floating<Port::PA, 0, AfTIM2_CH1_PA0>		TIM2_CH1_PA0_IN;
/// A default configuration to map TIM2 CH1 on PA5 pin (output)
typedef AnyAltOut<Port::PA, 5, AfTIM2_CH1_PA5>	TIM2_CH1_PA5_OUT;
/// A default configuration to map TIM2 CH1 on PA5 pin (input)
typedef Floating<Port::PA, 5, AfTIM2_CH1_PA5>		TIM2_CH1_PA5_IN;
/// A default configuration to map TIM2 CH1 on PA15 pin (output)
typedef AnyAltOut<Port::PA, 15, AfTIM2_CH1_PA15>	TIM2_CH1_PA15_OUT;
/// A default configuration to map TIM2 CH1 on PA15 pin (input)
typedef Floating<Port::PA, 15, AfTIM2_CH1_PA15>	TIM2_CH1_PA15_IN;

/// A default configuration to map TIM2 CH2 on PA1 pin (output)
typedef AnyAltOut<Port::PA, 1, AfTIM2_CH2_PA1>	TIM2_CH2_PA1_OUT;
/// A default configuration to map TIM2 CH2 on PA1 pin (input)
typedef Floating<Port::PA, 1, AfTIM2_CH2_PA1>		TIM2_CH2_PA1_IN;
/// A default configuration to map TIM2 CH2 on PB3 pin (output)
typedef AnyAltOut<Port::PB, 3, AfTIM2_CH2_PB3>	TIM2_CH2_PB3_OUT;
/// A default configuration to map TIM2 CH2 on PB3 pin (input)
typedef Floating<Port::PB, 3, AfTIM2_CH2_PB3>		TIM2_CH2_PB3_IN;

/// A default configuration to map TIM2 CH3 on PA2 pin (output)
typedef AnyAltOut<Port::PA, 2, AfTIM2_CH3_PA2>	TIM2_CH3_PA2_OUT;
/// A default configuration to map TIM2 CH3 on PA2 pin (input)
typedef Floating<Port::PA, 2, AfTIM2_CH3_PA2>		TIM2_CH3_PA2_IN;

/// A default configuration to map TIM2 CH4 on PA3 pin (output)
typedef AnyAltOut<Port::PA, 3, AfTIM2_CH4_PA3>	TIM2_CH4_PA3_OUT;
/// A default configuration to map TIM2 CH4 on PA3 pin (input)
typedef Floating<Port::PA, 3, AfTIM2_CH4_PA3>		TIM2_CH4_PA3_IN;

//////////////////////////////////////////////////////////////////////
// TIM15
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM15 BKIN on PA9 pin (input)
typedef Floating<Port::PA, 9, AfTIM15_BKIN_PA9>	TIM15_BKIN_PA9_IN;

/// A default configuration to map TIM15 CH1 on PA2 pin (output)
typedef AnyAltOut<Port::PA, 2, AfTIM15_CH1_PA2>	TIM15_CH1_PA2_OUT;
/// A default configuration to map TIM15 CH1 on PA2 pin (input)
typedef Floating<Port::PA, 2, AfTIM15_CH1_PA2>	TIM15_CH1_PA2_IN;
/// A default configuration to map TIM15 CH1N on PA7 pin (output)
typedef AnyAltOut<Port::PA, 7, AfTIM15_CH1N_PA1>	TIM15_CH1N_PA7_OUT;

/// A default configuration to map TIM15 CH2 on PA3 pin (output)
typedef AnyAltOut<Port::PA, 3, AfTIM15_CH2_PA3>	TIM15_CH2_PA3_OUT;
/// A default configuration to map TIM15 CH2 on PA3 pin (input)
typedef Floating<Port::PA, 3, AfTIM15_CH2_PA3>	TIM15_CH2_PA3_IN;

//////////////////////////////////////////////////////////////////////
// TIM16
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM16 BKIN on PB5 pin (input)
typedef Floating<Port::PB, 5, AfTIM16_BKIN_PB5>	TIM16_BKIN_PB5_IN;

/// A default configuration to map TIM16 CH1 on PA6 pin (output)
typedef AnyAltOut<Port::PA, 6, AfTIM16_CH1_PA6>	TIM16_CH1_PA6_OUT;
/// A default configuration to map TIM16 CH1 on PA6 pin (input)
typedef Floating<Port::PA, 6, AfTIM16_CH1_PA6>	TIM16_CH1_PA6_IN;
/// A default configuration to map TIM16 CH1N on PB6 pin (output)
typedef AnyAltOut<Port::PB, 6, AfTIM16_CH1N_PB6>	TIM16_CH1N_PB6_OUT;

//////////////////////////////////////////////////////////////////////
// LPTIM1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map LPTIM1 ETR on PB6 pin (input)
typedef Floating<Port::PB, 6, AfLPTIM1_ETR_PB6>		LPTIM1_ETR_PB6_IN;

/// A default configuration to map LPTIM1 OUT on PA14 pin (output)
typedef AnyAltOut<Port::PA, 16, AfLPTIM1_OUT_PA14>	LPTIM1_OUT_PA14;

/// A default configuration to map LPTIM1 IN1 on PB5 pin (input)
typedef Floating<Port::PB, 5, AfLPTIM1_IN1_PB5>		LPTIM1_IN1_PB6;
/// A default configuration to map LPTIM1 IN2 on PB7 pin (input)
typedef Floating<Port::PB, 7, AfLPTIM1_IN2_PB7>		LPTIM1_IN2_PB7;

//////////////////////////////////////////////////////////////////////
// LPTIM2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map LPTIM2 ETR on PA5 pin (input)
typedef Floating<Port::PA, 5, AfLPTIM2_ETR_PA5>		LPTIM2_ETR_PA5_IN;

/// A default configuration to map LPTIM2 OUT on PA4 pin (output)
typedef AnyAltOut<Port::PA, 4, AfLPTIM2_OUT_PA4>		LPTIM2_OUT_PA4;
/// A default configuration to map LPTIM2 OUT on PA8 pin (output)
typedef AnyAltOut<Port::PA, 8, AfLPTIM2_OUT_PA8>		LPTIM2_OUT_PA8;

/// A default configuration to map LPTIM2 IN1 on PB1 pin (input)
typedef Floating<Port::PB, 1, AfLPTIM2_IN1_PB1>		LPTIM2_IN1_PB1;

//////////////////////////////////////////////////////////////////////
// USART1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 TX on PA9 pin
typedef AnyAltOut<Port::PA, 9, AfUSART1_TX_PA9>		USART1_TX_PA9;
/// A generic configuration to map USART1 TX on PB6 pin
typedef AnyAltOut<Port::PB, 6, AfUSART1_TX_PB6>		USART1_TX_PB6;

/// A generic configuration to map USART1 RX on PA10 pin
typedef AnyInPu<Port::PA, 10, AfUSART1_RX_PA10>		USART1_RX_PA10;
/// A generic configuration to map USART1 RX on PB7 pin
typedef AnyInPu<Port::PB, 7, AfUSART1_RX_PB7>			USART1_RX_PB7;

/// A generic configuration to map USART1 CK on PA8 pin
typedef AnyAltOut<Port::PA, 8, AfUSART1_CK_PA8>		USART1_CK_PA8;
/// A generic configuration to map USART1 CK on PB5 pin
typedef AnyAltOut<Port::PB, 5, AfUSART1_CK_PB5>		USART1_CK_PB5;

/// A generic configuration to map USART1 CTS on PA11 pin
typedef AnyInPu<Port::PA, 11, AfUSART1_CTS_PA11>		USART1_CTS_PA11;
/// A generic configuration to map USART1 CTS on PB4 pin
typedef AnyInPu<Port::PB, 4, AfUSART1_CTS_PB4>		USART1_CTS_PB4;

/// A generic configuration to map USART1 RTS on PA12 pin
typedef AnyAltOut<Port::PA, 12, AfUSART1_RTS_PA12>	USART1_RTS_PA12;
/// A generic configuration to map USART1 RTS on PB3 pin
typedef AnyAltOut<Port::PB, 3, AfUSART1_RTS_PB3>		USART1_RTS_PB3;

//////////////////////////////////////////////////////////////////////
// USART2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART2 TX on PA2 pin
typedef AnyAltOut<Port::PA, 2, AfUSART2_TX_PA2>		USART2_TX_PA2;
/// A generic configuration to map USART2 RX on PA3 pin
typedef AnyInPu<Port::PA, 3, AfUSART2_RX_PA3>			USART2_RX_PA3;
/// A generic configuration to map USART2 RX on PA15 pin
typedef AnyInPu<Port::PA, 15, AfUSART2_RX_PA15>		USART2_RX_PA15;
/// A generic configuration to map USART2 CK on PA4 pin
typedef AnyAltOut<Port::PA, 4, AfUSART2_CK_PA4>		USART2_CK_PA4;
/// A generic configuration to map USART2 CTS on PA0 pin
typedef AnyInPu<Port::PA, 0, AfUSART2_CTS_PA0>		USART2_CTS_PA0;
/// A generic configuration to map USART2 RTS on PA1 pin
typedef AnyAltOut<Port::PA, 1, AfUSART2_RTS_PA1>		USART2_RTS_PA1;

//////////////////////////////////////////////////////////////////////
// USART3
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 CK on PB12 pin
typedef AnyAltOut<Port::PB, 0, AfUSART3_CK_PB0>		USART3_CK_PB0;
/// A generic configuration to map USART3 CTS on PA6 pin
typedef AnyInPu<Port::PA, 6, AfUSART3_CTS_PA6>		USART3_CTS_PA6;
/// A generic configuration to map USART3 RTS on PA15 pin
typedef AnyAltOut<Port::PA, 15, AfUSART3_RTS_PA15>	USART3_RTS_PA15;
/// A generic configuration to map USART3 RTS on PB1 pin
typedef AnyAltOut<Port::PB, 1, AfUSART3_RTS_PB1>		USART3_RTS_PB1;

//////////////////////////////////////////////////////////////////////
// LPUART1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map LPUART1 TX on PA2 pin
typedef AnyAltOut<Port::PA, 2, AfLPUART1_TX_PA2>		LPUART1_TX_PA2;
/// A generic configuration to map LPUART1 RX on PA3 pin
typedef AnyInPu<Port::PA, 3, AfLPUART1_RX_PA3>		LPUART1_RX_PA3;
/// A generic configuration to map LPUART1 CTS on PA6 pin
typedef AnyInPu<Port::PA, 6, AfLPUART1_CTS_PA6>		LPUART1_CTS_PA6;
/// A generic configuration to map LPUART1 RTS on PB1 pin
typedef AnyAltOut<Port::PB, 1, AfLPUART1_RTS_PB1>		LPUART1_RTS_PB1;

//////////////////////////////////////////////////////////////////////
// USB
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USB_CRS_SYNC on PA10 pin
typedef Floating<Port::PA, 10, AfUSB_CRS_SYNC_PA10>	USB_CRS_SYNC;
/// A generic configuration to map USB DM on PA11 pin
typedef Floating<Port::PA, 11, AfUSB_DM_PA11>			USB_DM_PA11;
/// A generic configuration to map USB DP on PA12 pin
typedef Floating<Port::PA, 12, AfUSB_DP_PA12>			USB_DP_PA12;
/// A generic configuration to map USB NOE on PA13 pin
typedef AnyAltOut<Port::PA, 13, AfUSB_NOE_PA13>		USB_NOE_PA13;

//////////////////////////////////////////////////////////////////////
// IR
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map IR OUT on PA13 pin
typedef AnyAltOut<Port::PA, 13, AfIR_OUT_PA13>		IR_OUT_PA13;

//////////////////////////////////////////////////////////////////////
// TSC
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TSC G3_IO1 on PA15 pin
typedef AnyAltOut<Port::PA, 15, AfTSC_G3_IO1_PA15>	TSC_G3_IO1_PA15;
/// A generic configuration to map TSC G2_IO1 on PB4 pin
typedef AnyAltOut<Port::PB, 4, AfTSC_G2_IO1_PB4>		TSC_G2_IO1_PB4;
/// A generic configuration to map TSC G2_IO2 on PB5 pin
typedef AnyAltOutOD<Port::PB, 5, AfTSC_G2_IO2_PB5>	TSC_G2_IO2_PB5;
/// A generic configuration to map TSC G2_IO3 on PB6 pin
typedef AnyAltOut<Port::PB, 6, AfTSC_G2_IO3_PB6>		TSC_G2_IO3_PB6;
/// A generic configuration to map TSC G2_IO4 on PB7 pin
typedef AnyAltOut<Port::PB, 7, AfTSC_G2_IO4_PB7>		TSC_G2_IO4_PB7;

//////////////////////////////////////////////////////////////////////
// QUADSPI
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map QUADSPI BK1_NCS on PA2 pin
typedef AnyAltOut<Port::PA, 2, AfQUADSPI_BK1_NCS_PA2>	QUADSPI_BK1_NCS_PA2;
/// A generic configuration to map QUADSPI CLK on PA3 pin
typedef AnyAltOut<Port::PA, 3, AfQUADSPI_CLK_PA3>		QUADSPI_CLK_PA3;
/// A generic configuration to map QUADSPI IO3 on PA6 pin
typedef AnyAltOut<Port::PA, 6, AfQUADSPI_IO3_PA6>		QUADSPI_IO3_PA6;
/// A generic configuration to map QUADSPI IO2 on PA7 pin
typedef AnyAltOut<Port::PA, 7, AfQUADSPI_IO2_PA7>		QUADSPI_IO2_PA7;
/// A generic configuration to map QUADSPI IO1 on PB0 pin
typedef AnyAltOut<Port::PB, 0, AfQUADSPI_IO1_PB0>		QUADSPI_IO1_PB0;
/// A generic configuration to map QUADSPI IO1 on PB1 pin
typedef AnyAltOut<Port::PB, 1, AfQUADSPI_IO0_PB1>		QUADSPI_IO0_PB1;

//////////////////////////////////////////////////////////////////////
// COMP1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map COMP1 OUT on PA0 pin
typedef AnyAnalog<Port::PA, 0, AfCOMP1_OUT_PA0>			COMP1_OUT_PA0;
/// A generic configuration to map COMP1 OUT on PA6 pin
typedef AnyAnalog<Port::PA, 6, AfCOMP1_OUT_PA6>			COMP1_OUT_PA6;
/// A generic configuration to map COMP1 OUT on PA11 pin
typedef AnyAnalog<Port::PA, 11, AfCOMP1_OUT_PA11>			COMP1_OUT_PA11;
/// A generic configuration to map COMP1 TIM1_BKIN2 on PA6 pin
typedef AnyAnalog<Port::PA, 6, AfCOMP1_TIM1_BKIN2_PA6>	COMP1_TIM1_BKIN2_PA6;
/// A generic configuration to map COMP1 OUT on PB0 pin
typedef AnyAnalog<Port::PB, 0, AfCOMP1_OUT_PB0>			COMP1_OUT_PB0;

//////////////////////////////////////////////////////////////////////
// COMP2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map COMP2 OUT on PA2 pin
typedef AnyAnalog<Port::PA, 2, AfCOMP2_OUT_PA2>			COMP2_OUT_PA2;
/// A generic configuration to map COMP2 TIM1_BKIN on PA6 pin
typedef AnyAnalog<Port::PA, 6, AfCOMP2_TIM1_BKIN_PA6>		COMP2_TIM1_BKIN_PA6;
/// A generic configuration to map COMP2 OUT on PA7 pin
typedef AnyAnalog<Port::PA, 7, AfCOMP2_OUT_PA7>			COMP2_OUT_PA7;
/// A generic configuration to map COMP2 OUT on PB5 pin
typedef AnyAnalog<Port::PB, 5, AfCOMP2_OUT_PB5>			COMP2_OUT_PB5;

//////////////////////////////////////////////////////////////////////
// SWPMI1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map SWPMI1 IO on PA8 pin
typedef AnyAltOut<Port::PA, 8, AfSWPMI1_IO_PA8>			SWPMI1_IO_PA8;
/// A generic configuration to map SWPMI1 TX on PA13 pin
typedef AnyAltOut<Port::PA, 13, AfSWPMI1_TX_PA13>			SWPMI1_TX_PA13;
/// A generic configuration to map SWPMI1 RX on PA14 pin
typedef AnyAltOut<Port::PA, 8, AfSWPMI1_RX_PA14>			SWPMI1_RX_PA14;
/// A generic configuration to map SWPMI1 SUSPEND on PA15 pin
typedef AnyAltOut<Port::PA, 15, AfSWPMI1_SUSPEND_PA15>	SWPMI1_SUSPEND_PA15;

//////////////////////////////////////////////////////////////////////
// EVENTOUT
//////////////////////////////////////////////////////////////////////
/// Generic configurations to map EVENTOUTs
typedef AnyAltOut<Port::PA, 0, AfEVENTOUT_PA0>	EVENTOUT_PA0;
typedef AnyAltOut<Port::PA, 1, AfEVENTOUT_PA1>	EVENTOUT_PA1;
typedef AnyAltOut<Port::PA, 2, AfEVENTOUT_PA2>	EVENTOUT_PA2;
typedef AnyAltOut<Port::PA, 3, AfEVENTOUT_PA3>	EVENTOUT_PA3;
typedef AnyAltOut<Port::PA, 4, AfEVENTOUT_PA4>	EVENTOUT_PA4;
typedef AnyAltOut<Port::PA, 5, AfEVENTOUT_PA5>	EVENTOUT_PA5;
typedef AnyAltOut<Port::PA, 6, AfEVENTOUT_PA6>	EVENTOUT_PA6;
typedef AnyAltOut<Port::PA, 7, AfEVENTOUT_PA7>	EVENTOUT_PA7;
typedef AnyAltOut<Port::PA, 8, AfEVENTOUT_PA8>	EVENTOUT_PA8;
typedef AnyAltOut<Port::PA, 9, AfEVENTOUT_PA9>	EVENTOUT_PA9;
typedef AnyAltOut<Port::PA, 10, AfEVENTOUT_PA10>	EVENTOUT_PA10;
typedef AnyAltOut<Port::PA, 11, AfEVENTOUT_PA11>	EVENTOUT_PA11;
typedef AnyAltOut<Port::PA, 12, AfEVENTOUT_PA12>	EVENTOUT_PA12;
typedef AnyAltOut<Port::PA, 13, AfEVENTOUT_PA13>	EVENTOUT_PA13;
typedef AnyAltOut<Port::PA, 14, AfEVENTOUT_PA14>	EVENTOUT_PA14;
typedef AnyAltOut<Port::PA, 15, AfEVENTOUT_PA15>	EVENTOUT_PA15;
typedef AnyAltOut<Port::PB, 0, AfEVENTOUT_PB0>	EVENTOUT_PB0;
typedef AnyAltOut<Port::PB, 1, AfEVENTOUT_PB1>	EVENTOUT_PB1;
typedef AnyAltOut<Port::PB, 2, AfEVENTOUT_PB2>	EVENTOUT_PB2;
typedef AnyAltOut<Port::PB, 3, AfEVENTOUT_PB3>	EVENTOUT_PB3;
typedef AnyAltOut<Port::PB, 4, AfEVENTOUT_PB4>	EVENTOUT_PB4;
typedef AnyAltOut<Port::PB, 5, AfEVENTOUT_PB5>	EVENTOUT_PB5;
typedef AnyAltOut<Port::PB, 6, AfEVENTOUT_PB6>	EVENTOUT_PB6;
typedef AnyAltOut<Port::PB, 7, AfEVENTOUT_PB7>	EVENTOUT_PB7;
typedef AnyAltOut<Port::PC, 14, AfEVENTOUT_PC14>	EVENTOUT_PC14;
typedef AnyAltOut<Port::PC, 15, AfEVENTOUT_PC15>	EVENTOUT_PC15;
typedef AnyAltOut<Port::PH, 3, AfEVENTOUT_PH3>	EVENTOUT_PH3;


}	// namespace Gpio
}	// namespace Bmt
