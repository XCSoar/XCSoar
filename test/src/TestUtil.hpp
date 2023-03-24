// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "time/Stamp.hpp"

#ifndef ACCURACY
/**
 * Define this macro before including TestUtil.hpp to compare with a
 * different accuracy.
 */
#define ACCURACY 10000
#endif

extern "C" {
#include "tap.h"
}

#undef fail

static inline bool
is_zero(const double value, const int accuracy=ACCURACY)
{
  return (long)(fabs(value) * accuracy) == 0;
}

static inline bool
is_one(const double value, const int accuracy=ACCURACY)
{
  return is_zero(value - 1, accuracy);
}

static inline bool
equals(const double a, const double b, const int accuracy=ACCURACY)
{
  if (is_zero(a, accuracy) || is_zero(b, accuracy))
    return is_zero(a, accuracy) && is_zero(b, accuracy);

  return is_one(a / b, accuracy);
}

static inline bool
equals(const FloatDuration a, const FloatDuration b,
       const int accuracy=ACCURACY) noexcept
{
  return equals(a.count(), b.count(), accuracy);
}

static inline bool
equals(const TimeStamp a, const TimeStamp b,
       const int accuracy=ACCURACY) noexcept
{
  return equals(a.ToDuration(), b.ToDuration(), accuracy);
}

static inline bool
equals(const double a, int b)
{
  return equals(a, double(b));
}

static inline bool
between(double x, double a, double b)
{
  return x >= a && x <= b;
}

static inline bool
equals(const Angle a, int b)
{
  return equals(a.Degrees(), double(b));
}

static inline bool
equals(const Angle a, double b)
{
  return equals(a.Degrees(), b);
}

static inline bool
equals(const Angle a, const Angle b)
{
  return is_zero((a - b).AsDelta().Degrees());
}

static inline bool
equals(const GeoPoint a, double lat, double lon)
{
  return equals(a.latitude, lat) && equals(a.longitude, lon);
}

static inline bool
equals(const GeoPoint a, const Angle lat, const Angle lon)
{
  return equals(a.latitude, lat) && equals(a.longitude, lon);
}

static inline bool
equals(const GeoPoint a, const GeoPoint b)
{
  return equals(a.latitude, b.latitude) && equals(a.longitude, b.longitude);
}
