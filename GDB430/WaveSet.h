#pragma once

#if OPT_USE_SPI_WAVE_GEN

#ifdef WAVESET_1_4th

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[4];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 4;

#elif WAVESET_2_9th

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[9];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 8;

#elif WAVESET_1_5th

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[5];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 4;

#elif WAVESET_2_11

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[11];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 8;

#elif WAVESET_1_6

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[3];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 2;

#elif WAVESET_1_7

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[7];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 4;

#elif WAVESET_1_8

/// Access to the array with the JTMS wave
extern const uint8_t g_JtmsWave[4];
/// Number of periods that the wave contains
const uint32_t kNumPeriods = 2;

#endif


/// SPI speed that will produce a up to 475 kHz JTMS
typedef Spi::SpiTemplate<
	kSpiForJtag
	, SysClk
	, kJtclkSpiClock	// 2 clocks per cycle!!
	, Spi::Role::kMaster
	, Spi::ClkPol::kMode3
	, Spi::Bits::k8
	, Spi::Props::kDefault
	, Spi::BiDi::kFullDuplex
> SpiJtmsWave;

#endif	// OPT_USE_SPI_WAVE_GEN
