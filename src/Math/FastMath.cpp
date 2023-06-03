// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FastMath.hpp"

#include <math.h>

/**
 * Calculates the square root of val
 *
 * See http://www.azillionmonkeys.com/qed/sqroot.html
 * @param val Value
 * @return Rounded square root of val
 */
unsigned
isqrt4(unsigned val) noexcept
{
#if defined(__i386__) || defined(__x86_64__)
  /* x86 FPUs are extremely fast */
  return (unsigned)sqrt((double)val);
#else
  unsigned int temp, g = 0;

  if (val >= 0x40000000) {
    g = 0x8000;
    val -= 0x40000000;
  }

  #define INNER_MBGSQRT(s)                    \
  temp = (g << (s)) + (1 << ((s) * 2 - 2));   \
  if (val >= temp) {                          \
    g += 1 << ((s)-1);                        \
    val -= temp;                              \
  }

  INNER_MBGSQRT (15)
  INNER_MBGSQRT (14)
  INNER_MBGSQRT (13)
  INNER_MBGSQRT (12)
  INNER_MBGSQRT (11)
  INNER_MBGSQRT (10)
  INNER_MBGSQRT ( 9)
  INNER_MBGSQRT ( 8)
  INNER_MBGSQRT ( 7)
  INNER_MBGSQRT ( 6)
  INNER_MBGSQRT ( 5)
  INNER_MBGSQRT ( 4)
  INNER_MBGSQRT ( 3)
  INNER_MBGSQRT ( 2)

  #undef INNER_MBGSQRT

  temp = g + g + 1;
  if (val >= temp)
    g++;

  return g;
#endif
}
