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

#define fixed_third fixed(1./3.)
#define fixed_two_thirds fixed(2./3.)

#define fixed_pi fixed(M_PI)
#define fixed_two_pi fixed(M_2PI)
#define fixed_half_pi fixed(M_PI_2)

#define fixed_sqrt_two fixed(1.4142135623730951)
#define fixed_sqrt_half fixed(0.70710678118654757)

typedef double fixed;

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

#endif
