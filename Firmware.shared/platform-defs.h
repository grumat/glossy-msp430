#pragma once

// BLOCK: OPT_JTAG_IMPLEMENTATION values
// No JTAG interface implemented
#define OPT_JTAG_IMPL_OFF			0
// JTAG using SPI for TDI/TDO and TIM1 for synchronised JTCK+TMS generation.
// Only supported variant — see .claude/docs/drivers/SPI_VARIANT_REMOVED.md
// and .claude/docs/drivers/TIM_VARIANT_REMOVED.md for the rationale.
#define OPT_JTAG_IMPL_DTRIG			5
// ENDBLOCK: OPT_JTAG_IMPLEMENTATION values

// BLOCK: OPT_SBW_IMPLEMENTATION values
// No SBW (Spy-Bi-Wire) interface implemented
#define OPT_SBW_IMPL_OFF			0
// SBW using TIM1_CH1 PWM for SBWCLK and BSRR DMA script for SBWTDIO +
// direction control. This is the "timdma" transport model (TIM single-shot
// fanning out DMA requests; no dtrig-style critical section — there is no
// competing peripheral to coordinate). A future SBW "dtrig" SPI-stream variant
// is sketched in DTRIG_SBW_SPI_ALT.md.
// See .claude/docs/drivers/TIM_SBW_DRIVER.md.
#define OPT_SBW_IMPL_TIM			5
// ENDBLOCK: OPT_SBW_IMPLEMENTATION values

// BLOCK: OPT_JTAG_TCLK_IMPLEMENTATION values
// No TCLK interface implemented in JTAG interface
#define OPT_JTCLK_IMPL_OFF			0
// TCLK signal generated with SPI output (the only supported generator; the
// legacy TIM+DMA wave-generator variants were removed — they were never the
// active path on any target and only created phantom TIM3/TIM4 resource use).
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

