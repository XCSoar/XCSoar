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

#include "Math/Screen.hpp"
#include "Math/Angle.hpp"
#include "Math/FastMath.hpp"
#include "FastRotation.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/BulkPoint.hpp"

#include <algorithm>

PixelPoint
ScreenClosestPoint(const PixelPoint &p1, const PixelPoint &p2,
                   const PixelPoint &p3, int offset) noexcept
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

    const auto f = std::clamp(double(proj) / mag12, 0., 1.);
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
template<int SHIFT>
static constexpr int
roundshift(int x) noexcept
{
  constexpr int ONE = 1 << SHIFT;
  constexpr int HALF = ONE / 2;

  if (x > 0) {
    x += HALF;
  } else if (x < 0) {
    x -= HALF;
  }
  return x >> SHIFT;
}

template<int SHIFT>
static constexpr PixelPoint
roundshift(PixelPoint p) noexcept
{
  return {roundshift<SHIFT>(p.x), roundshift<SHIFT>(p.y)};
}

void
PolygonRotateShift(std::span<BulkPixelPoint> poly,
                   const PixelPoint shift,
                   Angle angle,
                   int scale) noexcept
{
  constexpr int SCALE_SHIFT = 2;
  constexpr int TOTAL_SHIFT = FastIntegerRotation::SHIFT + SCALE_SHIFT;

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
  FastIntegerRotation fr(angle);
  fr.Scale(scale / (100 >> SCALE_SHIFT));

  for (auto &p : poly)
    p = roundshift<TOTAL_SHIFT>(fr.RotateRaw(p)) + shift;
}
