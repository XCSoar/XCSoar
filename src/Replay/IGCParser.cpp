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

#include "IGCParser.hpp"

#include <stdio.h>

bool
IGCParseFix(const char *buffer, IGCFix &fix)
{
  int DegLat, DegLon;
  int MinLat, MinLon;
  char NoS, EoW;
  int iAltitude, iPressureAltitude;
  int Hour = 0;
  int Minute = 0;
  int Second = 0;
  int lfound =
      sscanf(buffer, "B%02d%02d%02d%02d%05d%c%03d%05d%cA%05d%05d",
      &Hour, &Minute, &Second, &DegLat, &MinLat, &NoS, &DegLon,
      &MinLon, &EoW, &iPressureAltitude, &iAltitude);

  if (lfound == EOF)
    return false;

  if (lfound != 11)
    return false;

  fixed Latitude = fixed(DegLat) + fixed(MinLat) / 60000;
  if (NoS == 'S')
    Latitude *= -1;

  fixed Longitude = fixed(DegLon) + fixed(MinLon) / 60000;
  if (EoW == 'W')
    Longitude *= -1;

  fix.location.Latitude = Angle::degrees(Latitude);
  fix.location.Longitude = Angle::degrees(Longitude);

  fix.gps_altitude = fixed(iAltitude);
  fix.pressure_altitude = fixed(iPressureAltitude);

  // some loggers drop out GPS altitude, so when this happens, revert
  // to pressure altitude
  if ((iPressureAltitude != 0) && (iAltitude==0)) {
    fix.gps_altitude = fix.pressure_altitude;
  }

  fix.time = fixed(Hour * 3600 + Minute * 60 + Second);
  return true;
}
