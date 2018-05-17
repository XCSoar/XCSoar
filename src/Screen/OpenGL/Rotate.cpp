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

#include "Rotate.hpp"
#include "Features.hpp"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "Globals.hpp"
#include "Screen/Point.hpp"
#include "DisplayOrientation.hpp"

#include <algorithm>

void
OpenGL::ToViewport(PixelRect &rc)
{
  rc.left += translate.x;
  rc.top += translate.y;
  rc.right += translate.x;
  rc.bottom += translate.y;

  switch (display_orientation) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::LANDSCAPE:
    rc.top = viewport_size.y - rc.top;
    rc.bottom = viewport_size.y - rc.bottom;
    std::swap(rc.top, rc.bottom);
    break;

  case DisplayOrientation::PORTRAIT:
    std::swap(rc.left, rc.top);
    std::swap(rc.right, rc.bottom);
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    rc.left = viewport_size.x - rc.left;
    rc.right = viewport_size.x - rc.right;
    std::swap(rc.left, rc.right);
    break;

  case DisplayOrientation::REVERSE_PORTRAIT:
    rc.top = viewport_size.y - rc.top;
    rc.bottom = viewport_size.y - rc.bottom;
    rc.left = viewport_size.x - rc.left;
    rc.right = viewport_size.x - rc.right;
    std::swap(rc.left, rc.right);
    std::swap(rc.top, rc.bottom);
    std::swap(rc.left, rc.top);
    std::swap(rc.right, rc.bottom);
    break;
  }
}

#endif
