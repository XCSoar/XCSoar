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

#include "Math/Screen.hpp"
#include "Math/Angle.hpp"
#include "Math/FastMath.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Point.hpp"
#include "Screen/BulkPoint.hpp"
#include "Util/Clamp.hpp"

#include <algorithm>

PixelPoint
ScreenClosestPoint(const PixelPoint &p1, const PixelPoint &p2,
                   const PixelPoint &p3, int offset)
{
  const PixelPoint v12 = p2 - p1;
  const PixelPoint v13 = p3 - p1;

  const int mag = v12.MagnitudeSquared();
  if (mag > 1) {
    const int mag12 = isqrt4(mag);
    // projection of v13 along v12 = v12.v13/|v12|
    int proj = DotProduct(v12, v13) / mag12;
    // fractional distance
    if (offset > 0) {
      if (offset * 2 < mag12) {
        proj = std::max(0, std::min(proj, mag12));
        proj = std::max(offset, std::min(mag12 - offset, proj + offset));
      } else {
        proj = mag12 / 2;
      }
    }

    const auto f = Clamp(double(proj) / mag12, 0., 1.);
    // location of 'closest' point
    return PixelPoint(lround(v12.x * f) + p1.x,
                      lround(v12.y * f) + p1.y);
  } else {
    return p1;
  }
}

/*
 * Divide x by 2^12, rounded to nearest integer.
 */
static int
roundshift(int x)
{
  if (x > 0) {
    x += 2048;
  } else if (x < 0) {
    x -= 2048;
  }
  return x >> 12;
}

void
PolygonRotateShift(BulkPixelPoint *poly,
                   const int n,
                   const PixelPoint shift,
                   Angle angle,
                   int scale,
                   const bool use_fast_scale)
{
  const int xs = shift.x, ys = shift.y;
  if (use_fast_scale)
    scale = Layout::FastScale(scale);
  /*
   * About the scaling...
   *  - We want to divide the raster points by 100 in order to scale the
   *    range +/-50 to the size 'scale'.
   *  - The fast trig functions return 10-bit fraction fixed point values.
   *    I.e. we need to divide by 2^10 to convert to regular integers.
   *  - In total we need to divide by (2^10)*100. This is equal to (2^12)*25.
   *  - For precision we want to divide as late as possible, but for speed
   *    we want to avoid the division operation. Therefore we divide by 25
   *    early but outside the loop, and divide by 2^12 late, inside the
   *    loop using roundshift.
   */
  const int cost = angle.ifastcosine() * scale / 25;
  const int sint = angle.ifastsine() * scale / 25;

  BulkPixelPoint *p = poly;
  const BulkPixelPoint *pe = poly + n;

  while (p < pe) {
    int x = p->x;
    int y = p->y;
    p->x = roundshift(x * cost - y * sint) + xs;
    p->y = roundshift(y * cost + x * sint) + ys;
    p++;
  }
}
