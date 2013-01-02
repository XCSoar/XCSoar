/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/Util.hpp"
#include "Screen/Canvas.hpp"
#include "Util/Macros.hpp"
#include "Math/FastMath.h"

gcc_const
static RasterPoint
CirclePoint(int x, int y, int radius, unsigned angle)
{
  assert(angle < ARRAY_SIZE(ISINETABLE));

  return RasterPoint(x + ISINETABLE[angle] * radius / 1024,
                     y - ICOSTABLE[angle] * radius / 1024);
}

static void
segment_poly(RasterPoint* pt, const int x, const int y,
             const int radius, const unsigned istart, const unsigned iend,
             unsigned &npoly, const bool forward=true)
{
  assert(istart < ARRAY_SIZE(ISINETABLE));
  assert(iend < ARRAY_SIZE(ISINETABLE));

  // add start node
  pt[npoly++] = CirclePoint(x, y, radius, istart);

  // add intermediate nodes (if any)
  if (forward) {
    const unsigned ilast = istart < iend ? iend : iend + 4096;
    for (unsigned i = istart + 4096 / 64; i < ilast; i += 4096 / 64) {
      const unsigned angle = i & 0xfff;
      pt[npoly] = CirclePoint(x, y, radius, angle);

      if (pt[npoly].x != pt[npoly-1].x || pt[npoly].y != pt[npoly-1].y)
        npoly++;
    }
  } else {
    const unsigned ilast = istart > iend ? iend : iend - 4096;
    for (int i = istart + 4096 / 64; i > (int)ilast; i -= 4096 / 64) {
      const unsigned angle = i & 0xfff;
      pt[npoly] = CirclePoint(x, y, radius, angle);

      if (pt[npoly].x != pt[npoly-1].x || pt[npoly].y != pt[npoly-1].y)
        npoly++;
    }
  }

  // and end node
  pt[npoly++] = CirclePoint(x, y, radius, iend);
}

bool
Segment(Canvas &canvas, PixelScalar x, PixelScalar y, UPixelScalar radius,
        Angle start, Angle end, bool horizon)
{
  // dont draw if out of view
  PixelRect rc, bounds;
  SetRect(rc, 0, 0, canvas.GetWidth(), canvas.GetHeight());
  SetRect(bounds, x - radius, y - radius, x + radius, y + radius);
  if (!OverlapsRect(bounds, rc))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  RasterPoint pt[67];

  // add center point
  if (!horizon) {
    pt[0].x = x;
    pt[0].y = y;
    npoly = 1;
  }

  segment_poly(pt, x, y, radius, istart, iend, npoly);

  assert(npoly < ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawTriangleFan(pt, npoly);

  return true;
}
                   

bool
Annulus(Canvas &canvas, PixelScalar x, PixelScalar y, UPixelScalar radius,
        Angle start, Angle end, UPixelScalar inner_radius)
{
  // dont draw if out of view
  PixelRect rc, bounds;
  SetRect(rc, 0, 0, canvas.GetWidth(), canvas.GetHeight());
  SetRect(bounds, x - radius, y - radius, x + radius, y + radius);
  if (!OverlapsRect(bounds, rc))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  RasterPoint pt[66*2];

  segment_poly(pt, x, y, radius, istart, iend, npoly);
  segment_poly(pt, x, y, inner_radius, iend, istart, npoly, false);

  assert(npoly < ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawPolygon(pt, npoly);

  return true;
}

bool
KeyHole(Canvas &canvas, PixelScalar x, PixelScalar y, UPixelScalar radius,
        Angle start, Angle end, UPixelScalar inner_radius)
{
  // dont draw if out of view
  PixelRect rc, bounds;
  SetRect(rc, 0, 0, canvas.GetWidth(), canvas.GetHeight());
  SetRect(bounds, x - radius, y - radius, x + radius, y + radius);
  if (!OverlapsRect(bounds, rc))
    return false;

  const int istart = NATIVE_TO_INT(start.Native());
  const int iend = NATIVE_TO_INT(end.Native());

  unsigned npoly = 0;
  RasterPoint pt[66*2];

  segment_poly(pt, x, y, radius, istart, iend, npoly);
  segment_poly(pt, x, y, inner_radius, iend, istart, npoly);

  assert(npoly < ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawPolygon(pt, npoly);

  return true;
}

void
RoundRect(Canvas &canvas, PixelScalar left, PixelScalar top,
          PixelScalar right, PixelScalar bottom, UPixelScalar radius)
{
  unsigned npoly = 0;
  RasterPoint pt[66*4];

  segment_poly(pt, left + radius, top + radius, radius, 3072, 4095, npoly);
  segment_poly(pt, right - radius, top + radius, radius, 0, 1023, npoly);
  segment_poly(pt, right - radius, bottom - radius, radius, 1024, 2047, npoly);
  segment_poly(pt, left + radius, bottom - radius, radius, 2048, 3071, npoly);

  assert(npoly < ARRAY_SIZE(pt));
  if (npoly)
    canvas.DrawPolygon(pt, npoly);
}
