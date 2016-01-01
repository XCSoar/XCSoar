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

#ifndef TOPOGRAPHY_CONVERT_HPP
#define TOPOGRAPHY_CONVERT_HPP

#include "shapelib/mapprimitive.h"

static inline constexpr GeoBounds
ImportRect(const rectObj r)
{
  return GeoBounds(GeoPoint(Angle::Degrees(r.minx),
                            Angle::Degrees(r.maxy)),
                   GeoPoint(Angle::Degrees(r.maxx),
                            Angle::Degrees(r.miny)));
}

gcc_pure
static inline rectObj
ConvertRect(const GeoBounds &br)
{
  rectObj dest;
  dest.minx = br.GetWest().Degrees();
  dest.maxx = br.GetEast().Degrees();
  dest.miny = br.GetSouth().Degrees();
  dest.maxy = br.GetNorth().Degrees();
  return dest;
}

#endif
