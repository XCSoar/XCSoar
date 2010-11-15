/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Math/Screen.hpp"
#include "Math/FastMath.h"
#include "Screen/Layout.hpp"
#include "Screen/Point.hpp"

#include <math.h>

// note these use static vars! not thread-safe

void
ScreenClosestPoint(const RasterPoint &p1, const RasterPoint &p2,
                   const RasterPoint &p3, RasterPoint *p4, int offset)
{
  int v12x, v12y, v13x, v13y;

  v12x = p2.x - p1.x;
  v12y = p2.y - p1.y;
  v13x = p3.x - p1.x;
  v13y = p3.y - p1.y;

  int mag12 = isqrt4(v12x * v12x + v12y * v12y);
  if (mag12 > 1) {
    // projection of v13 along v12 = v12.v13/|v12|
    int proj = (v12x * v13x + v12y * v13y) / mag12;
    // fractional distance
    double f;
    if (offset > 0) {
      if (offset * 2 < mag12) {
        proj = max(0, min(proj, mag12));
        proj = max(offset, min(mag12 - offset, proj + offset));
      } else {
        proj = mag12 / 2;
      }
    }
    f = min(1.0, max(0.0, (double)proj / mag12));

    // location of 'closest' point
    p4->x = lround(v12x * f) + p1.x;
    p4->y = lround(v12y * f) + p1.y;
  } else {
    p4->x = p1.x;
    p4->y = p1.y;
  }
}

/**
 * Shifts and rotates the given polygon and also sizes it via FastScale()
 * @param poly Points specifying the polygon
 * @param n Number of points of the polygon
 * @param xs Pixels to shift in the x-direction
 * @param ys Pixels to shift in the y-direction
 * @param angle Angle of rotation
 */
void
PolygonRotateShift(RasterPoint *poly, const int n, const int xs, const int ys,
    Angle angle)
{
  static Angle lastangle = Angle::native(-fixed_one);
  static int cost = 1024, sint = 0;
  angle = angle.as_bearing();

  if (angle != lastangle) {
    lastangle = angle;
    cost = Layout::FastScale(angle.ifastcosine());
    sint = Layout::FastScale(angle.ifastsine());
  }

  const int xxs = xs * 1024 + 512;
  const int yys = ys * 1024 + 512;
  RasterPoint *p = poly;
  const RasterPoint *pe = poly + n;

  while (p < pe) {
    int x = p->x;
    int y = p->y;
    p->x = (x * cost - y * sint + xxs) / 1024;
    p->y = (y * cost + x * sint + yys) / 1024;
    p++;
  }
}

