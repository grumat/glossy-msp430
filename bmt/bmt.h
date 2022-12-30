#pragma once

/// Scope restriction
#ifndef OPT_LIB_LEVEL
#define OPT_LIB_LEVEL	99999
#endif

#include "otherlibs.h"
#include "systick.h"
#include "gpio-utils.h"
#include "stream.h"
#include "trace.h"
#include "spi.h"
#include "dma.h"

/// Isolate official classes from Example 2
#if (OPT_LIB_LEVEL >= 2)
#include "critical_section.h"
#include "tasks.h"
#endif

#if (OPT_LIB_LEVEL >= 3)
#include "timer.h"
#include "uart.h"
#include "stopwatch.h"
#endif
