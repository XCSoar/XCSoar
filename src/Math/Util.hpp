// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <math.h>

/**
 * Convert this number to a signed integer, with rounding.
 */
[[gnu::const]]
static inline int
iround(double x) noexcept
{
  return (int)lround(x);
}

/**
 * Convert this number to an unsigned integer, with rounding.
 *
 * The parameter must not be negative.  The result for a negative
 * parameter is undefined.
 */
constexpr unsigned
uround(const double x) noexcept
{
  return (unsigned)(x + 0.5);
}

template<typename T>
constexpr T
Square(T a) noexcept
{
  return a * a;
}

template<typename T>
constexpr T
Cubic(T a) noexcept
{
  return a * a * a;
}

/**
 * Calculate the length of the second cathetus, given the length of
 * the hypotenuse and the first cathetus.
 */
[[gnu::const]]
static inline double
Cathetus(double hypotenuse, double cathetus1) noexcept
{
  return sqrt(Square(hypotenuse) - Square(cathetus1));
}

/**
 * Calculate the length of the space diagonal of the given cuboid.
 */
[[gnu::const]]
static inline double
SpaceDiagonal(double a, double b, double c) noexcept
{
  return sqrt(Square(a) + Square(b) + Square(c));
}

/**
 * Implementation of a sigmoid function with a result range of [-1..1].
 */
[[gnu::const]]
static inline double
Sigmoid(double x) noexcept
{
  return 2.0 / (1.0 + exp(-x)) - 1.0;
}
