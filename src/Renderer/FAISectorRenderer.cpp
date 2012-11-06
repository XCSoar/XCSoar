/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FAISectorRenderer.hpp"
#include "Util/StaticArray.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Projection/Projection.hpp"
#include "Screen/Canvas.hpp"

gcc_const
static Angle
CalcAlpha(fixed dist_a, fixed dist_b, fixed dist_c, int dir)
{
    const fixed cos_alpha = (sqr(dist_b) + sqr(dist_c) - sqr(dist_a))
      / Double(dist_c * dist_b);
    return Angle::Radians(acos(cos_alpha) * dir);
}

void
RenderFAISector(Canvas &canvas, const Projection &projection,
                const GeoPoint &pt1, const GeoPoint &pt2,
                bool reverse)
{
  static constexpr fixed FAI_MIN_PERCENTAGE(0.28);
  static constexpr unsigned STEPS = 10;

  StaticArray<RasterPoint, 3 * STEPS> points;

  const GeoVector v = pt1.DistanceBearing(pt2);
  const fixed fDist_c = v.distance;
  const Angle fAngle = v.bearing;

  const fixed fDistMax = fDist_c / FAI_MIN_PERCENTAGE;
  const fixed fDistMin = fDist_c / (fixed_one - 2 * FAI_MIN_PERCENTAGE);

  const int dir = reverse > 0 ? 1 : -1;

  // calc right leg
  fixed fDelta_Dist = (fDistMax - fDistMin) / STEPS;
  fixed fDistTri = fDistMin;
  fixed fDist_a = fDistMin * FAI_MIN_PERCENTAGE;
  fixed fDist_b = fDistMin * FAI_MIN_PERCENTAGE;
  for (unsigned i = 0; i < STEPS; ++i) {
    const Angle alpha = CalcAlpha(fDist_a, fDist_b, fDist_c, dir);
    const GeoPoint ptd = GeoVector(fDist_b, fAngle + alpha).EndPoint(pt1);
    points.append() = projection.GeoToScreen(ptd);

    fDistTri += fDelta_Dist;
    fDist_a = FAI_MIN_PERCENTAGE * fDistTri;
    fDist_b = fDistTri - fDist_a - fDist_c;
  }

  // calc top leg
  fDelta_Dist = (fDistMax * (fixed_one - 3 * FAI_MIN_PERCENTAGE)) / STEPS;
  fDist_a = fDist_c;
  fDist_b = fDistMax - fDist_a - fDist_c;
  for (unsigned i = 0; i < STEPS; ++i) {
    const Angle alpha = CalcAlpha(fDist_a, fDist_b, fDist_c, dir);
    const GeoPoint ptd = GeoVector(fDist_b, fAngle + alpha).EndPoint(pt1);
    points.append() = projection.GeoToScreen(ptd);

    fDist_a += fDelta_Dist;
    fDist_b = fDistMax - fDist_a - fDist_c;
  }

  // calc left leg
  fDelta_Dist = (fDistMax - fDistMin) / STEPS;
  fDistTri = fDistMax;
  fDist_b = fDistMax * FAI_MIN_PERCENTAGE;
  fDist_a = fDistTri - fDist_b - fDist_c;
  for (unsigned i = 0; i < STEPS; ++i) {
    const Angle alpha = CalcAlpha(fDist_a, fDist_b, fDist_c, dir);
    const GeoPoint ptd = GeoVector(fDist_b, fAngle + alpha).EndPoint(pt1);
    points.append() = projection.GeoToScreen(ptd);

    fDistTri -= fDelta_Dist;
    fDist_b = FAI_MIN_PERCENTAGE * fDistTri;
    fDist_a = fDistTri - fDist_b - fDist_c;
  }

  // draw polygon
  canvas.DrawPolygon(points.begin(), points.size());
}
