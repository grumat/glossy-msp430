#pragma once

/// Optimizes a code forcing it to inline, even on DEBUG code
#define ALWAYS_INLINE	__attribute__((always_inline, optimize(2))) inline
/// Forces a function to generate a static code instance (no inline)
#define NO_INLINE	__attribute__((noinline))
/// Tag to force a function/method to be optimized even in Debug builds
#define OPTIMIZED	__attribute__((optimize(2)))
/// Tag to force a function/method to be always generated without optimizations and allow for debug
#define DEBUGGABLE 	__attribute__((optimize(0)))
/// Forces a structure to be always 4-byte aligned, which produces quite better code on ARM platform
#define ALIGNED		__attribute__((aligned(4)))
/// Marks a method as deprecated, a handy tool to help refactoring
#define DEPRECATED	__attribute__((deprecated))

/// Returns element count of compound items
#define _countof(t)		(sizeof(t)/sizeof(t[0]))

/// Breaks CPU flow if conditions aren´t met
#define assert(cond)	if ((cond) == false) { __BKPT(255); }
