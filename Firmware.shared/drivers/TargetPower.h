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

	//! Above this measured level the target is treated as externally powered, so
	//! EnsurePowered() leaves the drive off (avoids contention on a shared net).
	static constexpr uint32_t kVtgPresentThreshold = 1500;

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

	//! Auto-power the target the way a TI FET does on connect: if nothing is
	//! already driving it AND it isn't externally powered, bring the supply up to
	//! kVtgDefault_mV. Called by TapMcu::Open() so a scan powers the target
	//! without an explicit "power" command. Blocks up to ~3 s for the RC rail to
	//! settle when it actually drives (UIF likewise waits after MSP430_VCC). A
	//! no-op on sense-only / fixed-supply probes.
	static void EnsurePowered()
	{
#if OPT_TARGET_HAS_VDRIVE
		if (drive_mv_ != 0)
			return;					// already driving (explicit power or prior scan)
#if OPT_TARGET_HAS_VSENSE
		if (ReadMilliVolts() >= kVtgPresentThreshold)
			return;					// externally powered — don't fight it
#endif
		SetMilliVolts(kVtgDefault_mV);
#if OPT_TARGET_HAS_VSENSE
		// Wait for the RC rail to rise; bail on timeout so a missing/short load
		// cannot hang the open.
		StopWatch sw(TickTimer::M2T<Bmt::Timer::Msec(3000)>::kTicks);
		const uint32_t ready = kVtgDefault_mV - kVtgDefault_mV / 10;	// within ~10%
		while (sw.IsNotElapsed())
		{
			if (ReadMilliVolts() >= ready)
				break;
		}
#endif
#endif
	}

#if OPT_TARGET_HAS_VDRIVE
private:
	static inline uint32_t drive_mv_ = 0;
#endif
};
