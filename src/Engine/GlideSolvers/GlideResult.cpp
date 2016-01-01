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

#include "GlideResult.hpp"
#include "GlideState.hpp"

#include <assert.h>

GlideResult::GlideResult(const GlideState &task, const double V)
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
GlideResult::CalcDeferred()
{
  CalcCruiseBearing();
}

void
GlideResult::CalcCruiseBearing()
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
GlideResult::Add(const GlideResult &s2)
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
GlideResult::GlideAngleGround() const
{
  if (vector.distance > 0)
    return pure_glide_height / vector.distance;

  return 1000;
}

double
GlideResult::DestinationAngleGround() const
{
  if (vector.distance > 0)
    return (altitude_difference + pure_glide_height) / vector.distance;

  return 1000;
}

bool
GlideResult::IsFinalGlide() const
{
  return IsOk() && altitude_difference >= 0 && height_climb <= 0;
}

void
GlideResult::Reset()
{
  validity = Validity::NO_SOLUTION;
}
