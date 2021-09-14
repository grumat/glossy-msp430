#pragma once

//uint32_t CRC32_SW(uint32_t prev, const uint8_t *buf, uint32_t size);
//uint32_t CRC32_HW(uint32_t prev, const uint8_t *buf, uint32_t size);

class CRC32_HW
{
public:
	ALWAYS_INLINE CRC32_HW() { CRC->CR = 1; }
	ALWAYS_INLINE uint32_t Append(const void *data_p, uint32_t cnt)
	{
		const uint8_t *data = (const uint8_t *)data_p;
		const uint8_t *end = data + cnt;
		for (; data < end; ++data)
			CRC->IDR = *data;
		return CRC->DR;
	}
	ALWAYS_INLINE uint32_t Get() const { return CRC->DR; }
};

class CRC32_SW
{
public:
	ALWAYS_INLINE CRC32_SW() { crc_ = 0xFFFFFFFF; }
	uint32_t Append(const void *data_p, uint32_t cnt);
	ALWAYS_INLINE uint32_t Get() const { return ~crc_ & 0xffffffff; }
protected:
	uint32_t crc_;
};


typedef CRC32_HW CRC32;
