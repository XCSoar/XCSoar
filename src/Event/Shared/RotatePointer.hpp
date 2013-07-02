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

#ifndef XCSOAR_EVENT_ROTATE_POINTER_HPP
#define XCSOAR_EVENT_ROTATE_POINTER_HPP

#include <algorithm>

/**
 * This class knows how to rotate the coordinates of a pointer device
 * (touch screen) to match the coordinates of a rotated screen.
 */
class RotatePointer {
  /**
   * Swap x and y?
   */
  bool swap;

  /**
   * Invert x or y?
   */
  bool invert_x, invert_y;

  /**
   * Screen dimensions in pixels.
   */
  unsigned width, height;

public:
  void SetSize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
  }

  void SetSwap(bool _swap) {
    swap = _swap;
  }

  void SetInvert(bool _invert_x, bool _invert_y) {
    invert_x = _invert_x;
    invert_y = _invert_y;
  }

  void Do(RasterPoint &p) {
    if (swap)
      std::swap(p.x, p.y);

    if (invert_x)
      p.x = width - p.x;

    if (invert_y)
      p.y = height - p.y;
  }
};

#endif
