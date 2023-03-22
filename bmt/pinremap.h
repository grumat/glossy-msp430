#pragma once

#include "otherlibs.h"

namespace Bmt
{

#ifdef STM32F1

/// A template class for Alternate GPIO Function initialization
template<
	const uint32_t CONF			///< The specific configuration bits
	, const uint32_t MASK		///< The mask to clear the configuration bits
	>
struct AnyAFR
{
	/// A constant defining the bits that are set for the specific configuration
	static constexpr uint32_t kConf = CONF;
	/// A constant defining the mask to clear the specific configuration
	static constexpr uint32_t kMask = MASK;
	/// A constant indicating a bogus configuration used as conditional compilation
	static constexpr bool kNoRemap = (CONF == 0x00000000UL) && (MASK == 0xFFFFFFFFUL);

	/// Enables the alternate function
	ALWAYS_INLINE static void Enable(void)
	{
		if (kNoRemap == false)
			AFIO->MAPR = (AFIO->MAPR & MASK) | CONF;
	}
	/// Disables the alternate function
	ALWAYS_INLINE static void Disable(void)
	{
		if (kNoRemap == false)
			AFIO->MAPR = AFIO->MAPR & MASK;
	}
};


/// No pin remapping (for regular GPIO function)
typedef AnyAFR<0x00000000U, 0xFFFFFFFF> AfNoRemap;

// SPI1
/// SPI1 alternate configuration 1 using PA4, PA5, PA6 and PA7
typedef AnyAFR<0x00000000U, ~AFIO_MAPR_SPI1_REMAP_Msk> AfSpi1_PA4_5_6_7;
/// SPI1 alternate configuration 2 using PA15, PB3, PB4 and PB5
typedef AnyAFR<AFIO_MAPR_SPI1_REMAP, ~AFIO_MAPR_SPI1_REMAP_Msk> AfSpi1_PA15_PB3_4_5;

// I²C1
/// I²C1 alternate configuration 1 using PB6 and PB7
typedef AnyAFR<0x00000000U, ~AFIO_MAPR_I2C1_REMAP_Msk> AfI2C1_PB6_7;
/// I²C1 alternate configuration 2 using PB8 and PB9
typedef AnyAFR<AFIO_MAPR_I2C1_REMAP, ~AFIO_MAPR_I2C1_REMAP_Msk> AfI2C1_PB8_9;

// USART1
/// USART1 alternate configuration 1 using PA9 and PA10
typedef AnyAFR<0x00000000U, ~AFIO_MAPR_USART1_REMAP_Msk> AfUsart1_PA9_10;
/// USART1 alternate configuration 2 using PB6 and PB7
typedef AnyAFR<AFIO_MAPR_USART1_REMAP, ~AFIO_MAPR_USART1_REMAP_Msk> AfUsart1_PB6_7;

// USART2
/// USART2 alternate configuration 1 using PA0, PA1, PA2, PA3 and PA4
typedef AnyAFR<0x00000000U, ~AFIO_MAPR_USART2_REMAP_Msk> AfUsart2_PA0_1_2_3_4;
/// USART2 alternate configuration 2 using PD3, PD4, PD5, PD6 and PD7
typedef AnyAFR<AFIO_MAPR_USART2_REMAP, ~AFIO_MAPR_USART2_REMAP_Msk> AfUsart2_PD3_4_5_6_7;

// USART3
/// USART3 alternate configuration 1 using PB10, PB11, PB12, PB13 and PB14
typedef AnyAFR<AFIO_MAPR_USART3_REMAP_NOREMAP, ~AFIO_MAPR_USART3_REMAP_Msk> AfUsart3_PB10_11_12_13_14;
/// USART3 alternate configuration 2 using PC10, PC11, PC12, PC13 and PC14
typedef AnyAFR<AFIO_MAPR_USART3_REMAP_PARTIALREMAP, ~AFIO_MAPR_USART3_REMAP_Msk> AfUsart3_PC10_11_12_PB13_14;
/// USART3 alternate configuration 3 using PD8, PD9, PD10, PD11 and PD12
typedef AnyAFR<AFIO_MAPR_USART3_REMAP_FULLREMAP, ~AFIO_MAPR_USART3_REMAP_Msk> AfUsart3_PD8_9_10_11_12;

// TIM1
/// TIM1 alternate configuration 1 using PA12, PA8-11 and PB12-15
typedef AnyAFR<AFIO_MAPR_TIM1_REMAP_NOREMAP, ~AFIO_MAPR_TIM1_REMAP_Msk> AfTim1_PA12_8_9_10_11_PB12_13_14_15;
/// TIM1 alternate configuration 2 using PA12, PA8-11, PA6-7 and PB0-1
typedef AnyAFR<AFIO_MAPR_TIM1_REMAP_PARTIALREMAP, ~AFIO_MAPR_TIM1_REMAP_Msk> AfTim1_PA12_8_9_10_11_6_7_PB0_1;
/// TIM1 alternate configuration 3 using PE7, PE9, PE11, PE13-15, PE8, PE10 and PE12
typedef AnyAFR<AFIO_MAPR_TIM1_REMAP_FULLREMAP, ~AFIO_MAPR_TIM1_REMAP_Msk> AfTim1_PE7_9_11_13_14_15_8_10_12;

// TIM2
/// TIM2 alternate configuration 1 using PA0-3
typedef AnyAFR<AFIO_MAPR_TIM2_REMAP_NOREMAP, ~AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1_Msk> AfTim2_PA0_1_2_3;
/// TIM2 alternate configuration 2 using PA15, PB3, and PA2-3
typedef AnyAFR<AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1, ~AFIO_MAPR_TIM2_REMAP_Msk> AfTim2_PA15_PB3_PA2_3;
/// TIM2 alternate configuration 3 using PA0-1 and PB10-11
typedef AnyAFR<AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2, ~AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2_Msk> AfTim2_PA0_1_PB10_11;
/// TIM2 alternate configuration 4 using PA15, PB3 and PB10-11
typedef AnyAFR<AFIO_MAPR_TIM2_REMAP_FULLREMAP, ~AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2_Msk> AfTim2_PA15_PB3_10_11;

// TIM3
/// TIM3 alternate configuration 1 using PA6-7 and PB0-1
typedef AnyAFR<AFIO_MAPR_TIM3_REMAP_NOREMAP, ~AFIO_MAPR_TIM3_REMAP_Msk> AfTim3_PA6_7_PB0_1;
/// TIM3 alternate configuration 2 using PB4-5 and PB0-1
typedef AnyAFR<AFIO_MAPR_TIM3_REMAP_PARTIALREMAP, ~AFIO_MAPR_TIM3_REMAP_Msk> AfTim3_PB4_5_0_1;
/// TIM3 alternate configuration 3 using PC6-9
typedef AnyAFR<AFIO_MAPR_TIM3_REMAP_FULLREMAP, ~AFIO_MAPR_TIM3_REMAP_Msk> AfTim3_PC6_7_8_9;

// TIM4
/// TIM3 alternate configuration 1 using PB6-9
typedef AnyAFR<0x00000000U, ~AFIO_MAPR_TIM4_REMAP_Msk> AfTim4_PB6_7_8_9;
/// TIM3 alternate configuration 2 using PD12-15
typedef AnyAFR<AFIO_MAPR_TIM4_REMAP, ~AFIO_MAPR_TIM4_REMAP_Msk> AfTim4_PD12_13_14_15;

// CAN
/// CAN alternate configuration 1 using PA11-12
typedef AnyAFR<AFIO_MAPR_CAN_REMAP_REMAP1, ~AFIO_MAPR_CAN_REMAP_Msk> AfCan_PA11_12;
/// CAN alternate configuration 2 using PB8-9
typedef AnyAFR<AFIO_MAPR_CAN_REMAP_REMAP2, ~AFIO_MAPR_CAN_REMAP_Msk> AfCan_PB8_9;
/// CAN alternate configuration 3 using PD0-1
typedef AnyAFR<AFIO_MAPR_CAN_REMAP_REMAP3, ~AFIO_MAPR_CAN_REMAP_Msk> AfCan_PD0_1;

// OSC
/// OSC alternate configuration 1 using PD0-1
typedef AnyAFR<0x00000000U, ~AFIO_MAPR_PD01_REMAP_Msk> Af_PD01_OSC;
/// GPIO configuration for port PD0-1
typedef AnyAFR<AFIO_MAPR_PD01_REMAP, ~AFIO_MAPR_PD01_REMAP_Msk> Af_PD01_GPIO;

// JTAG/SWD
/// 5-pin JTAG bus active
typedef AnyAFR<AFIO_MAPR_SWJ_CFG_RESET, ~AFIO_MAPR_SWJ_CFG_Msk>		AfSwjFull;
/// 4-pin JTAG bus active
typedef AnyAFR<AFIO_MAPR_SWJ_CFG_NOJNTRST, ~AFIO_MAPR_SWJ_CFG_Msk>		AfSwj4Pin;
/// 2-pin JTAG bus active (3-pin with optional SWO)
typedef AnyAFR<AFIO_MAPR_SWJ_CFG_JTAGDISABLE, ~AFIO_MAPR_SWJ_CFG_Msk>	AfSwj2Pin;
/// No emulation active
typedef AnyAFR<AFIO_MAPR_SWJ_CFG_DISABLE, ~AFIO_MAPR_SWJ_CFG_Msk>		AfSwjDisabled;

#elif defined(STM32L4)

#include "mcu-system.h"

namespace Gpio
{


/// Alternate function value
enum class AF : uint8_t
{
	k0,
	k1,
	k2,
	k3,
	k4,
	k5,
	k6,
	k7,
	k8,
	k9,
	k10,
	k11,
	k12,
	k13,
	k14,
	k15,
};

/// A template class for Alternate GPIO Function initialization
template<
	const GpioPortId kPort		///< The specific configuration bits
	, const uint8_t kBit		///< Port bit for the configuration
	, const AF kAfr		///< The value for AFRx register
	>
struct AnyAFR
{
	/// A constant defining the port associated to the configuration
	static constexpr GpioPortId kPort_ = kPort;
	/// A constant defining the bit associated with the configuration
	static constexpr uint8_t kBit_ = kBit;
	/// A constant defining the AFRx register
	static constexpr AF kAfr_ = kAfr;
	/// value for the AFRL register
	static constexpr uint32_t kAFRL_ = (kBit_ >= 8) ? 0UL
		: uint32_t(kAfr_) * (16 * kBit);
	static constexpr uint32_t kAFRL_Mask_ = (kBit_ >= 8) ? 0UL
		: ~(0b1111UL * (16 * kBit));
	/// value for the AFRH register
	static constexpr uint32_t kAFRH_ = (kBit_ < 8) ? 0UL
		: uint32_t(kAfr_) * (16 * (kBit_ - 8));
	static constexpr uint32_t kAFRH_Mask_ = (kBit_ < 8) ? 0UL
		: ~(0b1111UL * (16 * (kBit_ - 8)));
	/// Base address of the port peripheral
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);

	/// Access to the peripheral memory space
	constexpr static volatile GPIO_TypeDef *Io() { return (volatile GPIO_TypeDef *)kPortBase_; }

	/// Enables the alternate function
	ALWAYS_INLINE static void Enable(void)
	{
		if (kAFRL_Mask_ != 0UL)
		{
			volatile GPIO_TypeDef *port = Io();
			port->AFR[0] = (port->AFR[0] & kAFRL_Mask_) | kAFRL_;
		}
		if (kAFRH_Mask_ != 0UL)
		{
			volatile GPIO_TypeDef *port = Io();
			port->AFR[1] = (port->AFR[1] & kAFRH_Mask_) | kAFRH_;
		}
	}
	/// Disables the alternate function
	ALWAYS_INLINE static void Disable(void)
	{
		if (kAFRL_Mask_ != 0UL)
			Io()->AFR[0] &= kAFRL_Mask_;
		if (kAFRH_Mask_ != 0UL)
			Io()->AFR[1] &= kAFRH_Mask_;
	}
};


// SYS
typedef AnyAFR<GpioPortId::PA, 8, AF::k0>	AfSYS_MCO_PA8;
typedef AnyAFR<GpioPortId::PA, 13, AF::k0>	AfSYS_JTMS_PA13;
typedef AnyAFR<GpioPortId::PA, 13, AF::k0>	AfSYS_SWDIO_PA13;
typedef AnyAFR<GpioPortId::PA, 14, AF::k0>	AfSYS_JTCK_PA14;
typedef AnyAFR<GpioPortId::PA, 14, AF::k0>	AfSYS_SWCLK_PA14;
typedef AnyAFR<GpioPortId::PA, 15, AF::k0>	AfSYS_JTDI_PA15;
typedef AnyAFR<GpioPortId::PB, 3, AF::k0>	AfSYS_JTDO_PB3;
typedef AnyAFR<GpioPortId::PB, 3, AF::k0>	AfSYS_TRACESWO_PB3;
typedef AnyAFR<GpioPortId::PB, 4, AF::k0>	AfSYS_NJTRST_PB4;

// TIM1
typedef AnyAFR<GpioPortId::PA, 6, AF::k1>	AfTIM1_BKIN_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k1>	AfTIM1_CH1N_PA7;
typedef AnyAFR<GpioPortId::PA, 8, AF::k1>	AfTIM1_CH1_PA8;
typedef AnyAFR<GpioPortId::PA, 9, AF::k1>	AfTIM1_CH2_PA9;
typedef AnyAFR<GpioPortId::PA, 10, AF::k1>	AfTIM1_CH3_PA10;
typedef AnyAFR<GpioPortId::PA, 11, AF::k1>	AfTIM1_CH4_PA11;
typedef AnyAFR<GpioPortId::PA, 11, AF::k2>	AfTIM1_BKIN2_PA11;
typedef AnyAFR<GpioPortId::PA, 12, AF::k1>	AfTIM1_ETR_PA12;
typedef AnyAFR<GpioPortId::PB, 0, AF::k1>	AfTIM1_CH2N_PB0;
typedef AnyAFR<GpioPortId::PB, 1, AF::k1>	AfTIM1_CH3N_PB1;

// TIM2
typedef AnyAFR<GpioPortId::PA, 0, AF::k1>	AfTIM2_CH1_PA0;
typedef AnyAFR<GpioPortId::PA, 1, AF::k1>	AfTIM2_CH2_PA1;
typedef AnyAFR<GpioPortId::PA, 2, AF::k1>	AfTIM2_CH3_PA2;
typedef AnyAFR<GpioPortId::PA, 3, AF::k1>	AfTIM2_CH4_PA3;
typedef AnyAFR<GpioPortId::PA, 5, AF::k1>	AfTIM2_CH1_PA5;
typedef AnyAFR<GpioPortId::PA, 5, AF::k2>	AfTIM2_ETR_PA5;
typedef AnyAFR<GpioPortId::PA, 15, AF::k1>	AfTIM2_CH1_PA15;
typedef AnyAFR<GpioPortId::PA, 15, AF::k2>	AfTIM2_ETR_PA15;
typedef AnyAFR<GpioPortId::PB, 3, AF::k1>	AfTIM2_CH2_PB3;

// LPTIM1
typedef AnyAFR<GpioPortId::PA, 14, AF::k1>	AfLPTIM1_OUT_PA14;
typedef AnyAFR<GpioPortId::PB, 5, AF::k1>	AfLPTIM1_IN1_PB5;
typedef AnyAFR<GpioPortId::PB, 6, AF::k1>	AfLPTIM1_ETR_PB6;
typedef AnyAFR<GpioPortId::PB, 7, AF::k1>	AfLPTIM1_IN2_PB7;

// IR
typedef AnyAFR<GpioPortId::PA, 13, AF::k1>	AfIR_OUT_PA13;

// I²C1
typedef AnyAFR<GpioPortId::PA, 1, AF::k4>	AfI2C1_SMBA_PA1;
typedef AnyAFR<GpioPortId::PA, 9, AF::k4>	AfI2C1_SCL_PA9;
typedef AnyAFR<GpioPortId::PA, 10, AF::k4>	AfI2C1_SDA_PA10;
typedef AnyAFR<GpioPortId::PA, 14, AF::k4>	AfI2C1_SMBA_PA14;
typedef AnyAFR<GpioPortId::PB, 5, AF::k4>	AfI2C1_SMBA_PA5;
typedef AnyAFR<GpioPortId::PB, 6, AF::k4>	AfI2C1_SCL_PB6;
typedef AnyAFR<GpioPortId::PB, 7, AF::k4>	AfI2C1_SDA_PB7;

// I²C3
typedef AnyAFR<GpioPortId::PA, 7, AF::k4>	AfI2C3_SCL_PA7;
typedef AnyAFR<GpioPortId::PB, 4, AF::k4>	AfI2C3_SDA_PB4;

// SPI1
typedef AnyAFR<GpioPortId::PA, 1, AF::k5>	AfSPI1SCK_PA1;
typedef AnyAFR<GpioPortId::PA, 4, AF::k5>	AfSPI1NSS_PA4;
typedef AnyAFR<GpioPortId::PA, 5, AF::k5>	AfSPI1SCK_PA5;
typedef AnyAFR<GpioPortId::PA, 6, AF::k5>	AfSPI1MISO_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k5>	AfSPI1MOSI_PA7;
typedef AnyAFR<GpioPortId::PA, 11, AF::k5>	AfSPI1MISO_PA11;
typedef AnyAFR<GpioPortId::PA, 12, AF::k5>	AfSPI1MOSI_PA12;
typedef AnyAFR<GpioPortId::PA, 15, AF::k5>	AfSPI1NSS_PA15;
typedef AnyAFR<GpioPortId::PB, 0, AF::k5>	AfSPI1NSS_PB0;
typedef AnyAFR<GpioPortId::PB, 3, AF::k5>	AfSPI1SCK_PB3;
typedef AnyAFR<GpioPortId::PB, 4, AF::k5>	AfSPI1MISO_PB4;
typedef AnyAFR<GpioPortId::PB, 5, AF::k5>	AfSPI1MOSI_PB5;

// SPI3
typedef AnyAFR<GpioPortId::PA, 4, AF::k6>	AfSPI3NSS_PA4;
typedef AnyAFR<GpioPortId::PA, 15, AF::k6>	AfSPI3NSS_PA15;
typedef AnyAFR<GpioPortId::PB, 3, AF::k6>	AfSPI3SCK_PB3;
typedef AnyAFR<GpioPortId::PB, 4, AF::k6>	AfSPI3MISO_PB4;
typedef AnyAFR<GpioPortId::PB, 5, AF::k6>	AfSPI3MOSI_PB5;

// USART1
typedef AnyAFR<GpioPortId::PA, 8, AF::k7>	AfUSART1_CK_PA8;
typedef AnyAFR<GpioPortId::PA, 9, AF::k7>	AfUSART1_TX_PA9;
typedef AnyAFR<GpioPortId::PA, 10, AF::k7>	AfUSART1_RX_PA10;
typedef AnyAFR<GpioPortId::PA, 11, AF::k7>	AfUSART1_CTS_PA11;
typedef AnyAFR<GpioPortId::PA, 12, AF::k7>	AfUSART1_RTS_PA12;
typedef AnyAFR<GpioPortId::PB, 3, AF::k7>	AfUSART1_RTS_PB3;
typedef AnyAFR<GpioPortId::PB, 4, AF::k7>	AfUSART1_CTS_PB4;
typedef AnyAFR<GpioPortId::PB, 5, AF::k7>	AfUSART1_CK_PB5;
typedef AnyAFR<GpioPortId::PB, 6, AF::k7>	AfUSART1_TX_PB6;
typedef AnyAFR<GpioPortId::PB, 7, AF::k7>	AfUSART1_RX_PB7;

// USART2
typedef AnyAFR<GpioPortId::PA, 0, AF::k7>	AfUSART2_CTS_PA0;
typedef AnyAFR<GpioPortId::PA, 1, AF::k7>	AfUSART2_RTS_PA1;
typedef AnyAFR<GpioPortId::PA, 2, AF::k7>	AfUSART2_TX_PA2;
typedef AnyAFR<GpioPortId::PA, 3, AF::k7>	AfUSART2_RX_PA3;
typedef AnyAFR<GpioPortId::PA, 4, AF::k7>	AfUSART2_CK_PA4;
typedef AnyAFR<GpioPortId::PA, 15, AF::k3>	AfUSART2_RX_PA15;

// USART3
typedef AnyAFR<GpioPortId::PA, 6, AF::k7>	AfUSART3_CTS_PA6;
typedef AnyAFR<GpioPortId::PA, 15, AF::k7>	AfUSART3_RTS_PA15;
typedef AnyAFR<GpioPortId::PB, 0, AF::k7>	AfUSART3_CK_PB0;
typedef AnyAFR<GpioPortId::PB, 1, AF::k7>	AfUSART3_RTS_PB1;

// LPUART1
typedef AnyAFR<GpioPortId::PA, 2, AF::k8>	AfUART1_TX_PA2;
typedef AnyAFR<GpioPortId::PA, 3, AF::k8>	AfUART1_RX_PA3;
typedef AnyAFR<GpioPortId::PA, 6, AF::k8>	AfUART1_CTS_PA6;
typedef AnyAFR<GpioPortId::PB, 1, AF::k8>	AfUART1_RTS_PB1;

// CAN1
typedef AnyAFR<GpioPortId::PA, 11, AF::k9>	AfCAN1_RX_PA11;
typedef AnyAFR<GpioPortId::PA, 12, AF::k9>	AfCAN1_TX_PA12;

// TSC
typedef AnyAFR<GpioPortId::PA, 15, AF::k9>	AfTSC_G3_IO1_PA15;
typedef AnyAFR<GpioPortId::PB, 4, AF::k9>	AfTSC_G2_IO1_PB4;
typedef AnyAFR<GpioPortId::PB, 5, AF::k9>	AfTSC_G2_IO2_PB5;
typedef AnyAFR<GpioPortId::PB, 6, AF::k9>	AfTSC_G2_IO3_PB6;
typedef AnyAFR<GpioPortId::PB, 7, AF::k9>	AfTSC_G2_IO4_PB7;

// USB
typedef AnyAFR<GpioPortId::PA, 10, AF::k10>	AfUSB_CRS_SYNC_PA10;
typedef AnyAFR<GpioPortId::PA, 11, AF::k10>	AfUSB_DM_PA11;
typedef AnyAFR<GpioPortId::PA, 12, AF::k10>	AfUSB_DP_PA12;
typedef AnyAFR<GpioPortId::PA, 13, AF::k10>	AfUSB_NOE_PA13;

// QUADSPI
typedef AnyAFR<GpioPortId::PA, 2, AF::k10>	AfQUADSPI_BK1_NCS_PA2;
typedef AnyAFR<GpioPortId::PA, 3, AF::k10>	AfQUADSPI_CLK_PA3;
typedef AnyAFR<GpioPortId::PA, 6, AF::k10>	AfQUADSPI_IO3_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k10>	AfQUADSPI_IO2_PA7;
typedef AnyAFR<GpioPortId::PB, 0, AF::k10>	AfQUADSPI_IO1_PB0;
typedef AnyAFR<GpioPortId::PB, 1, AF::k10>	AfQUADSPI_IO0_PB1;

// COMP1
typedef AnyAFR<GpioPortId::PA, 0, AF::k12>	AfCOMP1_OUT_PA0;
typedef AnyAFR<GpioPortId::PA, 6, AF::k6>	AfCOMP1_OUT_PA6;
typedef AnyAFR<GpioPortId::PA, 11, AF::k6>	AfCOMP1_OUT_PA11;
typedef AnyAFR<GpioPortId::PA, 11, AF::k12>	AfCOMP1_TIM1_BKIN2_PA6;
typedef AnyAFR<GpioPortId::PB, 0, AF::k12>	AfCOMP1_OUT_PB0;

// COMP2
typedef AnyAFR<GpioPortId::PA, 2, AF::k12>	AfCOMP2_OUT_PA2;
typedef AnyAFR<GpioPortId::PA, 6, AF::k12>	AfCOMP2_TIM1_BKIN_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k12>	AfCOMP2_OUT_PA7;
typedef AnyAFR<GpioPortId::PB, 5, AF::k12>	AfCOMP2_OUT_PB5;

// SWPMI1
typedef AnyAFR<GpioPortId::PA, 8, AF::k12>	AfSWPMI1_IO_PA8;
typedef AnyAFR<GpioPortId::PA, 13, AF::k12>	AfSWPMI1_TX_PA13;
typedef AnyAFR<GpioPortId::PA, 14, AF::k12>	AfSWPMI1_RX_PA14;
typedef AnyAFR<GpioPortId::PA, 15, AF::k12>	AfSWPMI1_SUSPEND_PA15;


}	// namespace Gpio

#else
#	error "Unsupported MCU"
#endif


}	// namespace Bmt
