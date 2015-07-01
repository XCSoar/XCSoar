/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "SimplifiedMath.hpp"
#include "FAISphere.hpp"
#include "GeoPoint.hpp"

#ifdef FIXED_MATH

static inline Angle
EarthASin(const fixed a)
{
  return Angle::asin(a);
}

#endif

static inline Angle
EarthDistance(const fixed a)
{
  if (!positive(a))
    return Angle::Zero();

#ifdef FIXED_MATH
  // acos(1-x) = 2*asin(sqrt(x/2))
  // acos(1-2*x) = 2*asin(sqrt(x))
  //    = 2*atan2(sqrt(x), sqrt(fixed(1)-x));
  return EarthASin(sqrt(a) >> fixed::accurate_cordic_shift) * 2;
#else
  return Angle::acos(fixed(1) - Double(a));
#endif
}

/**
 * Multiply two very small values (less than 4).  This is an optimised
 * fast path for fixed-point.
 */
constexpr
static inline fixed
SmallMult(fixed a, fixed b)
{
  return fast_mult(a, b, 0);
}

/**
 * Multiply three very small values (less than 2).  This is an
 * optimised fast path for fixed-point.
 */
constexpr
static inline fixed
SmallMult(fixed a, fixed b, fixed c)
{
  return SmallMult(SmallMult(a, b), c);
}

void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 Angle *distance, Angle *bearing)
{
  assert(loc1.IsValid());
  assert(loc2.IsValid());

  const auto sc1 = loc1.latitude.SinCos();
  fixed sin_lat1 = sc1.first, cos_lat1 = sc1.second;
  const auto sc2 = loc2.latitude.SinCos();
  fixed sin_lat2 = sc2.first, cos_lat2 = sc2.second;

  const Angle dlon = loc2.longitude - loc1.longitude;

  if (distance) {
    const fixed s1 = (loc2.latitude - loc1.latitude).accurate_half_sin();
    const fixed s2 = dlon.accurate_half_sin();
    const fixed a = sqr(s1) + SmallMult(cos_lat1, cos_lat2) * sqr(s2);

    Angle distance2 = EarthDistance(a);
    assert(!negative(distance2.Native()));
    *distance = distance2;
  }

  if (bearing) {
    // speedup for fixed since this is one call
    const auto sc = dlon.SinCos();
    const fixed sin_dlon = sc.first, cos_dlon = sc.second;

    const fixed y = SmallMult(sin_dlon, cos_lat2);
    const fixed x = SmallMult(cos_lat1, sin_lat2)
      - SmallMult(sin_lat1, cos_lat2, cos_dlon);

    *bearing = (x == fixed(0) && y == fixed(0))
      ? Angle::Zero()
      : Angle::FromXY(x, y).AsBearing();
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}

void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 fixed *distance, Angle *bearing)
{
  if (distance != nullptr) {
    Angle distance_angle;
    DistanceBearingS(loc1, loc2, &distance_angle, bearing);
    *distance = FAISphere::AngleToEarthDistance(distance_angle);
  } else
    DistanceBearingS(loc1, loc2, (Angle *)nullptr, bearing);
}
