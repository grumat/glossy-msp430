#pragma once

/*!
System-wide bus phase. Drives the per-board buffer-enable lines (where the board
has configurable buffers) and is a no-op on boards without them. The phases follow
the "States" table in Hardware/BluePill-G431/README.md: acquisition differs from
the active bus (e.g. during JTAG entry only the RST buffer is live; during SBW the
RST buffer is OFF because the RST/NMI role moves onto SBWTDIO). The probe drivers
set the matching phase as they progress: connect → kAcquiringX, post-entry → kX,
release → kStandby.

COM Open is intentionally NOT part of this enum — it is the independent target-UART
buffer (UartBusOn/UartBusOff), unrelated to the JTAG/SBW acquisition phases. It is
tracked separately (Issue #21).
*/
enum class BusState
{
	kStandby = 0,		///< whole bus Hi-Z (cold boot / all interfaces closed)
	kAcquiringJtag,		///< JTAG entry sequence in progress (only RST buffer live)
	kAcquiringSbw,		///< SBW entry sequence in progress
	kJtag,				///< JTAG acquired — full JTAG bus driving
	kSbw,				///< SBW acquired — SBW bus driving
};

// Sets hardware buffers in tri-state or driving
static inline void SetBusState(const BusState st);
