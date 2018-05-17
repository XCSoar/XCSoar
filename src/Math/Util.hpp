/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_MATH_UTIL_HPP
#define XCSOAR_MATH_UTIL_HPP

#include "Compiler.h"

#include <math.h>

/**
 * Convert this number to a signed integer, with rounding.
 */
gcc_const
static inline int
iround(double x)
{
  return (int)lround(x);
}

/**
 * Convert this number to an unsigned integer, with rounding.
 *
 * The parameter must not be negative.  The result for a negative
 * parameter is undefined.
 */
static inline constexpr unsigned
uround(const double x)
{
  return (unsigned)(x + 0.5);
}

template<typename T>
static inline constexpr T
Square(T a)
{
  return a * a;
}

template<typename T>
static inline constexpr T
Cubic(T a)
{
  return a * a * a;
}

/**
 * Calculate the length of the second cathetus, given the length of
 * the hypotenuse and the first cathetus.
 */
gcc_const
static inline double
Cathetus(double hypotenuse, double cathetus1)
{
  return sqrt(Square(hypotenuse) - Square(cathetus1));
}

/**
 * Calculate the length of the space diagonal of the given cuboid.
 */
gcc_const
static inline double
SpaceDiagonal(double a, double b, double c)
{
  return sqrt(Square(a) + Square(b) + Square(c));
}

/**
 * Implementation of a sigmoid function with a result range of [-1..1].
 */
gcc_const
static inline double
Sigmoid(double x)
{
  return 2.0 / (1.0 + exp(-x)) - 1.0;
}

#endif
