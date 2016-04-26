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

/**
 * This file handles the geoid separation
 * @file Geoid.cpp
 * @see http://en.wikipedia.org/wiki/EGM96
 */

#include "Geoid.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Util.hpp"

#include <stdint.h>

#define EGM96SIZE 16200

extern "C" const uint8_t egm96s_dem[];

double
EGM96::LookupSeparation(const GeoPoint &pt)
{
  int ilat, ilon;
  ilat = iround((Angle::QuarterCircle() - pt.latitude).Half().Degrees());
  ilon = iround(pt.longitude.AsBearing().Half().Degrees());

  int offset = ilat * 180 + ilon;
  if (offset >= EGM96SIZE)
    return 0;
  if (offset < 0)
    return 0;

  return (int)egm96s_dem[offset] - 127;
}
