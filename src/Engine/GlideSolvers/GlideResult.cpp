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
  Vector(task.Vector),
  DistanceToFinal(task.Vector.Distance),
  CruiseTrackBearing(task.Vector.Bearing),
  VOpt(V),
  HeightClimb(fixed_zero),
  HeightGlide(fixed_zero),
  TimeElapsed(fixed_zero),
  TimeVirtual(fixed_zero),
  AltitudeDifference(task.AltitudeDifference),
  AltitudeRequired(task.AltitudeDifference),
  EffectiveWindSpeed(task.EffectiveWindSpeed),
  EffectiveWindAngle(task.EffectiveWindAngle),
  HeadWind(task.HeadWind),
  Solution(RESULT_NOSOLUTION),
  MinHeight(task.MinHeight)
{
}

void
GlideResult::calc_deferred(const AIRCRAFT_STATE& state)
{
  AltitudeRequired = state.NavAltitude - AltitudeDifference;
  calc_cruise_bearing();
}

void
GlideResult::calc_cruise_bearing()
{
  CruiseTrackBearing = Vector.Bearing;
  if (!positive(EffectiveWindSpeed))
    return;

  const fixed sintheta = EffectiveWindAngle.sin();
  if (sintheta == fixed_zero)
    return;

  CruiseTrackBearing -=
    Angle::radians(half(asin(sintheta * EffectiveWindSpeed / VOpt)));
}

void
GlideResult::add(const GlideResult &s2) 
{
  TimeElapsed += s2.TimeElapsed;
  HeightGlide += s2.HeightGlide;
  HeightClimb += s2.HeightClimb;
  Vector.Distance += s2.Vector.Distance;
  DistanceToFinal += s2.DistanceToFinal;
  TimeVirtual += s2.TimeVirtual;

  if (negative(AltitudeDifference) || negative(s2.AltitudeDifference))
    AltitudeDifference =
        min(s2.AltitudeDifference + AltitudeDifference, AltitudeDifference);
  else
    AltitudeDifference = min(s2.AltitudeDifference, AltitudeDifference);
}

#define fixed_bignum fixed_int_constant(1000000) // error condition

fixed
GlideResult::calc_vspeed(const fixed inv_mc)
{
  if (!ok_or_partial()) {
    TimeVirtual = fixed_zero;
    return fixed_bignum;
  }

  if (Vector.Distance < fixed_one) {
    TimeVirtual = fixed_zero;
    return fixed_zero;
  }

  if (!positive(inv_mc)) {
    TimeVirtual = fixed_zero;
    // minimise 1.0/LD over ground
    return HeightGlide / Vector.Distance;
  }

  // equivalent time to gain the height that was used
  TimeVirtual = HeightGlide * inv_mc;
  return (TimeElapsed + TimeVirtual) / Vector.Distance;
}

fixed
GlideResult::glide_angle_ground() const
{
  if (positive(Vector.Distance))
    return HeightGlide / Vector.Distance;

  return fixed_int_constant(1000);
}

fixed
GlideResult::destination_angle_ground() const
{
  if (positive(Vector.Distance))
    return (AltitudeDifference+HeightGlide) / Vector.Distance;

  return fixed_int_constant(1000);
}

bool
GlideResult::glide_reachable(const bool final_glide) const
{
  if (final_glide)
    return (Solution == RESULT_OK)
            && positive(AltitudeDifference)
            && !positive(HeightClimb);

  return (Solution == RESULT_OK);
}

bool 
GlideResult::is_final_glide() const 
{
  return (Solution == RESULT_OK) && !positive(DistanceToFinal);
}

GeoPoint 
GlideResult::location_at_final(const GeoPoint &location) const
{
  return Vector.intermediate_point(location, DistanceToFinal);
}

void
GlideResult::reset()
{
  Solution = RESULT_NOSOLUTION;
}
