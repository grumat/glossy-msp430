#pragma once

namespace Bmt
{
namespace Gpio
{

// SYS
typedef AnyAFR<Port::PA, 8, AF::k0>		AfMCO_PA8;
typedef AnyAFR<Port::PA, 13, AF::k0>	AfJTMS_PA13;
typedef AnyAFR<Port::PA, 13, AF::k0>	AfSWDIO_PA13;
typedef AnyAFR<Port::PA, 14, AF::k0>	AfJTCK_PA14;
typedef AnyAFR<Port::PA, 14, AF::k0>	AfSWCLK_PA14;
typedef AnyAFR<Port::PA, 15, AF::k0>	AfJTDI_PA15;
typedef AnyAFR<Port::PB, 3, AF::k0>		AfJTDO_PB3;
typedef AnyAFR<Port::PB, 3, AF::k0>		AfTRACESWO_PB3;
typedef AnyAFR<Port::PB, 4, AF::k0>		AfNJTRST_PB4;

// TIM1
typedef AnyAFR<Port::PA, 6, AF::k1>		AfTIM1_BKIN_PA6;
typedef AnyAFR<Port::PA, 6, AF::k12>	AfTIM1_BKIN_COMP2_PA6;
typedef AnyAFR<Port::PA, 7, AF::k1>		AfTIM1_CH1N_PA7;
typedef AnyAFR<Port::PA, 8, AF::k1>		AfTIM1_CH1_PA8;
typedef AnyAFR<Port::PA, 9, AF::k1>		AfTIM1_CH2_PA9;
typedef AnyAFR<Port::PA, 10, AF::k1>	AfTIM1_CH3_PA10;
typedef AnyAFR<Port::PA, 11, AF::k1>	AfTIM1_CH4_PA11;
typedef AnyAFR<Port::PA, 11, AF::k2>	AfTIM1_BKIN2_PA11;
typedef AnyAFR<Port::PA, 11, AF::k12>	AfTIM1_BKIN2_COMP1_PA11;
typedef AnyAFR<Port::PA, 12, AF::k1>	AfTIM1_ETR_PA12;
typedef AnyAFR<Port::PB, 0, AF::k1>		AfTIM1_CH2N_PB0;
typedef AnyAFR<Port::PB, 1, AF::k1>		AfTIM1_CH3N_PB1;

// TIM2
typedef AnyAFR<Port::PA, 0, AF::k1>		AfTIM2_CH1_PA0;
typedef AnyAFR<Port::PA, 0, AF::k14>	AfTIM2_ETR_PA0;
typedef AnyAFR<Port::PA, 1, AF::k1>		AfTIM2_CH2_PA1;
typedef AnyAFR<Port::PA, 2, AF::k1>		AfTIM2_CH3_PA2;
typedef AnyAFR<Port::PA, 3, AF::k1>		AfTIM2_CH4_PA3;
typedef AnyAFR<Port::PA, 5, AF::k1>		AfTIM2_CH1_PA5;
typedef AnyAFR<Port::PA, 5, AF::k2>		AfTIM2_ETR_PA5;
typedef AnyAFR<Port::PA, 15, AF::k1>	AfTIM2_CH1_PA15;
typedef AnyAFR<Port::PA, 15, AF::k2>	AfTIM2_ETR_PA15;
typedef AnyAFR<Port::PB, 3, AF::k1>		AfTIM2_CH2_PB3;

// TIM15
typedef AnyAFR<Port::PA, 1, AF::k14>	AfTIM15_CH1N_PA1;
typedef AnyAFR<Port::PA, 2, AF::k14>	AfTIM15_CH1_PA2;
typedef AnyAFR<Port::PA, 3, AF::k14>	AfTIM15_CH2_PA3;
typedef AnyAFR<Port::PA, 9, AF::k14>	AfTIM15_BKIN_PA9;

// TIM16
typedef AnyAFR<Port::PA, 6, AF::k14>	AfTIM16_CH1_PA6;
typedef AnyAFR<Port::PB, 5, AF::k14>	AfTIM16_BKIN_PB5;
typedef AnyAFR<Port::PB, 6, AF::k14>	AfTIM16_CH1N_PB6;

// LPTIM1
typedef AnyAFR<Port::PA, 14, AF::k1>	AfLPTIM1_OUT_PA14;
typedef AnyAFR<Port::PB, 5, AF::k1>		AfLPTIM1_IN1_PB5;
typedef AnyAFR<Port::PB, 6, AF::k1>		AfLPTIM1_ETR_PB6;
typedef AnyAFR<Port::PB, 7, AF::k1>		AfLPTIM1_IN2_PB7;

// LPTIM2
typedef AnyAFR<Port::PA, 4, AF::k14>	AfLPTIM2_OUT_PA4;
typedef AnyAFR<Port::PA, 5, AF::k14>	AfLPTIM2_ETR_PA5;
typedef AnyAFR<Port::PA, 8, AF::k14>	AfLPTIM2_OUT_PA8;
typedef AnyAFR<Port::PB, 1, AF::k14>	AfLPTIM2_IN1_PB1;

// IR
typedef AnyAFR<Port::PA, 13, AF::k1>	AfIR_OUT_PA13;

// I2C1
typedef AnyAFR<Port::PA, 1, AF::k4>		AfI2C1_SMBA_PA1;

// I2C3
typedef AnyAFR<Port::PA, 7, AF::k4>		AfI2C3_SCL_PA7;

// I2C1
typedef AnyAFR<Port::PA, 9, AF::k4>		AfI2C1_SCL_PA9;
typedef AnyAFR<Port::PA, 10, AF::k4>	AfI2C1_SDA_PA10;
typedef AnyAFR<Port::PA, 14, AF::k4>	AfI2C1_SMBA_PA14;

// I2C3
typedef AnyAFR<Port::PB, 4, AF::k4>		AfI2C3_SDA_PB4;

// I2C1
typedef AnyAFR<Port::PB, 5, AF::k4>		AfI2C1_SMBA_PB5;
typedef AnyAFR<Port::PB, 6, AF::k4>		AfI2C1_SCL_PB6;
typedef AnyAFR<Port::PB, 7, AF::k4>		AfI2C1_SDA_PB7;

// SPI1
typedef AnyAFR<Port::PA, 1, AF::k5>		AfSPI1_SCK_PA1;
typedef AnyAFR<Port::PA, 4, AF::k5>		AfSPI1_NSS_PA4;
typedef AnyAFR<Port::PA, 5, AF::k5>		AfSPI1_SCK_PA5;
typedef AnyAFR<Port::PA, 6, AF::k5>		AfSPI1_MISO_PA6;
typedef AnyAFR<Port::PA, 7, AF::k5>		AfSPI1_MOSI_PA7;
typedef AnyAFR<Port::PA, 11, AF::k5>	AfSPI1_MISO_PA11;
typedef AnyAFR<Port::PA, 12, AF::k5>	AfSPI1_MOSI_PA12;
typedef AnyAFR<Port::PA, 15, AF::k5>	AfSPI1_NSS_PA15;
typedef AnyAFR<Port::PB, 0, AF::k5>		AfSPI1_NSS_PB0;
typedef AnyAFR<Port::PB, 3, AF::k5>		AfSPI1_SCK_PB3;
typedef AnyAFR<Port::PB, 4, AF::k5>		AfSPI1_MISO_PB4;
typedef AnyAFR<Port::PB, 5, AF::k5>		AfSPI1_MOSI_PB5;

// SPI3
typedef AnyAFR<Port::PA, 4, AF::k6>		AfSPI3_NSS_PA4;
typedef AnyAFR<Port::PA, 15, AF::k6>	AfSPI3_NSS_PA15;
typedef AnyAFR<Port::PB, 3, AF::k6>		AfSPI3_SCK_PB3;
typedef AnyAFR<Port::PB, 4, AF::k6>		AfSPI3_MISO_PB4;
typedef AnyAFR<Port::PB, 5, AF::k6>		AfSPI3_MOSI_PB5;

// USART1
typedef AnyAFR<Port::PA, 8, AF::k7>		AfUSART1_CK_PA8;
typedef AnyAFR<Port::PA, 9, AF::k7>		AfUSART1_TX_PA9;
typedef AnyAFR<Port::PA, 10, AF::k7>	AfUSART1_RX_PA10;
typedef AnyAFR<Port::PA, 11, AF::k7>	AfUSART1_CTS_PA11;
typedef AnyAFR<Port::PA, 12, AF::k7>	AfUSART1_DE_PA12;
typedef AnyAFR<Port::PA, 12, AF::k7>	AfUSART1_RTS_PA12;
typedef AnyAFR<Port::PB, 3, AF::k7>		AfUSART1_DE_PB3;
typedef AnyAFR<Port::PB, 3, AF::k7>		AfUSART1_RTS_PB3;
typedef AnyAFR<Port::PB, 4, AF::k7>		AfUSART1_CTS_PB4;
typedef AnyAFR<Port::PB, 5, AF::k7>		AfUSART1_CK_PB5;
typedef AnyAFR<Port::PB, 6, AF::k7>		AfUSART1_TX_PB6;
typedef AnyAFR<Port::PB, 7, AF::k7>		AfUSART1_RX_PB7;

// USART2
typedef AnyAFR<Port::PA, 0, AF::k7>		AfUSART2_CTS_PA0;
typedef AnyAFR<Port::PA, 1, AF::k7>		AfUSART2_DE_PA1;
typedef AnyAFR<Port::PA, 1, AF::k7>		AfUSART2_RTS_PA1;
typedef AnyAFR<Port::PA, 2, AF::k7>		AfUSART2_TX_PA2;
typedef AnyAFR<Port::PA, 3, AF::k7>		AfUSART2_RX_PA3;
typedef AnyAFR<Port::PA, 4, AF::k7>		AfUSART2_CK_PA4;
typedef AnyAFR<Port::PA, 15, AF::k3>	AfUSART2_RX_PA15;

// USART3
typedef AnyAFR<Port::PA, 6, AF::k7>		AfUSART3_CTS_PA6;
typedef AnyAFR<Port::PA, 15, AF::k7>	AfUSART3_DE_PA15;
typedef AnyAFR<Port::PA, 15, AF::k7>	AfUSART3_RTS_PA15;
typedef AnyAFR<Port::PB, 0, AF::k7>		AfUSART3_CK_PB0;
typedef AnyAFR<Port::PB, 1, AF::k7>		AfUSART3_DE_PB1;
typedef AnyAFR<Port::PB, 1, AF::k7>		AfUSART3_RTS_PB1;

// LPUART1
typedef AnyAFR<Port::PA, 2, AF::k8>		AfLPUART1_TX_PA2;
typedef AnyAFR<Port::PA, 3, AF::k8>		AfLPUART1_RX_PA3;
typedef AnyAFR<Port::PB, 1, AF::k8>		AfLPUART1_DE_PB1;
typedef AnyAFR<Port::PB, 1, AF::k8>		AfLPUART1_RTS_PB1;

// CAN1
typedef AnyAFR<Port::PA, 11, AF::k9>	AfCAN1_RX_PA11;
typedef AnyAFR<Port::PA, 12, AF::k9>	AfCAN1_TX_PA12;

// TSC
typedef AnyAFR<Port::PA, 15, AF::k9>	AfTSC_G3_IO1_PA15;
typedef AnyAFR<Port::PB, 4, AF::k9>		AfTSC_G2_IO1_PB4;
typedef AnyAFR<Port::PB, 5, AF::k9>		AfTSC_G2_IO2_PB5;
typedef AnyAFR<Port::PB, 6, AF::k9>		AfTSC_G2_IO3_PB6;
typedef AnyAFR<Port::PB, 7, AF::k9>		AfTSC_G2_IO4_PB7;

// USB
typedef AnyAFR<Port::PA, 10, AF::k10>	AfUSB_CRS_SYNC_PA10;
typedef AnyAFR<Port::PA, 11, AF::k10>	AfUSB_DM_PA11;
typedef AnyAFR<Port::PA, 12, AF::k10>	AfUSB_DP_PA12;
typedef AnyAFR<Port::PA, 13, AF::k10>	AfUSB_NOE_PA13;

// QUADSPI
typedef AnyAFR<Port::PA, 2, AF::k10>	AfQUADSPI_BK1_NCS_PA2;
typedef AnyAFR<Port::PA, 3, AF::k10>	AfQUADSPI_CLK_PA3;
typedef AnyAFR<Port::PA, 6, AF::k10>	AfQUADSPI_BK1_IO3_PA6;
typedef AnyAFR<Port::PA, 7, AF::k10>	AfQUADSPI_BK1_IO2_PA7;
typedef AnyAFR<Port::PB, 0, AF::k10>	AfQUADSPI_BK1_IO1_PB0;
typedef AnyAFR<Port::PB, 1, AF::k10>	AfQUADSPI_BK1_IO0_PB1;

// COMP1
typedef AnyAFR<Port::PA, 0, AF::k12>	AfCOMP1_OUT_PA0;
typedef AnyAFR<Port::PA, 6, AF::k6>		AfCOMP1_OUT_PA6;
typedef AnyAFR<Port::PA, 11, AF::k6>	AfCOMP1_OUT_PA11;
typedef AnyAFR<Port::PB, 0, AF::k12>	AfCOMP1_OUT_PB0;

// COMP2
typedef AnyAFR<Port::PA, 2, AF::k12>	AfCOMP2_OUT_PA2;
typedef AnyAFR<Port::PA, 7, AF::k12>	AfCOMP2_OUT_PA7;
typedef AnyAFR<Port::PB, 5, AF::k12>	AfCOMP2_OUT_PB5;

// SWPMI1
typedef AnyAFR<Port::PA, 8, AF::k12>	AfSWPMI1_IO_PA8;
typedef AnyAFR<Port::PA, 13, AF::k12>	AfSWPMI1_TX_PA13;
typedef AnyAFR<Port::PA, 14, AF::k12>	AfSWPMI1_RX_PA14;
typedef AnyAFR<Port::PA, 15, AF::k12>	AfSWPMI1_SUSPEND_PA15;

// SAI1
typedef AnyAFR<Port::PA, 0, AF::k13>	AfSAI1_EXTCLK_PA0;
typedef AnyAFR<Port::PA, 3, AF::k13>	AfSAI1_MCLK_A_PA3;
typedef AnyAFR<Port::PA, 4, AF::k13>	AfSAI1_FS_B_PA4;
typedef AnyAFR<Port::PA, 8, AF::k13>	AfSAI1_SCLK_A_PA8;
typedef AnyAFR<Port::PA, 9, AF::k13>	AfSAI1_FS_A_PA9;
typedef AnyAFR<Port::PA, 10, AF::k13>	AfSAI1_SD_A_PA10;
typedef AnyAFR<Port::PA, 14, AF::k13>	AfSAI1_SD_B_PA14;
typedef AnyAFR<Port::PA, 15, AF::k13>	AfSAI1_FS_B_PA15;
typedef AnyAFR<Port::PB, 0, AF::k13>	AfSAI1_EXTCLK_PB0;
typedef AnyAFR<Port::PB, 3, AF::k13>	AfSAI1_SCK_B_PB3;
typedef AnyAFR<Port::PB, 4, AF::k13>	AfSAI1_MCLK_B_PB4;
typedef AnyAFR<Port::PB, 5, AF::k13>	AfSAI1_SD_B_PB5;
typedef AnyAFR<Port::PB, 6, AF::k13>	AfSAI1_FS_B_PB6;

// EVENTOUT
typedef AnyAFR<Port::PA, 0, AF::k15>	AfEVENTOUT_PA0;
typedef AnyAFR<Port::PA, 1, AF::k15>	AfEVENTOUT_PA1;
typedef AnyAFR<Port::PA, 2, AF::k15>	AfEVENTOUT_PA2;
typedef AnyAFR<Port::PA, 3, AF::k15>	AfEVENTOUT_PA3;
typedef AnyAFR<Port::PA, 4, AF::k15>	AfEVENTOUT_PA4;
typedef AnyAFR<Port::PA, 5, AF::k15>	AfEVENTOUT_PA5;
typedef AnyAFR<Port::PA, 6, AF::k15>	AfEVENTOUT_PA6;
typedef AnyAFR<Port::PA, 7, AF::k15>	AfEVENTOUT_PA7;
typedef AnyAFR<Port::PA, 8, AF::k15>	AfEVENTOUT_PA8;
typedef AnyAFR<Port::PA, 9, AF::k15>	AfEVENTOUT_PA9;
typedef AnyAFR<Port::PA, 10, AF::k15>	AfEVENTOUT_PA10;
typedef AnyAFR<Port::PA, 11, AF::k15>	AfEVENTOUT_PA11;
typedef AnyAFR<Port::PA, 12, AF::k15>	AfEVENTOUT_PA12;
typedef AnyAFR<Port::PA, 13, AF::k15>	AfEVENTOUT_PA13;
typedef AnyAFR<Port::PA, 14, AF::k15>	AfEVENTOUT_PA14;
typedef AnyAFR<Port::PA, 15, AF::k15>	AfEVENTOUT_PA15;
typedef AnyAFR<Port::PB, 0, AF::k15>	AfEVENTOUT_PB0;
typedef AnyAFR<Port::PB, 1, AF::k15>	AfEVENTOUT_PB1;
typedef AnyAFR<Port::PB, 3, AF::k15>	AfEVENTOUT_PB3;
typedef AnyAFR<Port::PB, 4, AF::k15>	AfEVENTOUT_PB4;
typedef AnyAFR<Port::PB, 5, AF::k15>	AfEVENTOUT_PB5;
typedef AnyAFR<Port::PB, 6, AF::k15>	AfEVENTOUT_PB6;
typedef AnyAFR<Port::PB, 7, AF::k15>	AfEVENTOUT_PB7;
typedef AnyAFR<Port::PC, 14, AF::k15>	AfEVENTOUT_PC14;
typedef AnyAFR<Port::PC, 15, AF::k15>	AfEVENTOUT_PC15;
typedef AnyAFR<Port::PH, 3, AF::k15>	AfEVENTOUT_PH3;


}	// namespace Gpio
}	// namespace Bmt

