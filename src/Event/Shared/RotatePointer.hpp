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

#ifndef XCSOAR_EVENT_ROTATE_POINTER_HPP
#define XCSOAR_EVENT_ROTATE_POINTER_HPP

#include "Screen/Point.hpp"
#include "DisplayOrientation.hpp"
#include "Compiler.h"

#include <algorithm>

/**
 * This class knows how to rotate the coordinates of a pointer device
 * (touch screen) to match the coordinates of a rotated screen.
 */
class RotatePointer {
  /**
   * Swap x and y?
   */
  bool swap = false;

  /**
   * Invert x or y?
   */
  bool invert_x = false, invert_y = false;

  /**
   * Screen dimensions in pixels.
   */
  unsigned width = 0, height = 0;

public:
  constexpr unsigned GetWidth() const {
    return width;
  }

  constexpr unsigned GetHeight() const {
    return height;
  }

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

  void SetDisplayOrientation(DisplayOrientation orientation) {
    SetSwap(AreAxesSwapped(orientation));

    switch (TranslateDefaultDisplayOrientation(orientation)) {
    case DisplayOrientation::DEFAULT:
    case DisplayOrientation::PORTRAIT:
      SetInvert(true, false);
      break;

    case DisplayOrientation::LANDSCAPE:
      SetInvert(false, false);
      break;

    case DisplayOrientation::REVERSE_PORTRAIT:
      SetInvert(false, true);
      break;

    case DisplayOrientation::REVERSE_LANDSCAPE:
      SetInvert(true, true);
      break;
    }
  }

  gcc_pure
  PixelPoint DoRelative(PixelPoint p) const {
    if (swap)
      std::swap(p.x, p.y);

    return p;
  }

  gcc_pure
  PixelPoint DoAbsolute(PixelPoint p) const {
    p = DoRelative(p);

    if (invert_x)
      p.x = width - p.x;

    if (invert_y)
      p.y = height - p.y;

    return p;
  }
};

#endif
