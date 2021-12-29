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

#ifndef XCSOAR_MURPHY_HPP
#define XCSOAR_MURPHY_HPP

#include "Bresenham.hpp"
#include "Math/Angle.hpp"

#include <math.h>
#include <array>
#include <cstdint>
#include <utility>

/**
 * Implementation of the Murphy line drawing algorithm.  Based on
 * code from SDL_gfx.
 */
template<typename Canvas>
class MurphyIterator {
  using Point = typename Canvas::Point;

  Canvas &canvas;
  const typename Canvas::color_type color;
  const unsigned line_mask;
  unsigned line_mask_position;

  /* delta x , delta y */
  int u, v;

  /* loop constants */
  int ku, kt, kv, kd;

  bool oct2;
  bool quad4;
  Point last1, last2, first1, first2;

public:
  MurphyIterator(Canvas &_canvas, typename Canvas::color_type _color,
                 unsigned _line_mask, unsigned _line_mask_position) noexcept
    :canvas(_canvas), color(_color), line_mask(_line_mask),
     line_mask_position(_line_mask_position) {}

  unsigned GetLineMaskPosition() const noexcept {
    return line_mask_position + u;
  }

private:
  Point Paraline(Point p, int d1) noexcept {
    d1 = -d1;

    unsigned lmp = line_mask_position;
    for (int i = 0; i <= u; ++i) {
      if ((lmp++ | line_mask) == unsigned(-1))
        canvas.DrawPixel(p.x, p.y, color);

      if (d1 <= kt) {
        if (!oct2) {
          p.x++;
        } else {
          if (!quad4)
            p.y++;
          else
            p.y--;
        }
        d1 += kv;
      } else {
        p.x++;
        if (!quad4)
          p.y++;
        else
          p.y--;
        d1 += kd;
      }
    }

    return p;
  }

  void DrawThinLine(Point a, Point b) noexcept {
    BresenhamIterator bresenham{a, b};
      do {
        canvas.DrawPixel(bresenham.p.x, bresenham.p.y, color);
      } while (bresenham.Next());
  }

  void Iteration(uint8_t miter,
                 Point ml1b, Point ml2b,
                 Point ml1, Point ml2) noexcept {
    if (miter > 1 && first1.x != -32768) {
      const Point fi = (first1 + first2) / 2;
      const Point la = (last1 + last2) / 2;
      const Point cur = (ml1 + ml2) / 2;

      int ftmp1 = (fi - cur).MagnitudeSquared();
      int ftmp2 = (la - cur).MagnitudeSquared();

      Point m1, m2;
      if (ftmp1 <= ftmp2) {
        m1 = first1;
        m2 = first2;
      } else {
        m1 = last1;
        m2 = last2;
      }

      ftmp1 = (m2 - ml2).MagnitudeSquared();
      ftmp2 = (m2 - ml2b).MagnitudeSquared();

      if (ftmp2 >= ftmp1) {
        std::swap(ml2, ml2b);
        std::swap(ml1, ml1b);
      }

      DrawThinLine(m2, m1);
      DrawThinLine(m1, ml1b);
      DrawThinLine(ml1b, ml2b);
      DrawThinLine(ml2b, m2);

      const auto p = std::array{
        m1,
        m2,
        ml1b,
        ml2b,
      };

      canvas.FillPolygon(p.data(), p.size(), color);
    }

    last1 = ml1;
    last2 = ml2;
    first1 = ml1b;
    first2 = ml2b;
  }

  [[gnu::pure]]
  Point InitialPoint(Point p1, double width) const noexcept {
    const auto [sang, cang] = Angle::FromXY(u, v).SinCos();
    const double offset = width * 0.5;
    const int s_offset = (int)lrint(offset * sang);
    const int c_offset = (int)lrint(offset * cang);

    Point pt;
    if (!oct2) {
      pt.x = p1.x + s_offset;
      if (!quad4) {
        pt.y = p1.y - c_offset;
      } else {
        pt.y = p1.y + c_offset;
      }
    } else {
      pt.x = p1.x - c_offset;
      if (!quad4) {
        pt.y = p1.y + s_offset;
      } else {
        pt.y = p1.y - s_offset;
      }
    }

    return pt;
  }

public:
  void Wideline(Point p1, Point p2,
                uint8_t width, uint8_t miter) noexcept {
    assert(p1 != p2);

    /* Initialisation */
    u = p2.x - p1.x; /* delta x */
    v = p2.y - p1.y; /* delta y */

    if (u < 0) {
      /* swap to make sure we are in quadrants 1 or 4 */
      std::swap(p1, p2);
      u *= -1;
      v *= -1;
    }

    if (v < 0) {
      /* swap to 1st quadrant and flag */
      v *= -1;
      quad4 = true;
    } else {
      quad4 = false;
    }

    oct2 = v > u;
    if (oct2) {
      /* swap things if in 2 octant */
      std::swap(u, v);
    }

    ku = u + u; /* change in l for square shift */
    kv = v + v; /* change in d for square shift */
    kd = kv - ku; /* change in d for diagonal shift */
    kt = u - kv; /* diag/square decision threshold */

    /* difference terms d0=perpendicular to line, d1=along line */
    int d0 = 0, d1 = 0;

    /* distance along line */
    int dd = 0;

    /* angle for initial point calculation */
    Point pt = InitialPoint(p1, width);

    /* thickness threshold: used here for constant thickness line */
    const int tk = int(4. * hypot(pt.x - p1.x, pt.y - p1.y) * hypot(u, v));

    if (miter == 0) {
      first1 = first2 = last1 = last2 = {-32768, -32768};
    }

    /* outer loop, stepping perpendicular to line */
    Point ml1, ml2, ml1b, ml2b;
    for (unsigned q = 0; dd <= tk; q++) {

      /* call to inner loop - right edge */
      const auto temp = Paraline(pt, d1);
      if (q == 0) {
        ml1 = pt;
        ml1b = temp;
      } else {
        ml2 = pt;
        ml2b = temp;
      }

      if (d0 < kt) {
        /* square move */
        if (!oct2) {
          if (!quad4) {
            pt.y++;
          } else {
            pt.y--;
          }
        } else {
          pt.x++;
        }
      } else {
        /* diagonal move */
        dd += kv;
        d0 -= ku;
        if (d1 < kt) {
          /* normal diagonal */
          if (!oct2) {
            pt.x--;
            if (!quad4) {
              pt.y++;
            } else {
              pt.y--;
            }
          } else {
            pt.x++;
            if (!quad4) {
              pt.y--;
            } else {
              pt.y++;
            }
          }
          d1 += kv;
        } else {
          /* double square move, extra parallel line */
          if (!oct2) {
            pt.x--;
          } else {
            if (!quad4) {
              pt.y--;
            } else {
              pt.y++;
            }
          }
          d1 += kd;
          if (dd > tk) {
            Iteration(miter, ml1b, ml2b, ml1, ml2);
            /* breakout on the extra line */
            return;
          }
          Paraline(pt, d1);
          if (!oct2) {
            if (!quad4) {
              pt.y++;
            } else {

              pt.y--;
            }
          } else {
            pt.x++;
          }
        }
      }
      dd += ku;
      d0 += kv;
    }

    Iteration(miter, ml1b, ml2b, ml1, ml2);
  }
};

#endif
