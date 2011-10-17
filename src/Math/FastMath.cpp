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

#include "Math/FastMath.h"
#include "Math/Constants.h"

#include "MathTables.h"
#include <assert.h>

int
compare_squared(int a, int b, int c)
{
  int a2b2 = a*a+b*b;
  int c2 = c*c;
  if (a2b2 > c2)
    return 1;
  if (a2b2 < c2)
    return -1;
  return 0;
}

extern "C"
{

/**
 * Calculates the square root of val
 *
 * See http://www.azillionmonkeys.com/qed/sqroot.html
 * @param val Value
 * @return Rounded square root of val
 */
unsigned int
isqrt4(unsigned long val)
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

}

fixed
thermal_recency_fn(unsigned x)
{
  return x < THERMALRECENCY_SIZE
#ifdef FIXED_MATH
    ? fixed(fixed::internal(), THERMALRECENCY[x])
#else
    ? THERMALRECENCY[x]
#endif
    : fixed_zero;
}

// find inverse sqrt of x scaled to NORMALISE_BITS^2 by using
// bisector search
gcc_pure
static inline
unsigned i_rsqrt(const uint64_t x, const uint64_t yhint)
{
  int64_t y_last=yhint, y= yhint;
  while (1) {
    y= ((((int64_t)3<<(4*NORMALISE_BITS))-x*y*y)*y)>>(1+4*NORMALISE_BITS);
    if (y_last==y)
      return y;
    y_last = y;
  }
}

static inline
unsigned log2_fast(const unsigned v)
{
  static gcc_constexpr_data int8_t LogTable256[256] = {
#define L8(n) n, n, n, n, n, n, n, n
#define L16(n) L8(n), L8(n)
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    L16(4), L16(5), L16(5), L16(6), L16(6), L16(6), L16(6),
    L16(7), L16(7), L16(7), L16(7), L16(7), L16(7), L16(7), L16(7)
  };
  register unsigned int t, tt; // temporaries

  if ((tt = v >> 16))
    {
      return (t = (tt >> 8)) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
    }
  else
    {
      return (t = (v >> 8)) ? 8 + LogTable256[t] : LogTable256[v];
    }
}

gcc_pure
static inline
uint64_t normalise_hint2(const uint32_t x)
{
  static const uint64_t hint2_limit =
    sqrt((double)((uint64_t)3 << (NORMALISE_BITS * 4 - 1)));

  // taking x as maximum individual value of a size 2 vector,
  // this provides an initial estimate of 1/sqrt(vector)
  // scaled by NORMALISE_BITS^2.
  //
  // based on algorithm for computing log2 from
  // http://www-graphics.stanford.edu/~seander/bithacks.html

  uint64_t y = 1<< (NORMALISE_BITS*2-log2_fast(x));
  while (x*y > hint2_limit)
    y= y>>1;

  return y;
}


gcc_pure
static inline
uint64_t normalise_hint3(const uint32_t x)
{
  // as normalise_hint2 but for size 3 vector

  static const uint64_t hint3_limit =
    sqrt((double)((uint64_t)3 << (NORMALISE_BITS * 4))) / sqrt(3.0);

  uint64_t y = 1<< (NORMALISE_BITS*2-log2_fast(x));
  while (x*y > hint3_limit)
    y= y>>1;

  return y;
}


int i_normalise_mag2(const int mag,
                     const int x,
                     const int y)
{
  const uint32_t m_max = abs(x) | abs(y);
  assert(m_max);
  const uint64_t m = (long)x*x+(long)y*y;
  const uint64_t r = i_rsqrt(m, normalise_hint2(m_max));
  return (mag*r) >> (2*NORMALISE_BITS);
}

int i_normalise_mag3(const int mag,
                     const int x,
                     const int y,
                     const int z)
{
  const uint32_t m_max = abs(x) | abs(y) | abs(z);
  assert(m_max);
  const uint64_t m = (long)x*x+(long)y*y+(long)z*z;
  const uint64_t r = i_rsqrt(m, normalise_hint3(m_max));
  return (mag*r) >> (2*NORMALISE_BITS);
}


void i_normalise_fast(int &x,
                      int &y) {
  const uint32_t m_max = abs(x) | abs(y);
  if (!m_max)
    return;
  const uint64_t r = i_rsqrt((long)x*x+(long)y*y, normalise_hint2(m_max));

  x= (x*r)>> NORMALISE_BITS;
  y= (y*r)>> NORMALISE_BITS;
}

unsigned i_normalise_sine(unsigned x,
                          unsigned y)
{
  const uint32_t m_max = x>y? x:y;
  const uint32_t m_min = x>y? y:x;
  if (!m_max)
    return 0;
  const uint64_t r = i_rsqrt((unsigned long)x*x+(unsigned long)y*y, normalise_hint2(m_max));
  return (m_min*r)>> (NORMALISE_BITS*2-3);
}


void i_normalise(int &x,
                 int &y) {
  const unsigned m_max = std::max(abs(x), abs(y));
  if (!m_max)
    return;
  const int mag = lhypot(x, y);
  x= (x<<NORMALISE_BITS)/mag;
  y= (y<<NORMALISE_BITS)/mag;
}

void mag_rmag(fixed x,
              fixed y,
              fixed& __restrict__ dist,
              fixed& __restrict__ inv_dist)
{
  x = fabs(x);
  y = fabs(y);
  if (!positive(x) && !positive(y)) {
    dist = fixed_zero;
    inv_dist = fixed_zero;
    return;
  }
#ifdef FIXED_MATH
  unsigned d_shift = 1;
  while (std::max(x,y) > fixed(1000)) {
    x = half(x);
    y = half(y);
    d_shift = (d_shift << 1);
  }
#endif
  const fixed mag_sq = sqr(x)+sqr(y);
  inv_dist = rsqrt(mag_sq);
  assert(positive(inv_dist));
  dist = inv_dist*mag_sq;
#ifdef FIXED_MATH
  if (d_shift>1) {
    inv_dist /= d_shift;
    dist *= d_shift;
  }
#endif
}
