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

/**
 * This file handles the geoid separation
 * @file Geoid.cpp
 * @see http://en.wikipedia.org/wiki/EGM96
 */

#include "Geoid.hpp"
#include "ResourceLoader.hpp"

#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#define EGM96SIZE 16200

unsigned char* egm96data= NULL;

/**
 * Load the EGM96 geoid resource into egm96data.
 */
void
OpenGeoid(void)
{
  ResourceLoader::Data data = ResourceLoader::Load(_T("IDR_RASTER_EGM96S"),
                                                   _T("RASTERDATA"));
  if (data.first == NULL) {
    // unable to find the resource
    egm96data = NULL;
    return;
  }

  egm96data = (unsigned char *)malloc(data.second);
  memcpy(egm96data, data.first, data.second);
}

/**
 * Clear the EGM96 from the memory
 */
void
CloseGeoid(void)
{
  if (!egm96data)
    return;

  free(egm96data);
  egm96data = NULL;
}

/**
 * Returns the geoid separation between the EGS96
 * and the WGS84 at the given latitude and longitude
 * @param lat Latitude
 * @param lon Longitude
 * @return The geoid separation
 */
fixed
LookupGeoidSeparation(const GeoPoint pt)
{
  if (!egm96data)
    return fixed_zero;

  int ilat, ilon;
  ilat = iround((Angle::Degrees(fixed_90) - pt.latitude).Half().Degrees());
  ilon = iround(pt.longitude.AsBearing().Half().Degrees());

  int offset = ilat * 180 + ilon;
  if (offset >= EGM96SIZE)
    return fixed_zero;
  if (offset < 0)
    return fixed_zero;

  return fixed((int)egm96data[offset] - 127);
}
