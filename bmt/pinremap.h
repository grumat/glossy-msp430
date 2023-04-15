#pragma once

#include "otherlibs.h"
#include "mcu-system.h"

namespace Bmt
{
namespace Gpio
{


#ifdef STM32F1


/// A template class for Alternate GPIO Function initialization
template<
	const uint32_t CONF			///< The specific configuration bits
	, const uint32_t MASK		///< The mask to clear the configuration bits
	>
struct AnyAFR
{
	/// A constant defining the bits that are set for the specific configuration
	static constexpr uint32_t kConf = CONF;
	/// A constant defining the mask to clear the specific configuration
	static constexpr uint32_t kMask = MASK;
	/// A constant indicating a bogus configuration used as conditional compilation
	static constexpr bool kNoRemap = (CONF == 0x00000000UL) && (MASK == 0xFFFFFFFFUL);

	/// Enables the alternate function
	ALWAYS_INLINE static void Enable(void)
	{
		if (kNoRemap == false)
			AFIO->MAPR = (AFIO->MAPR & MASK) | CONF;
	}
	/// Disables the alternate function
	ALWAYS_INLINE static void Disable(void)
	{
		if (kNoRemap == false)
			AFIO->MAPR = AFIO->MAPR & MASK;
	}
};

/// No pin remapping (for regular GPIO function)
typedef AnyAFR<0x00000000U, 0xFFFFFFFF> AfNoRemap;



#else



/// Alternate function value
enum class AF : uint8_t
{
	k0,
	k1,
	k2,
	k3,
	k4,
	k5,
	k6,
	k7,
	k8,
	k9,
	k10,
	k11,
	k12,
	k13,
	k14,
	k15,
};

/// A template class for Alternate GPIO Function initialization
template<
	const Gpio::Port kPort		///< The specific configuration bits
	, const uint8_t kPin		///< Port bit for the configuration
	, const AF kAfr		///< The value for AFRx register
	>
struct AnyAFR
{
	/// A constant defining the port associated to the configuration
	static constexpr Gpio::Port kPort_ = kPort;
	/// A constant defining the bit associated with the configuration
	static constexpr uint8_t kPin_ = kPin;
	/// A constant defining the AFRx register
	static constexpr AF kAfr_ = kAfr;
	/// value for the AFRL register
	static constexpr uint32_t kAFRL_ = (kPin_ >= 8 || kPort == Port::kUnusedPort) ? 0UL
		: uint32_t(kAfr_) * (16 * kPin_);
	static constexpr uint32_t kAFRL_Mask_ = (kPin_ >= 8 || kPort == Port::kUnusedPort) ? 0UL
		: ~(0b1111UL * (16 * kPin_));
	/// value for the AFRH register
	static constexpr uint32_t kAFRH_ = (kPin_ < 8 || kPort == Port::kUnusedPort) ? 0UL
		: uint32_t(kAfr_) * (16 * (kPin_ - 8));
	static constexpr uint32_t kAFRH_Mask_ = (kPin_ < 8 || kPort == Port::kUnusedPort) ? 0UL
		: ~(0b1111UL * (16 * (kPin_ - 8)));
	/// Base address of the port peripheral
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);
	/// A constant indicating a bogus configuration used as conditional compilation
	static constexpr bool kNoRemap = (kPort == Port::kUnusedPort);

	/// Access to the peripheral memory space
	constexpr static volatile GPIO_TypeDef *Io() { return (volatile GPIO_TypeDef *)kPortBase_; }

	/// Enables the alternate function
	ALWAYS_INLINE static void Enable(void)
	{
		if (kAFRL_Mask_ != 0UL)
		{
			volatile GPIO_TypeDef *port = Io();
			port->AFR[0] = (port->AFR[0] & kAFRL_Mask_) | kAFRL_;
		}
		if (kAFRH_Mask_ != 0UL)
		{
			volatile GPIO_TypeDef *port = Io();
			port->AFR[1] = (port->AFR[1] & kAFRH_Mask_) | kAFRH_;
		}
	}
	/// Disables the alternate function (don't care)
	ALWAYS_INLINE static void Disable(void)
	{
	}
};

// Used to deactivate remapping
typedef AnyAFR<Port::kUnusedPort, 0, AF::k0> AfNoRemap;


#endif

}	// namespace Gpio
}	// namespace Bmt

	
#ifdef STM32F1
#	include "f1xx/pinremap.h"
#elif defined(STM32L4)
#	include "l4xx/pinremap.h"
#elif defined(STM32G4)
#	include "g4xx/pinremap.h"
#else
#	error "Unsupported MCU"
#endif
