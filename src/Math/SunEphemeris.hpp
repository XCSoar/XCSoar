/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef SUN_EPHEMERIS_HPP
#define SUN_EPHEMERIS_HPP

#include "Math/fixed.hpp"
#include "Compiler.h"

struct GeoPoint;
struct BrokenDateTime;
class Angle;

/**
 * Sun ephemeris model, used largely for calculations of sunset times
 */
class SunEphemeris {
  gcc_const
  static fixed FNday(const BrokenDateTime &date_time);

  gcc_const
  static Angle GetHourAngle(Angle lat, Angle declin);

  gcc_const
  static Angle GetHourAngleTwilight(Angle lat, Angle declin);

  gcc_pure
  Angle GetEclipticLongitude(fixed d, Angle L);

  gcc_pure
  Angle GetMeanSunLongitude(fixed d);

 public:
  fixed DayLength, MorningTwilight, EveningTwilight;
  fixed TimeOfNoon, TimeOfSunSet, TimeOfSunRise;

  void CalcSunTimes(const GeoPoint &location, const BrokenDateTime &date_time,
                    fixed TimeZone);
};

#endif
