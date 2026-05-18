
#include "stdproj.h"

#if OPT_INCLUDE_SBW_DTRIG_

#include "SbwDev.h"
#include "util/WaveSet.h"
#include "util/DtrigSbw.h"

using namespace Bmt::Dma;
using namespace Bmt::Timer;
using namespace JtagFrame;
using namespace WaveJtag;


// ── Constructors ──────────────────────────────────────────────────────────────

SbwDev::SbwDev()   {}


// ── SbwDev virtual method implementations ─────────────────────────────────────

bool SbwDev::OnOpen()
{
	// TODO: sovereign Init — force-disable shared DMA channels, then
	//   DtrigSbwInit_1::Init();
	//   AcquireSbwIdle();
	return true;
}


void SbwDev::OnClose()
{
	// TODO: drain in-flight DMA, tri-state SBW pins, stop TIM1.
}


void SbwDev::OnConnectJtag(BusSpeed speed)
{
	// TODO: set bus state, configure GPIO AF for SBWTCK / SBWTDIO, latch speed.
	speed_ = speed;
}


void SbwDev::SetSpeed(BusSpeed speed)
{
	// TODO: per-grade TIM1 PSC + cnt_offset selection.
	(void)speed;
}


void SbwDev::OnReleaseJtag()
{
	// TODO: drive TEST/RST low, tri-state pins.
}


void SbwDev::OnResetTap()
{
	// TODO: DtrigSbwGoIdle::DoGoIdle(...) then SetSpeed(speed_).
}


JtagPending<uint8_t> SbwDev::OnIrShift(uint8_t instruction)
{
	(void)instruction;
	// TODO: render BSRR + dir scripts, drain previous frame, Start, return pending.
	return { nullptr, +[](const uint32_t*) -> uint8_t { return 0; } };
}


JtagPending<uint8_t> SbwDev::OnDrShift8(uint8_t data)
{
	(void)data;
	// TODO
	return { nullptr, +[](const uint32_t*) -> uint8_t { return 0; } };
}


JtagPending<uint16_t> SbwDev::OnDrShift16(uint16_t data)
{
	(void)data;
	// TODO
	return { nullptr, +[](const uint32_t*) -> uint16_t { return 0; } };
}


JtagPending<uint32_t> SbwDev::OnDrShift20(uint32_t data)
{
	(void)data;
	// TODO
	return { nullptr, +[](const uint32_t*) -> uint32_t { return 0; } };
}


JtagPending<uint32_t> SbwDev::OnDrShift32(uint32_t data)
{
	(void)data;
	// TODO
	return { nullptr, +[](const uint32_t*) -> uint32_t { return 0; } };
}


void SbwDev::OnSetTclk()
{
	// TODO: latch TCLK=1 (next shift will fill TDI with 1 on head/tail bits).
}


void SbwDev::OnClearTclk()
{
	// TODO: latch TCLK=0.
}


void SbwDev::OnPulseTclk()
{
	// TODO: emit a single TCLK pulse frame on SBW.
}


void SbwDev::OnPulseTclkN()
{
	// TODO: emit a single inverted TCLK pulse frame on SBW.
}


void SbwDev::OnPulseTclk(int count)
{
	(void)count;
	// TODO: emit `count` TCLK pulses.
}


void SbwDev::OnFlashTclk(uint32_t min_pulses)
{
	(void)min_pulses;
	// TODO: drive TCLK at ~470 kHz for at least `min_pulses` cycles.
}


#endif // OPT_INCLUDE_SBW_DTRIG_
