#ifndef FIXED_HPP
#define FIXED_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
//
// Extensions and bug/compilation fixes by John Wharington 2009

#include "Compiler.h"
#include "Constants.h"

#include <utility>

#include <assert.h>

#define fixed_third fixed(1./3.)
#define fixed_two_thirds fixed(2./3.)

#define fixed_pi fixed(M_PI)
#define fixed_two_pi fixed(M_2PI)
#define fixed_half_pi fixed(M_HALFPI)

#define fixed_sqrt_two fixed(1.4142135623730951)
#define fixed_sqrt_half fixed(0.70710678118654757)

#include <math.h>

typedef double fixed;

gcc_const
static inline std::pair<fixed, fixed>
sin_cos(const fixed thetha)
{
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
  double s, c;
  sincos(thetha, &s, &c);
  return std::make_pair(s, c);
#else
  return std::make_pair(sin(thetha), cos(thetha));
#endif
}

static inline constexpr bool
positive(fixed x)
{
  return x > 0;
}

static inline constexpr bool
negative(fixed x)
{
  return x < 0;
}

gcc_const
static inline fixed
sigmoid(fixed x)
{
  return 2.0 / (1.0 + exp(-x)) - 1.0;
}

constexpr
static inline fixed
Half(fixed a)
{
  return a * 0.5;
}

constexpr
static inline fixed
Quarter(fixed a)
{
  return a * 0.25;
}

constexpr
static inline fixed
Double(fixed a)
{
  return a * 2;
}

constexpr
static inline fixed
Quadruple(fixed a)
{
  return a * 4;
}

gcc_const
inline fixed rsqrt(fixed a) {
  // not fast
  return 1.0/sqrt(a);
}

gcc_const
inline fixed fast_sqrt(fixed a) {
  // not fast
  return sqrt(a);
}

constexpr
inline fixed sqr(fixed a) {
  return a*a;
}

constexpr
inline fixed fast_mult(fixed a, gcc_unused int a_bits,
                       fixed b, gcc_unused int b_bits)
{
  return a * b;
}

constexpr
inline fixed fast_mult(fixed a, fixed b, gcc_unused int b_bits)
{
  return a * b;
}

gcc_const
inline fixed accurate_half_sin(fixed a) {
  return sin(a/2);
}

/**
 * Calculate the euclidian distance for "tiny" parameter values,
 * i.e. all values below 3.
 *
 * This function was calibrated for small delta angles,
 * e.g. reasonable distances on earth's surface.
 */
gcc_const
inline fixed
TinyHypot(fixed x, fixed y)
{
  return hypot(x, y);
}

/**
 * Calculate the euclidian distance for "small" parameter values,
 * i.e. values below 100,000.
 */
gcc_const
inline fixed
SmallHypot(fixed x, fixed y)
{
  return hypot(x, y);
}

/**
 * Calculate the euclidian distance for "medium" parameter values,
 * i.e. values below 1,000,000.
 */
gcc_const
inline fixed
MediumHypot(fixed x, fixed y)
{
  return hypot(x, y);
}

/**
 * Calculate the euclidian distance for "large" parameter values,
 * i.e. values below 8,000,000,000.
 */
gcc_const
inline fixed
LargeHypot(fixed x, fixed y)
{
  return hypot(x, y);
}

inline void limit_tolerance(fixed& f, const fixed tol_act) {
  if (fabs(f)<tol_act) {
    f = positive(f)? tol_act:-tol_act;
  }
}

/**
 * Convert this number to a signed integer, with rounding.
 */
gcc_const static inline int
iround(fixed x)
{
  return (int)lround(x);
}

/**
 * Convert this number to an unsigned integer, with rounding.
 */
gcc_const static inline unsigned
uround(const fixed x)
{
  return (unsigned)(x + fixed(0.5));
}

/**
 * The sign function.
 *
 * Returns 1 for positive values, -1 for negative values and 0 for zero.
 */
constexpr
static inline int
sgn(const fixed x)
{
  return positive(x) ? 1 : (negative(x) ? -1 : 0);
}

#endif
