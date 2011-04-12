/*
Copyright_License {

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

#include "LocalTime.hpp"
#include "Interface.hpp"
#include "NMEA/Info.hpp"
#include "Asset.hpp"

#include <windows.h>

int
TimeLocal(int localtime, int utc_offset)
{
  localtime += utc_offset;

  if (localtime < 0)
    localtime += 3600 * 24;

  return localtime;
}

int
TimeLocal(int localtime)
{
  return TimeLocal(localtime, GetUTCOffset());
}

int
DetectCurrentTime(const NMEA_INFO *Basic)
{
  int utc_time = (int)(Basic->Time);
  return TimeLocal(utc_time);
}


long
GetUTCOffset()
{
#ifdef WIN32
  if (is_altair() || !is_embedded())
    return XCSoarInterface::SettingsComputer().UTCOffset;

  long utcoffset = 0;
  // returns offset in seconds
  TIME_ZONE_INFORMATION TimeZoneInformation;
  DWORD tzi = GetTimeZoneInformation(&TimeZoneInformation);

  utcoffset = -TimeZoneInformation.Bias * 60;

  if (tzi == TIME_ZONE_ID_STANDARD) {
    utcoffset -= TimeZoneInformation.StandardBias * 60;
  }

  if (tzi == TIME_ZONE_ID_DAYLIGHT) {
    utcoffset -= TimeZoneInformation.DaylightBias * 60;
  }

  return utcoffset;
#else /* !WIN32 */
  // XXX
  return XCSoarInterface::SettingsComputer().UTCOffset;
#endif /* !WIN32 */
}

