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

/**
 * This file handles the geoid separation
 * @file Geoid.cpp
 * @see http://en.wikipedia.org/wiki/EGM96
 */

#include "Geoid.hpp"
#include "ResourceLoader.hpp"
#include "Geo/GeoPoint.hpp"

#include <tchar.h>
#include <stdint.h>

#define EGM96SIZE 16200

static const uint8_t *egm96data;

void
EGM96::Load()
{
  ResourceLoader::Data data = ResourceLoader::Load(_T("IDR_RASTER_EGM96S"),
                                                   _T("RASTERDATA"));
  assert(data.first != nullptr);
  assert(data.second == EGM96SIZE);

  egm96data = (const uint8_t *)data.first;
}

fixed
EGM96::LookupSeparation(const GeoPoint &pt)
{
  if (!egm96data)
    return fixed(0);

  int ilat, ilon;
  ilat = iround((Angle::QuarterCircle() - pt.latitude).Half().Degrees());
  ilon = iround(pt.longitude.AsBearing().Half().Degrees());

  int offset = ilat * 180 + ilon;
  if (offset >= EGM96SIZE)
    return fixed(0);
  if (offset < 0)
    return fixed(0);

  return fixed((int)egm96data[offset] - 127);
}
