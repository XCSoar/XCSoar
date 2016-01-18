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

/** Library for calculating on-screen coordinates */

#ifndef XCSOAR_MATH_SCREEN_HPP
#define XCSOAR_MATH_SCREEN_HPP

#include "Compiler.h"

struct BulkPixelPoint;
struct PixelPoint;
class Angle;

gcc_pure
PixelPoint
ScreenClosestPoint(const PixelPoint &p1, const PixelPoint &p2,
                   const PixelPoint &p3, int offset);

/**
 * Shifts, rotates and scales the given polygon.
 *
 * @param poly Points specifying the polygon
 * @param n Number of points of the polygon
 * @param shift The polygon is placed with position (0,0) centered here.
 * @param angle Angle of rotation
 * @param scale An input polygon with coordinates in the range -50 to +50
 *        is scaled to fill a square with the size of the 'scale' argument.
 *        (The scale value 100 preserves the size of the input polygon.)
 *        For best scaling precision, avoid 'scale' values smaller than
 *        the intended size of the polygon.
 * @param use_fast_scale If true, additional scaling via FastScale()
 *        will be applied. This flag is only intended for backwards
 *        compatibility as the use of FastScale() is deprecated.
 */
void
PolygonRotateShift(BulkPixelPoint *poly, int n, PixelPoint shift,
                   Angle angle, int scale = 100,
                   bool use_fast_scale = true);

#endif
