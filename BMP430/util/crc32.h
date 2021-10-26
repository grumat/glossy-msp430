#pragma once


class CRC32_M1
{
public:
	ALWAYS_INLINE CRC32_M1() { crc_ = 0xFFFFFFFF; }
	uint32_t Append(const void *data_p, uint32_t cnt);
	ALWAYS_INLINE uint32_t Get() const { return ~crc_ & 0xffffffff; }
protected:
	uint32_t crc_;
};


typedef CRC32_M1 CRC32;
