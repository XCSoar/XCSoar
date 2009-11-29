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

GlideResult::GlideResult(const GlideState &task, 
                           const double V):
    Vector(task.Vector),
    DistanceToFinal(task.Vector.Distance),
    CruiseTrackBearing(task.Vector.Bearing),
    VOpt(V),
    HeightClimb(0.0),
    HeightGlide(0.0),
    TimeElapsed(0.0),
    TimeVirtual(0.0),
    AltitudeDifference(task.AltitudeDifference),
    EffectiveWindSpeed(task.EffectiveWindSpeed),
    EffectiveWindAngle(task.EffectiveWindAngle),
    Solution(RESULT_NOSOLUTION)
{
}


void
GlideResult::calc_cruise_bearing()
{
  CruiseTrackBearing= Vector.Bearing;
  if (EffectiveWindSpeed>0.0) {
    const double sintheta = sin(DEG_TO_RAD*EffectiveWindAngle);
    if (sintheta==0.0) {
      return;
    }
    // Wn/sin(alpha) = V/sin(theta)
    //   (Wn/V)*sin(theta) = sin(alpha)
    CruiseTrackBearing -= 0.5*RAD_TO_DEG*asin(sintheta*EffectiveWindSpeed/VOpt);
  }
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

  if ((AltitudeDifference<0) || (s2.AltitudeDifference<0)) {
    AltitudeDifference= std::min(s2.AltitudeDifference+AltitudeDifference,
      AltitudeDifference);
  } else {
    AltitudeDifference= std::min(s2.AltitudeDifference, AltitudeDifference);
  }
}


double 
GlideResult::calc_vspeed(const double mc) 
{
  if (!ok_or_partial()) {
    TimeVirtual = 0.0;
    return 1.0e6;
  }
  if (Vector.Distance>0.0) {
    if (mc>0.0) {
      // equivalent time to gain the height that was used
      TimeVirtual = HeightGlide/mc;
      return (TimeElapsed+TimeVirtual)/Vector.Distance;
    } else {
      TimeVirtual = 0.0;
      // minimise 1.0/LD over ground 
      return -HeightGlide/Vector.Distance;
    }
  } else {
    TimeVirtual = 0.0;
    return 0.0;
  }
}

double 
GlideResult::glide_angle_ground() const
{
  if (Vector.Distance>0) {
    return HeightGlide/Vector.Distance;
  } else {
    return 100.0;
  }
}

