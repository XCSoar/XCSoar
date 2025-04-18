// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math/Screen.hpp"
#include "Math/Angle.hpp"
#include "FastRotation.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/BulkPoint.hpp"

#include <algorithm>

[[gnu::const]]
static PixelPoint
MultiplyRound(PixelPoint p, double f) noexcept
{
  return PixelPoint(lround(p.x * f), lround(p.y * f));
}

PixelPoint
ScreenClosestPoint(const PixelPoint &p1, const PixelPoint &p2,
                   const PixelPoint &p3, int _offset) noexcept
{
  const PixelPoint v12 = p2 - p1;
  const PixelPoint v13 = p3 - p1;

  const double mag12 = DoublePoint2D{v12}.Magnitude();
  if (mag12 > 1) {
    // projection of v13 along v12 = v12.v13/|v12|
    double proj = DotProduct(v12, v13) / mag12;
    // fractional distance
    if (_offset > 0) {
      const double offset = _offset;
      if (offset * 2 < mag12) {
        proj = std::max(0., std::min(proj, mag12));
        proj = std::max(offset, std::min(mag12 - offset, proj + offset));
      } else {
        proj = mag12 / 2;
      }
    }

    const auto f = std::clamp(proj / mag12, 0., 1.);
    // location of 'closest' point
    return p1 + MultiplyRound(v12, f);
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
