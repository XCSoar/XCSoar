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

#ifndef XCSOAR_SCREEN_GDI_BULK_POINT_HPP
#define XCSOAR_SCREEN_GDI_BULK_POINT_HPP

#include "Screen/Point.hpp"

#include <windows.h>

/**
 * A point structure to be used in arrays.
 */
struct BulkPixelPoint : public tagPOINT {
  BulkPixelPoint() = default;

  constexpr BulkPixelPoint(LONG _x, LONG _y)
    :tagPOINT({_x, _y}) {}

  explicit constexpr BulkPixelPoint(const POINT &other):tagPOINT(other) {}

  constexpr BulkPixelPoint(PixelPoint src)
    :tagPOINT({src.x, src.y}) {}

  constexpr operator PixelPoint() const {
    return PixelPoint(x, y);
  }

  constexpr BulkPixelPoint operator+(BulkPixelPoint other) const {
    return { x + other.x, y + other.y };
  }

  constexpr BulkPixelPoint operator-(BulkPixelPoint other) const {
    return { x - other.x, y - other.y };
  }
};

#endif
