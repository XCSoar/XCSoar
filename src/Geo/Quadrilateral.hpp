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

#ifndef XCSOAR_GEO_QUADRILATERAL_HPP
#define XCSOAR_GEO_QUADRILATERAL_HPP

#include "GeoPoint.hpp"

class GeoBounds;

/**
 * A quadrilateral on earth's surface.
 *
 * The four vertices describe the location of a planar rectangle
 * (e.g. a #Bitmap) on earth's surface.
 */
struct GeoQuadrilateral {
  GeoPoint top_left, top_right, bottom_left, bottom_right;

  static constexpr GeoQuadrilateral Undefined() {
    return {GeoPoint::Invalid(), GeoPoint::Invalid(),
        GeoPoint::Invalid(), GeoPoint::Invalid()};
  }

  constexpr bool IsDefined() const {
    return top_left.IsValid();
  }

  constexpr bool Check() const {
    return top_left.Check() && top_right.Check() &&
      bottom_left.Check() && bottom_right.Check();
  }

  gcc_pure
  GeoBounds GetBounds() const;
};

#endif
