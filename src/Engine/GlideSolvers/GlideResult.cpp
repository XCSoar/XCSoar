// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideResult.hpp"
#include "GlideState.hpp"

#include <cassert>

GlideResult::GlideResult(const GlideState &task, const double V) noexcept
  :head_wind(task.head_wind),
   v_opt(V),
#ifndef NDEBUG
   start_altitude(task.min_arrival_altitude + task.altitude_difference),
#endif
   min_arrival_altitude(task.min_arrival_altitude),
   vector(task.vector),
   pure_glide_min_arrival_altitude(task.min_arrival_altitude),
   pure_glide_altitude_difference(task.altitude_difference),
   altitude_difference(task.altitude_difference),
   effective_wind_speed(task.wind.norm),
   effective_wind_angle(task.effective_wind_angle),
   validity(Validity::NO_SOLUTION)
{
}

void
GlideResult::CalcDeferred() noexcept
{
  CalcCruiseBearing();
}

inline void
GlideResult::CalcCruiseBearing() noexcept
{
  if (!IsOk())
    return;

  cruise_track_bearing = vector.bearing;
  if (effective_wind_speed <= 0)
    return;

  const auto sintheta = effective_wind_angle.sin();
  if (sintheta == 0)
    return;

  cruise_track_bearing -=
    Angle::asin(sintheta * effective_wind_speed / v_opt).Half();
}

void
GlideResult::Add(const GlideResult &s2) noexcept
{
  if ((unsigned)s2.validity > (unsigned)validity)
    /* downgrade the validity */
    validity = s2.validity;

  if (!IsDefined())
    return;

  vector.distance += s2.vector.distance;

  if (!IsOk())
    /* the other attributes are not valid if validity is not OK or
       PARTIAL */
    return;

  if (s2.GetRequiredAltitudeWithDrift() < min_arrival_altitude) {
    /* must meet the safety height of the first leg */
    assert(s2.min_arrival_altitude < s2.GetArrivalAltitudeWithDrift(min_arrival_altitude));

    /* calculate a new minimum arrival height that considers the
       "mountain top" in the middle */
    min_arrival_altitude = s2.GetArrivalAltitudeWithDrift(min_arrival_altitude);
  } else {
    /* must meet the safety height of the second leg */

    /* apply the increased altitude requirement */
    altitude_difference -=
      s2.GetRequiredAltitudeWithDrift() - min_arrival_altitude;

    /* adopt the minimum height of the second leg */
    min_arrival_altitude = s2.min_arrival_altitude;
  }

  /* same as above, but for "pure glide" */

  if (s2.GetRequiredAltitude() < pure_glide_min_arrival_altitude) {
    /* must meet the safety height of the first leg */
    assert(s2.pure_glide_min_arrival_altitude <
           s2.GetArrivalAltitude(pure_glide_min_arrival_altitude));

    /* calculate a new minimum arrival height that considers the
       "mountain top" in the middle */
    pure_glide_min_arrival_altitude =
      s2.GetArrivalAltitude(pure_glide_min_arrival_altitude);
  } else {
    /* must meet the safety height of the second leg */

    /* apply the increased altitude requirement */
    pure_glide_altitude_difference -=
      s2.GetRequiredAltitude() - pure_glide_min_arrival_altitude;

    /* adopt the minimum height of the second leg */
    pure_glide_min_arrival_altitude = s2.pure_glide_min_arrival_altitude;
  }

  pure_glide_height += s2.pure_glide_height;
  time_elapsed += s2.time_elapsed;
  height_glide += s2.height_glide;
  height_climb += s2.height_climb;
  time_virtual += s2.time_virtual;
}

double
GlideResult::GlideAngleGround() const noexcept
{
  if (vector.distance > 0)
    return pure_glide_height / vector.distance;

  return 1000;
}

double
GlideResult::DestinationAngleGround() const noexcept
{
  if (vector.distance > 0)
    return (altitude_difference + pure_glide_height) / vector.distance;

  return 1000;
}

bool
GlideResult::IsFinalGlide() const noexcept
{
  return IsOk() && altitude_difference >= 0 && height_climb <= 0;
}
