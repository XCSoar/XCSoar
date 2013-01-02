/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Math/Angle.hpp"
#include "Util/StringUtil.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/UTM.hpp"

#include <stdio.h>
#include <stdlib.h>

bool
FormatLongitude(Angle longitude, TCHAR *buffer, size_t size,
                CoordinateFormat format)
{
  int dd, mm, ss;

  // Calculate Longitude sign
  TCHAR sign = negative(longitude.Native()) ? _T('W') : _T('E');

  double mlong(longitude.AbsoluteDegrees());

  switch (format) {
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
              dd, mm, ss, sign);
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
              dd, mm, mlong, sign);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%03d")_T(DEG)_T("%06.3f' %c"), dd, mlong, sign);
    break;

  case CoordinateFormat::DD_DDDD:
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%08.4f" DEG " %c"), mlong, sign);
    break;

  case CoordinateFormat::UTM:
    return false;
  }

  return true;
}

bool
FormatLatitude(Angle latitude, TCHAR *buffer, size_t size,
               CoordinateFormat format)
{
  int dd, mm, ss;

  // Calculate Latitude sign
  TCHAR sign = negative(latitude.Native()) ? _T('S') : _T('N');

  double mlat(latitude.AbsoluteDegrees());

  switch (format) {
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
              dd, mm, ss, sign);
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
              dd, mm, mlat, sign);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%02d")_T(DEG)_T("%06.3f' %c"), dd, mlat, sign);
    break;

  case CoordinateFormat::DD_DDDD:
    // Save the string to the buffer
    _sntprintf(buffer, size, _T("%07.4f" DEG " %c"), mlat, sign);
    break;

  case CoordinateFormat::UTM:
    return false;
  }

  return true;
}

static TCHAR *
FormatUTM(const GeoPoint &location, TCHAR *buffer, size_t size,
          TCHAR seperator = _T(' '))
{
  UTM utm = UTM::FromGeoPoint(location);
  _sntprintf(buffer, size, _T("%u%c%c%.0f%c%.0f"), utm.zone_number,
             utm.zone_letter, seperator, (double)utm.easting, seperator,
             (double)utm.northing);
  return buffer;
}

TCHAR *
FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size,
               CoordinateFormat format, TCHAR seperator)
{
  if (format == CoordinateFormat::UTM)
    return FormatUTM(location, buffer, size, seperator);

  if (!FormatLatitude(location.latitude, buffer, size, format))
    return NULL;

  TCHAR *end = buffer + size, *p = buffer + _tcslen(buffer);
  if (p >= end)
    return NULL;

  *p++ = seperator;

  if (!FormatLongitude(location.longitude, p, end - p, format))
    return NULL;

  return buffer;
}
