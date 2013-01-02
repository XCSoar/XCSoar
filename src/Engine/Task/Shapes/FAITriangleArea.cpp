/*
Copyright_License {

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

#include "FAITriangleArea.hpp"
#include "FAITriangleRules.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Geo/Math.hpp"

using namespace FAITriangleRules;

static constexpr unsigned STEPS = FAI_TRIANGLE_SECTOR_MAX / 3 / 8;

gcc_const
static Angle
CalcAlpha(fixed dist_a, fixed dist_b, fixed dist_c)
{
    const fixed cos_alpha = (sqr(dist_b) + sqr(dist_c) - sqr(dist_a))
      / Double(dist_c * dist_b);
    return Angle::acos(cos_alpha);
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

/**
 * Total=min..max; A=28%
 */
static GeoPoint *
GenerateFAITriangleRight(GeoPoint *dest,
                         const GeoPoint &origin, const GeoVector &leg_c,
                         const fixed dist_min, const fixed dist_max,
                         bool reverse)
{
  const fixed delta_distance = (dist_max - dist_min) / STEPS;
  fixed total_distance = dist_min;
  for (unsigned i = 0; i < STEPS && total_distance < LARGE_THRESHOLD; ++i,
         total_distance += delta_distance) {
    const fixed dist_a = SMALL_MIN_LEG * total_distance;
    const fixed dist_b = total_distance - dist_a - leg_c.distance;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

/**
 * Total=max
 */
static GeoPoint *
GenerateFAITriangleTop(GeoPoint *dest,
                       const GeoPoint &origin, const GeoVector &leg_c,
                       const fixed dist_max,
                       bool reverse)
{
  const fixed delta_distance = dist_max * (fixed(1) - 3 * SMALL_MIN_LEG)
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

/**
 * Total=max..min; B=28%
 */
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
    if (total_distance >= LARGE_THRESHOLD)
      continue;

    const fixed dist_b = SMALL_MIN_LEG * total_distance;
    const fixed dist_a = total_distance - dist_b - leg_c.distance;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

/**
 * Total=threshold; A=25%; B=30%..45%; C=45%..30%
 */
static GeoPoint *
GenerateFAITriangleLargeBottomRight(GeoPoint *dest,
                                    const GeoPoint &origin, const GeoVector &leg_c,
                                    bool reverse)
{
  const fixed max_leg = LARGE_THRESHOLD * LARGE_MAX_LEG;
  const fixed min_leg = LARGE_THRESHOLD - max_leg - leg_c.distance;
  assert(max_leg >= min_leg);

  const fixed min_a = LargeMinLeg(LARGE_THRESHOLD);

  const fixed a_start = LARGE_THRESHOLD * SMALL_MIN_LEG;
  const fixed a_end = std::max(min_leg, min_a);
  if (a_start <= a_end)
    return dest;

  fixed dist_a = a_start;
  fixed dist_b = LARGE_THRESHOLD - leg_c.distance - dist_a;

  const fixed delta_distance = (a_start - a_end) / STEPS;
  for (unsigned i = 0; i < STEPS; ++i,
         dist_a -= delta_distance, dist_b += delta_distance) {
    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

/**
 * Total=threshold..max[*]; A=25%; B=30%..45%; C=45%..30%
 */
static GeoPoint *
GenerateFAITriangleLargeRight1(GeoPoint *dest,
                               const GeoPoint &origin, const GeoVector &leg_c,
                               const fixed dist_min, const fixed dist_max,
                               bool reverse)
{
  if (dist_min >= LARGE_THRESHOLD)
    /* BottomRight already goes to Right2, and this arc doesn't
       exist */
    return dest;

  const fixed delta_distance = (dist_max - LARGE_THRESHOLD) / STEPS;
  fixed total_distance = LARGE_THRESHOLD;

  for (unsigned i = 0; i < STEPS; ++i,
         total_distance += delta_distance) {
    const fixed dist_a = LargeMinLeg(total_distance);
    const fixed dist_b = total_distance - dist_a - leg_c.distance;
    if (dist_b > total_distance * LARGE_MAX_LEG)
      break;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

/**
 * Total=min..max; A=25%..30%; B=45%; C=30%..25%
 */
static GeoPoint *
GenerateFAITriangleLargeRight2(GeoPoint *dest,
                               const GeoPoint &origin, const GeoVector &leg_c,
                               const fixed dist_min, const fixed dist_max,
                               bool reverse)
{
  /* this is the total distance where the Right1 arc ends; here, A is
     25% */
  const fixed min_total_for_a = leg_c.distance
    / (fixed(1) - LARGE_MAX_LEG - LARGE_MIN_LEG);

  const fixed delta_distance = (dist_max - dist_min) / STEPS;
  fixed total_distance = std::max(std::max(dist_min, LARGE_THRESHOLD),
                                  min_total_for_a);
  for (unsigned i = 0; i < STEPS && total_distance < dist_max; ++i,
         total_distance += delta_distance) {
    const fixed dist_b = total_distance * LARGE_MAX_LEG;
    const fixed dist_a = total_distance - dist_b - leg_c.distance;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

static GeoPoint *
GenerateFAITriangleLargeTop(GeoPoint *dest,
                            const GeoPoint &origin, const GeoVector &leg_c,
                            const fixed dist_max,
                            bool reverse)
{
  const fixed max_leg = dist_max * LARGE_MAX_LEG;
  const fixed min_leg = dist_max - leg_c.distance - max_leg;
  assert(max_leg >= min_leg);

  const fixed delta_distance = (max_leg - min_leg) / STEPS;
  fixed dist_a = min_leg, dist_b = max_leg;
  for (unsigned i = 0; i < STEPS; ++i,
         dist_a += delta_distance, dist_b -= delta_distance) {
    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

/**
 * Total=max..min; A=45%; B=30%..25%; C=25%..30%
 */
static GeoPoint *
GenerateFAITriangleLargeLeft2(GeoPoint *dest,
                              const GeoPoint &origin, const GeoVector &leg_c,
                              const fixed dist_min, const fixed dist_max,
                              bool reverse)
{
  const fixed delta_distance = (dist_max - dist_min) / STEPS;
  fixed total_distance = dist_max;
  for (unsigned i = 0; i < STEPS; ++i,
         total_distance -= delta_distance) {
    if (total_distance < LARGE_THRESHOLD)
      break;

    const fixed dist_a = total_distance * LARGE_MAX_LEG;
    const fixed dist_b = total_distance - dist_a - leg_c.distance;
    if (dist_b < LargeMinLeg(total_distance))
      break;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

/**
 * Total=min..threshold; A=45%..30%; B=25%; C=30%..45%
 */
static GeoPoint *
GenerateFAITriangleLargeLeft1(GeoPoint *dest,
                              const GeoPoint &origin, const GeoVector &leg_c,
                              const fixed dist_min, const fixed dist_max,
                              bool reverse)
{
  if (dist_min >= LARGE_THRESHOLD)
    /* Left2 already goes to BottomLeft, and this arc doesn't exist */
    return dest;

  /* this is the total distance where the Left1 arc starts; here, A is
     25% */
  const fixed max_total_for_a = leg_c.distance
    / (fixed(1) - LARGE_MAX_LEG - LARGE_MIN_LEG);

  const fixed total_start = std::min(dist_max, max_total_for_a);
  const fixed total_end = LARGE_THRESHOLD;

  const fixed delta_distance = (total_start - total_end) / STEPS;
  fixed total_distance = total_start;

  for (unsigned i = 0; i < STEPS; ++i,
         total_distance -= delta_distance) {
    const fixed dist_b = LargeMinLeg(total_distance);
    const fixed dist_a = total_distance - dist_b - leg_c.distance;

    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  //*dest++ = leg_c.EndPoint(origin);

  return dest;
}

/**
 * Total=threshold; A=30%..45%; B=25%; C=45%..30%
 */
static GeoPoint *
GenerateFAITriangleLargeBottomLeft(GeoPoint *dest,
                                    const GeoPoint &origin, const GeoVector &leg_c,
                                    bool reverse)
{
  const fixed max_leg = LARGE_THRESHOLD * LARGE_MAX_LEG;
  const fixed min_leg = LARGE_THRESHOLD - max_leg - leg_c.distance;
  assert(max_leg >= min_leg);

  const fixed min_b = LargeMinLeg(LARGE_THRESHOLD);

  const fixed b_start = std::max(min_leg, min_b);
  const fixed b_end = LARGE_THRESHOLD * SMALL_MIN_LEG;
  if (b_start >= b_end)
    return dest;

  fixed dist_b = b_start;
  fixed dist_a = LARGE_THRESHOLD - leg_c.distance - dist_b;

  const fixed delta_distance = (b_end - b_start) / STEPS;
  for (unsigned i = 0; i < STEPS; ++i,
         dist_a -= delta_distance, dist_b += delta_distance) {
    *dest++ = CalcGeoPoint(origin, leg_c.bearing,
                           dist_a, dist_b, leg_c.distance, reverse);
  }

  return dest;
}

GeoPoint *
GenerateFAITriangleArea(GeoPoint *dest,
                        const GeoPoint &pt1, const GeoPoint &pt2,
                        bool reverse)
{
  const GeoVector leg_c = pt1.DistanceBearing(pt2);

  const fixed dist_max = leg_c.distance / SMALL_MIN_LEG;
  const fixed dist_min = leg_c.distance / SMALL_MAX_LEG;

  const fixed large_dist_min = leg_c.distance / LARGE_MAX_LEG;
  const fixed large_dist_max = leg_c.distance / LARGE_MIN_LEG;

  dest = GenerateFAITriangleRight(dest, pt1, leg_c,
                                  dist_min, dist_max,
                                  reverse);

  if (large_dist_max > LARGE_THRESHOLD) {
    dest = GenerateFAITriangleLargeBottomRight(dest, pt1, leg_c,
                                               reverse);

    dest = GenerateFAITriangleLargeRight1(dest, pt1, leg_c,
                                          large_dist_min, large_dist_max,
                                          reverse);

    dest = GenerateFAITriangleLargeRight2(dest, pt1, leg_c,
                                          large_dist_min, large_dist_max,
                                          reverse);

    dest = GenerateFAITriangleLargeTop(dest, pt1, leg_c,
                                       large_dist_max,
                                       reverse);

    dest = GenerateFAITriangleLargeLeft2(dest, pt1, leg_c,
                                         large_dist_min, large_dist_max,
                                         reverse);

    dest = GenerateFAITriangleLargeLeft1(dest, pt1, leg_c,
                                         large_dist_min, large_dist_max,
                                         reverse);

    dest = GenerateFAITriangleLargeBottomLeft(dest, pt1, leg_c,
                                              reverse);
  } else {
    dest = GenerateFAITriangleTop(dest, pt1, leg_c,
                                  dist_max,
                                  reverse);
  }

  dest = GenerateFAITriangleLeft(dest, pt1, leg_c,
                                 dist_min, dist_max,
                                 reverse);

  return dest;
}
