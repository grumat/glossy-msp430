#include "stdproj.h"
#include "WaveSet.h"

#if OPT_USE_SPI_WAVE_GEN


#ifdef WAVESET_1_4th

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[4] = 
{ 
	0b00001111,
	0b00001111,
	0b00001111,
	0b00001111,
};

#elif WAVESET_2_9th

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[9] = 
{ 
	0b00000111,
	0b10000011,
	0b11000001,
	0b11100000,
	0b11110000,
	0b01111000,
	0b00111100,
	0b00011110,
	0b00001111,
};

#elif WAVESET_1_5th

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[5] = 
{ 
	0b00000111,
	0b11000001,
	0b11110000,
	0b01111100,
	0b00011111,
};

#elif WAVESET_2_11

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[11] = 
{ 
	0b00000011,
	0b11100000,
	0b01111100,
	0b00001111,
	0b10000001,
	0b11110000,
	0b00111110,
	0b00000111,
	0b11000000,
	0b11111000,
	0b00011111,
};

#elif WAVESET_1_6

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[3] = 
{
	0b00000011,
	0b11110000,
	0b00111111,
};

#elif WAVESET_1_7

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[7] = 
{
	0b00000001,
	0b11111100,
	0b00000111,
	0b11110000,
	0b00011111,
	0b11000000,
	0b01111111,
};

#elif WAVESET_1_8

/// SPI works in MSB bit order
const uint8_t g_JtmsWave[4] = 
{
	0b00000000,
	0b11111111,
	0b00000000,
	0b11111111,
};

#endif


#endif	// OPT_USE_SPI_WAVE_GEN

