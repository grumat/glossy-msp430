#pragma once


#define ALWAYS_INLINE	__attribute__((always_inline)) inline
#define NO_INLINE	__attribute__((noinline))
#define OPTIMIZED	__attribute__((optimize(2)))
#define DEBUGGABLE 	__attribute__((optimize(0)))
#define ALIGNED		__attribute__((aligned(4)))
#define DEPRECATED	__attribute__((deprecated))

#define _countof(t)		(sizeof(t)/sizeof(t[0]))

#define assert(cond)	if ((cond) == false) { __BKPT(255); }
