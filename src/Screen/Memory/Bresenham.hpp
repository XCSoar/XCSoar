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

#ifndef XCSOAR_BRESENHAM_HPP
#define XCSOAR_BRESENHAM_HPP

/**
 * Implementation of the Bresenham line drawing algorithm.  Based on
 * code from SDL_gfx.
 */
class BresenhamIterator {
  int dx, dy, sx, sy, error;
  bool swapdir;
  int x_e, y_e;

public:
  int x, y;
  unsigned count;

  BresenhamIterator() = default;

  BresenhamIterator(int x1, int y1, int x2, int y2)
    :dx(x2 - x1), dy(y2 - y1), x_e(x2), y_e(y2), x(x1), y(y1) {
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

    if (swapdir) {
      y += sy;
      while (error >= 0) {
        x+= sx;
        error -= dx;
      }
    } else {
      x += sx;
      while (error >= 0) {
        y+= sy;
        error -= dx;
      }
    }

    error += dy;
    count--;

    return count > 0;
  }

  bool AdvanceTo(const int y_t) {
    if (count == 0)
      return false; // past end point

    if (y == y_t) // newly visible, no need to advance
      return true;

    if (y > y_t)
      return false; // not yet visible

    if (swapdir) {

      while ((y< y_t) && count) {
        y += sy;
        while (error >= 0) {
          x+= sx;
          error -= dx;
        }
        error += dy;
        count--;
      }
    } else {

      while ((y< y_t) && count) {
        x += sx;
        while (error >= 0) {
          y+= sy;
          error -= dx;
        }
        error += dy;
        count--;
      }

    }

    if (y==y_e) {
      x= x_e;
      count = 0;
    }

    // true if reach end point on this iteration
    return count == 0;
  }

  static bool CompareHorizontal(const BresenhamIterator& i1,
                                const BresenhamIterator& i2) {
    if ((i2.count>0) && (i1.count==0))
      return true; // shift finished lines to left
    if ((i1.count>0) && (i2.count==0))
      return false; // shift finished lines to left
    
    if (i1.x < i2.x)
      return true;
    if (i2.x < i1.x)
      return false;
    
    return false; // TODO slope difference
  }

  static bool CompareVerticalHorizontal(const BresenhamIterator& i1,
                                        const BresenhamIterator& i2) {
    if (i1.y < i2.y) 
      return true;
    if (i1.y > i2.y)
      return false;
    return CompareHorizontal(i1, i2);
  }

};

#endif
