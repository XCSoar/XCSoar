/*
Copyright_License {

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

#include "NMEA/Info.hpp"

void
GPSState::Reset()
{
  fix_quality = FixQuality::NO_FIX;
  fix_quality_available.Clear();
  real = false;
  simulator = false;
#if defined(ANDROID) || defined(__APPLE__)
  nonexpiring_internal_gps = false;
#endif
  satellites_used_available.Clear();
  satellite_ids_available.Clear();
  hdop = -1;
  pdop = -1;
  vdop = -1;
  replay = false;
}

void
GPSState::Expire(double now)
{
  if (fix_quality_available.Expire(now, std::chrono::seconds(5)))
    fix_quality = FixQuality::NO_FIX;

  satellites_used_available.Expire(now, std::chrono::seconds(5));
  satellite_ids_available.Expire(now, std::chrono::minutes(1));
}
