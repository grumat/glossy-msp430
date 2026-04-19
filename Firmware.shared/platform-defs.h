#pragma once

// BLOCK: OPT_JTAG_IMPLEMENTATION values
// No JTAG interface implemented
#define OPT_JTAG_IMPL_OFF			0
// JTAG implemented using SPI and polled data move
#define OPT_JTAG_IMPL_SPI			1
// JTAG implemented using SPI and DMA moves data
#define OPT_JTAG_IMPL_SPI_DMA		2
// JTAG implemented using optimized TIM+DMA method
#define OPT_JTAG_IMPL_TIM_DMA		3
// JTAG implemented using TIM+DMA method, but compatible with STLinkV2 HW (slower)
#define OPT_JTAG_IMPL_TIM_DMA_SLOW	4
// ENDBLOCK: OPT_JTAG_IMPLEMENTATION values

// BLOCK: OPT_JTAG_TCLK_IMPLEMENTATION values
// No TCLK interface implemented in JTAG interface
#define OPT_JTCLK_IMPL_OFF			0
// TCLK signal generated with TIM and DMA association
#define OPT_JTCLK_IMPL_TIM_DMA		1
// TCLK signal generated with SPI output
#define OPT_JTCLK_IMPL_SPI			2
// ENDBLOCK: OPT_JTAG_TCLK_IMPLEMENTATION values

// BLOCK: OPT_GDB_IMPLEMENTATION values
// Implements GDB using USB VCP	(TODO)
#define OPT_GDB_IMPL_VCP		0
// Implements GDB using USART1 (used in development phase)
#define OPT_GDB_IMPL_USART1		1
// Implements GDB using USART1 (used in development phase)
#define OPT_GDB_IMPL_USART2		2
// Implements GDB using USART1 (used in development phase)
#define OPT_GDB_IMPL_USART3		3

