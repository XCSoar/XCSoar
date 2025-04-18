// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Constants.hpp"

#include <array>

static constexpr unsigned INT_ANGLE_RANGE = 4096;
static constexpr unsigned INT_ANGLE_MASK = INT_ANGLE_RANGE - 1;
static constexpr unsigned INT_QUARTER_CIRCLE = INT_ANGLE_RANGE / 4;

static constexpr double INT_ANGLE_MULT = INT_ANGLE_RANGE / M_2PI;

extern const std::array<double, INT_ANGLE_RANGE> SINETABLE;
extern const std::array<double, INT_ANGLE_RANGE> INVCOSINETABLE;

extern const std::array<short, INT_ANGLE_RANGE> ISINETABLE;

constexpr unsigned
NormalizeIntAngle(unsigned angle) noexcept
{
  return angle & INT_ANGLE_MASK;
}

constexpr unsigned
IntAngleForCos(unsigned angle) noexcept
{
  return NormalizeIntAngle(angle + INT_QUARTER_CIRCLE);
}

constexpr unsigned
UnsafeRadiansToIntAngle(double radians) noexcept
{
  /* add INT_ANGLE_RANGE to ensure that the result is not negative
     (assuming the parameter is "sane"); the caller will normalise the
     result using NormalizeIntAngle() */
  /* add 0.5 for rounding; by definition, casting to integer rounds
     towards zero, and adding 0.5 will apply correct rounding for
     non-negative values */
  return unsigned(radians * INT_ANGLE_MULT + (10 * INT_ANGLE_RANGE + 0.5));
}

constexpr unsigned
NATIVE_TO_INT(double x) noexcept
{
  return NormalizeIntAngle(UnsafeRadiansToIntAngle(x));
}

constexpr unsigned
NATIVE_TO_INT_COS(double x) noexcept
{
  return IntAngleForCos(UnsafeRadiansToIntAngle(x));
}

constexpr double
IntAngleToRadians(unsigned angle) noexcept
{
  return angle / INT_ANGLE_MULT;
}

[[gnu::const]]
static inline double
invfastcosine(double x) noexcept
{
  return INVCOSINETABLE[NATIVE_TO_INT(x)];
}

[[gnu::const]]
static inline int
ifastsine(double x) noexcept
{
  return ISINETABLE[NATIVE_TO_INT(x)];
}

[[gnu::const]]
static inline int
ifastcosine(double x) noexcept
{
  return ISINETABLE[NATIVE_TO_INT_COS(x)];
}

[[gnu::const]]
static inline double
fastsine(double x) noexcept
{
  return SINETABLE[NATIVE_TO_INT(x)];
}

[[gnu::const]]
static inline double
fastcosine(double x) noexcept
{
  return SINETABLE[NATIVE_TO_INT_COS(x)];
}
