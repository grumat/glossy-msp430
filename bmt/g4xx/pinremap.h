#pragma once

namespace Bmt
{
namespace Gpio
{


// SYS
typedef AnyAFR<Port::PA, 8, AF::k0>		AfMCO_PA8;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 10, AF::k0>	AfMCO_PG10;
#endif	// defined(GPIOG_BASE)
typedef AnyAFR<Port::PA, 14, AF::k0>	AfJTCK_PA14;
typedef AnyAFR<Port::PA, 15, AF::k0>	AfJTDI_PA15;
typedef AnyAFR<Port::PB, 3, AF::k0>		AfJTDO_PB3;
typedef AnyAFR<Port::PA, 13, AF::k0>	AfJTMS_PA13;
typedef AnyAFR<Port::PB, 4, AF::k0>		AfJTRST_PB4;
typedef AnyAFR<Port::PB, 3, AF::k0>		AfTRACESWO_PB3;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 2, AF::k0>		AfTRACECK_PE2;
typedef AnyAFR<Port::PE, 3, AF::k0>		AfTRACED0_PE3;
typedef AnyAFR<Port::PE, 4, AF::k0>		AfTRACED1_PE4;
typedef AnyAFR<Port::PE, 5, AF::k0>		AfTRACED2_PE5;
typedef AnyAFR<Port::PE, 6, AF::k0>		AfTRACED3_PE6;
#endif	// defined(GPIOE_BASE)
typedef AnyAFR<Port::PA, 14, AF::k0>	AfSWCLK_PA14;
typedef AnyAFR<Port::PA, 13, AF::k0>	AfSWDIO_PA13;

// CAN1
#if defined(CAN1_BASE)
typedef AnyAFR<Port::PA, 11, AF::k9>	AfCAN1_RX_PA11;
typedef AnyAFR<Port::PB, 8, AF::k9>		AfCAN1_RX_PB8;
typedef AnyAFR<Port::PA, 12, AF::k9>	AfCAN1_TX_PA12;
typedef AnyAFR<Port::PB, 9, AF::k9>		AfCAN1_TX_PB9;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 0, AF::k9>		AfCAN1_RX_PD0;
typedef AnyAFR<Port::PD, 1, AF::k9>		AfCAN1_TX_PD1;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k9>		AfCAN1_RXFD_PE0;
#endif	// defined(GPIOE_BASE)
#endif	// defined(CAN1_BASE)

// CAN2
#if defined(CAN2_BASE)
typedef AnyAFR<Port::PB, 5, AF::k9>		AfCAN2_RX_PB5;
typedef AnyAFR<Port::PB, 12, AF::k9>	AfCAN2_RX_PB12;
typedef AnyAFR<Port::PB, 6, AF::k9>		AfCAN2_TX_PB6;
typedef AnyAFR<Port::PB, 13, AF::k9>	AfCAN2_TX_PB13;
#endif	// defined(CAN2_BASE)

// CAN3
#if defined(CAN3_BASE)
typedef AnyAFR<Port::PA, 8, AF::k11>	AfCAN3_RX_PA8;
typedef AnyAFR<Port::PB, 3, AF::k11>	AfCAN3_RX_PB3;
typedef AnyAFR<Port::PA, 15, AF::k11>	AfCAN3_TX_PA15;
typedef AnyAFR<Port::PB, 4, AF::k11>	AfCAN3_TX_PB4;
#endif	// defined(CAN3_BASE)

// COMP1
#if defined(COMP1_BASE)
typedef AnyAFR<Port::PA, 0, AF::k8>		AfCOMP1_OUT_PA0;
typedef AnyAFR<Port::PA, 6, AF::k8>		AfCOMP1_OUT_PA6;
typedef AnyAFR<Port::PA, 11, AF::k8>	AfCOMP1_OUT_PA11;
typedef AnyAFR<Port::PB, 8, AF::k8>		AfCOMP1_OUT_PB8;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 4, AF::k2>		AfCOMP1_OUT_PF4;
#endif	// defined(GPIOF_BASE)
#endif	// defined(COMP1_BASE)

// COMP2
#if defined(COMP2_BASE)
typedef AnyAFR<Port::PA, 2, AF::k8>		AfCOMP2_OUT_PA2;
typedef AnyAFR<Port::PA, 7, AF::k8>		AfCOMP2_OUT_PA7;
typedef AnyAFR<Port::PA, 12, AF::k8>	AfCOMP2_OUT_PA12;
typedef AnyAFR<Port::PB, 9, AF::k8>		AfCOMP2_OUT_PB9;
#endif	// defined(COMP2_BASE)

// COMP3
#if defined(COMP3_BASE)
typedef AnyAFR<Port::PB, 7, AF::k8>		AfCOMP3_OUT_PB7;
typedef AnyAFR<Port::PB, 15, AF::k3>	AfCOMP3_OUT_PB15;
typedef AnyAFR<Port::PC, 2, AF::k3>		AfCOMP3_OUT_PC2;
#endif	// defined(COMP3_BASE)

// COMP4
#if defined(COMP4_BASE)
typedef AnyAFR<Port::PB, 1, AF::k8>		AfCOMP4_OUT_PB1;
typedef AnyAFR<Port::PB, 6, AF::k8>		AfCOMP4_OUT_PB6;
typedef AnyAFR<Port::PB, 14, AF::k8>	AfCOMP4_OUT_PB14;
#endif	// defined(COMP4_BASE)

// COMP5
#if defined(COMP5_BASE)
typedef AnyAFR<Port::PA, 9, AF::k8>		AfCOMP5_OUT_PA9;
typedef AnyAFR<Port::PC, 7, AF::k7>		AfCOMP5_OUT_PC7;
#endif	// defined(COMP5_BASE)

// COMP6
#if defined(COMP6_BASE)
typedef AnyAFR<Port::PA, 10, AF::k8>	AfCOMP6_OUT_PA10;
typedef AnyAFR<Port::PC, 6, AF::k7>		AfCOMP6_OUT_PC6;
#endif	// defined(COMP6_BASE)

// COMP7
#if defined(COMP7_BASE)
typedef AnyAFR<Port::PA, 8, AF::k8>		AfCOMP7_OUT_PA8;
typedef AnyAFR<Port::PC, 8, AF::k7>		AfCOMP7_OUT_PC8;
#endif	// defined(COMP7_BASE)

// FMC
#if defined(FMC_BASE)
typedef AnyAFR<Port::PB, 7, AF::k12>	AfFMC_NL_PB7;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 11, AF::k12>	AfFMC_A16_PD11;
typedef AnyAFR<Port::PD, 12, AF::k12>	AfFMC_A17_PD12;
typedef AnyAFR<Port::PD, 13, AF::k12>	AfFMC_A18_PD13;
typedef AnyAFR<Port::PD, 3, AF::k12>	AfFMC_CLK_PD3;
typedef AnyAFR<Port::PD, 14, AF::k12>	AfFMC_D0_PD14;
typedef AnyAFR<Port::PD, 15, AF::k12>	AfFMC_D1_PD15;
typedef AnyAFR<Port::PD, 8, AF::k12>	AfFMC_D13_PD8;
typedef AnyAFR<Port::PD, 9, AF::k12>	AfFMC_D14_PD9;
typedef AnyAFR<Port::PD, 10, AF::k12>	AfFMC_D15_PD10;
typedef AnyAFR<Port::PD, 0, AF::k12>	AfFMC_D2_PD0;
typedef AnyAFR<Port::PD, 1, AF::k12>	AfFMC_D3_PD1;
typedef AnyAFR<Port::PD, 7, AF::k12>	AfFMC_NCE_PD7;
typedef AnyAFR<Port::PD, 7, AF::k12>	AfFMC_NE1_PD7;
typedef AnyAFR<Port::PD, 4, AF::k12>	AfFMC_NOE_PD4;
typedef AnyAFR<Port::PD, 6, AF::k12>	AfFMC_NWAIT_PD6;
typedef AnyAFR<Port::PD, 5, AF::k12>	AfFMC_NWE_PD5;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 3, AF::k12>	AfFMC_A19_PE3;
typedef AnyAFR<Port::PE, 4, AF::k12>	AfFMC_A20_PE4;
typedef AnyAFR<Port::PE, 5, AF::k12>	AfFMC_A21_PE5;
typedef AnyAFR<Port::PE, 6, AF::k12>	AfFMC_A22_PE6;
typedef AnyAFR<Port::PE, 2, AF::k12>	AfFMC_A23_PE2;
typedef AnyAFR<Port::PE, 13, AF::k12>	AfFMC_D10_PE13;
typedef AnyAFR<Port::PE, 14, AF::k12>	AfFMC_D11_PE14;
typedef AnyAFR<Port::PE, 15, AF::k12>	AfFMC_D12_PE15;
typedef AnyAFR<Port::PE, 7, AF::k12>	AfFMC_D4_PE7;
typedef AnyAFR<Port::PE, 8, AF::k12>	AfFMC_D5_PE8;
typedef AnyAFR<Port::PE, 9, AF::k12>	AfFMC_D6_PE9;
typedef AnyAFR<Port::PE, 10, AF::k12>	AfFMC_D7_PE10;
typedef AnyAFR<Port::PE, 11, AF::k12>	AfFMC_D8_PE11;
typedef AnyAFR<Port::PE, 12, AF::k12>	AfFMC_D9_PE12;
typedef AnyAFR<Port::PE, 0, AF::k12>	AfFMC_NBL0_PE0;
typedef AnyAFR<Port::PE, 1, AF::k12>	AfFMC_NBL1_PE1;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 10, AF::k12>	AfFMC_A0_PF10;
typedef AnyAFR<Port::PF, 7, AF::k12>	AfFMC_A1_PF7;
typedef AnyAFR<Port::PF, 2, AF::k12>	AfFMC_A2_PF2;
typedef AnyAFR<Port::PF, 8, AF::k12>	AfFMC_A24_PF8;
typedef AnyAFR<Port::PF, 9, AF::k12>	AfFMC_A25_PF9;
typedef AnyAFR<Port::PF, 3, AF::k12>	AfFMC_A3_PF3;
typedef AnyAFR<Port::PF, 4, AF::k12>	AfFMC_A4_PF4;
typedef AnyAFR<Port::PF, 5, AF::k12>	AfFMC_A5_PF5;
typedef AnyAFR<Port::PF, 12, AF::k12>	AfFMC_A6_PF12;
typedef AnyAFR<Port::PF, 13, AF::k12>	AfFMC_A7_PF13;
typedef AnyAFR<Port::PF, 14, AF::k12>	AfFMC_A8_PF14;
typedef AnyAFR<Port::PF, 15, AF::k12>	AfFMC_A9_PF15;
typedef AnyAFR<Port::PF, 11, AF::k12>	AfFMC_NE4_PF11;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 0, AF::k12>	AfFMC_A10_PG0;
typedef AnyAFR<Port::PG, 1, AF::k12>	AfFMC_A11_PG1;
typedef AnyAFR<Port::PG, 2, AF::k12>	AfFMC_A12_PG2;
typedef AnyAFR<Port::PG, 3, AF::k12>	AfFMC_A13_PG3;
typedef AnyAFR<Port::PG, 4, AF::k12>	AfFMC_A14_PG4;
typedef AnyAFR<Port::PG, 5, AF::k12>	AfFMC_A15_PG5;
typedef AnyAFR<Port::PG, 6, AF::k12>	AfFMC_INT_PG6;
typedef AnyAFR<Port::PG, 7, AF::k12>	AfFMC_INT_PG7;
typedef AnyAFR<Port::PG, 9, AF::k12>	AfFMC_NCE_PG9;
typedef AnyAFR<Port::PG, 9, AF::k12>	AfFMC_NE2_PG9;
typedef AnyAFR<Port::PG, 8, AF::k12>	AfFMC_NE3_PG8;
#endif	// defined(GPIOG_BASE)
#endif	// defined(FMC_BASE)

// HRTIM1
#if defined(HRTIM1_BASE)
typedef AnyAFR<Port::PA, 8, AF::k13>	AfHRTIM1_CHA1_PA8;
typedef AnyAFR<Port::PA, 9, AF::k13>	AfHRTIM1_CHA2_PA9;
typedef AnyAFR<Port::PA, 10, AF::k13>	AfHRTIM1_CHB1_PA10;
typedef AnyAFR<Port::PA, 11, AF::k13>	AfHRTIM1_CHB2_PA11;
typedef AnyAFR<Port::PB, 12, AF::k13>	AfHRTIM1_CHC1_PB12;
typedef AnyAFR<Port::PB, 13, AF::k13>	AfHRTIM1_CHC2_PB13;
typedef AnyAFR<Port::PB, 14, AF::k13>	AfHRTIM1_CHD1_PB14;
typedef AnyAFR<Port::PB, 15, AF::k13>	AfHRTIM1_CHD2_PB15;
typedef AnyAFR<Port::PC, 8, AF::k3>		AfHRTIM1_CHE1_PC8;
typedef AnyAFR<Port::PC, 9, AF::k3>		AfHRTIM1_CHE2_PC9;
typedef AnyAFR<Port::PC, 6, AF::k13>	AfHRTIM1_CHF1_PC6;
typedef AnyAFR<Port::PC, 7, AF::k13>	AfHRTIM1_CHF2_PC7;
typedef AnyAFR<Port::PC, 12, AF::k3>	AfHRTIM1_EEV1_PC12;
typedef AnyAFR<Port::PC, 5, AF::k13>	AfHRTIM1_EEV10_PC5;
typedef AnyAFR<Port::PC, 6, AF::k3>		AfHRTIM1_EEV10_PC6;
typedef AnyAFR<Port::PC, 11, AF::k3>	AfHRTIM1_EEV2_PC11;
typedef AnyAFR<Port::PB, 7, AF::k13>	AfHRTIM1_EEV3_PB7;
typedef AnyAFR<Port::PB, 6, AF::k13>	AfHRTIM1_EEV4_PB6;
typedef AnyAFR<Port::PB, 9, AF::k13>	AfHRTIM1_EEV5_PB9;
typedef AnyAFR<Port::PB, 5, AF::k13>	AfHRTIM1_EEV6_PB5;
typedef AnyAFR<Port::PB, 4, AF::k13>	AfHRTIM1_EEV7_PB4;
typedef AnyAFR<Port::PB, 8, AF::k13>	AfHRTIM1_EEV8_PB8;
typedef AnyAFR<Port::PB, 3, AF::k13>	AfHRTIM1_EEV9_PB3;
typedef AnyAFR<Port::PA, 12, AF::k13>	AfHRTIM1_FLT1_PA12;
typedef AnyAFR<Port::PA, 15, AF::k13>	AfHRTIM1_FLT2_PA15;
typedef AnyAFR<Port::PB, 10, AF::k13>	AfHRTIM1_FLT3_PB10;
typedef AnyAFR<Port::PB, 11, AF::k13>	AfHRTIM1_FLT4_PB11;
typedef AnyAFR<Port::PB, 0, AF::k13>	AfHRTIM1_FLT5_PB0;
typedef AnyAFR<Port::PC, 7, AF::k3>		AfHRTIM1_FLT5_PC7;
typedef AnyAFR<Port::PC, 10, AF::k13>	AfHRTIM1_FLT6_PC10;
typedef AnyAFR<Port::PB, 2, AF::k13>	AfHRTIM1_SCIN_PB2;
typedef AnyAFR<Port::PB, 6, AF::k12>	AfHRTIM1_SCIN_PB6;
typedef AnyAFR<Port::PB, 1, AF::k13>	AfHRTIM1_SCOUT_PB1;
typedef AnyAFR<Port::PB, 3, AF::k12>	AfHRTIM1_SCOUT_PB3;
#endif	// defined(HRTIM1_BASE)

// IR
#if defined(TIM16_BASE)
typedef AnyAFR<Port::PA, 13, AF::k5>	AfIR_OUT_PA13;
typedef AnyAFR<Port::PB, 9, AF::k6>		AfIR_OUT_PB9;
#endif	// defined(TIM16_BASE)

// I2C1
#if defined(I2C1_BASE)
typedef AnyAFR<Port::PA, 13, AF::k4>	AfI2C1_SCL_PA13;
typedef AnyAFR<Port::PA, 15, AF::k4>	AfI2C1_SCL_PA15;
typedef AnyAFR<Port::PB, 8, AF::k4>		AfI2C1_SCL_PB8;
typedef AnyAFR<Port::PA, 14, AF::k4>	AfI2C1_SDA_PA14;
typedef AnyAFR<Port::PB, 7, AF::k4>		AfI2C1_SDA_PB7;
typedef AnyAFR<Port::PB, 9, AF::k4>		AfI2C1_SDA_PB9;
typedef AnyAFR<Port::PB, 5, AF::k4>		AfI2C1_SMBA_PB5;
#endif	// defined(I2C1_BASE)

// I2C2
#if defined(I2C2_BASE)
typedef AnyAFR<Port::PA, 9, AF::k4>		AfI2C2_SCL_PA9;
typedef AnyAFR<Port::PC, 4, AF::k4>		AfI2C2_SCL_PC4;
typedef AnyAFR<Port::PA, 8, AF::k4>		AfI2C2_SDA_PA8;
typedef AnyAFR<Port::PA, 10, AF::k4>	AfI2C2_SMBA_PA10;
typedef AnyAFR<Port::PB, 12, AF::k4>	AfI2C2_SMBA_PB12;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 6, AF::k4>		AfI2C2_SCL_PF6;
typedef AnyAFR<Port::PF, 0, AF::k4>		AfI2C2_SDA_PF0;
typedef AnyAFR<Port::PF, 2, AF::k4>		AfI2C2_SMBA_PF2;
#endif	// defined(GPIOF_BASE)
#endif	// defined(I2C2_BASE)

// I2C3
#if defined(I2C3_BASE)
typedef AnyAFR<Port::PA, 8, AF::k2>		AfI2C3_SCL_PA8;
typedef AnyAFR<Port::PC, 8, AF::k8>		AfI2C3_SCL_PC8;
typedef AnyAFR<Port::PB, 5, AF::k8>		AfI2C3_SDA_PB5;
typedef AnyAFR<Port::PC, 9, AF::k8>		AfI2C3_SDA_PC9;
typedef AnyAFR<Port::PC, 11, AF::k8>	AfI2C3_SDA_PC11;
typedef AnyAFR<Port::PA, 9, AF::k2>		AfI2C3_SMBA_PA9;
typedef AnyAFR<Port::PB, 2, AF::k4>		AfI2C3_SMBA_PB2;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 3, AF::k4>		AfI2C3_SCL_PF3;
typedef AnyAFR<Port::PF, 4, AF::k4>		AfI2C3_SDA_PF4;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 7, AF::k4>		AfI2C3_SCL_PG7;
typedef AnyAFR<Port::PG, 8, AF::k4>		AfI2C3_SDA_PG8;
typedef AnyAFR<Port::PG, 6, AF::k4>		AfI2C3_SMBA_PG6;
#endif	// defined(GPIOG_BASE)
#endif	// defined(I2C3_BASE)

// I2C4
#if defined(I2C4_BASE)
typedef AnyAFR<Port::PA, 13, AF::k3>	AfI2C4_SCL_PA13;
typedef AnyAFR<Port::PC, 6, AF::k8>		AfI2C4_SCL_PC6;
typedef AnyAFR<Port::PB, 7, AF::k3>		AfI2C4_SDA_PB7;
typedef AnyAFR<Port::PC, 7, AF::k8>		AfI2C4_SDA_PC7;
typedef AnyAFR<Port::PA, 14, AF::k3>	AfI2C4_SMBA_PA14;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 11, AF::k4>	AfI2C4_SMBA_PD11;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 14, AF::k4>	AfI2C4_SCL_PF14;
typedef AnyAFR<Port::PF, 15, AF::k4>	AfI2C4_SDA_PF15;
typedef AnyAFR<Port::PF, 13, AF::k4>	AfI2C4_SMBA_PF13;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 3, AF::k4>		AfI2C4_SCL_PG3;
typedef AnyAFR<Port::PG, 4, AF::k4>		AfI2C4_SDA_PG4;
#endif	// defined(GPIOG_BASE)
#endif	// defined(I2C4_BASE)

// I2S2
#if defined(I2S2_BASE)
typedef AnyAFR<Port::PB, 13, AF::k5>	AfI2S2_CK_PB13;
typedef AnyAFR<Port::PA, 8, AF::k5>		AfI2S2_MCK_PA8;
typedef AnyAFR<Port::PC, 6, AF::k6>		AfI2S2_MCK_PC6;
typedef AnyAFR<Port::PA, 11, AF::k5>	AfI2S2_SD_PA11;
typedef AnyAFR<Port::PB, 15, AF::k5>	AfI2S2_SD_PB15;
typedef AnyAFR<Port::PB, 12, AF::k5>	AfI2S2_WS_PB12;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 1, AF::k5>		AfI2S2_CK_PF1;
typedef AnyAFR<Port::PF, 0, AF::k5>		AfI2S2_WS_PF0;
#endif	// defined(GPIOF_BASE)
#endif	// defined(I2S2_BASE)

// I2S3
#if defined(I2S3_BASE)
typedef AnyAFR<Port::PB, 3, AF::k6>		AfI2S3_CK_PB3;
typedef AnyAFR<Port::PC, 10, AF::k6>	AfI2S3_CK_PC10;
typedef AnyAFR<Port::PA, 9, AF::k5>		AfI2S3_MCK_PA9;
typedef AnyAFR<Port::PC, 7, AF::k6>		AfI2S3_MCK_PC7;
typedef AnyAFR<Port::PB, 5, AF::k6>		AfI2S3_SD_PB5;
typedef AnyAFR<Port::PC, 12, AF::k6>	AfI2S3_SD_PC12;
typedef AnyAFR<Port::PA, 4, AF::k6>		AfI2S3_WS_PA4;
typedef AnyAFR<Port::PA, 15, AF::k6>	AfI2S3_WS_PA15;
#endif	// defined(I2S3_BASE)

// I2SCKIN
#if defined(SPI_I2S_SUPPORT)
typedef AnyAFR<Port::PA, 12, AF::k5>	AfI2SCKIN_PA12;
typedef AnyAFR<Port::PC, 9, AF::k5>		AfI2SCKIN_PC9;
#endif	// defined(SPI_I2S_SUPPORT)

// LPTIM1
#if defined(LPTIM1_BASE)
typedef AnyAFR<Port::PB, 6, AF::k11>	AfLPTIM1_ETR_PB6;
typedef AnyAFR<Port::PC, 3, AF::k1>		AfLPTIM1_ETR_PC3;
typedef AnyAFR<Port::PB, 5, AF::k11>	AfLPTIM1_IN1_PB5;
typedef AnyAFR<Port::PC, 0, AF::k1>		AfLPTIM1_IN1_PC0;
typedef AnyAFR<Port::PB, 7, AF::k11>	AfLPTIM1_IN2_PB7;
typedef AnyAFR<Port::PC, 2, AF::k1>		AfLPTIM1_IN2_PC2;
typedef AnyAFR<Port::PA, 14, AF::k1>	AfLPTIM1_OUT_PA14;
typedef AnyAFR<Port::PB, 2, AF::k1>		AfLPTIM1_OUT_PB2;
typedef AnyAFR<Port::PC, 1, AF::k1>		AfLPTIM1_OUT_PC1;
#endif	// defined(LPTIM1_BASE)

// LPUART1
#if defined(LPUART1_BASE)
typedef AnyAFR<Port::PA, 6, AF::k12>	AfLPUART1_CTS_PA6;
typedef AnyAFR<Port::PB, 13, AF::k8>	AfLPUART1_CTS_PB13;
typedef AnyAFR<Port::PB, 1, AF::k12>	AfLPUART1_RTS_DE_PB1;
typedef AnyAFR<Port::PB, 12, AF::k8>	AfLPUART1_RTS_DE_PB12;
typedef AnyAFR<Port::PA, 3, AF::k12>	AfLPUART1_RX_PA3;
typedef AnyAFR<Port::PB, 10, AF::k8>	AfLPUART1_RX_PB10;
typedef AnyAFR<Port::PC, 0, AF::k8>		AfLPUART1_RX_PC0;
typedef AnyAFR<Port::PA, 2, AF::k12>	AfLPUART1_TX_PA2;
typedef AnyAFR<Port::PB, 11, AF::k8>	AfLPUART1_TX_PB11;
typedef AnyAFR<Port::PC, 1, AF::k8>		AfLPUART1_TX_PC1;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 5, AF::k8>		AfLPUART1_CTS_PG5;
typedef AnyAFR<Port::PG, 6, AF::k8>		AfLPUART1_RTS_DE_PG6;
typedef AnyAFR<Port::PG, 8, AF::k8>		AfLPUART1_RX_PG8;
typedef AnyAFR<Port::PG, 7, AF::k8>		AfLPUART1_TX_PG7;
#endif	// defined(GPIOG_BASE)
#endif	// defined(LPUART1_BASE)

// QUADSPI1
#if defined(QUADSPI)
typedef AnyAFR<Port::PB, 1, AF::k10>	AfQUADSPI1_BK1_IO0_PB1;
typedef AnyAFR<Port::PB, 0, AF::k10>	AfQUADSPI1_BK1_IO1_PB0;
typedef AnyAFR<Port::PA, 7, AF::k10>	AfQUADSPI1_BK1_IO2_PA7;
typedef AnyAFR<Port::PA, 6, AF::k10>	AfQUADSPI1_BK1_IO3_PA6;
typedef AnyAFR<Port::PA, 2, AF::k10>	AfQUADSPI1_BK1_NCS_PA2;
typedef AnyAFR<Port::PB, 11, AF::k10>	AfQUADSPI1_BK1_NCS_PB11;
typedef AnyAFR<Port::PC, 1, AF::k10>	AfQUADSPI1_BK2_IO0_PC1;
typedef AnyAFR<Port::PB, 2, AF::k10>	AfQUADSPI1_BK2_IO1_PB2;
typedef AnyAFR<Port::PC, 2, AF::k10>	AfQUADSPI1_BK2_IO1_PC2;
typedef AnyAFR<Port::PC, 3, AF::k10>	AfQUADSPI1_BK2_IO2_PC3;
typedef AnyAFR<Port::PC, 4, AF::k10>	AfQUADSPI1_BK2_IO3_PC4;
typedef AnyAFR<Port::PA, 3, AF::k10>	AfQUADSPI1_CLK_PA3;
typedef AnyAFR<Port::PB, 10, AF::k10>	AfQUADSPI1_CLK_PB10;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 4, AF::k10>	AfQUADSPI1_BK2_IO0_PD4;
typedef AnyAFR<Port::PD, 5, AF::k10>	AfQUADSPI1_BK2_IO1_PD5;
typedef AnyAFR<Port::PD, 6, AF::k10>	AfQUADSPI1_BK2_IO2_PD6;
typedef AnyAFR<Port::PD, 7, AF::k10>	AfQUADSPI1_BK2_IO3_PD7;
typedef AnyAFR<Port::PD, 3, AF::k10>	AfQUADSPI1_BK2_NCS_PD3;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 12, AF::k10>	AfQUADSPI1_BK1_IO0_PE12;
typedef AnyAFR<Port::PE, 13, AF::k10>	AfQUADSPI1_BK1_IO1_PE13;
typedef AnyAFR<Port::PE, 14, AF::k10>	AfQUADSPI1_BK1_IO2_PE14;
typedef AnyAFR<Port::PE, 15, AF::k10>	AfQUADSPI1_BK1_IO3_PE15;
typedef AnyAFR<Port::PE, 11, AF::k10>	AfQUADSPI1_BK1_NCS_PE11;
typedef AnyAFR<Port::PE, 10, AF::k10>	AfQUADSPI1_CLK_PE10;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 8, AF::k10>	AfQUADSPI1_BK1_IO0_PF8;
typedef AnyAFR<Port::PF, 9, AF::k10>	AfQUADSPI1_BK1_IO1_PF9;
typedef AnyAFR<Port::PF, 7, AF::k10>	AfQUADSPI1_BK1_IO2_PF7;
typedef AnyAFR<Port::PF, 6, AF::k10>	AfQUADSPI1_BK1_IO3_PF6;
typedef AnyAFR<Port::PF, 10, AF::k10>	AfQUADSPI1_CLK_PF10;
#endif	// defined(GPIOF_BASE)
#endif	// defined(QUADSPI)

// RTC
#if defined(RTC_BASE)
typedef AnyAFR<Port::PB, 2, AF::k0>		AfRTC_OUT2_PB2;
typedef AnyAFR<Port::PA, 1, AF::k0>		AfRTC_REFIN_PA1;
typedef AnyAFR<Port::PB, 15, AF::k0>	AfRTC_REFIN_PB15;
#endif	// defined(RTC_BASE)

// SAI1
#if defined(SAI1_BASE)
typedef AnyAFR<Port::PA, 3, AF::k3>		AfSAI1_CK1_PA3;
typedef AnyAFR<Port::PB, 8, AF::k3>		AfSAI1_CK1_PB8;
typedef AnyAFR<Port::PA, 8, AF::k12>	AfSAI1_CK2_PA8;
typedef AnyAFR<Port::PA, 10, AF::k12>	AfSAI1_D1_PA10;
typedef AnyAFR<Port::PC, 3, AF::k3>		AfSAI1_D1_PC3;
typedef AnyAFR<Port::PB, 9, AF::k3>		AfSAI1_D2_PB9;
typedef AnyAFR<Port::PC, 5, AF::k3>		AfSAI1_D3_PC5;
typedef AnyAFR<Port::PA, 9, AF::k14>	AfSAI1_FS_A_PA9;
typedef AnyAFR<Port::PB, 9, AF::k14>	AfSAI1_FS_A_PB9;
typedef AnyAFR<Port::PA, 4, AF::k13>	AfSAI1_FS_B_PA4;
typedef AnyAFR<Port::PA, 14, AF::k13>	AfSAI1_FS_B_PA14;
typedef AnyAFR<Port::PB, 6, AF::k14>	AfSAI1_FS_B_PB6;
typedef AnyAFR<Port::PA, 3, AF::k13>	AfSAI1_MCLK_A_PA3;
typedef AnyAFR<Port::PB, 8, AF::k14>	AfSAI1_MCLK_A_PB8;
typedef AnyAFR<Port::PB, 4, AF::k14>	AfSAI1_MCLK_B_PB4;
typedef AnyAFR<Port::PA, 8, AF::k14>	AfSAI1_SCK_A_PA8;
typedef AnyAFR<Port::PB, 10, AF::k14>	AfSAI1_SCK_A_PB10;
typedef AnyAFR<Port::PB, 3, AF::k14>	AfSAI1_SCK_B_PB3;
typedef AnyAFR<Port::PA, 10, AF::k14>	AfSAI1_SD_A_PA10;
typedef AnyAFR<Port::PC, 1, AF::k13>	AfSAI1_SD_A_PC1;
typedef AnyAFR<Port::PC, 3, AF::k13>	AfSAI1_SD_A_PC3;
typedef AnyAFR<Port::PA, 13, AF::k13>	AfSAI1_SD_B_PA13;
typedef AnyAFR<Port::PB, 5, AF::k12>	AfSAI1_SD_B_PB5;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 6, AF::k3>		AfSAI1_D1_PD6;
typedef AnyAFR<Port::PD, 6, AF::k13>	AfSAI1_SD_A_PD6;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 2, AF::k3>		AfSAI1_CK1_PE2;
typedef AnyAFR<Port::PE, 5, AF::k3>		AfSAI1_CK2_PE5;
typedef AnyAFR<Port::PE, 6, AF::k3>		AfSAI1_D1_PE6;
typedef AnyAFR<Port::PE, 4, AF::k3>		AfSAI1_D2_PE4;
typedef AnyAFR<Port::PE, 4, AF::k13>	AfSAI1_FS_A_PE4;
typedef AnyAFR<Port::PE, 9, AF::k13>	AfSAI1_FS_B_PE9;
typedef AnyAFR<Port::PE, 2, AF::k13>	AfSAI1_MCLK_A_PE2;
typedef AnyAFR<Port::PE, 10, AF::k13>	AfSAI1_MCLK_B_PE10;
typedef AnyAFR<Port::PE, 5, AF::k13>	AfSAI1_SCK_A_PE5;
typedef AnyAFR<Port::PE, 8, AF::k13>	AfSAI1_SCK_B_PE8;
typedef AnyAFR<Port::PE, 6, AF::k13>	AfSAI1_SD_A_PE6;
typedef AnyAFR<Port::PE, 3, AF::k13>	AfSAI1_SD_B_PE3;
typedef AnyAFR<Port::PE, 7, AF::k13>	AfSAI1_SD_B_PE7;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 10, AF::k13>	AfSAI1_D3_PF10;
typedef AnyAFR<Port::PF, 9, AF::k13>	AfSAI1_FS_B_PF9;
typedef AnyAFR<Port::PF, 7, AF::k13>	AfSAI1_MCLK_B_PF7;
typedef AnyAFR<Port::PF, 8, AF::k13>	AfSAI1_SCK_B_PF8;
typedef AnyAFR<Port::PF, 6, AF::k3>		AfSAI1_SD_B_PF6;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 7, AF::k3>		AfSAI1_CK1_PG7;
typedef AnyAFR<Port::PG, 7, AF::k13>	AfSAI1_MCLK_A_PG7;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SAI1_BASE)

// SPI1
#if defined(SPI1_BASE)
typedef AnyAFR<Port::PA, 6, AF::k5>		AfSPI1_MISO_PA6;
typedef AnyAFR<Port::PB, 4, AF::k5>		AfSPI1_MISO_PB4;
typedef AnyAFR<Port::PA, 7, AF::k5>		AfSPI1_MOSI_PA7;
typedef AnyAFR<Port::PB, 5, AF::k5>		AfSPI1_MOSI_PB5;
typedef AnyAFR<Port::PA, 4, AF::k5>		AfSPI1_NSS_PA4;
typedef AnyAFR<Port::PA, 15, AF::k5>	AfSPI1_NSS_PA15;
typedef AnyAFR<Port::PA, 5, AF::k5>		AfSPI1_SCK_PA5;
typedef AnyAFR<Port::PB, 3, AF::k5>		AfSPI1_SCK_PB3;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 3, AF::k5>		AfSPI1_MISO_PG3;
typedef AnyAFR<Port::PG, 4, AF::k5>		AfSPI1_MOSI_PG4;
typedef AnyAFR<Port::PG, 5, AF::k5>		AfSPI1_NSS_PG5;
typedef AnyAFR<Port::PG, 2, AF::k5>		AfSPI1_SCK_PG2;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SPI1_BASE)

// SPI2
#if defined(SPI2_BASE)
typedef AnyAFR<Port::PA, 10, AF::k5>	AfSPI2_MISO_PA10;
typedef AnyAFR<Port::PB, 14, AF::k5>	AfSPI2_MISO_PB14;
typedef AnyAFR<Port::PA, 11, AF::k5>	AfSPI2_MOSI_PA11;
typedef AnyAFR<Port::PB, 15, AF::k5>	AfSPI2_MOSI_PB15;
typedef AnyAFR<Port::PB, 12, AF::k5>	AfSPI2_NSS_PB12;
typedef AnyAFR<Port::PB, 13, AF::k5>	AfSPI2_SCK_PB13;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 15, AF::k6>	AfSPI2_NSS_PD15;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 0, AF::k5>		AfSPI2_NSS_PF0;
typedef AnyAFR<Port::PF, 1, AF::k5>		AfSPI2_SCK_PF1;
typedef AnyAFR<Port::PF, 9, AF::k5>		AfSPI2_SCK_PF9;
typedef AnyAFR<Port::PF, 10, AF::k5>	AfSPI2_SCK_PF10;
#endif	// defined(GPIOF_BASE)
#endif	// defined(SPI2_BASE)

// SPI3
#if defined(SPI3_BASE)
typedef AnyAFR<Port::PB, 4, AF::k6>		AfSPI3_MISO_PB4;
typedef AnyAFR<Port::PC, 11, AF::k6>	AfSPI3_MISO_PC11;
typedef AnyAFR<Port::PB, 5, AF::k6>		AfSPI3_MOSI_PB5;
typedef AnyAFR<Port::PC, 12, AF::k6>	AfSPI3_MOSI_PC12;
typedef AnyAFR<Port::PA, 4, AF::k6>		AfSPI3_NSS_PA4;
typedef AnyAFR<Port::PA, 15, AF::k6>	AfSPI3_NSS_PA15;
typedef AnyAFR<Port::PB, 3, AF::k6>		AfSPI3_SCK_PB3;
typedef AnyAFR<Port::PC, 10, AF::k6>	AfSPI3_SCK_PC10;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 9, AF::k6>		AfSPI3_SCK_PG9;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SPI3_BASE)

// SPI4
#if defined(SPI4_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 5, AF::k5>		AfSPI4_MISO_PE5;
typedef AnyAFR<Port::PE, 13, AF::k5>	AfSPI4_MISO_PE13;
typedef AnyAFR<Port::PE, 6, AF::k5>		AfSPI4_MOSI_PE6;
typedef AnyAFR<Port::PE, 14, AF::k5>	AfSPI4_MOSI_PE14;
typedef AnyAFR<Port::PE, 3, AF::k5>		AfSPI4_NSS_PE3;
typedef AnyAFR<Port::PE, 4, AF::k5>		AfSPI4_NSS_PE4;
typedef AnyAFR<Port::PE, 11, AF::k5>	AfSPI4_NSS_PE11;
typedef AnyAFR<Port::PE, 2, AF::k5>		AfSPI4_SCK_PE2;
typedef AnyAFR<Port::PE, 12, AF::k5>	AfSPI4_SCK_PE12;
#endif	// defined(GPIOE_BASE)
#endif	// defined(SPI4_BASE)

// TIM1
#if defined(TIM1_BASE)
typedef AnyAFR<Port::PA, 6, AF::k6>		AfTIM1_BKIN_PA6;
typedef AnyAFR<Port::PA, 14, AF::k6>	AfTIM1_BKIN_PA14;
typedef AnyAFR<Port::PA, 15, AF::k9>	AfTIM1_BKIN_PA15;
typedef AnyAFR<Port::PB, 8, AF::k12>	AfTIM1_BKIN_PB8;
typedef AnyAFR<Port::PB, 10, AF::k12>	AfTIM1_BKIN_PB10;
typedef AnyAFR<Port::PB, 12, AF::k6>	AfTIM1_BKIN_PB12;
typedef AnyAFR<Port::PC, 13, AF::k2>	AfTIM1_BKIN_PC13;
typedef AnyAFR<Port::PA, 11, AF::k12>	AfTIM1_BKIN2_PA11;
typedef AnyAFR<Port::PC, 3, AF::k6>		AfTIM1_BKIN2_PC3;
typedef AnyAFR<Port::PA, 8, AF::k6>		AfTIM1_CH1_PA8;
typedef AnyAFR<Port::PC, 0, AF::k2>		AfTIM1_CH1_PC0;
typedef AnyAFR<Port::PA, 7, AF::k6>		AfTIM1_CH1N_PA7;
typedef AnyAFR<Port::PA, 11, AF::k6>	AfTIM1_CH1N_PA11;
typedef AnyAFR<Port::PB, 13, AF::k6>	AfTIM1_CH1N_PB13;
typedef AnyAFR<Port::PC, 13, AF::k4>	AfTIM1_CH1N_PC13;
typedef AnyAFR<Port::PA, 9, AF::k6>		AfTIM1_CH2_PA9;
typedef AnyAFR<Port::PC, 1, AF::k2>		AfTIM1_CH2_PC1;
typedef AnyAFR<Port::PA, 12, AF::k6>	AfTIM1_CH2N_PA12;
typedef AnyAFR<Port::PB, 0, AF::k6>		AfTIM1_CH2N_PB0;
typedef AnyAFR<Port::PB, 14, AF::k6>	AfTIM1_CH2N_PB14;
typedef AnyAFR<Port::PA, 10, AF::k6>	AfTIM1_CH3_PA10;
typedef AnyAFR<Port::PC, 2, AF::k2>		AfTIM1_CH3_PC2;
typedef AnyAFR<Port::PB, 1, AF::k6>		AfTIM1_CH3N_PB1;
typedef AnyAFR<Port::PB, 9, AF::k12>	AfTIM1_CH3N_PB9;
typedef AnyAFR<Port::PB, 15, AF::k4>	AfTIM1_CH3N_PB15;
typedef AnyAFR<Port::PA, 11, AF::k11>	AfTIM1_CH4_PA11;
typedef AnyAFR<Port::PC, 3, AF::k2>		AfTIM1_CH4_PC3;
typedef AnyAFR<Port::PC, 5, AF::k6>		AfTIM1_CH4N_PC5;
typedef AnyAFR<Port::PA, 12, AF::k11>	AfTIM1_ETR_PA12;
typedef AnyAFR<Port::PC, 4, AF::k2>		AfTIM1_ETR_PC4;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 15, AF::k2>	AfTIM1_BKIN_PE15;
typedef AnyAFR<Port::PE, 14, AF::k6>	AfTIM1_BKIN2_PE14;
typedef AnyAFR<Port::PE, 9, AF::k2>		AfTIM1_CH1_PE9;
typedef AnyAFR<Port::PE, 8, AF::k2>		AfTIM1_CH1N_PE8;
typedef AnyAFR<Port::PE, 11, AF::k2>	AfTIM1_CH2_PE11;
typedef AnyAFR<Port::PE, 10, AF::k2>	AfTIM1_CH2N_PE10;
typedef AnyAFR<Port::PE, 13, AF::k2>	AfTIM1_CH3_PE13;
typedef AnyAFR<Port::PE, 12, AF::k2>	AfTIM1_CH3N_PE12;
typedef AnyAFR<Port::PE, 14, AF::k2>	AfTIM1_CH4_PE14;
typedef AnyAFR<Port::PE, 15, AF::k6>	AfTIM1_CH4N_PE15;
typedef AnyAFR<Port::PE, 7, AF::k2>		AfTIM1_ETR_PE7;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 0, AF::k6>		AfTIM1_CH3N_PF0;
#endif	// defined(GPIOF_BASE)
#endif	// defined(TIM1_BASE)

// TIM2
#if defined(TIM2_BASE)
typedef AnyAFR<Port::PA, 0, AF::k1>		AfTIM2_CH1_PA0;
typedef AnyAFR<Port::PA, 5, AF::k1>		AfTIM2_CH1_PA5;
typedef AnyAFR<Port::PA, 15, AF::k1>	AfTIM2_CH1_PA15;
typedef AnyAFR<Port::PA, 1, AF::k1>		AfTIM2_CH2_PA1;
typedef AnyAFR<Port::PB, 3, AF::k1>		AfTIM2_CH2_PB3;
typedef AnyAFR<Port::PA, 2, AF::k1>		AfTIM2_CH3_PA2;
typedef AnyAFR<Port::PA, 9, AF::k10>	AfTIM2_CH3_PA9;
typedef AnyAFR<Port::PB, 10, AF::k1>	AfTIM2_CH3_PB10;
typedef AnyAFR<Port::PA, 3, AF::k1>		AfTIM2_CH4_PA3;
typedef AnyAFR<Port::PA, 10, AF::k10>	AfTIM2_CH4_PA10;
typedef AnyAFR<Port::PB, 11, AF::k1>	AfTIM2_CH4_PB11;
typedef AnyAFR<Port::PA, 0, AF::k14>	AfTIM2_ETR_PA0;
typedef AnyAFR<Port::PA, 5, AF::k2>		AfTIM2_ETR_PA5;
typedef AnyAFR<Port::PA, 15, AF::k14>	AfTIM2_ETR_PA15;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 3, AF::k2>		AfTIM2_CH1_PD3;
typedef AnyAFR<Port::PD, 4, AF::k2>		AfTIM2_CH2_PD4;
typedef AnyAFR<Port::PD, 7, AF::k2>		AfTIM2_CH3_PD7;
typedef AnyAFR<Port::PD, 6, AF::k2>		AfTIM2_CH4_PD6;
typedef AnyAFR<Port::PD, 3, AF::k2>		AfTIM2_ETR_PD3;
#endif	// defined(GPIOD_BASE)
#endif	// defined(TIM2_BASE)

// TIM3
#if defined(TIM3_BASE)
typedef AnyAFR<Port::PA, 6, AF::k2>		AfTIM3_CH1_PA6;
typedef AnyAFR<Port::PB, 4, AF::k2>		AfTIM3_CH1_PB4;
typedef AnyAFR<Port::PC, 6, AF::k2>		AfTIM3_CH1_PC6;
typedef AnyAFR<Port::PA, 4, AF::k2>		AfTIM3_CH2_PA4;
typedef AnyAFR<Port::PA, 7, AF::k2>		AfTIM3_CH2_PA7;
typedef AnyAFR<Port::PB, 5, AF::k2>		AfTIM3_CH2_PB5;
typedef AnyAFR<Port::PC, 7, AF::k2>		AfTIM3_CH2_PC7;
typedef AnyAFR<Port::PB, 0, AF::k2>		AfTIM3_CH3_PB0;
typedef AnyAFR<Port::PC, 8, AF::k2>		AfTIM3_CH3_PC8;
typedef AnyAFR<Port::PB, 1, AF::k2>		AfTIM3_CH4_PB1;
typedef AnyAFR<Port::PB, 7, AF::k10>	AfTIM3_CH4_PB7;
typedef AnyAFR<Port::PC, 9, AF::k2>		AfTIM3_CH4_PC9;
typedef AnyAFR<Port::PB, 3, AF::k10>	AfTIM3_ETR_PB3;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k2>		AfTIM3_ETR_PD2;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 2, AF::k2>		AfTIM3_CH1_PE2;
typedef AnyAFR<Port::PE, 3, AF::k2>		AfTIM3_CH2_PE3;
typedef AnyAFR<Port::PE, 4, AF::k2>		AfTIM3_CH3_PE4;
typedef AnyAFR<Port::PE, 5, AF::k2>		AfTIM3_CH4_PE5;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM3_BASE)

// TIM4
#if defined(TIM4_BASE)
typedef AnyAFR<Port::PA, 11, AF::k10>	AfTIM4_CH1_PA11;
typedef AnyAFR<Port::PB, 6, AF::k2>		AfTIM4_CH1_PB6;
typedef AnyAFR<Port::PA, 12, AF::k10>	AfTIM4_CH2_PA12;
typedef AnyAFR<Port::PB, 7, AF::k2>		AfTIM4_CH2_PB7;
typedef AnyAFR<Port::PA, 13, AF::k10>	AfTIM4_CH3_PA13;
typedef AnyAFR<Port::PB, 8, AF::k2>		AfTIM4_CH3_PB8;
typedef AnyAFR<Port::PB, 9, AF::k2>		AfTIM4_CH4_PB9;
typedef AnyAFR<Port::PA, 8, AF::k10>	AfTIM4_ETR_PA8;
typedef AnyAFR<Port::PB, 3, AF::k2>		AfTIM4_ETR_PB3;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 12, AF::k2>	AfTIM4_CH1_PD12;
typedef AnyAFR<Port::PD, 13, AF::k2>	AfTIM4_CH2_PD13;
typedef AnyAFR<Port::PD, 14, AF::k2>	AfTIM4_CH3_PD14;
typedef AnyAFR<Port::PD, 15, AF::k2>	AfTIM4_CH4_PD15;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k2>		AfTIM4_ETR_PE0;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 6, AF::k2>		AfTIM4_CH4_PF6;
#endif	// defined(GPIOF_BASE)
#endif	// defined(TIM4_BASE)

// TIM5
#if defined(TIM5_BASE)
typedef AnyAFR<Port::PA, 0, AF::k2>		AfTIM5_CH1_PA0;
typedef AnyAFR<Port::PB, 2, AF::k2>		AfTIM5_CH1_PB2;
typedef AnyAFR<Port::PA, 1, AF::k2>		AfTIM5_CH2_PA1;
typedef AnyAFR<Port::PC, 12, AF::k1>	AfTIM5_CH2_PC12;
typedef AnyAFR<Port::PA, 2, AF::k2>		AfTIM5_CH3_PA2;
typedef AnyAFR<Port::PA, 3, AF::k2>		AfTIM5_CH4_PA3;
typedef AnyAFR<Port::PB, 12, AF::k2>	AfTIM5_ETR_PB12;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 11, AF::k1>	AfTIM5_ETR_PD11;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 8, AF::k1>		AfTIM5_CH3_PE8;
typedef AnyAFR<Port::PE, 9, AF::k1>		AfTIM5_CH4_PE9;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 6, AF::k6>		AfTIM5_CH1_PF6;
typedef AnyAFR<Port::PF, 7, AF::k6>		AfTIM5_CH2_PF7;
typedef AnyAFR<Port::PF, 8, AF::k6>		AfTIM5_CH3_PF8;
typedef AnyAFR<Port::PF, 9, AF::k6>		AfTIM5_CH4_PF9;
typedef AnyAFR<Port::PF, 6, AF::k1>		AfTIM5_ETR_PF6;
#endif	// defined(GPIOF_BASE)
#endif	// defined(TIM5_BASE)

// TIM8
#if defined(TIM8_BASE)
typedef AnyAFR<Port::PA, 0, AF::k9>		AfTIM8_BKIN_PA0;
typedef AnyAFR<Port::PA, 6, AF::k4>		AfTIM8_BKIN_PA6;
typedef AnyAFR<Port::PA, 10, AF::k11>	AfTIM8_BKIN_PA10;
typedef AnyAFR<Port::PB, 7, AF::k5>		AfTIM8_BKIN_PB7;
typedef AnyAFR<Port::PB, 6, AF::k10>	AfTIM8_BKIN2_PB6;
typedef AnyAFR<Port::PC, 9, AF::k6>		AfTIM8_BKIN2_PC9;
typedef AnyAFR<Port::PA, 15, AF::k2>	AfTIM8_CH1_PA15;
typedef AnyAFR<Port::PB, 6, AF::k5>		AfTIM8_CH1_PB6;
typedef AnyAFR<Port::PC, 6, AF::k4>		AfTIM8_CH1_PC6;
typedef AnyAFR<Port::PA, 7, AF::k4>		AfTIM8_CH1N_PA7;
typedef AnyAFR<Port::PB, 3, AF::k4>		AfTIM8_CH1N_PB3;
typedef AnyAFR<Port::PC, 10, AF::k4>	AfTIM8_CH1N_PC10;
typedef AnyAFR<Port::PA, 14, AF::k5>	AfTIM8_CH2_PA14;
typedef AnyAFR<Port::PB, 8, AF::k10>	AfTIM8_CH2_PB8;
typedef AnyAFR<Port::PC, 7, AF::k4>		AfTIM8_CH2_PC7;
typedef AnyAFR<Port::PB, 0, AF::k4>		AfTIM8_CH2N_PB0;
typedef AnyAFR<Port::PB, 4, AF::k4>		AfTIM8_CH2N_PB4;
typedef AnyAFR<Port::PC, 11, AF::k4>	AfTIM8_CH2N_PC11;
typedef AnyAFR<Port::PB, 9, AF::k10>	AfTIM8_CH3_PB9;
typedef AnyAFR<Port::PC, 8, AF::k4>		AfTIM8_CH3_PC8;
typedef AnyAFR<Port::PB, 1, AF::k4>		AfTIM8_CH3N_PB1;
typedef AnyAFR<Port::PB, 5, AF::k3>		AfTIM8_CH3N_PB5;
typedef AnyAFR<Port::PC, 12, AF::k4>	AfTIM8_CH3N_PC12;
typedef AnyAFR<Port::PC, 9, AF::k4>		AfTIM8_CH4_PC9;
typedef AnyAFR<Port::PC, 13, AF::k6>	AfTIM8_CH4N_PC13;
typedef AnyAFR<Port::PA, 0, AF::k10>	AfTIM8_ETR_PA0;
typedef AnyAFR<Port::PB, 6, AF::k6>		AfTIM8_ETR_PB6;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k4>		AfTIM8_BKIN_PD2;
typedef AnyAFR<Port::PD, 1, AF::k6>		AfTIM8_BKIN2_PD1;
typedef AnyAFR<Port::PD, 1, AF::k4>		AfTIM8_CH4_PD1;
typedef AnyAFR<Port::PD, 0, AF::k6>		AfTIM8_CH4N_PD0;
#endif	// defined(GPIOD_BASE)
#endif	// defined(TIM8_BASE)

// TIM15
#if defined(TIM15_BASE)
typedef AnyAFR<Port::PA, 9, AF::k9>		AfTIM15_BKIN_PA9;
typedef AnyAFR<Port::PC, 5, AF::k2>		AfTIM15_BKIN_PC5;
typedef AnyAFR<Port::PA, 2, AF::k9>		AfTIM15_CH1_PA2;
typedef AnyAFR<Port::PB, 14, AF::k1>	AfTIM15_CH1_PB14;
typedef AnyAFR<Port::PA, 1, AF::k9>		AfTIM15_CH1N_PA1;
typedef AnyAFR<Port::PB, 15, AF::k2>	AfTIM15_CH1N_PB15;
typedef AnyAFR<Port::PA, 3, AF::k9>		AfTIM15_CH2_PA3;
typedef AnyAFR<Port::PB, 15, AF::k1>	AfTIM15_CH2_PB15;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 9, AF::k3>		AfTIM15_CH1_PF9;
typedef AnyAFR<Port::PF, 10, AF::k3>	AfTIM15_CH2_PF10;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 9, AF::k14>	AfTIM15_CH1N_PG9;
#endif	// defined(GPIOG_BASE)
#endif	// defined(TIM15_BASE)

// TIM16
#if defined(TIM16_BASE)
typedef AnyAFR<Port::PB, 5, AF::k1>		AfTIM16_BKIN_PB5;
typedef AnyAFR<Port::PA, 6, AF::k1>		AfTIM16_CH1_PA6;
typedef AnyAFR<Port::PA, 12, AF::k1>	AfTIM16_CH1_PA12;
typedef AnyAFR<Port::PB, 4, AF::k1>		AfTIM16_CH1_PB4;
typedef AnyAFR<Port::PB, 8, AF::k1>		AfTIM16_CH1_PB8;
typedef AnyAFR<Port::PA, 13, AF::k1>	AfTIM16_CH1N_PA13;
typedef AnyAFR<Port::PB, 6, AF::k1>		AfTIM16_CH1N_PB6;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k4>		AfTIM16_CH1_PE0;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM16_BASE)

// TIM17
#if defined(TIM17_BASE)
typedef AnyAFR<Port::PA, 10, AF::k1>	AfTIM17_BKIN_PA10;
typedef AnyAFR<Port::PB, 4, AF::k10>	AfTIM17_BKIN_PB4;
typedef AnyAFR<Port::PA, 7, AF::k1>		AfTIM17_CH1_PA7;
typedef AnyAFR<Port::PB, 5, AF::k10>	AfTIM17_CH1_PB5;
typedef AnyAFR<Port::PB, 9, AF::k1>		AfTIM17_CH1_PB9;
typedef AnyAFR<Port::PB, 7, AF::k1>		AfTIM17_CH1N_PB7;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 1, AF::k4>		AfTIM17_CH1_PE1;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM17_BASE)

// TIM20
#if defined(TIM20_BASE)
typedef AnyAFR<Port::PB, 2, AF::k3>		AfTIM20_CH1_PB2;
typedef AnyAFR<Port::PC, 2, AF::k6>		AfTIM20_CH2_PC2;
typedef AnyAFR<Port::PC, 8, AF::k6>		AfTIM20_CH3_PC8;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 2, AF::k6>		AfTIM20_CH1_PE2;
typedef AnyAFR<Port::PE, 4, AF::k6>		AfTIM20_CH1N_PE4;
typedef AnyAFR<Port::PE, 3, AF::k6>		AfTIM20_CH2_PE3;
typedef AnyAFR<Port::PE, 5, AF::k6>		AfTIM20_CH2N_PE5;
typedef AnyAFR<Port::PE, 6, AF::k6>		AfTIM20_CH3N_PE6;
typedef AnyAFR<Port::PE, 1, AF::k6>		AfTIM20_CH4_PE1;
typedef AnyAFR<Port::PE, 0, AF::k3>		AfTIM20_CH4N_PE0;
typedef AnyAFR<Port::PE, 0, AF::k6>		AfTIM20_ETR_PE0;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 7, AF::k2>		AfTIM20_BKIN_PF7;
typedef AnyAFR<Port::PF, 9, AF::k2>		AfTIM20_BKIN_PF9;
typedef AnyAFR<Port::PF, 8, AF::k2>		AfTIM20_BKIN2_PF8;
typedef AnyAFR<Port::PF, 10, AF::k2>	AfTIM20_BKIN2_PF10;
typedef AnyAFR<Port::PF, 12, AF::k2>	AfTIM20_CH1_PF12;
typedef AnyAFR<Port::PF, 4, AF::k3>		AfTIM20_CH1N_PF4;
typedef AnyAFR<Port::PF, 13, AF::k2>	AfTIM20_CH2_PF13;
typedef AnyAFR<Port::PF, 5, AF::k2>		AfTIM20_CH2N_PF5;
typedef AnyAFR<Port::PF, 2, AF::k2>		AfTIM20_CH3_PF2;
typedef AnyAFR<Port::PF, 14, AF::k2>	AfTIM20_CH3_PF14;
typedef AnyAFR<Port::PF, 3, AF::k2>		AfTIM20_CH4_PF3;
typedef AnyAFR<Port::PF, 15, AF::k2>	AfTIM20_CH4_PF15;
typedef AnyAFR<Port::PF, 11, AF::k2>	AfTIM20_ETR_PF11;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 3, AF::k2>		AfTIM20_BKIN_PG3;
typedef AnyAFR<Port::PG, 6, AF::k2>		AfTIM20_BKIN_PG6;
typedef AnyAFR<Port::PG, 4, AF::k2>		AfTIM20_BKIN2_PG4;
typedef AnyAFR<Port::PG, 0, AF::k2>		AfTIM20_CH1N_PG0;
typedef AnyAFR<Port::PG, 1, AF::k2>		AfTIM20_CH2N_PG1;
typedef AnyAFR<Port::PG, 2, AF::k2>		AfTIM20_CH3N_PG2;
typedef AnyAFR<Port::PG, 3, AF::k6>		AfTIM20_CH4N_PG3;
typedef AnyAFR<Port::PG, 5, AF::k2>		AfTIM20_ETR_PG5;
#endif	// defined(GPIOG_BASE)
#endif	// defined(TIM20_BASE)

// USART1
#if defined(USART1_BASE)
typedef AnyAFR<Port::PA, 8, AF::k7>		AfUSART1_CK_PA8;
typedef AnyAFR<Port::PA, 11, AF::k7>	AfUSART1_CTS_PA11;
typedef AnyAFR<Port::PA, 12, AF::k7>	AfUSART1_RTS_DE_PA12;
typedef AnyAFR<Port::PA, 10, AF::k7>	AfUSART1_RX_PA10;
typedef AnyAFR<Port::PB, 7, AF::k7>		AfUSART1_RX_PB7;
typedef AnyAFR<Port::PC, 5, AF::k7>		AfUSART1_RX_PC5;
typedef AnyAFR<Port::PA, 9, AF::k7>		AfUSART1_TX_PA9;
typedef AnyAFR<Port::PB, 6, AF::k7>		AfUSART1_TX_PB6;
typedef AnyAFR<Port::PC, 4, AF::k7>		AfUSART1_TX_PC4;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 1, AF::k7>		AfUSART1_RX_PE1;
typedef AnyAFR<Port::PE, 0, AF::k7>		AfUSART1_TX_PE0;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 9, AF::k7>		AfUSART1_TX_PG9;
#endif	// defined(GPIOG_BASE)
#endif	// defined(USART1_BASE)

// USART2
#if defined(USART2_BASE)
typedef AnyAFR<Port::PA, 4, AF::k7>		AfUSART2_CK_PA4;
typedef AnyAFR<Port::PB, 5, AF::k7>		AfUSART2_CK_PB5;
typedef AnyAFR<Port::PA, 0, AF::k7>		AfUSART2_CTS_PA0;
typedef AnyAFR<Port::PA, 1, AF::k7>		AfUSART2_RTS_DE_PA1;
typedef AnyAFR<Port::PA, 3, AF::k7>		AfUSART2_RX_PA3;
typedef AnyAFR<Port::PA, 15, AF::k7>	AfUSART2_RX_PA15;
typedef AnyAFR<Port::PB, 4, AF::k7>		AfUSART2_RX_PB4;
typedef AnyAFR<Port::PA, 2, AF::k7>		AfUSART2_TX_PA2;
typedef AnyAFR<Port::PA, 14, AF::k7>	AfUSART2_TX_PA14;
typedef AnyAFR<Port::PB, 3, AF::k7>		AfUSART2_TX_PB3;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 7, AF::k7>		AfUSART2_CK_PD7;
typedef AnyAFR<Port::PD, 3, AF::k7>		AfUSART2_CTS_PD3;
typedef AnyAFR<Port::PD, 4, AF::k7>		AfUSART2_RTS_DE_PD4;
typedef AnyAFR<Port::PD, 6, AF::k7>		AfUSART2_RX_PD6;
typedef AnyAFR<Port::PD, 5, AF::k7>		AfUSART2_TX_PD5;
#endif	// defined(GPIOD_BASE)
#endif	// defined(USART2_BASE)

// USART3
#if defined(USART3_BASE)
typedef AnyAFR<Port::PB, 12, AF::k7>	AfUSART3_CK_PB12;
typedef AnyAFR<Port::PC, 12, AF::k7>	AfUSART3_CK_PC12;
typedef AnyAFR<Port::PA, 13, AF::k7>	AfUSART3_CTS_PA13;
typedef AnyAFR<Port::PB, 13, AF::k7>	AfUSART3_CTS_PB13;
typedef AnyAFR<Port::PB, 14, AF::k7>	AfUSART3_RTS_DE_PB14;
typedef AnyAFR<Port::PB, 8, AF::k7>		AfUSART3_RX_PB8;
typedef AnyAFR<Port::PB, 11, AF::k7>	AfUSART3_RX_PB11;
typedef AnyAFR<Port::PC, 11, AF::k7>	AfUSART3_RX_PC11;
typedef AnyAFR<Port::PB, 9, AF::k7>		AfUSART3_TX_PB9;
typedef AnyAFR<Port::PB, 10, AF::k7>	AfUSART3_TX_PB10;
typedef AnyAFR<Port::PC, 10, AF::k7>	AfUSART3_TX_PC10;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 10, AF::k7>	AfUSART3_CK_PD10;
typedef AnyAFR<Port::PD, 11, AF::k7>	AfUSART3_CTS_PD11;
typedef AnyAFR<Port::PD, 12, AF::k7>	AfUSART3_RTS_DE_PD12;
typedef AnyAFR<Port::PD, 9, AF::k7>		AfUSART3_RX_PD9;
typedef AnyAFR<Port::PD, 8, AF::k7>		AfUSART3_TX_PD8;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 15, AF::k7>	AfUSART3_RX_PE15;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 6, AF::k7>		AfUSART3_RTS_PF6;
#endif	// defined(GPIOF_BASE)
#endif	// defined(USART3_BASE)

// UART4
#if defined(UART4_BASE)
typedef AnyAFR<Port::PB, 7, AF::k14>	AfUART4_CTS_PB7;
typedef AnyAFR<Port::PA, 15, AF::k8>	AfUART4_RTS_DE_PA15;
typedef AnyAFR<Port::PC, 11, AF::k5>	AfUART4_RX_PC11;
typedef AnyAFR<Port::PC, 10, AF::k5>	AfUART4_TX_PC10;
#endif	// defined(UART4_BASE)

// UART5
#if defined(UART5_BASE)
typedef AnyAFR<Port::PB, 5, AF::k14>	AfUART5_CTS_PB5;
typedef AnyAFR<Port::PB, 4, AF::k8>		AfUART5_RTS_DE_PB4;
typedef AnyAFR<Port::PC, 12, AF::k5>	AfUART5_TX_PC12;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k5>		AfUART5_RX_PD2;
#endif	// defined(GPIOD_BASE)
#endif	// defined(UART5_BASE)

// USB
#if defined(USB_BASE)
typedef AnyAFR<Port::PA, 10, AF::k3>	AfUSB_CRS_SYNC_PA10;
typedef AnyAFR<Port::PB, 3, AF::k3>		AfUSB_CRS_SYNC_PB3;
#endif	// defined(USB_BASE)

// UCPD1
#if defined(UCPD1_BASE)
typedef AnyAFR<Port::PA, 2, AF::k14>	AfUCPD1_FRSTX_PA2;
typedef AnyAFR<Port::PA, 5, AF::k14>	AfUCPD1_FRSTX_PA5;
typedef AnyAFR<Port::PA, 7, AF::k14>	AfUCPD1_FRSTX_PA7;
typedef AnyAFR<Port::PB, 0, AF::k14>	AfUCPD1_FRSTX_PB0;
typedef AnyAFR<Port::PC, 12, AF::k14>	AfUCPD1_FRSTX_PC12;
#endif	// defined(UCPD1_BASE)

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
typedef AnyAFR<Port::PB, 2, AF::k15>	AfEVENTOUT_PB2;
typedef AnyAFR<Port::PB, 3, AF::k15>	AfEVENTOUT_PB3;
typedef AnyAFR<Port::PB, 4, AF::k15>	AfEVENTOUT_PB4;
typedef AnyAFR<Port::PB, 5, AF::k15>	AfEVENTOUT_PB5;
typedef AnyAFR<Port::PB, 6, AF::k15>	AfEVENTOUT_PB6;
typedef AnyAFR<Port::PB, 7, AF::k15>	AfEVENTOUT_PB7;
typedef AnyAFR<Port::PB, 8, AF::k15>	AfEVENTOUT_PB8;
typedef AnyAFR<Port::PB, 9, AF::k15>	AfEVENTOUT_PB9;
typedef AnyAFR<Port::PB, 10, AF::k15>	AfEVENTOUT_PB10;
typedef AnyAFR<Port::PB, 11, AF::k15>	AfEVENTOUT_PB11;
typedef AnyAFR<Port::PB, 12, AF::k15>	AfEVENTOUT_PB12;
typedef AnyAFR<Port::PB, 13, AF::k15>	AfEVENTOUT_PB13;
typedef AnyAFR<Port::PB, 14, AF::k15>	AfEVENTOUT_PB14;
typedef AnyAFR<Port::PB, 15, AF::k15>	AfEVENTOUT_PB15;
typedef AnyAFR<Port::PC, 0, AF::k15>	AfEVENTOUT_PC0;
typedef AnyAFR<Port::PC, 1, AF::k15>	AfEVENTOUT_PC1;
typedef AnyAFR<Port::PC, 2, AF::k15>	AfEVENTOUT_PC2;
typedef AnyAFR<Port::PC, 3, AF::k15>	AfEVENTOUT_PC3;
typedef AnyAFR<Port::PC, 4, AF::k15>	AfEVENTOUT_PC4;
typedef AnyAFR<Port::PC, 5, AF::k15>	AfEVENTOUT_PC5;
typedef AnyAFR<Port::PC, 6, AF::k15>	AfEVENTOUT_PC6;
typedef AnyAFR<Port::PC, 7, AF::k15>	AfEVENTOUT_PC7;
typedef AnyAFR<Port::PC, 8, AF::k15>	AfEVENTOUT_PC8;
typedef AnyAFR<Port::PC, 9, AF::k15>	AfEVENTOUT_PC9;
typedef AnyAFR<Port::PC, 10, AF::k15>	AfEVENTOUT_PC10;
typedef AnyAFR<Port::PC, 11, AF::k15>	AfEVENTOUT_PC11;
typedef AnyAFR<Port::PC, 12, AF::k15>	AfEVENTOUT_PC12;
typedef AnyAFR<Port::PC, 13, AF::k15>	AfEVENTOUT_PC13;
typedef AnyAFR<Port::PC, 14, AF::k15>	AfEVENTOUT_PC14;
typedef AnyAFR<Port::PC, 15, AF::k15>	AfEVENTOUT_PC15;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 0, AF::k15>	AfEVENTOUT_PD0;
typedef AnyAFR<Port::PD, 1, AF::k15>	AfEVENTOUT_PD1;
typedef AnyAFR<Port::PD, 2, AF::k15>	AfEVENTOUT_PD2;
typedef AnyAFR<Port::PD, 3, AF::k15>	AfEVENTOUT_PD3;
typedef AnyAFR<Port::PD, 4, AF::k15>	AfEVENTOUT_PD4;
typedef AnyAFR<Port::PD, 5, AF::k15>	AfEVENTOUT_PD5;
typedef AnyAFR<Port::PD, 6, AF::k15>	AfEVENTOUT_PD6;
typedef AnyAFR<Port::PD, 7, AF::k15>	AfEVENTOUT_PD7;
typedef AnyAFR<Port::PD, 8, AF::k15>	AfEVENTOUT_PD8;
typedef AnyAFR<Port::PD, 9, AF::k15>	AfEVENTOUT_PD9;
typedef AnyAFR<Port::PD, 10, AF::k15>	AfEVENTOUT_PD10;
typedef AnyAFR<Port::PD, 11, AF::k15>	AfEVENTOUT_PD11;
typedef AnyAFR<Port::PD, 12, AF::k15>	AfEVENTOUT_PD12;
typedef AnyAFR<Port::PD, 13, AF::k15>	AfEVENTOUT_PD13;
typedef AnyAFR<Port::PD, 14, AF::k15>	AfEVENTOUT_PD14;
typedef AnyAFR<Port::PD, 15, AF::k15>	AfEVENTOUT_PD15;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k15>	AfEVENTOUT_PE0;
typedef AnyAFR<Port::PE, 1, AF::k15>	AfEVENTOUT_PE1;
typedef AnyAFR<Port::PE, 2, AF::k15>	AfEVENTOUT_PE2;
typedef AnyAFR<Port::PE, 3, AF::k15>	AfEVENTOUT_PE3;
typedef AnyAFR<Port::PE, 4, AF::k15>	AfEVENTOUT_PE4;
typedef AnyAFR<Port::PE, 5, AF::k15>	AfEVENTOUT_PE5;
typedef AnyAFR<Port::PE, 6, AF::k15>	AfEVENTOUT_PE6;
typedef AnyAFR<Port::PE, 7, AF::k15>	AfEVENTOUT_PE7;
typedef AnyAFR<Port::PE, 8, AF::k15>	AfEVENTOUT_PE8;
typedef AnyAFR<Port::PE, 9, AF::k15>	AfEVENTOUT_PE9;
typedef AnyAFR<Port::PE, 10, AF::k15>	AfEVENTOUT_PE10;
typedef AnyAFR<Port::PE, 11, AF::k15>	AfEVENTOUT_PE11;
typedef AnyAFR<Port::PE, 12, AF::k15>	AfEVENTOUT_PE12;
typedef AnyAFR<Port::PE, 13, AF::k15>	AfEVENTOUT_PE13;
typedef AnyAFR<Port::PE, 14, AF::k15>	AfEVENTOUT_PE14;
typedef AnyAFR<Port::PE, 15, AF::k15>	AfEVENTOUT_PE15;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 0, AF::k15>	AfEVENTOUT_PF0;
typedef AnyAFR<Port::PF, 1, AF::k15>	AfEVENTOUT_PF1;
typedef AnyAFR<Port::PF, 2, AF::k15>	AfEVENTOUT_PF2;
typedef AnyAFR<Port::PF, 3, AF::k15>	AfEVENTOUT_PF3;
typedef AnyAFR<Port::PF, 4, AF::k15>	AfEVENTOUT_PF4;
typedef AnyAFR<Port::PF, 5, AF::k15>	AfEVENTOUT_PF5;
typedef AnyAFR<Port::PF, 6, AF::k15>	AfEVENTOUT_PF6;
typedef AnyAFR<Port::PF, 7, AF::k15>	AfEVENTOUT_PF7;
typedef AnyAFR<Port::PF, 8, AF::k15>	AfEVENTOUT_PF8;
typedef AnyAFR<Port::PF, 9, AF::k15>	AfEVENTOUT_PF9;
typedef AnyAFR<Port::PF, 10, AF::k15>	AfEVENTOUT_PF10;
typedef AnyAFR<Port::PF, 11, AF::k15>	AfEVENTOUT_PF11;
typedef AnyAFR<Port::PF, 12, AF::k15>	AfEVENTOUT_PF12;
typedef AnyAFR<Port::PF, 13, AF::k15>	AfEVENTOUT_PF13;
typedef AnyAFR<Port::PF, 14, AF::k15>	AfEVENTOUT_PF14;
typedef AnyAFR<Port::PF, 15, AF::k15>	AfEVENTOUT_PF15;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 0, AF::k15>	AfEVENTOUT_PG0;
typedef AnyAFR<Port::PG, 1, AF::k15>	AfEVENTOUT_PG1;
typedef AnyAFR<Port::PG, 2, AF::k15>	AfEVENTOUT_PG2;
typedef AnyAFR<Port::PG, 3, AF::k15>	AfEVENTOUT_PG3;
typedef AnyAFR<Port::PG, 4, AF::k15>	AfEVENTOUT_PG4;
typedef AnyAFR<Port::PG, 5, AF::k15>	AfEVENTOUT_PG5;
typedef AnyAFR<Port::PG, 6, AF::k15>	AfEVENTOUT_PG6;
typedef AnyAFR<Port::PG, 7, AF::k15>	AfEVENTOUT_PG7;
typedef AnyAFR<Port::PG, 8, AF::k15>	AfEVENTOUT_PG8;
typedef AnyAFR<Port::PG, 9, AF::k15>	AfEVENTOUT_PG9;
typedef AnyAFR<Port::PG, 10, AF::k15>	AfEVENTOUT_PG10;
#endif	// defined(GPIOG_BASE)


}	// namespace Gpio
}	// namespace Bmt

