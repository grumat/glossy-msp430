#pragma once

#include "gpio.h"

// Up to 4 bits that can enumerates a binary sequence out to GPIO lines
template <
	const GpioPortId kPort				/// The GPIO port number
	, typename Bit0						/// Definition for bit 0 (defaults to unused pin, i.e an inputs)
	, typename Bit1						/// Definition for bit 1 (defaults to unused pin, i.e an inputs)
	, typename Bit2 = PinUnused<2>		/// Definition for bit 2 (defaults to unused pin, i.e an inputs)
	, typename Bit3 = PinUnused<3>		/// Definition for bit 3 (defaults to unused pin, i.e an inputs)
>
class GpioEnum
{
public:
	/// The GPIO port peripheral
	static constexpr GpioPortId kPort_ = kPort;
	/// The base address for the GPIO peripheral registers
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);
	/// Values for the BSRR register
	static constexpr uint32_t kValues[16] =
	{
		Bit0::kBsrrValue_ | Bit1::kBsrrValue_ | Bit2::kBsrrValue_ | Bit3::kBsrrValue_,
		Bit0::kBitValue_  | Bit1::kBsrrValue_ | Bit2::kBsrrValue_ | Bit3::kBsrrValue_,
		Bit0::kBsrrValue_ | Bit1::kBitValue_  | Bit2::kBsrrValue_ | Bit3::kBsrrValue_,
		Bit0::kBitValue_  | Bit1::kBitValue_  | Bit2::kBsrrValue_ | Bit3::kBsrrValue_,
		Bit0::kBsrrValue_ | Bit1::kBsrrValue_ | Bit2::kBitValue_  | Bit3::kBsrrValue_,
		Bit0::kBitValue_  | Bit1::kBsrrValue_ | Bit2::kBitValue_  | Bit3::kBsrrValue_,
		Bit0::kBsrrValue_ | Bit1::kBitValue_  | Bit2::kBitValue_  | Bit3::kBsrrValue_,
		Bit0::kBitValue_  | Bit1::kBitValue_  | Bit2::kBitValue_  | Bit3::kBsrrValue_,
		Bit0::kBsrrValue_ | Bit1::kBsrrValue_ | Bit2::kBsrrValue_ | Bit3::kBitValue_,
		Bit0::kBitValue_  | Bit1::kBsrrValue_ | Bit2::kBsrrValue_ | Bit3::kBitValue_,
		Bit0::kBsrrValue_ | Bit1::kBitValue_  | Bit2::kBsrrValue_ | Bit3::kBitValue_,
		Bit0::kBitValue_  | Bit1::kBitValue_  | Bit2::kBsrrValue_ | Bit3::kBitValue_,
		Bit0::kBsrrValue_ | Bit1::kBsrrValue_ | Bit2::kBitValue_  | Bit3::kBitValue_,
		Bit0::kBitValue_  | Bit1::kBsrrValue_ | Bit2::kBitValue_  | Bit3::kBitValue_,
		Bit0::kBsrrValue_ | Bit1::kBitValue_  | Bit2::kBitValue_  | Bit3::kBitValue_,
		Bit0::kBitValue_  | Bit1::kBitValue_  | Bit2::kBitValue_  | Bit3::kBitValue_,
	};
	/// Mask to limit the enumeration
	static constexpr uint32_t kTop = 
		(Bit3::kBitValue_ != 0) ? 15
		: (Bit2::kBitValue_ != 0) ? 7
		: (Bit1::kBitValue_ != 0) ? 3
		: (Bit0::kBitValue_ != 0) ? 1
		: 0
		;

	/// Access to the hardware IO data structure
	ALWAYS_INLINE static volatile GPIO_TypeDef & Io() { return *(volatile GPIO_TypeDef*)kPortBase_; }

	/// Apply an enum value in the range of 0-15
	ALWAYS_INLINE static void SetValue(const uint32_t val)
	{
		// Compilation will fail here if GPIO port number of pin does not match that of the peripheral!!!
		static_assert(
			(Bit0::kPort_ == kPort_)
			&& (Bit1::kPort_ == GpioPortId::kUnusedPort || Bit1::kPort_ == kPort_)
			&& (Bit2::kPort_ == GpioPortId::kUnusedPort || Bit2::kPort_ == kPort_)
			&& (Bit3::kPort_ == GpioPortId::kUnusedPort || Bit3::kPort_ == kPort_)
			, "Inconsistent port number"
			);
		// At least one bit is required to form an enumeration
		static_assert(kTop >= 1);

		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// Use bit-set-reset register to apply output
		port.BSRR = kValues[val & kTop];
	}

	/// Apply an enum value in the range of 0-15 (negative logic)
	ALWAYS_INLINE static void SetComplement(const uint32_t val)
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// Use bit-set-reset register to apply output
		port.BSRR = kValues[kTop - (val & kTop)];
	}
};

