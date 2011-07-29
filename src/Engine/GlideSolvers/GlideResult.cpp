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
  vector(task.vector),
  distance_to_final(task.vector.Distance),
  cruise_track_bearing(task.vector.Bearing),
  v_opt(V),
  height_climb(fixed_zero),
  height_glide(fixed_zero),
  time_elapsed(fixed_zero),
  time_virtual(fixed_zero),
  altitude_difference(task.altitude_difference),
  altitude_required(task.altitude_difference),
  effective_wind_speed(task.wind_speed),
  effective_wind_angle(task.effective_wind_angle),
  head_wind(task.head_wind),
  validity(RESULT_NOSOLUTION),
  min_height(task.min_height)
{
}

void
GlideResult::CalcDeferred(const AircraftState& state)
{
  altitude_required = state.altitude - altitude_difference;
  CalcCruiseBearing();
}

void
GlideResult::CalcCruiseBearing()
{
  cruise_track_bearing = vector.Bearing;
  if (!positive(effective_wind_speed))
    return;

  const fixed sintheta = effective_wind_angle.sin();
  if (sintheta == fixed_zero)
    return;

  cruise_track_bearing -=
    Angle::radians(half(asin(sintheta * effective_wind_speed / v_opt)));
}

void
GlideResult::Add(const GlideResult &s2) 
{
  time_elapsed += s2.time_elapsed;
  height_glide += s2.height_glide;
  height_climb += s2.height_climb;
  vector.Distance += s2.vector.Distance;
  distance_to_final += s2.distance_to_final;
  time_virtual += s2.time_virtual;

  if (negative(altitude_difference) || negative(s2.altitude_difference))
    altitude_difference =
        min(s2.altitude_difference + altitude_difference, altitude_difference);
  else
    altitude_difference = min(s2.altitude_difference, altitude_difference);
}

#define fixed_bignum fixed_int_constant(1000000) // error condition

fixed
GlideResult::CalcVSpeed(const fixed inv_mc)
{
  if (!IsOkOrPartial()) {
    time_virtual = fixed_zero;
    return fixed_bignum;
  }

  if (vector.Distance < fixed_one) {
    time_virtual = fixed_zero;
    return fixed_zero;
  }

  if (!positive(inv_mc)) {
    time_virtual = fixed_zero;
    // minimise 1.0/LD over ground
    return height_glide / vector.Distance;
  }

  // equivalent time to gain the height that was used
  time_virtual = height_glide * inv_mc;
  return (time_elapsed + time_virtual) / vector.Distance;
}

fixed
GlideResult::GlideAngleGround() const
{
  if (positive(vector.Distance))
    return height_glide / vector.Distance;

  return fixed_int_constant(1000);
}

fixed
GlideResult::DestinationAngleGround() const
{
  if (positive(vector.Distance))
    return (altitude_difference+height_glide) / vector.Distance;

  return fixed_int_constant(1000);
}

bool
GlideResult::IsAchievable(const bool final_glide) const
{
  if (final_glide)
    return (validity == RESULT_OK)
            && positive(altitude_difference)
            && !positive(height_climb);

  return (validity == RESULT_OK);
}

bool 
GlideResult::IsFinalGlide() const 
{
  return (validity == RESULT_OK) && !positive(distance_to_final);
}

GeoPoint 
GlideResult::FinalGlideStartLocation(const GeoPoint &location) const
{
  return vector.intermediate_point(location, distance_to_final);
}

void
GlideResult::Reset()
{
  validity = RESULT_NOSOLUTION;
}
