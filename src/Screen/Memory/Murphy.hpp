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

#ifndef XCSOAR_MURPHY_HPP
#define XCSOAR_MURPHY_HPP

#include "Bresenham.hpp"

#include <algorithm>

#include <math.h>
#include <stdint.h>

/**
 * Implementation of the Murphy line drawing algorithm.  Based on
 * code from SDL_gfx.
 */
template<typename Canvas>
struct MurphyIterator {
  Canvas &canvas;
  const typename Canvas::color_type color;
  const unsigned line_mask;
  unsigned line_mask_position;

  /* delta x , delta y */
  int u, v;

  /* loop constants */
  int ku, kt, kv, kd;

  int oct2;
  bool quad4;
  int last1x, last1y, last2x, last2y;
  int first1x, first1y, first2x, first2y;
  int tempx, tempy;

public:
  MurphyIterator(Canvas &_canvas, typename Canvas::color_type _color,
                 unsigned _line_mask, unsigned _line_mask_position)
    :canvas(_canvas), color(_color), line_mask(_line_mask),
     line_mask_position(_line_mask_position) {}

  unsigned GetLineMaskPosition() const {
    return line_mask_position + u;
  }

  void Paraline(int x, int y, int d1) {
    d1 = -d1;

    unsigned lmp = line_mask_position;
    for (int p = 0; p <= u; p++) {
      if ((lmp++ | line_mask) == unsigned(-1))
        canvas.DrawPixel(x, y, color);

      if (d1 <= kt) {
        if (oct2 == 0) {
          x++;
        } else {
          if (!quad4)
            y++;
          else
            y--;
        }
        d1 += kv;
      } else {
        x++;
        if (!quad4)
          y++;
        else
          y--;
        d1 += kd;
      }
    }

    tempx = x;
    tempy = y;
  }

  void Iteration(uint8_t miter,
                 unsigned ml1bx, unsigned ml1by,
                 unsigned ml2bx, unsigned ml2by,
                 unsigned ml1x, unsigned ml1y,
                 unsigned ml2x, unsigned ml2y) {
    if (miter > 1 && first1x != -32768) {
      const int fix = (first1x + first2x) / 2;
      const int fiy = (first1y + first2y) / 2;
      const int lax = (last1x + last2x) / 2;
      const int lay = (last1y + last2y) / 2;
      const int curx = (ml1x + ml2x) / 2;
      const int cury = (ml1y + ml2y) / 2;

      int atemp1 = (fix - curx);
      int atemp2 = (fiy - cury);
      int ftmp1 = atemp1 * atemp1 + atemp2 * atemp2;
      atemp1 = (lax - curx);
      atemp2 = (lay - cury);
      int ftmp2 = atemp1 * atemp1 + atemp2 * atemp2;

      unsigned m1x, m1y, m2x, m2y;
      if (ftmp1 <= ftmp2) {
        m1x = first1x;
        m1y = first1y;
        m2x = first2x;
        m2y = first2y;
      } else {
        m1x = last1x;
        m1y = last1y;
        m2x = last2x;
        m2y = last2y;
      }

      atemp1 = (m2x - ml2x);
      atemp2 = (m2y - ml2y);
      ftmp1 = atemp1 * atemp1 + atemp2 * atemp2;
      atemp1 = (m2x - ml2bx);
      atemp2 = (m2y - ml2by);
      ftmp2 = atemp1 * atemp1 + atemp2 * atemp2;

      if (ftmp2 >= ftmp1) {
        ftmp1 = ml2bx;
        ftmp2 = ml2by;
        ml2bx = ml2x;
        ml2by = ml2y;
        ml2x = ftmp1;
        ml2y = ftmp2;
        ftmp1 = ml1bx;
        ftmp2 = ml1by;
        ml1bx = ml1x;
        ml1by = ml1y;
        ml1x = ftmp1;
        ml1y = ftmp2;
      }

      BresenhamIterator b(m2x, m2y, m1x, m1y);
      do {
        canvas.DrawPixel(b.x, b.y, color);
      } while (b.Next());

      b = BresenhamIterator(m1x, m1y, ml1bx, ml1by);
      do {
        canvas.DrawPixel(b.x, b.y, color);
      } while (b.Next());

      b = BresenhamIterator(ml1bx, ml1by, ml2bx, ml2by);
      do {
        canvas.DrawPixel(b.x, b.y, color);
      } while (b.Next());

      b = BresenhamIterator(ml2bx, ml2by, m2x, m2y);
      do {
        canvas.DrawPixel(b.x, b.y, color);
      } while (b.Next());

      const PixelPoint p[4] = {
        { m1x, m1y },
        { m2x, m2y },
        { ml1bx, ml1by },
        { ml2bx, ml2by },
      };

      canvas.FillPolygon(p, 4, color);
    }

    last1x = ml1x;
    last1y = ml1y;
    last2x = ml2x;
    last2y = ml2y;
    first1x = ml1bx;
    first1y = ml1by;
    first2x = ml2bx;
    first2y = ml2by;
  }

  void Wideline(int x1, int y1, int x2, int y2, uint8_t width, uint8_t miter) {
    assert(x1 != x2 || y1 != y2);

    float offset = (float)width / 2.f;

    /* Initialisation */
    u = x2 - x1; /* delta x */
    v = y2 - y1; /* delta y */

    if (u < 0) {
      /* swap to make sure we are in quadrants 1 or 4 */
      std::swap(x1, x2);
      std::swap(y1, y2);
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

    if (v > u) {
      /* swap things if in 2 octant */
      std::swap(u, v);
      oct2 = 1;
    } else {
      oct2 = 0;
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
    const double ang = atan((double) v / (double) u);
    const double sang = sin(ang);
    const double cang = cos(ang);

    int ptx, pty;
    if (oct2 == 0) {
      ptx = x1 + (int)lrint(offset * sang);
      if (!quad4) {
        pty = y1 - (int)lrint(offset * cang);
      } else {
        pty = y1 + (int)lrint(offset * cang);
      }
    } else {
      ptx = x1 - (int)lrint(offset * cang);
      if (!quad4) {
        pty = y1 + (int)lrint(offset * sang);
      } else {
        pty = y1 - (int)lrint(offset * sang);
      }
    }

    /* thickness threshold: used here for constant thickness line */
    const int tk = int(4. * hypot(ptx - x1, pty - y1) * hypot(u, v));

    if (miter == 0) {
      first1x = -32768;
      first1y = -32768;
      first2x = -32768;
      first2y = -32768;
      last1x = -32768;
      last1y = -32768;
      last2x = -32768;
      last2y = -32768;
    }

    /* outer loop, stepping perpendicular to line */
    int ml1x, ml1y, ml2x, ml2y, ml1bx, ml1by, ml2bx, ml2by;
    for (unsigned q = 0; dd <= tk; q++) {

      /* call to inner loop - right edge */
      Paraline(ptx, pty, d1);
      if (q == 0) {
        ml1x = ptx;
        ml1y = pty;
        ml1bx = tempx;
        ml1by = tempy;
      } else {
        ml2x = ptx;
        ml2y = pty;
        ml2bx = tempx;
        ml2by = tempy;
      }

      if (d0 < kt) {
        /* square move */
        if (oct2 == 0) {
          if (!quad4) {
            pty++;
          } else {
            pty--;
          }
        } else {
          ptx++;
        }
      } else {
        /* diagonal move */
        dd += kv;
        d0 -= ku;
        if (d1 < kt) {
          /* normal diagonal */
          if (oct2 == 0) {
            ptx--;
            if (!quad4) {
              pty++;
            } else {
              pty--;
            }
          } else {
            ptx++;
            if (!quad4) {
              pty--;
            } else {
              pty++;
            }
          }
          d1 += kv;
        } else {
          /* double square move, extra parallel line */
          if (oct2 == 0) {
            ptx--;
          } else {
            if (!quad4) {
              pty--;
            } else {
              pty++;
            }
          }
          d1 += kd;
          if (dd > tk) {
            Iteration(miter,
                      ml1bx, ml1by, ml2bx, ml2by,
                      ml1x, ml1y, ml2x, ml2y);
            /* breakout on the extra line */
            return;
          }
          Paraline(ptx, pty, d1);
          if (oct2 == 0) {
            if (!quad4) {
              pty++;
            } else {

              pty--;
            }
          } else {
            ptx++;
          }
        }
      }
      dd += ku;
      d0 += kv;
    }

    Iteration(miter, ml1bx, ml1by, ml2bx, ml2by, ml1x, ml1y, ml2x, ml2y);
  }
};

#endif
