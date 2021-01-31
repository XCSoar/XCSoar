/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

class Canvas;
class Angle;
struct PixelPoint;
struct PixelRect;

bool
Segment(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, bool horizon=false) noexcept;

bool
Annulus(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, unsigned inner_radius) noexcept;

bool
KeyHole(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, unsigned inner_radius) noexcept;

void
RoundRect(Canvas &canvas, PixelRect r, unsigned radius) noexcept;

bool
Arc(Canvas &canvas, PixelPoint center, unsigned radius,
    Angle start, Angle end) noexcept;

#endif
