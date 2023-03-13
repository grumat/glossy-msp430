#pragma once

#include "otherlibs.h"

#ifdef STM32F1

namespace Bmt
{

/// A template class for Alternate GPIO Function initialization
template<
	const uint32_t CONF			///< The specific configuration bits
	, const uint32_t MASK		///< The mask to clear the configuration bits
	>
struct AfRemapTemplate
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
typedef AfRemapTemplate<0x00000000U, 0xFFFFFFFF> AfNoRemap;

// SPI1
/// SPI1 alternate configuration 1 using PA4, PA5, PA6 and PA7
typedef AfRemapTemplate<0x00000000U, ~AFIO_MAPR_SPI1_REMAP_Msk> AfSpi1_PA4_5_6_7;
/// SPI1 alternate configuration 2 using PA15, PB3, PB4 and PB5
typedef AfRemapTemplate<AFIO_MAPR_SPI1_REMAP, ~AFIO_MAPR_SPI1_REMAP_Msk> AfSpi1_PA15_PB3_4_5;

// I²C1
/// I²C1 alternate configuration 1 using PB6 and PB7
typedef AfRemapTemplate<0x00000000U, ~AFIO_MAPR_I2C1_REMAP_Msk> AfI2C1_PB6_7;
/// I²C1 alternate configuration 2 using PB8 and PB9
typedef AfRemapTemplate<AFIO_MAPR_I2C1_REMAP, ~AFIO_MAPR_I2C1_REMAP_Msk> AfI2C1_PB8_9;

// USART1
/// USART1 alternate configuration 1 using PA9 and PA10
typedef AfRemapTemplate<0x00000000U, ~AFIO_MAPR_USART1_REMAP_Msk> AfUsart1_PA9_10;
/// USART1 alternate configuration 2 using PB6 and PB7
typedef AfRemapTemplate<AFIO_MAPR_USART1_REMAP, ~AFIO_MAPR_USART1_REMAP_Msk> AfUsart1_PB6_7;

// USART2
/// USART2 alternate configuration 1 using PA0, PA1, PA2, PA3 and PA4
typedef AfRemapTemplate<0x00000000U, ~AFIO_MAPR_USART2_REMAP_Msk> AfUsart2_PA0_1_2_3_4;
/// USART2 alternate configuration 2 using PD3, PD4, PD5, PD6 and PD7
typedef AfRemapTemplate<AFIO_MAPR_USART2_REMAP, ~AFIO_MAPR_USART2_REMAP_Msk> AfUsart2_PD3_4_5_6_7;

// USART3
/// USART3 alternate configuration 1 using PB10, PB11, PB12, PB13 and PB14
typedef AfRemapTemplate<AFIO_MAPR_USART3_REMAP_NOREMAP, ~AFIO_MAPR_USART3_REMAP_Msk> AfUsart3_PB10_11_12_13_14;
/// USART3 alternate configuration 2 using PC10, PC11, PC12, PC13 and PC14
typedef AfRemapTemplate<AFIO_MAPR_USART3_REMAP_PARTIALREMAP, ~AFIO_MAPR_USART3_REMAP_Msk> AfUsart3_PC10_11_12_PB13_14;
/// USART3 alternate configuration 3 using PD8, PD9, PD10, PD11 and PD12
typedef AfRemapTemplate<AFIO_MAPR_USART3_REMAP_FULLREMAP, ~AFIO_MAPR_USART3_REMAP_Msk> AfUsart3_PD8_9_10_11_12;

// TIM1
/// TIM1 alternate configuration 1 using PA12, PA8-11 and PB12-15
typedef AfRemapTemplate<AFIO_MAPR_TIM1_REMAP_NOREMAP, ~AFIO_MAPR_TIM1_REMAP_Msk> AfTim1_PA12_8_9_10_11_PB12_13_14_15;
/// TIM1 alternate configuration 2 using PA12, PA8-11, PA6-7 and PB0-1
typedef AfRemapTemplate<AFIO_MAPR_TIM1_REMAP_PARTIALREMAP, ~AFIO_MAPR_TIM1_REMAP_Msk> AfTim1_PA12_8_9_10_11_6_7_PB0_1;
/// TIM1 alternate configuration 3 using PE7, PE9, PE11, PE13-15, PE8, PE10 and PE12
typedef AfRemapTemplate<AFIO_MAPR_TIM1_REMAP_FULLREMAP, ~AFIO_MAPR_TIM1_REMAP_Msk> AfTim1_PE7_9_11_13_14_15_8_10_12;

// TIM2
/// TIM2 alternate configuration 1 using PA0-3
typedef AfRemapTemplate<AFIO_MAPR_TIM2_REMAP_NOREMAP, ~AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1_Msk> AfTim2_PA0_1_2_3;
/// TIM2 alternate configuration 2 using PA15, PB3, and PA2-3
typedef AfRemapTemplate<AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1, ~AFIO_MAPR_TIM2_REMAP_Msk> AfTim2_PA15_PB3_PA2_3;
/// TIM2 alternate configuration 3 using PA0-1 and PB10-11
typedef AfRemapTemplate<AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2, ~AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2_Msk> AfTim2_PA0_1_PB10_11;
/// TIM2 alternate configuration 4 using PA15, PB3 and PB10-11
typedef AfRemapTemplate<AFIO_MAPR_TIM2_REMAP_FULLREMAP, ~AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2_Msk> AfTim2_PA15_PB3_10_11;

// TIM3
/// TIM3 alternate configuration 1 using PA6-7 and PB0-1
typedef AfRemapTemplate<AFIO_MAPR_TIM3_REMAP_NOREMAP, ~AFIO_MAPR_TIM3_REMAP_Msk> AfTim3_PA6_7_PB0_1;
/// TIM3 alternate configuration 2 using PB4-5 and PB0-1
typedef AfRemapTemplate<AFIO_MAPR_TIM3_REMAP_PARTIALREMAP, ~AFIO_MAPR_TIM3_REMAP_Msk> AfTim3_PB4_5_0_1;
/// TIM3 alternate configuration 3 using PC6-9
typedef AfRemapTemplate<AFIO_MAPR_TIM3_REMAP_FULLREMAP, ~AFIO_MAPR_TIM3_REMAP_Msk> AfTim3_PC6_7_8_9;

// TIM4
/// TIM3 alternate configuration 1 using PB6-9
typedef AfRemapTemplate<0x00000000U, ~AFIO_MAPR_TIM4_REMAP_Msk> AfTim4_PB6_7_8_9;
/// TIM3 alternate configuration 2 using PD12-15
typedef AfRemapTemplate<AFIO_MAPR_TIM4_REMAP, ~AFIO_MAPR_TIM4_REMAP_Msk> AfTim4_PD12_13_14_15;

// CAN
/// CAN alternate configuration 1 using PA11-12
typedef AfRemapTemplate<AFIO_MAPR_CAN_REMAP_REMAP1, ~AFIO_MAPR_CAN_REMAP_Msk> AfCan_PA11_12;
/// CAN alternate configuration 2 using PB8-9
typedef AfRemapTemplate<AFIO_MAPR_CAN_REMAP_REMAP2, ~AFIO_MAPR_CAN_REMAP_Msk> AfCan_PB8_9;
/// CAN alternate configuration 3 using PD0-1
typedef AfRemapTemplate<AFIO_MAPR_CAN_REMAP_REMAP3, ~AFIO_MAPR_CAN_REMAP_Msk> AfCan_PD0_1;

// OSC
/// OSC alternate configuration 1 using PD0-1
typedef AfRemapTemplate<0x00000000U, ~AFIO_MAPR_PD01_REMAP_Msk> Af_PD01_OSC;
/// GPIO configuration for port PD0-1
typedef AfRemapTemplate<AFIO_MAPR_PD01_REMAP, ~AFIO_MAPR_PD01_REMAP_Msk> Af_PD01_GPIO;

// JTAG/SWD
/// 5-pin JTAG bus active
typedef AfRemapTemplate<AFIO_MAPR_SWJ_CFG_RESET, ~AFIO_MAPR_SWJ_CFG_Msk>		AfSwjFull;
/// 4-pin JTAG bus active
typedef AfRemapTemplate<AFIO_MAPR_SWJ_CFG_NOJNTRST, ~AFIO_MAPR_SWJ_CFG_Msk>		AfSwj4Pin;
/// 2-pin JTAG bus active (3-pin with optional SWO)
typedef AfRemapTemplate<AFIO_MAPR_SWJ_CFG_JTAGDISABLE, ~AFIO_MAPR_SWJ_CFG_Msk>	AfSwj2Pin;
/// No emulation active
typedef AfRemapTemplate<AFIO_MAPR_SWJ_CFG_DISABLE, ~AFIO_MAPR_SWJ_CFG_Msk>		AfSwjDisabled;


}	// namespace Bmt

#endif
