#pragma once

#include "stdproj.h"		// pulls bmt.h, which now exposes <adc.h>


/*!
Per-target target-voltage helper behind the GDB monitor "power" command (#46).

Capabilities and the ADC / PWM wiring are declared in the active platform.h
(OPT_TARGET_HAS_VSENSE / OPT_TARGET_HAS_VDRIVE, kVtg*_mV, VSenseAdc, VtgPwm*);
this class is the shared glue:
- SENSE: ReadMilliVolts() does a single ADC conversion (e.g. the ST-Link V2 clone
  is sense-only — fixed 3.3 V supply, no controllable output).
- DRIVE: SetMilliVolts()/Off() drive the PWM_VT regulator (TIM4_CH4) where the
  board supports it (bluepill / g431). CCR maps 1:1 to millivolts; the RC filter
  ramps the rail over ~5τ. The output boots OFF.
*/
class TargetPower
{
public:
	//! One-shot bring-up: sense ADC and/or the PWM_VT drive regulator (each a
	//! no-op on probes that lack it). The drive output comes up OFF (CCR=0).
	//! Call once at startup after the clock tree is configured.
	static void Init()
	{
#if OPT_TARGET_HAS_VSENSE
		VSenseAdc::Init();
#endif
#if OPT_TARGET_HAS_VDRIVE
		VtgPwmTimer::Setup();		// TIM4 time base (PSC/ARR/CR1)
		VtgPwm::Setup();			// CH4 PWM1, output enabled
		VtgPwm::SetCompare(0);		// 0% duty → 0 V → supply off
		PWM_VT::Setup();			// route PB9 to TIM4_CH4 alternate function
		VtgPwmTimer::CounterStart();
		drive_mv_ = 0;
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

	//! Drive the target supply to 'mv' (clamped to kVtgMax_mV). Returns false on
	//! a fixed-supply probe with no controllable output. The PWM CCR equals the
	//! millivolt value 1:1; the RC filter ramps the rail over ~5τ.
	static bool SetMilliVolts(uint32_t mv)
	{
#if OPT_TARGET_HAS_VDRIVE
		if (mv > kVtgMax_mV)
			mv = kVtgMax_mV;
		VtgPwm::SetCompare((uint16_t)mv);
		drive_mv_ = mv;
		return true;
#else
		(void)mv;
		return false;
#endif
	}

	//! Last commanded drive level in mV (0 = off / unset). 0 on sense-only probes.
	static uint32_t DriveMilliVolts()
	{
#if OPT_TARGET_HAS_VDRIVE
		return drive_mv_;
#else
		return 0;
#endif
	}

	//! Remove the target supply (drive-capable probes only).
	static void Off()
	{
#if OPT_TARGET_HAS_VDRIVE
		VtgPwm::SetCompare(0);
		drive_mv_ = 0;
#endif
	}

#if OPT_TARGET_HAS_VDRIVE
private:
	static inline uint32_t drive_mv_ = 0;
#endif
};
