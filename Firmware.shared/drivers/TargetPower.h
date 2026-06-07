#pragma once

#include "stdproj.h"		// pulls bmt.h, which now exposes <adc.h>


/*!
Per-target target-voltage helper behind the GDB monitor "power" command (#46).

Capabilities and the ADC / PWM wiring are declared in the active platform.h
(OPT_TARGET_HAS_VSENSE / OPT_TARGET_HAS_VDRIVE, kVtg*_mV, VSenseAdc); this class
is the shared glue. PASS 2a implements voltage SENSE only (e.g. the ST-Link V2
clone, which has a fixed 3.3 V supply). Variable-voltage DRIVE via PWM_VT is
PASS 2b and lands behind OPT_TARGET_HAS_VDRIVE.
*/
class TargetPower
{
public:
	//! One-shot bring-up of the sense ADC (no-op on probes without sense).
	//! Call once at startup after the clock tree is configured.
	static void Init()
	{
#if OPT_TARGET_HAS_VSENSE
		VSenseAdc::Init();
#endif
	}

	static constexpr bool HasSense() { return OPT_TARGET_HAS_VSENSE; }
	static constexpr bool HasDrive() { return OPT_TARGET_HAS_VDRIVE; }

	//! Measured target voltage in mV, or 0 if this probe cannot sense it.
	static uint32_t ReadMilliVolts()
	{
#if OPT_TARGET_HAS_VSENSE
		auto *adc = Adc::Peripheral<Adc::Unit::k1>::GetDevice();
		Adc::StartConversion(adc);
		const uint32_t raw = Adc::ReadData(adc);	// 12-bit, right-aligned
		// pin = raw/4095 * VDDA; target VCC = pin * divider ratio
		return raw * kVtgAdcRef_mV * kVtgSenseMul / 4095u;
#else
		return 0;
#endif
	}

	//! Drive the target supply to 'mv'. Returns false on a fixed-supply probe
	//! (no controllable output). DRIVE is implemented in PASS 2b.
	static bool SetMilliVolts(uint32_t mv)
	{
		(void)mv;
		return false;
	}

	//! Remove the target supply (drive-capable probes only).
	static void Off() { }
};
