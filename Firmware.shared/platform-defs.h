#pragma once

// BLOCK: OPT_JTAG_IMPLEMENTATION values
// No JTAG interface implemented
#define OPT_JTAG_IMPL_OFF			0
// JTAG using SPI for TDI/TDO and TIM1 for synchronised JTCK+TMS generation.
// Only supported variant — see .claude/docs/drivers/SPI_VARIANT_REMOVED.md
// and .claude/docs/drivers/TIM_VARIANT_REMOVED.md for the rationale.
#define OPT_JTAG_IMPL_DTRIG			5
// ENDBLOCK: OPT_JTAG_IMPLEMENTATION values

// BLOCK: OPT_JTAG_TCLK_IMPLEMENTATION values
// No TCLK interface implemented in JTAG interface
#define OPT_JTCLK_IMPL_OFF			0
// TCLK signal generated with TIM and DMA association
#define OPT_JTCLK_IMPL_TIM_DMA		1
// TCLK signal generated with TIM and DMA association (option using a CC stopper)
#define OPT_JTCLK_IMPL_TIM_DMA_2	2
// TCLK signal generated with SPI output
#define OPT_JTCLK_IMPL_SPI			3
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

