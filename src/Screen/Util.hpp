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
 * @file
 * @brief Small Windows GDI helper functions
 */

#ifndef XCSOAR_SCREEN_UTIL_HPP
#define XCSOAR_SCREEN_UTIL_HPP

#include "Math/Angle.hpp"
#include "Screen/Point.hpp"

class Canvas;

bool
Segment(Canvas &canvas, int x, int y, unsigned radius,
        Angle start, Angle end, bool horizon=false);

bool
Annulus(Canvas &canvas, int x, int y, unsigned radius,
        Angle start, Angle end, unsigned inner_radius);

bool
KeyHole(Canvas &canvas, int x, int y, unsigned radius,
        Angle start, Angle end, unsigned inner_radius);

void
RoundRect(Canvas &canvas, int left, int top,
          int right, int bottom, unsigned radius);

#endif
