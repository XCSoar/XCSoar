/* Copyright_License {

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
#ifndef POLYGON_INTERIOR_HPP
#define POLYGON_INTERIOR_HPP

#include "Geo/SearchPointVector.hpp"
#include "Compiler.h"

struct GeoPoint;
struct FlatGeoPoint;
class SearchPoint;

/**
 * Note that this expects the vector to be closed, that is, starting point
 * and ending point are the same
 */
gcc_pure bool
PolygonInterior(const GeoPoint &p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);

gcc_pure bool
PolygonInterior(const FlatGeoPoint &p,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end);

#endif
