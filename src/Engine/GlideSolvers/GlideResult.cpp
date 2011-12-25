/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include <math.h>
#include "Navigation/Aircraft.hpp"

GlideResult::GlideResult(const GlideState &task, const fixed V):
  head_wind(task.head_wind),
  v_opt(V),
#ifndef NDEBUG
  start_altitude(task.min_height + task.altitude_difference),
#endif
  min_height(task.min_height),
  vector(task.vector),
  distance_to_final(task.vector.distance),
  altitude_difference(task.altitude_difference),
  effective_wind_speed(task.wind.norm),
  effective_wind_angle(task.effective_wind_angle),
  validity(Validity::RESULT_NOSOLUTION)
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
  cruise_track_bearing = vector.bearing;
  if (!positive(effective_wind_speed))
    return;

  const fixed sintheta = effective_wind_angle.sin();
  if (sintheta == fixed_zero)
    return;

  cruise_track_bearing -=
    Angle::Radians(half(asin(sintheta * effective_wind_speed / v_opt)));
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

  if (s2.GetRequiredAltitude() < min_height) {
    /* must meet the safety height of the first leg */
    assert(s2.min_height < s2.GetArrivalAltitude(min_height));

    /* calculate a new minimum arrival height that considers the
       "mountain top" in the middle */
    min_height = s2.GetArrivalAltitude(min_height);
  } else {
    /* must meet the safety height of the second leg */

    /* apply the increased altitude requirement */
    altitude_difference -= s2.GetRequiredAltitude() - min_height;

    /* adopt the minimum height of the second leg */
    min_height = s2.min_height;
  }

  time_elapsed += s2.time_elapsed;
  height_glide += s2.height_glide;
  height_climb += s2.height_climb;
  distance_to_final += s2.distance_to_final;
  time_virtual += s2.time_virtual;
}

#define fixed_bignum fixed_int_constant(1000000) // error condition

fixed
GlideResult::CalcVInvSpeed(const fixed inv_mc)
{
  if (!IsOk()) {
    time_virtual = fixed_zero;
    return fixed_bignum;
  }

  if (vector.distance < fixed_one) {
    time_virtual = fixed_zero;
    return fixed_zero;
  }

  if (!positive(inv_mc)) {
    time_virtual = fixed_zero;
    // minimise 1.0/LD over ground
    return height_glide / vector.distance;
  }

  // equivalent time to gain the height that was used
  time_virtual = height_glide * inv_mc;
  return (time_elapsed + time_virtual) / vector.distance;
}

fixed
GlideResult::GlideAngleGround() const
{
  if (positive(vector.distance))
    return height_glide / vector.distance;

  return fixed_int_constant(1000);
}

fixed
GlideResult::DestinationAngleGround() const
{
  if (positive(vector.distance))
    return (altitude_difference+height_glide) / vector.distance;

  return fixed_int_constant(1000);
}

bool 
GlideResult::IsFinalGlide() const 
{
  return IsOk() && !negative(altitude_difference) && !positive(height_climb);
}

GeoPoint 
GlideResult::FinalGlideStartLocation(const GeoPoint &location) const
{
  return vector.IntermediatePoint(location, distance_to_final);
}

void
GlideResult::Reset()
{
  validity = Validity::RESULT_NOSOLUTION;
}
