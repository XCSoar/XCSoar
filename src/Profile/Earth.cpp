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

#include "Profile/Profile.hpp"
#include "Geo/GeoPoint.hpp"

#include <stdlib.h>

bool
Profile::GetGeoPoint(const TCHAR *key, GeoPoint &value)
{
  const TCHAR *p = Get(key);
  if (p == NULL)
    return false;

  TCHAR *endptr;
  double longitude = _tcstod(p, &endptr);
  if (endptr == p || *endptr != _T(' ') ||
      longitude < -180.0 || longitude > 180.0)
    return false;

  p = endptr + 1;
  double latitude = _tcstod(p, &endptr);
  if (endptr == p || *endptr != _T('\0') ||
      longitude < -90.0 || longitude > 90.0)
    return false;

  value.longitude = Angle::Degrees(longitude);
  value.latitude = Angle::Degrees(latitude);
  return true;
}

void
Profile::SetGeoPoint(const TCHAR *key, const GeoPoint &value)
{
  StaticString<128> buffer;
  buffer.Format(_T("%f %f"),
                (double)value.longitude.Degrees(),
                (double)value.latitude.Degrees());
  Set(key, buffer);
}
