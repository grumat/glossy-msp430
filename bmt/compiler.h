#pragma once

/// Optimizes a code forcing it to inline, even on DEBUG code
#define ALWAYS_INLINE	inline __attribute__((always_inline, optimize("Os")))
/// Forces a function to generate a static code instance (no inline)
#define NO_INLINE	__attribute__((noinline))
/// Tag to force a function/method to be optimized even in Debug builds
#define OPTIMIZED	__attribute__((optimize("Os")))
/// Tag to force a function/method to be always generated without optimizations and allow for debug
#define DEBUGGABLE 	__attribute__((optimize("O0")))
/// Forces a structure to be always 4-byte aligned, which produces quite better code on ARM platform
#define ALIGNED		__attribute__((aligned(4)))
/// Marks a method as deprecated, a handy tool to help refactoring
#define DEPRECATED	__attribute__((deprecated))
/// Tag to force a function/method to optimized, single instanced, even multiply defined
#define BMT_STATIC_API	__attribute__((optimize("Os"),noinline,weak))

/// Returns element count of compound items
#define _countof(t)		(sizeof(t)/sizeof(t[0]))

/// Breaks CPU flow if conditions aren´t met
#define assert(cond)	if ((cond) == false) { __BKPT(255); }
