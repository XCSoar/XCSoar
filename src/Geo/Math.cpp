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

#include "Math.hpp"
#include "Constants.hpp"
#include "GeoPoint.hpp"

#include <assert.h>

#ifdef INSTRUMENT_TASK
// global, used for test harness
unsigned count_distbearing = 0;
#endif

static inline Angle
EarthASin(const fixed a)
{
  return Angle::asin(a);
}

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

gcc_pure
static GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  Angle dthis, Angle dtotal)
{
  assert(loc1.IsValid());
  assert(loc2.IsValid());

  if (loc1.longitude == loc2.longitude &&
      loc1.latitude == loc2.latitude)
    return loc1;

  if (!positive(dtotal.Native()))
    return loc1;

  // dthis can be larger than dtotal or even smaller than 0, which extrapolates
  // the gc between loc1 and loc2 to loc3
  //assert(dthis <= dtotal && !negative(dthis.Native()));

  const fixed A = (dtotal - dthis).sin();
  const fixed B = dthis.sin();

  const auto sc1 = loc1.latitude.SinCos();
  const fixed sin_loc1_lat = sc1.first, cos_loc1_lat = sc1.second;

  const auto sc2 = loc2.latitude.SinCos();
  const fixed sin_loc2_lat = sc2.first, cos_loc2_lat = sc2.second;

  const auto sc3 = loc1.longitude.SinCos();
  const fixed sin_loc1_lon = sc3.first, cos_loc1_lon = sc3.second;

  const auto sc4 = loc2.longitude.SinCos();
  const fixed sin_loc2_lon = sc4.first, cos_loc2_lon = sc4.second;

  const fixed a_cos_loc1_lat = SmallMult(A, cos_loc1_lat);
  const fixed b_cos_loc2_lat = SmallMult(B, cos_loc2_lat);

  const fixed x = SmallMult(a_cos_loc1_lat, cos_loc1_lon)
    + SmallMult(b_cos_loc2_lat, cos_loc2_lon);
  const fixed y = SmallMult(a_cos_loc1_lat, sin_loc1_lon)
    + SmallMult(b_cos_loc2_lat, sin_loc2_lon);
  const fixed z = SmallMult(A, sin_loc1_lat) + SmallMult(B, sin_loc2_lat);

  GeoPoint loc3;
  loc3.latitude = Angle::FromXY(TinyHypot(x, y), z);
  loc3.longitude = Angle::FromXY(x, y);
  loc3.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc3;
}

GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  const fixed dthis)
{
  const fixed dtotal = ::Distance(loc1, loc2);

  if (dthis >= dtotal)
    return loc2;

  return IntermediatePoint(loc1, loc2,
                           EarthDistanceToAngle(dthis),
                           EarthDistanceToAngle(dtotal));
}

GeoPoint
Middle(const GeoPoint &a, const GeoPoint &b)
{
  // TODO: optimize this naive approach
  const fixed distance = Distance(a, b);
  return IntermediatePoint(a, b, Half(distance));
}

/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
static void
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
DistanceBearing(const GeoPoint &loc1, const GeoPoint &loc2,
                fixed *distance, Angle *bearing)
{
#ifdef USE_WGS84
  const auto lon21 = loc2.longitude - loc1.longitude;

  fixed u1 = atan((fixed(1) - FLATTENING) * loc1.latitude.tan()),
        u2 = atan((fixed(1) - FLATTENING) * loc2.latitude.tan());

  fixed sinu1 = sin(u1),
        cosu1 = cos(u1);

  fixed sinu2 = sin(u2),
        cosu2 = cos(u2);

  fixed lambda = lon21.Radians(),
        lambda_p = Angle::FullCircle().Radians();

  unsigned iterLimit = 20;
  fixed cos_sq_alpha = fixed(0),
        sin_sigma = fixed(0),
        cos_sigma = fixed(0),
        cos_2_sigma_m = fixed(0),
        sigma = fixed(0);

  while (fabs(lambda - lambda_p) > fixed(1e-7) && --iterLimit) {
    fixed sin_lambda = sin(lambda),
          cos_lambda = cos(lambda);

#ifdef FIXED_MATH
    fixed sqr_cosu2_sinlambda = sqr(cosu2 * sin_lambda<<12);
    fixed rest = sqr(cosu1 * sinu2 - sinu1 * cosu2 * cos_lambda<<12);

    if (sqr_cosu2_sinlambda + rest == fixed(0)) {
      // coincident points...
      if (distance != nullptr) *distance = fixed(0);
      if (bearing != nullptr) *bearing = Angle::Zero();
      return;
    }

    sin_sigma = sqrt(sqr_cosu2_sinlambda + rest)>>12;
#else
    sin_sigma = sqrt(sqr(cosu2 * sin_lambda) +
                     sqr(cosu1 * sinu2 - sinu1 * cosu2 * cos_lambda));

    if (sin_sigma == fixed(0)) {
      // coincident points...
      if (distance != nullptr) *distance = fixed(0);
      if (bearing != nullptr) *bearing = Angle::Zero();
      return;
    }
#endif

    cos_sigma = sinu1 * sinu2 + cosu1 * cosu2 * cos_lambda;
    sigma = atan2(sin_sigma, cos_sigma);

    fixed inner_alpha = cosu1 * cosu2 * sin_lambda / sin_sigma;
    cos_sq_alpha = fixed(1) - sqr(inner_alpha);

    if (fabs(loc1.latitude.Radians()) < fixed(1e-7) &&
        fabs(loc2.latitude.Radians()) < fixed(1e-7)) {
      // both points are on equator.
      cos_2_sigma_m = fixed(-1);
      lambda_p = lambda;
      lambda = lon21.Radians() + FLATTENING * inner_alpha * sigma;
    } else {
      cos_2_sigma_m = cos_sigma - fixed(2) * sinu1 * sinu2 / cos_sq_alpha;

      fixed c = FLATTENING/fixed(16) * cos_sq_alpha *
                (fixed(4) + FLATTENING * (fixed(4) - fixed(3) * cos_sq_alpha));

      lambda_p = lambda;
      lambda = lon21.Radians() + (fixed(1) - c) * FLATTENING * inner_alpha *
        (sigma + c * sin_sigma * (cos_2_sigma_m + c * cos_sigma *
        (fixed(-1) + fixed(2) * sqr(cos_2_sigma_m))));
    }
  }

  if (iterLimit == 0) {
    assert(false || !"WGS84 convergence failed");
    return;  // formula failed to converge
  }

  if (distance != nullptr) {
    //fixed u_sq = cos_sq_alpha * (sqr(REARTH_A) - sqr(REARTH_B)) / sqr(REARTH_B);
    fixed u_sq = cos_sq_alpha * fixed(0.006739497);

    fixed A = fixed(16384) + u_sq * (fixed(4096) + u_sq * (fixed(-768) +
      u_sq * (fixed(320) - fixed(175) * u_sq)));

    A /= fixed(16384);

    fixed B = u_sq * (fixed(256) + u_sq * (fixed(-128) +
      u_sq * (fixed(74) - fixed(47) * u_sq)));

    B /= fixed(1024);

    fixed delta_sigma = B * sin_sigma * (
      cos_2_sigma_m +
      B/fixed(4) * (cos_sigma * (fixed(-1) + fixed(2) * sqr(cos_2_sigma_m)) -
                    B/fixed(6) * cos_2_sigma_m *
                                 (fixed(-3) + fixed(4) * sqr(sin_sigma)) *
                                 (fixed(-3) + fixed(4) * sqr(cos_2_sigma_m))));

    *distance = REARTH_B * A * (sigma - delta_sigma);
  }

  if (bearing != nullptr)
    *bearing = Angle::Radians(atan2(cosu2 * sin(lambda),
      cosu1 * sinu2 - sinu1 * cosu2 * cos(lambda))).AsBearing();

#else
  if (distance != nullptr) {
    Angle distance_angle;
    DistanceBearingS(loc1, loc2, &distance_angle, bearing);
    *distance = AngleToEarthDistance(distance_angle);
  } else
    DistanceBearingS(loc1, loc2, nullptr, bearing);
#endif
}

fixed
CrossTrackError(const GeoPoint &loc1, const GeoPoint &loc2,
                const GeoPoint &loc3, GeoPoint *loc4)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();

  // cross track distance
  const Angle cross_track_distance =
    EarthASin(SmallMult(sindist_AD, (crs_AD - crs_AB).sin()));

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

#ifdef USE_WGS84
  const auto sc = cross_track_distance.SinCos();
  const fixed sinXTD = sc.first, cosXTD = sc.second;

  const Angle along_track_distance =
    EarthASin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

  auto loc4_tmp = IntermediatePoint(loc1, loc2, along_track_distance, dist_AB);

  if (loc4)
    *loc4 = loc4_tmp;

  return Distance(loc3, loc4_tmp);

#else
  if (loc4) {
    const auto sc = cross_track_distance.SinCos();
    const fixed sinXTD = sc.first, cosXTD = sc.second;

    const Angle along_track_distance =
      EarthASin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

    *loc4 = IntermediatePoint(loc1, loc2, along_track_distance, dist_AB);
  }

  return AngleToEarthDistance(cross_track_distance);
#endif
}

fixed
ProjectedDistance(const GeoPoint &loc1, const GeoPoint &loc2,
                  const GeoPoint &loc3)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);
  if (!positive(dist_AD.Native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed(0);

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);
  if (!positive(dist_AB.Native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed(0);

  // The "along track distance", along_track_distance, the distance from A along the
  // course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  const Angle cross_track_distance =
    EarthASin(SmallMult(sindist_AD, (crs_AD - crs_AB).sin()));

  const auto sc = cross_track_distance.SinCos();
  const fixed sinXTD = sc.first, cosXTD = sc.second;

  // along track distance
  const Angle along_track_distance =
    EarthASin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

#ifdef USE_WGS84
  auto projected = IntermediatePoint(loc1, loc2, along_track_distance, dist_AB);

  return Distance(loc1, projected);
#else
  return AngleToEarthDistance(along_track_distance);
#endif
}


fixed
DoubleDistance(const GeoPoint &loc1, const GeoPoint &loc2,
               const GeoPoint &loc3)
{
  assert(loc1.IsValid());
  assert(loc2.IsValid());
  assert(loc3.IsValid());

  const fixed cos_loc1_lat = loc1.latitude.cos();
  const fixed cos_loc2_lat = loc2.latitude.cos();
  const fixed cos_loc3_lat = loc3.latitude.cos();

  const fixed s21 = (loc2.latitude - loc1.latitude).accurate_half_sin();
  const fixed sl21 = (loc2.longitude - loc1.longitude).accurate_half_sin();
  const fixed s32 = (loc3.latitude - loc2.latitude).accurate_half_sin();
  const fixed sl32 = (loc3.longitude - loc2.longitude).accurate_half_sin();

  const fixed a12 = sqr(s21)
    + SmallMult(cos_loc1_lat, cos_loc2_lat) * sqr(sl21);
  const fixed a23 = sqr(s32)
    + SmallMult(cos_loc2_lat, cos_loc3_lat) * sqr(sl32);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return (2 * REARTH) *
    (EarthDistance(a12) + EarthDistance(a23)).Radians();
}

GeoPoint
FindLatitudeLongitude(const GeoPoint &loc, const Angle bearing,
                      fixed distance)
{
  assert(loc.IsValid());

  assert(!negative(distance));
  if (!positive(distance))
    return loc;

  GeoPoint loc_out;

#ifdef USE_WGS84
  const fixed lon1 = loc.longitude.Radians();
  const fixed lat1 = loc.latitude.Radians();

  //const fixed alpha1 = bearing.Radians();
  const fixed sin_alpha1 = bearing.SinCos().first;
  const fixed cos_alpha1 = bearing.SinCos().second;

  const fixed tan_u1 = (fixed(1) - FLATTENING) * tan(lat1);
  const fixed cos_u1 = fixed(1) / sqrt(fixed(1) + sqr(tan_u1));
  const fixed sin_u1 = tan_u1 * cos_u1;

  const fixed sigma1 = atan2(tan_u1, cos_alpha1);

  const fixed sin_alpha = cos_u1 * sin_alpha1;
  const fixed cos_sq_alpha = fixed(1) - sqr(sin_alpha);

  const fixed u_sq = cos_sq_alpha * fixed(0.006739497);
  const fixed A = (fixed(16384) + u_sq * (fixed(4096) + u_sq * (fixed(-768) +
                  u_sq * (fixed(320) - fixed(175) * u_sq)))) / fixed(16384);

  const fixed B = (u_sq * (fixed(256) + u_sq * (fixed(-128) +
                  u_sq * (fixed(74) - fixed(47) * u_sq)))) / fixed(1024);

  fixed sigma = distance / (REARTH_B * A);
  fixed sigmaP = fixed_two_pi;

  fixed sin_sigma, cos_sigma, cos_2_sigma_m;

  while (fabs(sigma - sigmaP) > fixed(1e-7)) {
    cos_2_sigma_m = cos(fixed(2) * sigma1 + sigma);
    sin_sigma = sin(sigma);
    cos_sigma = cos(sigma);

    fixed delta_sigma = B * sin_sigma * (cos_2_sigma_m + B/fixed(4) * (cos_sigma *
      (fixed(-1) + fixed(2) * sqr(cos_2_sigma_m)) - B/fixed(6) * cos_2_sigma_m *
      (fixed(-3) + fixed(4) * sqr(sin_sigma)) * (fixed(-3) + fixed(4) * sqr(cos_2_sigma_m))));

    sigmaP = sigma;
    sigma = distance / (REARTH_B * A) + delta_sigma;
  }

  const fixed tmp = sin_u1 * sin_sigma - cos_u1 * cos_sigma * cos_alpha1;
  const fixed lat2 = atan2(sin_u1 * cos_sigma + cos_u1 * sin_sigma * cos_alpha1,
    (fixed(1) - FLATTENING) * sqrt(sqr(sin_alpha) + sqr(tmp)));

  const fixed lambda = atan2(sin_sigma * sin_alpha1, cos_u1 * cos_sigma -
    sin_u1 * sin_sigma * cos_alpha1);

  const fixed C = FLATTENING/fixed(16) * cos_sq_alpha *
    (fixed(4) + FLATTENING * (fixed(4) - fixed(3) * cos_sq_alpha));

  const fixed L = lambda - (fixed(1) - C) * FLATTENING * sin_alpha *
    (sigma + C * sin_sigma * (cos_2_sigma_m + C * cos_sigma * (fixed(-1) +
    fixed(2) * sqr(cos_2_sigma_m))));

  loc_out.longitude = Angle::Radians(lon1 + L);
  loc_out.latitude = Angle::Radians(lat2);

#else
  const Angle distance_angle = EarthDistanceToAngle(distance);

  const auto scd = distance_angle.SinCos();
  const fixed sin_distance = scd.first, cos_distance = scd.second;

  const auto scb = bearing.SinCos();
  const fixed sin_bearing = scb.first, cos_bearing = scb.second;

  const auto scl = loc.latitude.SinCos();
  const fixed sin_latitude = scl.first, cos_latitude = scl.second;

  loc_out.latitude = EarthASin(SmallMult(sin_latitude, cos_distance)
                               + SmallMult(cos_latitude, sin_distance,
                                           cos_bearing));

  loc_out.longitude = loc.longitude +
    Angle::FromXY(cos_distance - SmallMult(sin_latitude,
                                           loc_out.latitude.sin()),
                  SmallMult(sin_bearing, sin_distance, cos_latitude));
#endif

  loc_out.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc_out;
}

fixed
Distance(const GeoPoint &loc1, const GeoPoint &loc2)
{
  fixed distance;
  DistanceBearing(loc1, loc2, &distance, nullptr);
  return distance;
}

Angle
Bearing(const GeoPoint &loc1, const GeoPoint &loc2)
{
  Angle bearing;
  DistanceBearing(loc1, loc2, nullptr, &bearing);
  return bearing;
}
