/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_MATH_FASTMATH_H
#define XCSOAR_MATH_FASTMATH_H

#include "Compiler.h"
#include "Math/Constants.h"
#include <math.h>
#include <stdint.h>

#ifdef __cplusplus

#include "Math/fixed.hpp"

/**
 * Compares a^2 + b^2 against c^2
 * Use this instead of hypot when comparing
 * @return 1 if a^2 + b^2 > c^2,
 *         0 if a^2 + b^2 = c^2,
 *        -1 if a^2 + b^2 < c^2,
 */
gcc_const int compare_squared(int a, int b, int c);

gcc_const static inline int
iround(fixed x)
{
  return (int)floor(x + fixed_half);
}

#ifdef FIXED_MATH
extern const int COSTABLE[4096];
extern const int SINETABLE[4096];
extern const fixed::value_t INVCOSINETABLE[4096];
#else
extern const fixed COSTABLE[4096];
extern const fixed SINETABLE[4096];
extern const fixed INVCOSINETABLE[4096];
#endif

extern const short ISINETABLE[4096];
extern const short ICOSTABLE[4096];

#ifdef RADIANS
#define INT_ANGLE_MULT fixed_constant(4096.0 / M_2PI, 0x28be60db93LL)
#else
#define INT_ANGLE_MULT fixed_constant(4096.0 / 360, 0xb60b60b6LL)
#endif

gcc_const
static inline int
NATIVE_TO_INT(fixed x)
{
  return uround(fast_mult(INT_ANGLE_MULT, 8, x, 2)) & 0xfff;
}

gcc_const
static inline fixed
invfastcosine(fixed x)
{
#ifdef FIXED_MATH
  return fixed(fixed::internal(), INVCOSINETABLE[NATIVE_TO_INT(x)]);
#else
  return INVCOSINETABLE[NATIVE_TO_INT(x)];
#endif
}

gcc_const
static inline int
ifastsine(fixed x)
{
  return ISINETABLE[NATIVE_TO_INT(x)];
}

gcc_const
static inline int
ifastcosine(fixed x)
{
  return ICOSTABLE[NATIVE_TO_INT(x)];
}

gcc_const
static inline fixed
fastsine(fixed x)
{
#ifdef FIXED_MATH
  return fixed(fixed::internal(), SINETABLE[NATIVE_TO_INT(x)]);
#else
  return SINETABLE[NATIVE_TO_INT(x)];
#endif
}

gcc_const
static inline fixed
fastcosine(fixed x)
{
#ifdef FIXED_MATH
  return fixed(fixed::internal(), COSTABLE[NATIVE_TO_INT(x)]);
#else
  return COSTABLE[NATIVE_TO_INT(x)];
#endif
}

#define NORMALISE_BITS 7

/**
 * normalise a vector scaled to NORMALISE_BITS without
 * using sqrt or div operator
 */
void i_normalise_fast(int &x,
                      int &y);

/**
 * calculate normalised value of lesser of x and y scaled
 * to 3 bits without using sqrt or div operator
 */
gcc_const
unsigned i_normalise_sine(unsigned x,
                          unsigned y);

/**
 * normalise a vector scaled to NORMALISE_BITS (slow)
 */
void i_normalise(int &x,
                 int &y);

/**
 * normalise a magnitude based on a 3-vector to NORMALISE_BITS
 * without using sqrt or div
 *
 * result = mag/sqrt(x*x+y*y+z*z)
 */
gcc_const
int i_normalise_mag3(const int mag,
                     const int x,
                     const int y,
                     const int z);

/**
 * normalise a magnitude based on a 2-vector to NORMALISE_BITS
 * without using sqrt or div
 *
 * result = mag/sqrt(x*x+y*y)
 */
gcc_const
int i_normalise_mag2(const int mag,
                     const int x,
                     const int y);

/**
 * Find magnitude and reciprocal magnitude simultaneously
 */
void mag_rmag(const fixed x,
              const fixed y,
              fixed& __restrict__ s,
              fixed& __restrict__ is);

gcc_const
fixed thermal_recency_fn(unsigned x);

inline unsigned int
CombinedDivAndMod(unsigned int &lx)
{
  unsigned int ox = lx & 0xff;
  // JMW no need to check max since overflow will result in
  // beyond max dimensions
  lx = lx >> 8;
  return ox;
}

extern "C"
{
#endif

gcc_const
unsigned int isqrt4(unsigned long val);

#ifdef __cplusplus
}
#endif

#endif
