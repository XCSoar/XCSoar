/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Math/NavFunctions.hpp"
#include "Navigation/Aircraft.hpp"

GlideResult::GlideResult():
    Vector(fixed_zero,fixed_zero),
    DistanceToFinal(fixed_zero),
    CruiseTrackBearing(fixed_zero),
    VOpt(fixed_zero),
    HeightClimb(fixed_zero),
    HeightGlide(fixed_zero),
    TimeElapsed(fixed_zero),
    TimeVirtual(fixed_zero),
    AltitudeDifference(fixed_zero),
    EffectiveWindSpeed(fixed_zero),
    EffectiveWindAngle(fixed_zero),
    Solution(RESULT_NOSOLUTION)
{
  // default is null result
}

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
    Solution(RESULT_NOSOLUTION)
{
}

void
GlideResult::calc_deferred(const AIRCRAFT_STATE& state)
{
  AltitudeRequired = AltitudeDifference + state.NavAltitude;
  calc_cruise_bearing();
}

void
GlideResult::calc_cruise_bearing()
{
  CruiseTrackBearing = Vector.Bearing;
  if (!positive(EffectiveWindSpeed))
    return;

  const fixed sintheta = sin(fixed_deg_to_rad * EffectiveWindAngle);
  if (sintheta == fixed_zero)
    return;

  // Wn/sin(alpha) = V/sin(theta)
  // (Wn/V)*sin(theta) = sin(alpha)
  CruiseTrackBearing -= fixed_half * fixed_rad_to_deg *
      asin(sintheta * EffectiveWindSpeed / VOpt);
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

  if (negative(AltitudeDifference) || negative(s2.AltitudeDifference)) {
    AltitudeDifference = min(s2.AltitudeDifference + AltitudeDifference,
        AltitudeDifference);
  } else {
    AltitudeDifference = min(s2.AltitudeDifference, AltitudeDifference);
  }
}

static const fixed fixed_bignum(1e6); // error condition

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
  static const fixed fixed_100;

  if (positive(Vector.Distance)) {
    return HeightGlide / Vector.Distance;
  } else {
    return fixed_100;
  }
}

bool
GlideResult::glide_reachable(const bool final_glide) const
{
  if (final_glide) {
    return (Solution == RESULT_OK)
           && positive(AltitudeDifference)
           && !positive(HeightClimb);
  } else {
    return (Solution == RESULT_OK);
  }
}

bool 
GlideResult::is_final_glide() const 
{
  return (Solution == RESULT_OK) && !positive(DistanceToFinal);
}
