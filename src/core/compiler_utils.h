#pragma once

// To disable buffer overrun checks when not needed
#if defined(GN_COMPILER_MSVC)
	#define GN_DISABLE_SECURITY_COOKIE_CHECK __declspec(safebuffers)
#else
	#define GN_DISABLE_SECURITY_COOKIE_CHECK
#endif

// Force inline macro
#if defined(GN_COMPILER_MSVC)
	#define GN_FORCE_INLINE __forceinline
#elif defined(GN_COMPILER_GCC) || defined(GN_COMPILER_CLANG)
	#define GN_FORCE_INLINE __attribute__((always_inline)) inline
#else
	#define GN_FORCE_INLINE inline
#endif