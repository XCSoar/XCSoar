/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_TEST_UTIL_HPP
#define XCSOAR_TEST_UTIL_HPP

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"

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

static inline bool
is_zero(const fixed value, const int accuracy=ACCURACY)
{
  return (long)(fabs(value) * accuracy) == 0;
}

static inline bool
is_one(const fixed value, const int accuracy=ACCURACY)
{
  return is_zero(value - fixed_one, accuracy);
}

static inline bool
equals(const fixed a, const fixed b, const int accuracy=ACCURACY)
{
  if (is_zero(a, accuracy) || is_zero(b, accuracy))
    return is_zero(a, accuracy) && is_zero(b, accuracy);

  return is_one(a / b, accuracy);
}

#ifdef FIXED_MATH
static inline bool
equals(const fixed a, double b)
{
  return equals(a, fixed(b));
}
#endif

static inline bool
equals(const fixed a, int b)
{
  return equals(a, fixed(b));
}

#ifdef FIXED_MATH
static inline bool
between(double x, double a, double b)
{
  return x >= a && x <= b;
}
#endif

static inline bool
between(fixed x, fixed a, fixed b)
{
  return x >= a && x <= b;
}

#ifdef FIXED_MATH

static inline bool
between(fixed x, double a, double b)
{
  return between(x, fixed(a), fixed(b));
}

#endif

static inline bool
equals(const Angle a, int b)
{
  return equals(a.Degrees(), fixed(b));
}

static inline bool
equals(const Angle a, double b)
{
  return equals(a.Degrees(), fixed(b));
}

static inline bool
equals(const Angle a, const Angle b)
{
  return equals(a.Degrees(), b.Degrees());
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

#endif
