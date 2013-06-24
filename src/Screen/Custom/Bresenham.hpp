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

#ifndef XCSOAR_BRESENHAM_HPP
#define XCSOAR_BRESENHAM_HPP

/**
 * Implementation of the Bresenham line drawing algorithm.  Based on
 * code from SDL_gfx.
 */
class BresenhamIterator {
  int dx, dy, sx, sy, error;
  bool swapdir;
  unsigned count;

public:
  int x, y;

  BresenhamIterator() = default;

  BresenhamIterator(int x1, int y1, int x2, int y2)
    :dx(x2 - x1), dy(y2 - y1), x(x1), y(y1) {
    if (dx != 0) {
      if (dx < 0) {
        dx = -dx;
        sx = -1;
      } else {
        sx = 1;
      }
    } else {
      sx = 0;
    }

    if (dy != 0) {
      if (dy < 0) {
        dy = -dy;
        sy = -1;
      } else {
        sy = 1;
      }
    } else {
      sy = 0;
    }

    if (dy > dx) {
      std::swap(dx, dy);
      swapdir = true;
    } else {
      swapdir = false;
    }

    count = (dx<0) ? 0 : (unsigned int)dx;
    dy <<= 1;
    error = dy - dx;
    dx <<= 1;
  }

  bool Next() {
    /* last point check */
    if (count == 0)
      return false;

    while (error >= 0) {
      if (swapdir) {
        x += sx;
      } else  {
        y += sy;
      }

      error -= dx;
    }

    if (swapdir) {
      y += sy;
    } else {
      x += sx;
    }

    error += dy;
    count--;

    return count > 0;
  }
};

#endif
