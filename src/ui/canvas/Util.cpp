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

#include "Util.hpp"
#include "Canvas.hpp"
#include "util/Macros.hpp"
#include "Math/Angle.hpp"

static constexpr unsigned CIRCLE_SEGS = 64;

[[gnu::const]]
static PixelPoint
CirclePoint(int radius, unsigned angle) noexcept
{
  assert(angle < ISINETABLE.size());

  return PixelPoint(ISINETABLE[angle] * radius / 1024,
                    -ISINETABLE[(angle + INT_QUARTER_CIRCLE) & INT_ANGLE_MASK] * radius / 1024);
}

[[gnu::const]]
static PixelPoint
CirclePoint(PixelPoint p, int radius, unsigned angle) noexcept
{
  return p + CirclePoint(radius, angle);
}

static void
segment_poly(BulkPixelPoint *pt, const PixelPoint center,
             const int radius, const unsigned istart, const unsigned iend,
             unsigned &npoly, const bool forward=true) noexcept
{
  assert(istart < ISINETABLE.size());
  assert(iend < ISINETABLE.size());

  // add start node
  pt[npoly++] = CirclePoint(center, radius, istart);

  // add intermediate nodes (if any)
  if (forward) {
    const unsigned ilast = istart < iend ? iend : iend + INT_ANGLE_RANGE;
    for (unsigned i = istart + INT_ANGLE_RANGE / CIRCLE_SEGS; i < ilast;
         i += INT_ANGLE_RANGE / CIRCLE_SEGS) {
      const unsigned angle = i & INT_ANGLE_MASK;
      pt[npoly] = CirclePoint(center, radius, angle);

      if (pt[npoly].x != pt[npoly-1].x || pt[npoly].y != pt[npoly-1].y)
        npoly++;
    }
  } else {
    const unsigned ilast = istart > iend ? iend + INT_ANGLE_RANGE: iend;
    for (unsigned i = istart + INT_ANGLE_RANGE / CIRCLE_SEGS + INT_ANGLE_RANGE; i > ilast;
         i -= INT_ANGLE_RANGE / CIRCLE_SEGS) {
      const unsigned angle = i & INT_ANGLE_MASK;
      pt[npoly] = CirclePoint(center, radius, angle);

      if (pt[npoly].x != pt[npoly-1].x || pt[npoly].y != pt[npoly-1].y)
        npoly++;
    }
  }

  // and end node
  pt[npoly++] = CirclePoint(center, radius, iend);
}

[[gnu::pure]]
static bool
IsCircleVisible(const Canvas &canvas,
                PixelPoint center, unsigned radius) noexcept
{
  return int(center.x + radius) >= 0 && center.x < int(canvas.GetWidth() + radius) &&
    int(center.y + radius) >= 0 && center.y < int(canvas.GetHeight() + radius);
}

bool
Segment(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, bool horizon) noexcept
{
  // dont draw if out of view
  if (!IsCircleVisible(canvas, center, radius))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  BulkPixelPoint pt[CIRCLE_SEGS+3];

  // add center point
  if (!horizon) {
    pt[0] = center;
    npoly = 1;
  }

  segment_poly(pt, center, radius, istart, iend, npoly);

  assert(npoly <= ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawTriangleFan(pt, npoly);

  return true;
}

bool
Annulus(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, unsigned inner_radius) noexcept
{
  // dont draw if out of view
  if (!IsCircleVisible(canvas, center, radius))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  BulkPixelPoint pt[(CIRCLE_SEGS+2)*2];

  segment_poly(pt, center, radius, istart, iend, npoly);
  segment_poly(pt, center, inner_radius, iend, istart, npoly, false);

  assert(npoly <= ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawPolygon(pt, npoly);

  return true;
}

bool
KeyHole(Canvas &canvas, PixelPoint center, unsigned radius,
        Angle start, Angle end, unsigned inner_radius) noexcept
{
  // dont draw if out of view
  if (!IsCircleVisible(canvas, center, radius))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  BulkPixelPoint pt[(CIRCLE_SEGS+2)*2];

  segment_poly(pt, center, radius, istart, iend, npoly);
  segment_poly(pt, center, inner_radius, iend, istart, npoly);

  assert(npoly <= ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawPolygon(pt, npoly);

  return true;
}

void
RoundRect(Canvas &canvas, PixelRect r, unsigned radius) noexcept
{
  unsigned npoly = 0;
  BulkPixelPoint pt[(CIRCLE_SEGS+2)*4];

  segment_poly(pt, r.GetTopLeft().At(radius, radius), radius,
               INT_ANGLE_RANGE * 3 / 4,
               INT_ANGLE_RANGE - 1,
               npoly);
  segment_poly(pt, r.GetTopRight().At(-(int)radius, radius), radius,
               0, INT_ANGLE_RANGE / 4 - 1,
               npoly);
  segment_poly(pt, r.GetBottomRight().At(-(int)radius, -(int)radius), radius,
               INT_ANGLE_RANGE / 4,
               INT_ANGLE_RANGE / 2 - 1,
               npoly);
  segment_poly(pt, r.GetBottomLeft().At(radius, -(int)radius), radius,
               INT_ANGLE_RANGE / 2,
               INT_ANGLE_RANGE * 3 / 4 - 1,
               npoly);

  assert(npoly <= ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawTriangleFan(pt, npoly);
}

bool
Arc(Canvas &canvas, PixelPoint center, unsigned radius,
    Angle start, Angle end) noexcept
{
  // dont draw if out of view
  if (!IsCircleVisible(canvas, center, radius))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  BulkPixelPoint pt[CIRCLE_SEGS+3];

  segment_poly(pt, center, radius, istart, iend, npoly);

  assert(npoly <= ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawPolyline(pt, npoly);

  return true;
}
