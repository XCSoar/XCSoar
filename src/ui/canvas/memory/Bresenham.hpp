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

#ifndef XCSOAR_BRESENHAM_HPP
#define XCSOAR_BRESENHAM_HPP

#include "Math/Point2D.hpp"

#include <utility>

/**
 * Implementation of the Bresenham line drawing algorithm.  Based on
 * code from SDL_gfx.
 */
class BresenhamIterator {
  IntPoint2D d, s;
  int error;
  bool swapdir;
  IntPoint2D e;

public:
  IntPoint2D p;
  unsigned count;

  BresenhamIterator() noexcept = default;

  constexpr BresenhamIterator(IntPoint2D p1, IntPoint2D p2) noexcept
    :d(p2 - p1), e(p2), p(p1) {
    if (d.x != 0) {
      if (d.x < 0) {
        d.x = -d.x;
        s.x = -1;
      } else {
        s.x = 1;
      }
    } else {
      s.x = 0;
    }

    if (d.y != 0) {
      if (d.y < 0) {
        d.y = -d.y;
        s.y = -1;
      } else {
        s.y = 1;
      }
    } else {
      s.y = 0;
    }

    if (d.y > d.x) {
      std::swap(d.x, d.y);
      swapdir = true;
    } else {
      swapdir = false;
    }

    count = std::max(0, d.x);
    d.y <<= 1;
    error = d.y - d.x;
    d.x <<= 1;
  }

  constexpr bool Next() noexcept {
    /* last point check */
    if (count == 0)
      return false;

    if (swapdir) {
      p.y += s.y;
      while (error >= 0) {
        p.x += s.x;
        error -= d.x;
      }
    } else {
      p.x += s.x;
      while (error >= 0) {
        p.y += s.y;
        error -= d.x;
      }
    }

    error += d.y;
    count--;

    return count > 0;
  }

  constexpr bool AdvanceTo(const int y_t) noexcept {
    if (count == 0)
      return false; // past end point

    if (p.y == y_t) // newly visible, no need to advance
      return true;

    if (p.y > y_t)
      return false; // not yet visible

    if (swapdir) {

      while (p.y < y_t && count) {
        p.y += s.y;
        while (error >= 0) {
          p.x += s.x;
          error -= d.x;
        }
        error += d.y;
        count--;
      }
    } else {

      while (p.y < y_t && count) {
        p.x += s.x;
        while (error >= 0) {
          p.y += s.y;
          error -= d.x;
        }
        error += d.y;
        count--;
      }

    }

    if (p.y == e.y) {
      p.x = e.x;
      count = 0;
    }

    // true if reach end point on this iteration
    return count == 0;
  }

  static constexpr bool
  CompareHorizontal(const BresenhamIterator &i1,
                    const BresenhamIterator &i2) noexcept
  {
    if ((i2.count>0) && (i1.count==0))
      return true; // shift finished lines to left
    if ((i1.count>0) && (i2.count==0))
      return false; // shift finished lines to left

    if (i1.p.x < i2.p.x)
      return true;
    if (i2.p.x < i1.p.x)
      return false;

    return false; // TODO slope difference
  }

  static constexpr bool
  CompareVerticalHorizontal(const BresenhamIterator &i1,
                            const BresenhamIterator &i2) noexcept
  {
    if (i1.p.y < i2.p.y)
      return true;
    if (i1.p.y > i2.p.y)
      return false;
    return CompareHorizontal(i1, i2);
  }

};

#endif
