#pragma once

#ifdef __cplusplus
#include <bits/c++config.h>

/*
 * The GCC 10 libstdc++ headers come from Debian bullseye, while KOBO_NICKEL
 * targets Nickel's older glibc sysroot.  Disable pthread header feature paths
 * which assume libc declarations not present in Nickel.
 */
#undef _GLIBCXX_USE_PTHREAD_COND_CLOCKWAIT
#undef _GLIBCXX_USE_PTHREAD_MUTEX_CLOCKLOCK
#undef _GLIBCXX_USE_PTHREAD_RWLOCK_CLOCKLOCK

/*
 * Preload <cmath> while libstdc++ can set up std:: math functions, then make
 * subsequent <math.h> includes go directly to Nickel's C header.  This avoids
 * libstdc++'s compatibility math.h wrapper re-exporting std::isinf/isnan over
 * Nickel's older C declarations.
 */
#include <cmath>
#define _GLIBCXX_INCLUDE_NEXT_C_HEADERS
#define _GLIBCXX_MATH_H 1
#endif
