/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FAITriangleSector.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Geo/Math.hpp"

static constexpr fixed FAI_MIN_PERCENTAGE(0.28);
static constexpr unsigned STEPS = FAI_TRIANGLE_SECTOR_MAX / 3;

gcc_const
static Angle
CalcAlpha(fixed dist_a, fixed dist_b, fixed dist_c)
{
    const fixed cos_alpha = (sqr(dist_b) + sqr(dist_c) - sqr(dist_a))
      / Double(dist_c * dist_b);
    return Angle::Radians(acos(cos_alpha));
}

gcc_const
static Angle
CalcAngle(Angle angle, fixed dist_a, fixed dist_b, fixed dist_c, bool reverse)
{
  const Angle alpha = CalcAlpha(dist_a, dist_b, dist_c);
  return reverse
    ? angle + alpha
    : angle - alpha;
}

gcc_const
static GeoPoint
CalcGeoPoint(const GeoPoint &origin, Angle angle,
             fixed dist_a, fixed dist_b, fixed dist_c, bool reverse)
{
  return FindLatitudeLongitude(origin, CalcAngle(angle, dist_a, dist_b, dist_c,
                                                 reverse), dist_b);
}

static GeoPoint *
GenerateFAITriangleRight(GeoPoint *dest,
                         const GeoPoint &origin, const GeoVector &leg_c,
                         const fixed dist_min, const fixed dist_max,
                         bool reverse)
{
  const fixed delta_distance = (dist_max - dist_min) / STEPS;
  fixed total_distance = dist_min;
  for (unsigned i = 0; i < STEPS; ++i,
         total_distance += delta_distance) {
    const fixed dist_a = FAI_MIN_PERCENTAGE * total_distance;
    const fixed dist_b = total_distance - dist_a - leg_c.distance;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

static GeoPoint *
GenerateFAITriangleTop(GeoPoint *dest,
                       const GeoPoint &origin, const GeoVector &leg_c,
                       const fixed dist_max,
                       bool reverse)
{
  const fixed delta_distance = dist_max * (fixed_one - 3 * FAI_MIN_PERCENTAGE)
    / STEPS;
  fixed dist_a = leg_c.distance;
  fixed dist_b = dist_max - dist_a - leg_c.distance;
  for (unsigned i = 0; i < STEPS; ++i,
         dist_a += delta_distance,
         dist_b -= delta_distance) {
    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

static GeoPoint *
GenerateFAITriangleLeft(GeoPoint *dest,
                        const GeoPoint &origin, const GeoVector &leg_c,
                        const fixed dist_min, const fixed dist_max,
                        bool reverse)
{
  const fixed delta_distance = (dist_max - dist_min) / STEPS;
  fixed total_distance = dist_max;
  for (unsigned i = 0; i < STEPS; ++i,
         total_distance -= delta_distance) {
    const fixed dist_b = FAI_MIN_PERCENTAGE * total_distance;
    const fixed dist_a = total_distance - dist_b - leg_c.distance;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

GeoPoint *
GenerateFAITriangleSector(GeoPoint *dest,
                          const GeoPoint &pt1, const GeoPoint &pt2,
                          bool reverse)
{
  const GeoVector leg_c = pt1.DistanceBearing(pt2);

  const fixed dist_max = leg_c.distance / FAI_MIN_PERCENTAGE;
  const fixed dist_min = leg_c.distance / (fixed_one - 2 * FAI_MIN_PERCENTAGE);

  dest = GenerateFAITriangleRight(dest, pt1, leg_c,
                                  dist_min, dist_max,
                                  reverse);

  dest = GenerateFAITriangleTop(dest, pt1, leg_c,
                                dist_max,
                                reverse);

  dest = GenerateFAITriangleLeft(dest, pt1, leg_c,
                                 dist_min, dist_max,
                                 reverse);

  return dest;
}
