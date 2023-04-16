#pragma once

namespace Bmt
{
namespace Gpio
{


// SYS
typedef AnyAFR<Port::PA, 8, AF::k0>		AfMCO_PA8;
typedef AnyAFR<Port::PA, 14, AF::k0>	AfJTCK_PA14;
typedef AnyAFR<Port::PA, 15, AF::k0>	AfJTDI_PA15;
typedef AnyAFR<Port::PB, 3, AF::k0>		AfJTDO_PB3;
typedef AnyAFR<Port::PA, 13, AF::k0>	AfJTMS_PA13;
typedef AnyAFR<Port::PB, 4, AF::k0>		AfNJTRST_PB4;
typedef AnyAFR<Port::PC, 1, AF::k0>		AfTRACED0_PC1;
typedef AnyAFR<Port::PC, 10, AF::k0>	AfTRACED1_PC10;
typedef AnyAFR<Port::PC, 12, AF::k0>	AfTRACED3_PC12;
typedef AnyAFR<Port::PB, 3, AF::k0>		AfTRACESWO_PB3;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k0>		AfTRACED2_PD2;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 2, AF::k0>		AfTRACECLK_PE2;
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
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 13, AF::k9>	AfCAN1_TX_PH13;
#endif	// defined(GPIOH_BASE)
#if defined(GPIOI_BASE)
typedef AnyAFR<Port::PI, 9, AF::k9>		AfCAN1_RX_PI9;
#endif	// defined(GPIOI_BASE)
#endif	// defined(CAN1_BASE)

// CAN2
#if defined(CAN2_BASE)
typedef AnyAFR<Port::PB, 5, AF::k3>		AfCAN2_RX_PB5;
typedef AnyAFR<Port::PB, 12, AF::k10>	AfCAN2_RX_PB12;
typedef AnyAFR<Port::PB, 6, AF::k8>		AfCAN2_TX_PB6;
typedef AnyAFR<Port::PB, 13, AF::k10>	AfCAN2_TX_PB13;
#endif	// defined(CAN2_BASE)

// COMP1
#if defined(COMP1_BASE)
typedef AnyAFR<Port::PB, 0, AF::k12>	AfCOMP1_OUT_PB0;
typedef AnyAFR<Port::PB, 10, AF::k12>	AfCOMP1_OUT_PB10;
#endif	// defined(COMP1_BASE)

// COMP2
#if defined(COMP2_BASE)
typedef AnyAFR<Port::PB, 5, AF::k12>	AfCOMP2_OUT_PB5;
typedef AnyAFR<Port::PB, 11, AF::k12>	AfCOMP2_OUT_PB11;
#endif	// defined(COMP2_BASE)

// DFSDM1
#if defined(DFSDM1_BASE)
typedef AnyAFR<Port::PB, 2, AF::k6>		AfDFSDM1_CKIN0_PB2;
typedef AnyAFR<Port::PB, 13, AF::k6>	AfDFSDM1_CKIN1_PB13;
typedef AnyAFR<Port::PB, 15, AF::k6>	AfDFSDM1_CKIN2_PB15;
typedef AnyAFR<Port::PC, 6, AF::k6>		AfDFSDM1_CKIN3_PC6;
typedef AnyAFR<Port::PC, 1, AF::k6>		AfDFSDM1_CKIN4_PC1;
typedef AnyAFR<Port::PB, 7, AF::k6>		AfDFSDM1_CKIN5_PB7;
typedef AnyAFR<Port::PB, 9, AF::k6>		AfDFSDM1_CKIN6_PB9;
typedef AnyAFR<Port::PB, 11, AF::k6>	AfDFSDM1_CKIN7_PB11;
typedef AnyAFR<Port::PC, 2, AF::k6>		AfDFSDM1_CKOUT_PC2;
typedef AnyAFR<Port::PB, 1, AF::k6>		AfDFSDM1_DATIN0_PB1;
typedef AnyAFR<Port::PB, 12, AF::k6>	AfDFSDM1_DATIN1_PB12;
typedef AnyAFR<Port::PB, 14, AF::k6>	AfDFSDM1_DATIN2_PB14;
typedef AnyAFR<Port::PC, 7, AF::k6>		AfDFSDM1_DATIN3_PC7;
typedef AnyAFR<Port::PC, 0, AF::k6>		AfDFSDM1_DATIN4_PC0;
typedef AnyAFR<Port::PB, 6, AF::k6>		AfDFSDM1_DATIN5_PB6;
typedef AnyAFR<Port::PB, 8, AF::k6>		AfDFSDM1_DATIN6_PB8;
typedef AnyAFR<Port::PB, 10, AF::k6>	AfDFSDM1_DATIN7_PB10;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 4, AF::k6>		AfDFSDM1_CKIN0_PD4;
typedef AnyAFR<Port::PD, 7, AF::k6>		AfDFSDM1_CKIN1_PD7;
typedef AnyAFR<Port::PD, 1, AF::k6>		AfDFSDM1_CKIN7_PD1;
typedef AnyAFR<Port::PD, 3, AF::k6>		AfDFSDM1_DATIN0_PD3;
typedef AnyAFR<Port::PD, 6, AF::k6>		AfDFSDM1_DATIN1_PD6;
typedef AnyAFR<Port::PD, 0, AF::k6>		AfDFSDM1_DATIN7_PD0;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 8, AF::k6>		AfDFSDM1_CKIN2_PE8;
typedef AnyAFR<Port::PE, 5, AF::k6>		AfDFSDM1_CKIN3_PE5;
typedef AnyAFR<Port::PE, 11, AF::k6>	AfDFSDM1_CKIN4_PE11;
typedef AnyAFR<Port::PE, 13, AF::k6>	AfDFSDM1_CKIN5_PE13;
typedef AnyAFR<Port::PE, 9, AF::k6>		AfDFSDM1_CKOUT_PE9;
typedef AnyAFR<Port::PE, 7, AF::k6>		AfDFSDM1_DATIN2_PE7;
typedef AnyAFR<Port::PE, 4, AF::k6>		AfDFSDM1_DATIN3_PE4;
typedef AnyAFR<Port::PE, 10, AF::k6>	AfDFSDM1_DATIN4_PE10;
typedef AnyAFR<Port::PE, 12, AF::k6>	AfDFSDM1_DATIN5_PE12;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 14, AF::k6>	AfDFSDM1_CKIN6_PF14;
typedef AnyAFR<Port::PF, 13, AF::k6>	AfDFSDM1_DATIN6_PF13;
#endif	// defined(GPIOF_BASE)
#endif	// defined(DFSDM1_BASE)

// DCMI
#if defined(DCMI_BASE)
typedef AnyAFR<Port::PA, 9, AF::k5>		AfDCMI_D0_PA9;
typedef AnyAFR<Port::PC, 6, AF::k10>	AfDCMI_D0_PC6;
typedef AnyAFR<Port::PA, 10, AF::k5>	AfDCMI_D1_PA10;
typedef AnyAFR<Port::PC, 7, AF::k10>	AfDCMI_D1_PC7;
typedef AnyAFR<Port::PB, 5, AF::k10>	AfDCMI_D10_PB5;
typedef AnyAFR<Port::PB, 4, AF::k10>	AfDCMI_D12_PB4;
typedef AnyAFR<Port::PC, 8, AF::k10>	AfDCMI_D2_PC8;
typedef AnyAFR<Port::PC, 9, AF::k4>		AfDCMI_D3_PC9;
typedef AnyAFR<Port::PC, 11, AF::k10>	AfDCMI_D4_PC11;
typedef AnyAFR<Port::PB, 6, AF::k10>	AfDCMI_D5_PB6;
typedef AnyAFR<Port::PB, 8, AF::k10>	AfDCMI_D6_PB8;
typedef AnyAFR<Port::PB, 9, AF::k10>	AfDCMI_D7_PB9;
typedef AnyAFR<Port::PC, 10, AF::k10>	AfDCMI_D8_PC10;
typedef AnyAFR<Port::PC, 12, AF::k10>	AfDCMI_D9_PC12;
typedef AnyAFR<Port::PA, 4, AF::k10>	AfDCMI_HSYNC_PA4;
typedef AnyAFR<Port::PA, 6, AF::k4>		AfDCMI_PIXCLK_PA6;
typedef AnyAFR<Port::PB, 7, AF::k10>	AfDCMI_VSYNC_PB7;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 6, AF::k4>		AfDCMI_D10_PD6;
typedef AnyAFR<Port::PD, 2, AF::k10>	AfDCMI_D11_PD2;
typedef AnyAFR<Port::PD, 3, AF::k4>		AfDCMI_D5_PD3;
typedef AnyAFR<Port::PD, 8, AF::k10>	AfDCMI_HSYNC_PD8;
typedef AnyAFR<Port::PD, 9, AF::k10>	AfDCMI_PIXCLK_PD9;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k10>	AfDCMI_D2_PE0;
typedef AnyAFR<Port::PE, 1, AF::k10>	AfDCMI_D3_PE1;
typedef AnyAFR<Port::PE, 4, AF::k10>	AfDCMI_D4_PE4;
typedef AnyAFR<Port::PE, 5, AF::k10>	AfDCMI_D6_PE5;
typedef AnyAFR<Port::PE, 6, AF::k10>	AfDCMI_D7_PE6;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 10, AF::k10>	AfDCMI_D11_PF10;
typedef AnyAFR<Port::PF, 11, AF::k10>	AfDCMI_D12_PF11;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 15, AF::k10>	AfDCMI_D13_PG15;
#endif	// defined(GPIOG_BASE)
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 9, AF::k10>	AfDCMI_D0_PH9;
typedef AnyAFR<Port::PH, 10, AF::k10>	AfDCMI_D1_PH10;
typedef AnyAFR<Port::PH, 15, AF::k10>	AfDCMI_D11_PH15;
typedef AnyAFR<Port::PH, 11, AF::k10>	AfDCMI_D2_PH11;
typedef AnyAFR<Port::PH, 12, AF::k10>	AfDCMI_D3_PH12;
typedef AnyAFR<Port::PH, 14, AF::k10>	AfDCMI_D4_PH14;
typedef AnyAFR<Port::PH, 6, AF::k10>	AfDCMI_D8_PH6;
typedef AnyAFR<Port::PH, 7, AF::k10>	AfDCMI_D9_PH7;
typedef AnyAFR<Port::PH, 8, AF::k10>	AfDCMI_HSYNC_PH8;
typedef AnyAFR<Port::PH, 5, AF::k10>	AfDCMI_PIXCLK_PH5;
#endif	// defined(GPIOH_BASE)
#if defined(GPIOI_BASE)
typedef AnyAFR<Port::PI, 3, AF::k10>	AfDCMI_D10_PI3;
typedef AnyAFR<Port::PI, 8, AF::k10>	AfDCMI_D12_PI8;
typedef AnyAFR<Port::PI, 0, AF::k10>	AfDCMI_D13_PI0;
typedef AnyAFR<Port::PI, 4, AF::k10>	AfDCMI_D5_PI4;
typedef AnyAFR<Port::PI, 6, AF::k10>	AfDCMI_D6_PI6;
typedef AnyAFR<Port::PI, 7, AF::k10>	AfDCMI_D7_PI7;
typedef AnyAFR<Port::PI, 1, AF::k10>	AfDCMI_D8_PI1;
typedef AnyAFR<Port::PI, 2, AF::k10>	AfDCMI_D9_PI2;
typedef AnyAFR<Port::PI, 5, AF::k10>	AfDCMI_VSYNC_PI5;
#endif	// defined(GPIOI_BASE)
#endif	// defined(DCMI_BASE)

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
typedef AnyAFR<Port::PF, 0, AF::k12>	AfFMC_A0_PF0;
typedef AnyAFR<Port::PF, 1, AF::k12>	AfFMC_A1_PF1;
typedef AnyAFR<Port::PF, 2, AF::k12>	AfFMC_A2_PF2;
typedef AnyAFR<Port::PF, 3, AF::k12>	AfFMC_A3_PF3;
typedef AnyAFR<Port::PF, 4, AF::k12>	AfFMC_A4_PF4;
typedef AnyAFR<Port::PF, 5, AF::k12>	AfFMC_A5_PF5;
typedef AnyAFR<Port::PF, 12, AF::k12>	AfFMC_A6_PF12;
typedef AnyAFR<Port::PF, 13, AF::k12>	AfFMC_A7_PF13;
typedef AnyAFR<Port::PF, 14, AF::k12>	AfFMC_A8_PF14;
typedef AnyAFR<Port::PF, 15, AF::k12>	AfFMC_A9_PF15;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 0, AF::k12>	AfFMC_A10_PG0;
typedef AnyAFR<Port::PG, 1, AF::k12>	AfFMC_A11_PG1;
typedef AnyAFR<Port::PG, 2, AF::k12>	AfFMC_A12_PG2;
typedef AnyAFR<Port::PG, 3, AF::k12>	AfFMC_A13_PG3;
typedef AnyAFR<Port::PG, 4, AF::k12>	AfFMC_A14_PG4;
typedef AnyAFR<Port::PG, 5, AF::k12>	AfFMC_A15_PG5;
typedef AnyAFR<Port::PG, 13, AF::k12>	AfFMC_A24_PG13;
typedef AnyAFR<Port::PG, 14, AF::k12>	AfFMC_A25_PG14;
typedef AnyAFR<Port::PG, 7, AF::k12>	AfFMC_INT_PG7;
typedef AnyAFR<Port::PG, 9, AF::k12>	AfFMC_NCE_PG9;
typedef AnyAFR<Port::PG, 9, AF::k12>	AfFMC_NE2_PG9;
typedef AnyAFR<Port::PG, 10, AF::k12>	AfFMC_NE3_PG10;
typedef AnyAFR<Port::PG, 12, AF::k12>	AfFMC_NE4_PG12;
#endif	// defined(GPIOG_BASE)
#endif	// defined(FMC_BASE)

// IR
#if defined(TIM16_BASE)
typedef AnyAFR<Port::PA, 13, AF::k1>	AfIR_OUT_PA13;
typedef AnyAFR<Port::PB, 9, AF::k1>		AfIR_OUT_PB9;
#endif	// defined(TIM16_BASE)

// I2C1
#if defined(I2C1_BASE)
typedef AnyAFR<Port::PA, 9, AF::k4>		AfI2C1_SCL_PA9;
typedef AnyAFR<Port::PB, 6, AF::k4>		AfI2C1_SCL_PB6;
typedef AnyAFR<Port::PB, 8, AF::k4>		AfI2C1_SCL_PB8;
typedef AnyAFR<Port::PA, 10, AF::k4>	AfI2C1_SDA_PA10;
typedef AnyAFR<Port::PB, 7, AF::k4>		AfI2C1_SDA_PB7;
typedef AnyAFR<Port::PB, 9, AF::k4>		AfI2C1_SDA_PB9;
typedef AnyAFR<Port::PA, 1, AF::k4>		AfI2C1_SMBA_PA1;
typedef AnyAFR<Port::PA, 14, AF::k4>	AfI2C1_SMBA_PA14;
typedef AnyAFR<Port::PB, 5, AF::k4>		AfI2C1_SMBA_PB5;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 14, AF::k4>	AfI2C1_SCL_PG14;
typedef AnyAFR<Port::PG, 13, AF::k4>	AfI2C1_SDA_PG13;
typedef AnyAFR<Port::PG, 15, AF::k4>	AfI2C1_SMBA_PG15;
#endif	// defined(GPIOG_BASE)
#endif	// defined(I2C1_BASE)

// I2C2
#if defined(I2C2_BASE)
typedef AnyAFR<Port::PB, 10, AF::k4>	AfI2C2_SCL_PB10;
typedef AnyAFR<Port::PB, 13, AF::k4>	AfI2C2_SCL_PB13;
typedef AnyAFR<Port::PB, 11, AF::k4>	AfI2C2_SDA_PB11;
typedef AnyAFR<Port::PB, 14, AF::k4>	AfI2C2_SDA_PB14;
typedef AnyAFR<Port::PB, 12, AF::k4>	AfI2C2_SMBA_PB12;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 1, AF::k4>		AfI2C2_SCL_PF1;
typedef AnyAFR<Port::PF, 0, AF::k4>		AfI2C2_SDA_PF0;
typedef AnyAFR<Port::PF, 2, AF::k4>		AfI2C2_SMBA_PF2;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 4, AF::k4>		AfI2C2_SCL_PH4;
typedef AnyAFR<Port::PH, 5, AF::k4>		AfI2C2_SDA_PH5;
typedef AnyAFR<Port::PH, 6, AF::k4>		AfI2C2_SMBA_PH6;
#endif	// defined(GPIOH_BASE)
#endif	// defined(I2C2_BASE)

// I2C3
#if defined(I2C3_BASE)
typedef AnyAFR<Port::PA, 7, AF::k4>		AfI2C3_SCL_PA7;
typedef AnyAFR<Port::PC, 0, AF::k4>		AfI2C3_SCL_PC0;
typedef AnyAFR<Port::PB, 4, AF::k4>		AfI2C3_SDA_PB4;
typedef AnyAFR<Port::PC, 1, AF::k4>		AfI2C3_SDA_PC1;
typedef AnyAFR<Port::PC, 9, AF::k6>		AfI2C3_SDA_PC9;
typedef AnyAFR<Port::PB, 2, AF::k4>		AfI2C3_SMBA_PB2;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 7, AF::k4>		AfI2C3_SCL_PG7;
typedef AnyAFR<Port::PG, 8, AF::k4>		AfI2C3_SDA_PG8;
typedef AnyAFR<Port::PG, 6, AF::k4>		AfI2C3_SMBA_PG6;
#endif	// defined(GPIOG_BASE)
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 7, AF::k4>		AfI2C3_SCL_PH7;
typedef AnyAFR<Port::PH, 8, AF::k4>		AfI2C3_SDA_PH8;
typedef AnyAFR<Port::PH, 9, AF::k4>		AfI2C3_SMBA_PH9;
#endif	// defined(GPIOH_BASE)
#endif	// defined(I2C3_BASE)

// I2C4
#if defined(I2C4_BASE)
typedef AnyAFR<Port::PB, 6, AF::k5>		AfI2C4_SCL_PB6;
typedef AnyAFR<Port::PB, 10, AF::k3>	AfI2C4_SCL_PB10;
typedef AnyAFR<Port::PC, 0, AF::k2>		AfI2C4_SCL_PC0;
typedef AnyAFR<Port::PB, 7, AF::k5>		AfI2C4_SDA_PB7;
typedef AnyAFR<Port::PB, 11, AF::k3>	AfI2C4_SDA_PB11;
typedef AnyAFR<Port::PC, 1, AF::k2>		AfI2C4_SDA_PC1;
typedef AnyAFR<Port::PA, 14, AF::k5>	AfI2C4_SMBA_PA14;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 12, AF::k4>	AfI2C4_SCL_PD12;
typedef AnyAFR<Port::PD, 13, AF::k4>	AfI2C4_SDA_PD13;
typedef AnyAFR<Port::PD, 11, AF::k4>	AfI2C4_SMBA_PD11;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 14, AF::k4>	AfI2C4_SCL_PF14;
typedef AnyAFR<Port::PF, 15, AF::k4>	AfI2C4_SDA_PF15;
typedef AnyAFR<Port::PF, 13, AF::k4>	AfI2C4_SMBA_PF13;
#endif	// defined(GPIOF_BASE)
#endif	// defined(I2C4_BASE)

// LCD
#if defined(LCD_BASE)
typedef AnyAFR<Port::PA, 8, AF::k11>	AfLCD_COM0_PA8;
typedef AnyAFR<Port::PA, 9, AF::k11>	AfLCD_COM1_PA9;
typedef AnyAFR<Port::PA, 10, AF::k11>	AfLCD_COM2_PA10;
typedef AnyAFR<Port::PB, 9, AF::k11>	AfLCD_COM3_PB9;
typedef AnyAFR<Port::PC, 10, AF::k11>	AfLCD_COM4_PC10;
typedef AnyAFR<Port::PC, 11, AF::k11>	AfLCD_COM5_PC11;
typedef AnyAFR<Port::PC, 12, AF::k11>	AfLCD_COM6_PC12;
typedef AnyAFR<Port::PA, 1, AF::k11>	AfLCD_SEG0_PA1;
typedef AnyAFR<Port::PA, 2, AF::k11>	AfLCD_SEG1_PA2;
typedef AnyAFR<Port::PB, 10, AF::k11>	AfLCD_SEG10_PB10;
typedef AnyAFR<Port::PB, 11, AF::k11>	AfLCD_SEG11_PB11;
typedef AnyAFR<Port::PB, 12, AF::k11>	AfLCD_SEG12_PB12;
typedef AnyAFR<Port::PB, 13, AF::k11>	AfLCD_SEG13_PB13;
typedef AnyAFR<Port::PB, 14, AF::k11>	AfLCD_SEG14_PB14;
typedef AnyAFR<Port::PB, 15, AF::k11>	AfLCD_SEG15_PB15;
typedef AnyAFR<Port::PB, 8, AF::k11>	AfLCD_SEG16_PB8;
typedef AnyAFR<Port::PA, 15, AF::k11>	AfLCD_SEG17_PA15;
typedef AnyAFR<Port::PC, 0, AF::k11>	AfLCD_SEG18_PC0;
typedef AnyAFR<Port::PC, 1, AF::k11>	AfLCD_SEG19_PC1;
typedef AnyAFR<Port::PA, 3, AF::k11>	AfLCD_SEG2_PA3;
typedef AnyAFR<Port::PC, 2, AF::k11>	AfLCD_SEG20_PC2;
typedef AnyAFR<Port::PB, 7, AF::k11>	AfLCD_SEG21_PB7;
typedef AnyAFR<Port::PC, 4, AF::k11>	AfLCD_SEG22_PC4;
typedef AnyAFR<Port::PC, 5, AF::k11>	AfLCD_SEG23_PC5;
typedef AnyAFR<Port::PC, 6, AF::k11>	AfLCD_SEG24_PC6;
typedef AnyAFR<Port::PC, 7, AF::k11>	AfLCD_SEG25_PC7;
typedef AnyAFR<Port::PC, 8, AF::k11>	AfLCD_SEG26_PC8;
typedef AnyAFR<Port::PC, 9, AF::k11>	AfLCD_SEG27_PC9;
typedef AnyAFR<Port::PC, 10, AF::k11>	AfLCD_SEG28_PC10;
typedef AnyAFR<Port::PC, 11, AF::k11>	AfLCD_SEG29_PC11;
typedef AnyAFR<Port::PA, 6, AF::k11>	AfLCD_SEG3_PA6;
typedef AnyAFR<Port::PC, 12, AF::k11>	AfLCD_SEG30_PC12;
typedef AnyAFR<Port::PA, 7, AF::k11>	AfLCD_SEG4_PA7;
typedef AnyAFR<Port::PC, 10, AF::k11>	AfLCD_SEG40_PC10;
typedef AnyAFR<Port::PC, 11, AF::k11>	AfLCD_SEG41_PC11;
typedef AnyAFR<Port::PC, 12, AF::k11>	AfLCD_SEG42_PC12;
typedef AnyAFR<Port::PB, 0, AF::k11>	AfLCD_SEG5_PB0;
typedef AnyAFR<Port::PB, 1, AF::k11>	AfLCD_SEG6_PB1;
typedef AnyAFR<Port::PB, 3, AF::k11>	AfLCD_SEG7_PB3;
typedef AnyAFR<Port::PB, 4, AF::k11>	AfLCD_SEG8_PB4;
typedef AnyAFR<Port::PB, 5, AF::k11>	AfLCD_SEG9_PB5;
typedef AnyAFR<Port::PB, 2, AF::k11>	AfLCD_VLCD_PB2;
typedef AnyAFR<Port::PC, 3, AF::k11>	AfLCD_VLCD_PC3;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k11>	AfLCD_COM7_PD2;
typedef AnyAFR<Port::PD, 8, AF::k11>	AfLCD_SEG28_PD8;
typedef AnyAFR<Port::PD, 9, AF::k11>	AfLCD_SEG29_PD9;
typedef AnyAFR<Port::PD, 10, AF::k11>	AfLCD_SEG30_PD10;
typedef AnyAFR<Port::PD, 2, AF::k11>	AfLCD_SEG31_PD2;
typedef AnyAFR<Port::PD, 11, AF::k11>	AfLCD_SEG31_PD11;
typedef AnyAFR<Port::PD, 12, AF::k11>	AfLCD_SEG32_PD12;
typedef AnyAFR<Port::PD, 13, AF::k11>	AfLCD_SEG33_PD13;
typedef AnyAFR<Port::PD, 14, AF::k11>	AfLCD_SEG34_PD14;
typedef AnyAFR<Port::PD, 15, AF::k11>	AfLCD_SEG35_PD15;
typedef AnyAFR<Port::PD, 2, AF::k11>	AfLCD_SEG43_PD2;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k11>	AfLCD_SEG36_PE0;
typedef AnyAFR<Port::PE, 1, AF::k11>	AfLCD_SEG37_PE1;
typedef AnyAFR<Port::PE, 2, AF::k11>	AfLCD_SEG38_PE2;
typedef AnyAFR<Port::PE, 3, AF::k11>	AfLCD_SEG39_PE3;
#endif	// defined(GPIOE_BASE)
#endif	// defined(LCD_BASE)

// LPTIM1
#if defined(LPTIM1_BASE)
typedef AnyAFR<Port::PB, 6, AF::k1>		AfLPTIM1_ETR_PB6;
typedef AnyAFR<Port::PC, 3, AF::k1>		AfLPTIM1_ETR_PC3;
typedef AnyAFR<Port::PB, 5, AF::k1>		AfLPTIM1_IN1_PB5;
typedef AnyAFR<Port::PC, 0, AF::k1>		AfLPTIM1_IN1_PC0;
typedef AnyAFR<Port::PB, 7, AF::k1>		AfLPTIM1_IN2_PB7;
typedef AnyAFR<Port::PC, 2, AF::k1>		AfLPTIM1_IN2_PC2;
typedef AnyAFR<Port::PA, 14, AF::k1>	AfLPTIM1_OUT_PA14;
typedef AnyAFR<Port::PB, 2, AF::k1>		AfLPTIM1_OUT_PB2;
typedef AnyAFR<Port::PC, 1, AF::k1>		AfLPTIM1_OUT_PC1;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 12, AF::k1>	AfLPTIM1_ETR_PG12;
typedef AnyAFR<Port::PG, 10, AF::k1>	AfLPTIM1_IN1_PG10;
typedef AnyAFR<Port::PG, 11, AF::k1>	AfLPTIM1_IN2_PG11;
typedef AnyAFR<Port::PG, 15, AF::k1>	AfLPTIM1_OUT_PG15;
#endif	// defined(GPIOG_BASE)
#endif	// defined(LPTIM1_BASE)

// LPTIM2
#if defined(LPTIM2_BASE)
typedef AnyAFR<Port::PA, 5, AF::k14>	AfLPTIM2_ETR_PA5;
typedef AnyAFR<Port::PC, 3, AF::k14>	AfLPTIM2_ETR_PC3;
typedef AnyAFR<Port::PB, 1, AF::k14>	AfLPTIM2_IN1_PB1;
typedef AnyAFR<Port::PC, 0, AF::k14>	AfLPTIM2_IN1_PC0;
typedef AnyAFR<Port::PA, 4, AF::k14>	AfLPTIM2_OUT_PA4;
typedef AnyAFR<Port::PA, 8, AF::k14>	AfLPTIM2_OUT_PA8;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 11, AF::k14>	AfLPTIM2_ETR_PD11;
typedef AnyAFR<Port::PD, 12, AF::k14>	AfLPTIM2_IN1_PD12;
typedef AnyAFR<Port::PD, 13, AF::k14>	AfLPTIM2_OUT_PD13;
#endif	// defined(GPIOD_BASE)
#endif	// defined(LPTIM2_BASE)

// LPUART1
#if defined(LPUART1_BASE)
typedef AnyAFR<Port::PA, 6, AF::k8>		AfLPUART1_CTS_PA6;
typedef AnyAFR<Port::PB, 13, AF::k8>	AfLPUART1_CTS_PB13;
typedef AnyAFR<Port::PB, 1, AF::k8>		AfLPUART1_DE_PB1;
typedef AnyAFR<Port::PB, 12, AF::k8>	AfLPUART1_DE_PB12;
typedef AnyAFR<Port::PB, 1, AF::k8>		AfLPUART1_RTS_PB1;
typedef AnyAFR<Port::PB, 12, AF::k8>	AfLPUART1_RTS_PB12;
typedef AnyAFR<Port::PA, 3, AF::k8>		AfLPUART1_RX_PA3;
typedef AnyAFR<Port::PB, 10, AF::k8>	AfLPUART1_RX_PB10;
typedef AnyAFR<Port::PC, 0, AF::k8>		AfLPUART1_RX_PC0;
typedef AnyAFR<Port::PA, 2, AF::k8>		AfLPUART1_TX_PA2;
typedef AnyAFR<Port::PB, 11, AF::k8>	AfLPUART1_TX_PB11;
typedef AnyAFR<Port::PC, 1, AF::k8>		AfLPUART1_TX_PC1;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 5, AF::k8>		AfLPUART1_CTS_PG5;
typedef AnyAFR<Port::PG, 6, AF::k8>		AfLPUART1_DE_PG6;
typedef AnyAFR<Port::PG, 6, AF::k8>		AfLPUART1_RTS_PG6;
typedef AnyAFR<Port::PG, 8, AF::k8>		AfLPUART1_RX_PG8;
typedef AnyAFR<Port::PG, 7, AF::k8>		AfLPUART1_TX_PG7;
#endif	// defined(GPIOG_BASE)
#endif	// defined(LPUART1_BASE)

// OTG
#if defined(OTG_BASE)
typedef AnyAFR<Port::PB, 3, AF::k10>	AfOTG_FS_CRS_SYNC_PB3;
typedef AnyAFR<Port::PA, 11, AF::k10>	AfOTG_FS_DM_PA11;
typedef AnyAFR<Port::PA, 12, AF::k10>	AfOTG_FS_DP_PA12;
typedef AnyAFR<Port::PA, 10, AF::k10>	AfOTG_FS_ID_PA10;
typedef AnyAFR<Port::PA, 13, AF::k10>	AfOTG_FS_NOE_PA13;
typedef AnyAFR<Port::PC, 9, AF::k10>	AfOTG_FS_NOE_PC9;
typedef AnyAFR<Port::PA, 8, AF::k10>	AfOTG_FS_SOF_PA8;
typedef AnyAFR<Port::PA, 14, AF::k10>	AfOTG_FS_SOF_PA14;
#endif	// defined(OTG_BASE)

// QUADSPI
#if defined(QUADSPI)
typedef AnyAFR<Port::PB, 1, AF::k10>	AfQUADSPI_BK1_IO0_PB1;
typedef AnyAFR<Port::PB, 0, AF::k10>	AfQUADSPI_BK1_IO1_PB0;
typedef AnyAFR<Port::PA, 7, AF::k10>	AfQUADSPI_BK1_IO2_PA7;
typedef AnyAFR<Port::PA, 6, AF::k10>	AfQUADSPI_BK1_IO3_PA6;
typedef AnyAFR<Port::PA, 2, AF::k10>	AfQUADSPI_BK1_NCS_PA2;
typedef AnyAFR<Port::PB, 11, AF::k10>	AfQUADSPI_BK1_NCS_PB11;
typedef AnyAFR<Port::PC, 1, AF::k10>	AfQUADSPI_BK2_IO0_PC1;
typedef AnyAFR<Port::PC, 2, AF::k10>	AfQUADSPI_BK2_IO1_PC2;
typedef AnyAFR<Port::PC, 3, AF::k10>	AfQUADSPI_BK2_IO2_PC3;
typedef AnyAFR<Port::PC, 4, AF::k10>	AfQUADSPI_BK2_IO3_PC4;
typedef AnyAFR<Port::PC, 11, AF::k5>	AfQUADSPI_BK2_NCS_PC11;
typedef AnyAFR<Port::PA, 3, AF::k10>	AfQUADSPI_CLK_PA3;
typedef AnyAFR<Port::PB, 10, AF::k10>	AfQUADSPI_CLK_PB10;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 4, AF::k10>	AfQUADSPI_BK2_IO0_PD4;
typedef AnyAFR<Port::PD, 5, AF::k10>	AfQUADSPI_BK2_IO1_PD5;
typedef AnyAFR<Port::PD, 6, AF::k5>		AfQUADSPI_BK2_IO1_PD6;
typedef AnyAFR<Port::PD, 6, AF::k10>	AfQUADSPI_BK2_IO2_PD6;
typedef AnyAFR<Port::PD, 7, AF::k10>	AfQUADSPI_BK2_IO3_PD7;
typedef AnyAFR<Port::PD, 3, AF::k10>	AfQUADSPI_BK2_NCS_PD3;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 12, AF::k10>	AfQUADSPI_BK1_IO0_PE12;
typedef AnyAFR<Port::PE, 13, AF::k10>	AfQUADSPI_BK1_IO1_PE13;
typedef AnyAFR<Port::PE, 14, AF::k10>	AfQUADSPI_BK1_IO2_PE14;
typedef AnyAFR<Port::PE, 15, AF::k10>	AfQUADSPI_BK1_IO3_PE15;
typedef AnyAFR<Port::PE, 11, AF::k10>	AfQUADSPI_BK1_NCS_PE11;
typedef AnyAFR<Port::PE, 10, AF::k10>	AfQUADSPI_CLK_PE10;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 8, AF::k10>	AfQUADSPI_BK1_IO0_PF8;
typedef AnyAFR<Port::PF, 9, AF::k10>	AfQUADSPI_BK1_IO1_PF9;
typedef AnyAFR<Port::PF, 7, AF::k10>	AfQUADSPI_BK1_IO2_PF7;
typedef AnyAFR<Port::PF, 6, AF::k10>	AfQUADSPI_BK1_IO3_PF6;
typedef AnyAFR<Port::PF, 10, AF::k3>	AfQUADSPI_CLK_PF10;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 2, AF::k3>		AfQUADSPI_BK2_IO0_PH2;
#endif	// defined(GPIOH_BASE)
#endif	// defined(QUADSPI)

// RTC
#if defined(RTC_BASE)
typedef AnyAFR<Port::PB, 2, AF::k0>		AfRTC_OUT_PB2;
typedef AnyAFR<Port::PB, 15, AF::k0>	AfRTC_REFIN_PB15;
#endif	// defined(RTC_BASE)

// SAI1
#if defined(SAI1_BASE)
typedef AnyAFR<Port::PA, 0, AF::k13>	AfSAI1_EXTCLK_PA0;
typedef AnyAFR<Port::PB, 0, AF::k13>	AfSAI1_EXTCLK_PB0;
typedef AnyAFR<Port::PA, 9, AF::k13>	AfSAI1_FS_A_PA9;
typedef AnyAFR<Port::PB, 9, AF::k13>	AfSAI1_FS_A_PB9;
typedef AnyAFR<Port::PA, 4, AF::k13>	AfSAI1_FS_B_PA4;
typedef AnyAFR<Port::PA, 14, AF::k13>	AfSAI1_FS_B_PA14;
typedef AnyAFR<Port::PB, 6, AF::k13>	AfSAI1_FS_B_PB6;
typedef AnyAFR<Port::PA, 3, AF::k13>	AfSAI1_MCLK_A_PA3;
typedef AnyAFR<Port::PB, 8, AF::k13>	AfSAI1_MCLK_A_PB8;
typedef AnyAFR<Port::PB, 4, AF::k13>	AfSAI1_MCLK_B_PB4;
typedef AnyAFR<Port::PA, 8, AF::k13>	AfSAI1_SCK_A_PA8;
typedef AnyAFR<Port::PB, 10, AF::k13>	AfSAI1_SCK_A_PB10;
typedef AnyAFR<Port::PB, 3, AF::k13>	AfSAI1_SCK_B_PB3;
typedef AnyAFR<Port::PA, 10, AF::k13>	AfSAI1_SD_A_PA10;
typedef AnyAFR<Port::PC, 1, AF::k13>	AfSAI1_SD_A_PC1;
typedef AnyAFR<Port::PC, 3, AF::k13>	AfSAI1_SD_A_PC3;
typedef AnyAFR<Port::PA, 13, AF::k13>	AfSAI1_SD_B_PA13;
typedef AnyAFR<Port::PB, 5, AF::k13>	AfSAI1_SD_B_PB5;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 6, AF::k13>	AfSAI1_SD_A_PD6;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
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
typedef AnyAFR<Port::PF, 9, AF::k13>	AfSAI1_FS_B_PF9;
typedef AnyAFR<Port::PF, 7, AF::k13>	AfSAI1_MCLK_B_PF7;
typedef AnyAFR<Port::PF, 8, AF::k13>	AfSAI1_SCK_B_PF8;
typedef AnyAFR<Port::PF, 6, AF::k13>	AfSAI1_SD_B_PF6;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 7, AF::k13>	AfSAI1_MCLK_A_PG7;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SAI1_BASE)

// SAI2
#if defined(SAI2_BASE)
typedef AnyAFR<Port::PA, 2, AF::k13>	AfSAI2_EXTCLK_PA2;
typedef AnyAFR<Port::PC, 9, AF::k13>	AfSAI2_EXTCLK_PC9;
typedef AnyAFR<Port::PB, 12, AF::k13>	AfSAI2_FS_A_PB12;
typedef AnyAFR<Port::PA, 15, AF::k13>	AfSAI2_FS_B_PA15;
typedef AnyAFR<Port::PB, 14, AF::k13>	AfSAI2_MCLK_A_PB14;
typedef AnyAFR<Port::PC, 6, AF::k13>	AfSAI2_MCLK_A_PC6;
typedef AnyAFR<Port::PC, 7, AF::k13>	AfSAI2_MCLK_B_PC7;
typedef AnyAFR<Port::PC, 11, AF::k13>	AfSAI2_MCLK_B_PC11;
typedef AnyAFR<Port::PB, 13, AF::k13>	AfSAI2_SCK_A_PB13;
typedef AnyAFR<Port::PC, 10, AF::k13>	AfSAI2_SCK_B_PC10;
typedef AnyAFR<Port::PB, 15, AF::k13>	AfSAI2_SD_A_PB15;
typedef AnyAFR<Port::PC, 12, AF::k13>	AfSAI2_SD_B_PC12;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 12, AF::k13>	AfSAI2_FS_A_PD12;
typedef AnyAFR<Port::PD, 9, AF::k13>	AfSAI2_MCLK_A_PD9;
typedef AnyAFR<Port::PD, 10, AF::k13>	AfSAI2_SCK_A_PD10;
typedef AnyAFR<Port::PD, 11, AF::k13>	AfSAI2_SD_A_PD11;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 10, AF::k13>	AfSAI2_FS_A_PG10;
typedef AnyAFR<Port::PG, 3, AF::k13>	AfSAI2_FS_B_PG3;
typedef AnyAFR<Port::PG, 11, AF::k13>	AfSAI2_MCLK_A_PG11;
typedef AnyAFR<Port::PG, 4, AF::k13>	AfSAI2_MCLK_B_PG4;
typedef AnyAFR<Port::PG, 9, AF::k13>	AfSAI2_SCK_A_PG9;
typedef AnyAFR<Port::PG, 2, AF::k13>	AfSAI2_SCK_B_PG2;
typedef AnyAFR<Port::PG, 12, AF::k13>	AfSAI2_SD_A_PG12;
typedef AnyAFR<Port::PG, 5, AF::k13>	AfSAI2_SD_B_PG5;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SAI2_BASE)

// SDMMC1
#if defined(SDMMC1_BASE)
typedef AnyAFR<Port::PC, 12, AF::k12>	AfSDMMC1_CK_PC12;
typedef AnyAFR<Port::PC, 8, AF::k12>	AfSDMMC1_D0_PC8;
typedef AnyAFR<Port::PC, 9, AF::k12>	AfSDMMC1_D1_PC9;
typedef AnyAFR<Port::PC, 10, AF::k12>	AfSDMMC1_D2_PC10;
typedef AnyAFR<Port::PC, 11, AF::k12>	AfSDMMC1_D3_PC11;
typedef AnyAFR<Port::PB, 8, AF::k12>	AfSDMMC1_D4_PB8;
typedef AnyAFR<Port::PB, 9, AF::k12>	AfSDMMC1_D5_PB9;
typedef AnyAFR<Port::PC, 6, AF::k12>	AfSDMMC1_D6_PC6;
typedef AnyAFR<Port::PC, 7, AF::k12>	AfSDMMC1_D7_PC7;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k12>	AfSDMMC1_CMD_PD2;
#endif	// defined(GPIOD_BASE)
#endif	// defined(SDMMC1_BASE)

// SPI1
#if defined(SPI1_BASE)
typedef AnyAFR<Port::PA, 6, AF::k5>		AfSPI1_MISO_PA6;
typedef AnyAFR<Port::PA, 11, AF::k5>	AfSPI1_MISO_PA11;
typedef AnyAFR<Port::PB, 4, AF::k5>		AfSPI1_MISO_PB4;
typedef AnyAFR<Port::PA, 7, AF::k5>		AfSPI1_MOSI_PA7;
typedef AnyAFR<Port::PA, 12, AF::k5>	AfSPI1_MOSI_PA12;
typedef AnyAFR<Port::PB, 5, AF::k5>		AfSPI1_MOSI_PB5;
typedef AnyAFR<Port::PA, 4, AF::k5>		AfSPI1_NSS_PA4;
typedef AnyAFR<Port::PA, 15, AF::k5>	AfSPI1_NSS_PA15;
typedef AnyAFR<Port::PB, 0, AF::k5>		AfSPI1_NSS_PB0;
typedef AnyAFR<Port::PA, 1, AF::k5>		AfSPI1_SCK_PA1;
typedef AnyAFR<Port::PA, 5, AF::k5>		AfSPI1_SCK_PA5;
typedef AnyAFR<Port::PB, 3, AF::k5>		AfSPI1_SCK_PB3;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 14, AF::k5>	AfSPI1_MISO_PE14;
typedef AnyAFR<Port::PE, 15, AF::k5>	AfSPI1_MOSI_PE15;
typedef AnyAFR<Port::PE, 12, AF::k5>	AfSPI1_NSS_PE12;
typedef AnyAFR<Port::PE, 13, AF::k5>	AfSPI1_SCK_PE13;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 3, AF::k5>		AfSPI1_MISO_PG3;
typedef AnyAFR<Port::PG, 4, AF::k5>		AfSPI1_MOSI_PG4;
typedef AnyAFR<Port::PG, 5, AF::k5>		AfSPI1_NSS_PG5;
typedef AnyAFR<Port::PG, 2, AF::k5>		AfSPI1_SCK_PG2;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SPI1_BASE)

// SPI2
#if defined(SPI2_BASE)
typedef AnyAFR<Port::PB, 14, AF::k5>	AfSPI2_MISO_PB14;
typedef AnyAFR<Port::PC, 2, AF::k5>		AfSPI2_MISO_PC2;
typedef AnyAFR<Port::PB, 15, AF::k5>	AfSPI2_MOSI_PB15;
typedef AnyAFR<Port::PC, 1, AF::k3>		AfSPI2_MOSI_PC1;
typedef AnyAFR<Port::PC, 3, AF::k5>		AfSPI2_MOSI_PC3;
typedef AnyAFR<Port::PB, 9, AF::k5>		AfSPI2_NSS_PB9;
typedef AnyAFR<Port::PB, 12, AF::k5>	AfSPI2_NSS_PB12;
typedef AnyAFR<Port::PA, 9, AF::k3>		AfSPI2_SCK_PA9;
typedef AnyAFR<Port::PB, 10, AF::k5>	AfSPI2_SCK_PB10;
typedef AnyAFR<Port::PB, 13, AF::k5>	AfSPI2_SCK_PB13;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 3, AF::k5>		AfSPI2_MISO_PD3;
typedef AnyAFR<Port::PD, 4, AF::k5>		AfSPI2_MOSI_PD4;
typedef AnyAFR<Port::PD, 0, AF::k5>		AfSPI2_NSS_PD0;
typedef AnyAFR<Port::PD, 1, AF::k5>		AfSPI2_SCK_PD1;
typedef AnyAFR<Port::PD, 3, AF::k3>		AfSPI2_SCK_PD3;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOI_BASE)
typedef AnyAFR<Port::PI, 2, AF::k5>		AfSPI2_MISO_PI2;
typedef AnyAFR<Port::PI, 3, AF::k5>		AfSPI2_MOSI_PI3;
typedef AnyAFR<Port::PI, 0, AF::k5>		AfSPI2_NSS_PI0;
typedef AnyAFR<Port::PI, 1, AF::k5>		AfSPI2_SCK_PI1;
#endif	// defined(GPIOI_BASE)
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
typedef AnyAFR<Port::PG, 10, AF::k6>	AfSPI3_MISO_PG10;
typedef AnyAFR<Port::PG, 11, AF::k6>	AfSPI3_MOSI_PG11;
typedef AnyAFR<Port::PG, 12, AF::k6>	AfSPI3_NSS_PG12;
typedef AnyAFR<Port::PG, 9, AF::k6>		AfSPI3_SCK_PG9;
#endif	// defined(GPIOG_BASE)
#endif	// defined(SPI3_BASE)

// SWPMI1
#if defined(SWPMI1_BASE)
typedef AnyAFR<Port::PA, 8, AF::k12>	AfSWPMI1_IO_PA8;
typedef AnyAFR<Port::PB, 12, AF::k12>	AfSWPMI1_IO_PB12;
typedef AnyAFR<Port::PA, 14, AF::k12>	AfSWPMI1_RX_PA14;
typedef AnyAFR<Port::PB, 14, AF::k12>	AfSWPMI1_RX_PB14;
typedef AnyAFR<Port::PA, 15, AF::k12>	AfSWPMI1_SUSPEND_PA15;
typedef AnyAFR<Port::PB, 15, AF::k12>	AfSWPMI1_SUSPEND_PB15;
typedef AnyAFR<Port::PA, 13, AF::k12>	AfSWPMI1_TX_PA13;
typedef AnyAFR<Port::PB, 13, AF::k12>	AfSWPMI1_TX_PB13;
#endif	// defined(SWPMI1_BASE)

// TIM1
#if defined(TIM1_BASE)
typedef AnyAFR<Port::PA, 6, AF::k1>		AfTIM1_BKIN_PA6;
typedef AnyAFR<Port::PB, 12, AF::k1>	AfTIM1_BKIN_PB12;
typedef AnyAFR<Port::PA, 11, AF::k2>	AfTIM1_BKIN2_PA11;
typedef AnyAFR<Port::PA, 11, AF::k12>	AfTIM1_BKIN2_COMP1_PA11;
typedef AnyAFR<Port::PA, 6, AF::k12>	AfTIM1_BKIN_COMP2_PA6;
typedef AnyAFR<Port::PB, 12, AF::k3>	AfTIM1_BKIN_COMP2_PB12;
typedef AnyAFR<Port::PA, 8, AF::k1>		AfTIM1_CH1_PA8;
typedef AnyAFR<Port::PA, 7, AF::k1>		AfTIM1_CH1N_PA7;
typedef AnyAFR<Port::PB, 13, AF::k1>	AfTIM1_CH1N_PB13;
typedef AnyAFR<Port::PA, 9, AF::k1>		AfTIM1_CH2_PA9;
typedef AnyAFR<Port::PB, 0, AF::k1>		AfTIM1_CH2N_PB0;
typedef AnyAFR<Port::PB, 14, AF::k1>	AfTIM1_CH2N_PB14;
typedef AnyAFR<Port::PA, 10, AF::k1>	AfTIM1_CH3_PA10;
typedef AnyAFR<Port::PB, 1, AF::k1>		AfTIM1_CH3N_PB1;
typedef AnyAFR<Port::PB, 15, AF::k1>	AfTIM1_CH3N_PB15;
typedef AnyAFR<Port::PA, 11, AF::k1>	AfTIM1_CH4_PA11;
typedef AnyAFR<Port::PA, 12, AF::k1>	AfTIM1_ETR_PA12;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 15, AF::k1>	AfTIM1_BKIN_PE15;
typedef AnyAFR<Port::PE, 14, AF::k2>	AfTIM1_BKIN2_PE14;
typedef AnyAFR<Port::PE, 14, AF::k3>	AfTIM1_BKIN2_COMP2_PE14;
typedef AnyAFR<Port::PE, 15, AF::k3>	AfTIM1_BKIN_COMP1_PE15;
typedef AnyAFR<Port::PE, 9, AF::k1>		AfTIM1_CH1_PE9;
typedef AnyAFR<Port::PE, 8, AF::k1>		AfTIM1_CH1N_PE8;
typedef AnyAFR<Port::PE, 11, AF::k1>	AfTIM1_CH2_PE11;
typedef AnyAFR<Port::PE, 10, AF::k1>	AfTIM1_CH2N_PE10;
typedef AnyAFR<Port::PE, 13, AF::k1>	AfTIM1_CH3_PE13;
typedef AnyAFR<Port::PE, 12, AF::k1>	AfTIM1_CH3N_PE12;
typedef AnyAFR<Port::PE, 14, AF::k1>	AfTIM1_CH4_PE14;
typedef AnyAFR<Port::PE, 7, AF::k1>		AfTIM1_ETR_PE7;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM1_BASE)

// TIM2
#if defined(TIM2_BASE)
typedef AnyAFR<Port::PA, 0, AF::k1>		AfTIM2_CH1_PA0;
typedef AnyAFR<Port::PA, 5, AF::k1>		AfTIM2_CH1_PA5;
typedef AnyAFR<Port::PA, 15, AF::k1>	AfTIM2_CH1_PA15;
typedef AnyAFR<Port::PA, 1, AF::k1>		AfTIM2_CH2_PA1;
typedef AnyAFR<Port::PB, 3, AF::k1>		AfTIM2_CH2_PB3;
typedef AnyAFR<Port::PA, 2, AF::k1>		AfTIM2_CH3_PA2;
typedef AnyAFR<Port::PB, 10, AF::k1>	AfTIM2_CH3_PB10;
typedef AnyAFR<Port::PA, 3, AF::k1>		AfTIM2_CH4_PA3;
typedef AnyAFR<Port::PB, 11, AF::k1>	AfTIM2_CH4_PB11;
typedef AnyAFR<Port::PA, 0, AF::k14>	AfTIM2_ETR_PA0;
typedef AnyAFR<Port::PA, 5, AF::k2>		AfTIM2_ETR_PA5;
typedef AnyAFR<Port::PA, 15, AF::k2>	AfTIM2_ETR_PA15;
#endif	// defined(TIM2_BASE)

// TIM3
#if defined(TIM3_BASE)
typedef AnyAFR<Port::PA, 6, AF::k2>		AfTIM3_CH1_PA6;
typedef AnyAFR<Port::PB, 4, AF::k2>		AfTIM3_CH1_PB4;
typedef AnyAFR<Port::PC, 6, AF::k2>		AfTIM3_CH1_PC6;
typedef AnyAFR<Port::PA, 7, AF::k2>		AfTIM3_CH2_PA7;
typedef AnyAFR<Port::PB, 5, AF::k2>		AfTIM3_CH2_PB5;
typedef AnyAFR<Port::PC, 7, AF::k2>		AfTIM3_CH2_PC7;
typedef AnyAFR<Port::PB, 0, AF::k2>		AfTIM3_CH3_PB0;
typedef AnyAFR<Port::PC, 8, AF::k2>		AfTIM3_CH3_PC8;
typedef AnyAFR<Port::PB, 1, AF::k2>		AfTIM3_CH4_PB1;
typedef AnyAFR<Port::PC, 9, AF::k2>		AfTIM3_CH4_PC9;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k2>		AfTIM3_ETR_PD2;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 3, AF::k2>		AfTIM3_CH1_PE3;
typedef AnyAFR<Port::PE, 4, AF::k2>		AfTIM3_CH2_PE4;
typedef AnyAFR<Port::PE, 5, AF::k2>		AfTIM3_CH3_PE5;
typedef AnyAFR<Port::PE, 6, AF::k2>		AfTIM3_CH4_PE6;
typedef AnyAFR<Port::PE, 2, AF::k2>		AfTIM3_ETR_PE2;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM3_BASE)

// TIM4
#if defined(TIM4_BASE)
typedef AnyAFR<Port::PB, 6, AF::k2>		AfTIM4_CH1_PB6;
typedef AnyAFR<Port::PB, 7, AF::k2>		AfTIM4_CH2_PB7;
typedef AnyAFR<Port::PB, 8, AF::k2>		AfTIM4_CH3_PB8;
typedef AnyAFR<Port::PB, 9, AF::k2>		AfTIM4_CH4_PB9;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 12, AF::k2>	AfTIM4_CH1_PD12;
typedef AnyAFR<Port::PD, 13, AF::k2>	AfTIM4_CH2_PD13;
typedef AnyAFR<Port::PD, 14, AF::k2>	AfTIM4_CH3_PD14;
typedef AnyAFR<Port::PD, 15, AF::k2>	AfTIM4_CH4_PD15;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k2>		AfTIM4_ETR_PE0;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM4_BASE)

// TIM5
#if defined(TIM5_BASE)
typedef AnyAFR<Port::PA, 0, AF::k2>		AfTIM5_CH1_PA0;
typedef AnyAFR<Port::PA, 1, AF::k2>		AfTIM5_CH2_PA1;
typedef AnyAFR<Port::PA, 2, AF::k2>		AfTIM5_CH3_PA2;
typedef AnyAFR<Port::PA, 3, AF::k2>		AfTIM5_CH4_PA3;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 6, AF::k2>		AfTIM5_CH1_PF6;
typedef AnyAFR<Port::PF, 7, AF::k2>		AfTIM5_CH2_PF7;
typedef AnyAFR<Port::PF, 8, AF::k2>		AfTIM5_CH3_PF8;
typedef AnyAFR<Port::PF, 9, AF::k2>		AfTIM5_CH4_PF9;
typedef AnyAFR<Port::PF, 6, AF::k1>		AfTIM5_ETR_PF6;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 10, AF::k2>	AfTIM5_CH1_PH10;
typedef AnyAFR<Port::PH, 11, AF::k2>	AfTIM5_CH2_PH11;
typedef AnyAFR<Port::PH, 12, AF::k2>	AfTIM5_CH3_PH12;
#endif	// defined(GPIOH_BASE)
#if defined(GPIOI_BASE)
typedef AnyAFR<Port::PI, 0, AF::k2>		AfTIM5_CH4_PI0;
#endif	// defined(GPIOI_BASE)
#endif	// defined(TIM5_BASE)

// TIM8
#if defined(TIM8_BASE)
typedef AnyAFR<Port::PA, 6, AF::k3>		AfTIM8_BKIN_PA6;
typedef AnyAFR<Port::PB, 7, AF::k3>		AfTIM8_BKIN_PB7;
typedef AnyAFR<Port::PB, 6, AF::k3>		AfTIM8_BKIN2_PB6;
typedef AnyAFR<Port::PC, 9, AF::k1>		AfTIM8_BKIN2_PC9;
typedef AnyAFR<Port::PC, 9, AF::k14>	AfTIM8_BKIN2_COMP1_PC9;
typedef AnyAFR<Port::PB, 6, AF::k12>	AfTIM8_BKIN2_COMP2_PB6;
typedef AnyAFR<Port::PB, 7, AF::k13>	AfTIM8_BKIN_COMP1_PB7;
typedef AnyAFR<Port::PA, 6, AF::k13>	AfTIM8_BKIN_COMP2_PA6;
typedef AnyAFR<Port::PC, 6, AF::k3>		AfTIM8_CH1_PC6;
typedef AnyAFR<Port::PA, 5, AF::k3>		AfTIM8_CH1N_PA5;
typedef AnyAFR<Port::PA, 7, AF::k3>		AfTIM8_CH1N_PA7;
typedef AnyAFR<Port::PC, 7, AF::k3>		AfTIM8_CH2_PC7;
typedef AnyAFR<Port::PB, 0, AF::k3>		AfTIM8_CH2N_PB0;
typedef AnyAFR<Port::PB, 14, AF::k3>	AfTIM8_CH2N_PB14;
typedef AnyAFR<Port::PC, 8, AF::k3>		AfTIM8_CH3_PC8;
typedef AnyAFR<Port::PB, 1, AF::k3>		AfTIM8_CH3N_PB1;
typedef AnyAFR<Port::PB, 15, AF::k3>	AfTIM8_CH3N_PB15;
typedef AnyAFR<Port::PC, 9, AF::k3>		AfTIM8_CH4_PC9;
typedef AnyAFR<Port::PA, 0, AF::k3>		AfTIM8_ETR_PA0;
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 13, AF::k3>	AfTIM8_CH1N_PH13;
typedef AnyAFR<Port::PH, 14, AF::k3>	AfTIM8_CH2N_PH14;
typedef AnyAFR<Port::PH, 15, AF::k3>	AfTIM8_CH3N_PH15;
#endif	// defined(GPIOH_BASE)
#if defined(GPIOI_BASE)
typedef AnyAFR<Port::PI, 4, AF::k3>		AfTIM8_BKIN_PI4;
typedef AnyAFR<Port::PI, 5, AF::k3>		AfTIM8_CH1_PI5;
typedef AnyAFR<Port::PI, 6, AF::k3>		AfTIM8_CH2_PI6;
typedef AnyAFR<Port::PI, 7, AF::k3>		AfTIM8_CH3_PI7;
typedef AnyAFR<Port::PI, 2, AF::k3>		AfTIM8_CH4_PI2;
typedef AnyAFR<Port::PI, 3, AF::k3>		AfTIM8_ETR_PI3;
#endif	// defined(GPIOI_BASE)
#endif	// defined(TIM8_BASE)

// TIM15
#if defined(TIM15_BASE)
typedef AnyAFR<Port::PA, 9, AF::k14>	AfTIM15_BKIN_PA9;
typedef AnyAFR<Port::PB, 12, AF::k14>	AfTIM15_BKIN_PB12;
typedef AnyAFR<Port::PA, 2, AF::k14>	AfTIM15_CH1_PA2;
typedef AnyAFR<Port::PB, 14, AF::k14>	AfTIM15_CH1_PB14;
typedef AnyAFR<Port::PA, 1, AF::k14>	AfTIM15_CH1N_PA1;
typedef AnyAFR<Port::PB, 13, AF::k14>	AfTIM15_CH1N_PB13;
typedef AnyAFR<Port::PA, 3, AF::k14>	AfTIM15_CH2_PA3;
typedef AnyAFR<Port::PB, 15, AF::k14>	AfTIM15_CH2_PB15;
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 9, AF::k14>	AfTIM15_CH1_PF9;
typedef AnyAFR<Port::PF, 10, AF::k14>	AfTIM15_CH2_PF10;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 10, AF::k14>	AfTIM15_CH1_PG10;
typedef AnyAFR<Port::PG, 9, AF::k14>	AfTIM15_CH1N_PG9;
typedef AnyAFR<Port::PG, 11, AF::k14>	AfTIM15_CH2_PG11;
#endif	// defined(GPIOG_BASE)
#endif	// defined(TIM15_BASE)

// TIM16
#if defined(TIM16_BASE)
typedef AnyAFR<Port::PB, 5, AF::k14>	AfTIM16_BKIN_PB5;
typedef AnyAFR<Port::PA, 6, AF::k14>	AfTIM16_CH1_PA6;
typedef AnyAFR<Port::PB, 8, AF::k14>	AfTIM16_CH1_PB8;
typedef AnyAFR<Port::PB, 6, AF::k14>	AfTIM16_CH1N_PB6;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 0, AF::k14>	AfTIM16_CH1_PE0;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM16_BASE)

// TIM17
#if defined(TIM17_BASE)
typedef AnyAFR<Port::PA, 10, AF::k14>	AfTIM17_BKIN_PA10;
typedef AnyAFR<Port::PB, 4, AF::k14>	AfTIM17_BKIN_PB4;
typedef AnyAFR<Port::PA, 7, AF::k14>	AfTIM17_CH1_PA7;
typedef AnyAFR<Port::PB, 9, AF::k14>	AfTIM17_CH1_PB9;
typedef AnyAFR<Port::PB, 7, AF::k14>	AfTIM17_CH1N_PB7;
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 1, AF::k14>	AfTIM17_CH1_PE1;
#endif	// defined(GPIOE_BASE)
#endif	// defined(TIM17_BASE)

// TSC
#if defined(TSC_BASE)
typedef AnyAFR<Port::PB, 12, AF::k9>	AfTSC_G1_IO1_PB12;
typedef AnyAFR<Port::PB, 13, AF::k9>	AfTSC_G1_IO2_PB13;
typedef AnyAFR<Port::PB, 14, AF::k9>	AfTSC_G1_IO3_PB14;
typedef AnyAFR<Port::PB, 15, AF::k9>	AfTSC_G1_IO4_PB15;
typedef AnyAFR<Port::PB, 4, AF::k9>		AfTSC_G2_IO1_PB4;
typedef AnyAFR<Port::PB, 5, AF::k9>		AfTSC_G2_IO2_PB5;
typedef AnyAFR<Port::PB, 6, AF::k9>		AfTSC_G2_IO3_PB6;
typedef AnyAFR<Port::PB, 7, AF::k9>		AfTSC_G2_IO4_PB7;
typedef AnyAFR<Port::PA, 15, AF::k9>	AfTSC_G3_IO1_PA15;
typedef AnyAFR<Port::PC, 10, AF::k9>	AfTSC_G3_IO2_PC10;
typedef AnyAFR<Port::PC, 11, AF::k9>	AfTSC_G3_IO3_PC11;
typedef AnyAFR<Port::PC, 12, AF::k9>	AfTSC_G3_IO4_PC12;
typedef AnyAFR<Port::PC, 6, AF::k9>		AfTSC_G4_IO1_PC6;
typedef AnyAFR<Port::PC, 7, AF::k9>		AfTSC_G4_IO2_PC7;
typedef AnyAFR<Port::PC, 8, AF::k9>		AfTSC_G4_IO3_PC8;
typedef AnyAFR<Port::PC, 9, AF::k9>		AfTSC_G4_IO4_PC9;
typedef AnyAFR<Port::PB, 10, AF::k9>	AfTSC_SYNC_PB10;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 10, AF::k9>	AfTSC_G6_IO1_PD10;
typedef AnyAFR<Port::PD, 11, AF::k9>	AfTSC_G6_IO2_PD11;
typedef AnyAFR<Port::PD, 12, AF::k9>	AfTSC_G6_IO3_PD12;
typedef AnyAFR<Port::PD, 13, AF::k9>	AfTSC_G6_IO4_PD13;
typedef AnyAFR<Port::PD, 2, AF::k9>		AfTSC_SYNC_PD2;
#endif	// defined(GPIOD_BASE)
#if defined(GPIOE_BASE)
typedef AnyAFR<Port::PE, 10, AF::k9>	AfTSC_G5_IO1_PE10;
typedef AnyAFR<Port::PE, 11, AF::k9>	AfTSC_G5_IO2_PE11;
typedef AnyAFR<Port::PE, 12, AF::k9>	AfTSC_G5_IO3_PE12;
typedef AnyAFR<Port::PE, 13, AF::k9>	AfTSC_G5_IO4_PE13;
typedef AnyAFR<Port::PE, 2, AF::k9>		AfTSC_G7_IO1_PE2;
typedef AnyAFR<Port::PE, 3, AF::k9>		AfTSC_G7_IO2_PE3;
typedef AnyAFR<Port::PE, 4, AF::k9>		AfTSC_G7_IO3_PE4;
typedef AnyAFR<Port::PE, 5, AF::k9>		AfTSC_G7_IO4_PE5;
#endif	// defined(GPIOE_BASE)
#if defined(GPIOF_BASE)
typedef AnyAFR<Port::PF, 14, AF::k9>	AfTSC_G8_IO1_PF14;
typedef AnyAFR<Port::PF, 15, AF::k9>	AfTSC_G8_IO2_PF15;
#endif	// defined(GPIOF_BASE)
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 0, AF::k9>		AfTSC_G8_IO3_PG0;
typedef AnyAFR<Port::PG, 1, AF::k9>		AfTSC_G8_IO4_PG1;
#endif	// defined(GPIOG_BASE)
#endif	// defined(TSC_BASE)

// USART1
#if defined(USART1_BASE)
typedef AnyAFR<Port::PA, 8, AF::k7>		AfUSART1_CK_PA8;
typedef AnyAFR<Port::PB, 5, AF::k7>		AfUSART1_CK_PB5;
typedef AnyAFR<Port::PA, 11, AF::k7>	AfUSART1_CTS_PA11;
typedef AnyAFR<Port::PB, 4, AF::k7>		AfUSART1_CTS_PB4;
typedef AnyAFR<Port::PA, 12, AF::k7>	AfUSART1_DE_PA12;
typedef AnyAFR<Port::PB, 3, AF::k7>		AfUSART1_DE_PB3;
typedef AnyAFR<Port::PA, 12, AF::k7>	AfUSART1_RTS_PA12;
typedef AnyAFR<Port::PB, 3, AF::k7>		AfUSART1_RTS_PB3;
typedef AnyAFR<Port::PA, 10, AF::k7>	AfUSART1_RX_PA10;
typedef AnyAFR<Port::PB, 7, AF::k7>		AfUSART1_RX_PB7;
typedef AnyAFR<Port::PA, 9, AF::k7>		AfUSART1_TX_PA9;
typedef AnyAFR<Port::PB, 6, AF::k7>		AfUSART1_TX_PB6;
#if defined(GPIOG_BASE)
typedef AnyAFR<Port::PG, 13, AF::k7>	AfUSART1_CK_PG13;
typedef AnyAFR<Port::PG, 11, AF::k7>	AfUSART1_CTS_PG11;
typedef AnyAFR<Port::PG, 12, AF::k7>	AfUSART1_DE_PG12;
typedef AnyAFR<Port::PG, 12, AF::k7>	AfUSART1_RTS_PG12;
typedef AnyAFR<Port::PG, 10, AF::k7>	AfUSART1_RX_PG10;
typedef AnyAFR<Port::PG, 9, AF::k7>		AfUSART1_TX_PG9;
#endif	// defined(GPIOG_BASE)
#endif	// defined(USART1_BASE)

// USART2
#if defined(USART2_BASE)
typedef AnyAFR<Port::PA, 4, AF::k7>		AfUSART2_CK_PA4;
typedef AnyAFR<Port::PA, 0, AF::k7>		AfUSART2_CTS_PA0;
typedef AnyAFR<Port::PA, 1, AF::k7>		AfUSART2_DE_PA1;
typedef AnyAFR<Port::PA, 1, AF::k7>		AfUSART2_RTS_PA1;
typedef AnyAFR<Port::PA, 3, AF::k7>		AfUSART2_RX_PA3;
typedef AnyAFR<Port::PA, 15, AF::k3>	AfUSART2_RX_PA15;
typedef AnyAFR<Port::PA, 2, AF::k7>		AfUSART2_TX_PA2;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 7, AF::k7>		AfUSART2_CK_PD7;
typedef AnyAFR<Port::PD, 3, AF::k7>		AfUSART2_CTS_PD3;
typedef AnyAFR<Port::PD, 4, AF::k7>		AfUSART2_DE_PD4;
typedef AnyAFR<Port::PD, 4, AF::k7>		AfUSART2_RTS_PD4;
typedef AnyAFR<Port::PD, 6, AF::k7>		AfUSART2_RX_PD6;
typedef AnyAFR<Port::PD, 5, AF::k7>		AfUSART2_TX_PD5;
#endif	// defined(GPIOD_BASE)
#endif	// defined(USART2_BASE)

// USART3
#if defined(USART3_BASE)
typedef AnyAFR<Port::PB, 0, AF::k7>		AfUSART3_CK_PB0;
typedef AnyAFR<Port::PB, 12, AF::k7>	AfUSART3_CK_PB12;
typedef AnyAFR<Port::PC, 12, AF::k7>	AfUSART3_CK_PC12;
typedef AnyAFR<Port::PA, 6, AF::k7>		AfUSART3_CTS_PA6;
typedef AnyAFR<Port::PB, 13, AF::k7>	AfUSART3_CTS_PB13;
typedef AnyAFR<Port::PA, 15, AF::k7>	AfUSART3_DE_PA15;
typedef AnyAFR<Port::PB, 1, AF::k7>		AfUSART3_DE_PB1;
typedef AnyAFR<Port::PB, 14, AF::k7>	AfUSART3_DE_PB14;
typedef AnyAFR<Port::PA, 15, AF::k7>	AfUSART3_RTS_PA15;
typedef AnyAFR<Port::PB, 1, AF::k7>		AfUSART3_RTS_PB1;
typedef AnyAFR<Port::PB, 14, AF::k7>	AfUSART3_RTS_PB14;
typedef AnyAFR<Port::PB, 11, AF::k7>	AfUSART3_RX_PB11;
typedef AnyAFR<Port::PC, 5, AF::k7>		AfUSART3_RX_PC5;
typedef AnyAFR<Port::PC, 11, AF::k7>	AfUSART3_RX_PC11;
typedef AnyAFR<Port::PB, 10, AF::k7>	AfUSART3_TX_PB10;
typedef AnyAFR<Port::PC, 4, AF::k7>		AfUSART3_TX_PC4;
typedef AnyAFR<Port::PC, 10, AF::k7>	AfUSART3_TX_PC10;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 10, AF::k7>	AfUSART3_CK_PD10;
typedef AnyAFR<Port::PD, 11, AF::k7>	AfUSART3_CTS_PD11;
typedef AnyAFR<Port::PD, 2, AF::k7>		AfUSART3_DE_PD2;
typedef AnyAFR<Port::PD, 12, AF::k7>	AfUSART3_DE_PD12;
typedef AnyAFR<Port::PD, 2, AF::k7>		AfUSART3_RTS_PD2;
typedef AnyAFR<Port::PD, 12, AF::k7>	AfUSART3_RTS_PD12;
typedef AnyAFR<Port::PD, 9, AF::k7>		AfUSART3_RX_PD9;
typedef AnyAFR<Port::PD, 8, AF::k7>		AfUSART3_TX_PD8;
#endif	// defined(GPIOD_BASE)
#endif	// defined(USART3_BASE)

// UART4
#if defined(UART4_BASE)
typedef AnyAFR<Port::PB, 7, AF::k8>		AfUART4_CTS_PB7;
typedef AnyAFR<Port::PA, 15, AF::k8>	AfUART4_DE_PA15;
typedef AnyAFR<Port::PA, 15, AF::k8>	AfUART4_RTS_PA15;
typedef AnyAFR<Port::PA, 1, AF::k8>		AfUART4_RX_PA1;
typedef AnyAFR<Port::PC, 11, AF::k8>	AfUART4_RX_PC11;
typedef AnyAFR<Port::PA, 0, AF::k8>		AfUART4_TX_PA0;
typedef AnyAFR<Port::PC, 10, AF::k8>	AfUART4_TX_PC10;
#endif	// defined(UART4_BASE)

// UART5
#if defined(UART5_BASE)
typedef AnyAFR<Port::PB, 5, AF::k8>		AfUART5_CTS_PB5;
typedef AnyAFR<Port::PB, 4, AF::k8>		AfUART5_DE_PB4;
typedef AnyAFR<Port::PB, 4, AF::k8>		AfUART5_RTS_PB4;
typedef AnyAFR<Port::PC, 12, AF::k8>	AfUART5_TX_PC12;
#if defined(GPIOD_BASE)
typedef AnyAFR<Port::PD, 2, AF::k8>		AfUART5_RX_PD2;
#endif	// defined(GPIOD_BASE)
#endif	// defined(UART5_BASE)

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
typedef AnyAFR<Port::PG, 11, AF::k15>	AfEVENTOUT_PG11;
typedef AnyAFR<Port::PG, 12, AF::k15>	AfEVENTOUT_PG12;
typedef AnyAFR<Port::PG, 13, AF::k15>	AfEVENTOUT_PG13;
typedef AnyAFR<Port::PG, 14, AF::k15>	AfEVENTOUT_PG14;
typedef AnyAFR<Port::PG, 15, AF::k15>	AfEVENTOUT_PG15;
#endif	// defined(GPIOG_BASE)
#if defined(GPIOH_BASE)
typedef AnyAFR<Port::PH, 0, AF::k15>	AfEVENTOUT_PH0;
typedef AnyAFR<Port::PH, 1, AF::k15>	AfEVENTOUT_PH1;
typedef AnyAFR<Port::PH, 2, AF::k15>	AfEVENTOUT_PH2;
typedef AnyAFR<Port::PH, 3, AF::k15>	AfEVENTOUT_PH3;
typedef AnyAFR<Port::PH, 4, AF::k15>	AfEVENTOUT_PH4;
typedef AnyAFR<Port::PH, 5, AF::k15>	AfEVENTOUT_PH5;
typedef AnyAFR<Port::PH, 6, AF::k15>	AfEVENTOUT_PH6;
typedef AnyAFR<Port::PH, 7, AF::k15>	AfEVENTOUT_PH7;
typedef AnyAFR<Port::PH, 8, AF::k15>	AfEVENTOUT_PH8;
typedef AnyAFR<Port::PH, 9, AF::k15>	AfEVENTOUT_PH9;
typedef AnyAFR<Port::PH, 10, AF::k15>	AfEVENTOUT_PH10;
typedef AnyAFR<Port::PH, 11, AF::k15>	AfEVENTOUT_PH11;
typedef AnyAFR<Port::PH, 12, AF::k15>	AfEVENTOUT_PH12;
typedef AnyAFR<Port::PH, 13, AF::k15>	AfEVENTOUT_PH13;
typedef AnyAFR<Port::PH, 14, AF::k15>	AfEVENTOUT_PH14;
typedef AnyAFR<Port::PH, 15, AF::k15>	AfEVENTOUT_PH15;
#endif	// defined(GPIOH_BASE)
#if defined(GPIOI_BASE)
typedef AnyAFR<Port::PI, 0, AF::k15>	AfEVENTOUT_PI0;
typedef AnyAFR<Port::PI, 1, AF::k15>	AfEVENTOUT_PI1;
typedef AnyAFR<Port::PI, 2, AF::k15>	AfEVENTOUT_PI2;
typedef AnyAFR<Port::PI, 3, AF::k15>	AfEVENTOUT_PI3;
typedef AnyAFR<Port::PI, 4, AF::k15>	AfEVENTOUT_PI4;
typedef AnyAFR<Port::PI, 5, AF::k15>	AfEVENTOUT_PI5;
typedef AnyAFR<Port::PI, 6, AF::k15>	AfEVENTOUT_PI6;
typedef AnyAFR<Port::PI, 7, AF::k15>	AfEVENTOUT_PI7;
typedef AnyAFR<Port::PI, 8, AF::k15>	AfEVENTOUT_PI8;
typedef AnyAFR<Port::PI, 9, AF::k15>	AfEVENTOUT_PI9;
typedef AnyAFR<Port::PI, 10, AF::k15>	AfEVENTOUT_PI10;
typedef AnyAFR<Port::PI, 11, AF::k15>	AfEVENTOUT_PI11;
#endif	// defined(GPIOI_BASE)


}	// namespace Gpio
}	// namespace Bmt

