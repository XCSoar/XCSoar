/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "GeoPointFormatter.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Math/Angle.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdlib.h>

bool
Units::LongitudeToString(Angle longitude, TCHAR *buffer, size_t size)
{
  static gcc_constexpr_data TCHAR EW[] = _T("WEE");
  int dd, mm, ss;

  // Calculate Longitude sign
  int sign = longitude.Sign() + 1;
  double mlong(longitude.AbsoluteDegrees());

  switch (current.coordinate_format) {
  case CoordinateFormat::DDMMSS:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    mm = (int)(mlong);
    // Calculate seconds
    mlong = (mlong - mm) * 60.0;
    ss = (int)(mlong + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%03d")_T(DEG)_T("%02d'%02d\" %c"),
              dd, mm, ss, EW[sign]);
    break;

  case CoordinateFormat::DDMMSS_SS:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    mm = (int)(mlong);
    // Calculate seconds
    mlong = (mlong - mm) * 60.0;
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%03d")_T(DEG)_T("%02d'%05.2f\" %c"),
              dd, mm, mlong, EW[sign]);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%03d")_T(DEG)_T("%06.3f' %c"), dd, mlong, EW[sign]);
    break;

  case CoordinateFormat::DD_DDDD:
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%08.4f %c")_T(DEG), mlong, EW[sign]);
    break;

  default:
    return false;
  }

  return true;
}

bool
Units::LatitudeToString(Angle latitude, TCHAR *buffer, size_t size)
{
  static gcc_constexpr_data TCHAR EW[] = _T("SNN");
  int dd, mm, ss;

  // Calculate Latitude sign
  int sign = latitude.Sign() + 1;
  double mlat(latitude.AbsoluteDegrees());

  switch (current.coordinate_format) {
  case CoordinateFormat::DDMMSS:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    mm = (int)(mlat);
    // Calculate seconds
    mlat = (mlat - mm) * 60.0;
    ss = (int)(mlat + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%02d")_T(DEG)_T("%02d'%02d\" %c"),
              dd, mm, ss, EW[sign]);
    break;

  case CoordinateFormat::DDMMSS_SS:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    mm = (int)(mlat);
    // Calculate seconds
    mlat = (mlat - mm) * 60.0;
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%02d")_T(DEG)_T("%02d'%05.2f\" %c"),
              dd, mm, mlat, EW[sign]);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%02d")_T(DEG)_T("%06.3f' %c"), dd, mlat, EW[sign]);
    break;

  case CoordinateFormat::DD_DDDD:
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%07.4f %c")_T(DEG), mlat, EW[sign]);
    break;

  default:
    return false;
  }

  return true;
}

TCHAR *
Units::FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size)
{
  if (!LatitudeToString(location.latitude, buffer, size))
    return NULL;

  TCHAR *end = buffer + size, *p = buffer + _tcslen(buffer);
  if (p >= end)
    return NULL;

  *p++ = _T(' ');

  if (!LongitudeToString(location.longitude, p, end - p))
    return NULL;

  return buffer;
}
