/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Math/FastMath.h"

static 
void segment_poly(RasterPoint* pt,
                  const long x,
                  const long y,
                  const int radius,
                  const int istart,
                  const int iend,
                  int &npoly,
                  const bool forward=true)
{
  // add start node
  pt[npoly].x = x + ISINETABLE[istart] * radius / 1024;
  pt[npoly].y = y - ICOSTABLE[istart] * radius / 1024;
  npoly++;

  // add intermediate nodes (if any)
  if (forward) {
    int ilast = istart < iend ? iend : iend + 4096;
    for (int i = istart + 4096 / 64; i < ilast; i += 4096 / 64) {
      int angle = i & 0xfff;
      pt[npoly].x = x + ISINETABLE[angle] * radius / 1024;
      pt[npoly].y = y - ICOSTABLE[angle] * radius / 1024;
      
      if ((pt[npoly].x != pt[npoly-1].x) || (pt[npoly].y != pt[npoly-1].y)) {
        npoly++;
      }
    }
  } else {
    int ilast = istart > iend ? iend : iend - 4096;
    for (int i = istart + 4096 / 64; i > ilast; i -= 4096 / 64) {
      int angle = i & 0xfff;
      pt[npoly].x = x + ISINETABLE[angle] * radius / 1024;
      pt[npoly].y = y - ICOSTABLE[angle] * radius / 1024;
      
      if ((pt[npoly].x != pt[npoly-1].x) || (pt[npoly].y != pt[npoly-1].y)) {
        npoly++;
      }
    }
  }

  // and end node
  pt[npoly].x = x + ISINETABLE[iend] * radius / 1024;
  pt[npoly].y = y - ICOSTABLE[iend] * radius / 1024;
  npoly++;
}

bool
Segment(Canvas &canvas, long x, long y, int radius,
        Angle start, Angle end, bool horizon)
{
  // dont draw if out of view
  PixelRect rc, bounds;
  SetRect(&rc, 0, 0, canvas.get_width(), canvas.get_height());
  SetRect(&bounds, x - radius, y - radius, x + radius, y + radius);
  if (!IntersectRect(&bounds, &bounds, &rc))
    return false;

  const int istart = NATIVE_TO_INT(start.value_native());
  const int iend = NATIVE_TO_INT(end.value_native());

  int npoly = 0;
  RasterPoint pt[66];

  // add center point
  if (!horizon) {
    pt[0].x = x;
    pt[0].y = y;
    npoly = 1;
  }

  segment_poly(pt, x, y, radius, istart, iend, npoly);

  assert(npoly <= 66);
  if (npoly) {
    canvas.TriangleFan(pt, npoly);
  }

  return true;
}
                   

bool
Annulus(Canvas &canvas, long x, long y, int radius,
        Angle start, Angle end, int inner_radius)
{
  // dont draw if out of view
  PixelRect rc, bounds;
  SetRect(&rc, 0, 0, canvas.get_width(), canvas.get_height());
  SetRect(&bounds, x - radius, y - radius, x + radius, y + radius);
  if (!IntersectRect(&bounds, &bounds, &rc))
    return false;

  const int istart = NATIVE_TO_INT(start.value_native());
  const int iend = NATIVE_TO_INT(end.value_native());

  int npoly = 0;
  RasterPoint pt[66*2];

  segment_poly(pt, x, y, radius, istart, iend, npoly);
  segment_poly(pt, x, y, inner_radius, iend, istart, npoly, false);

  assert(npoly <= 66*2);
  if (npoly) {
    canvas.polygon(pt, npoly);
  }

  return true;
}

bool
KeyHole(Canvas &canvas, long x, long y, int radius,
        Angle start, Angle end, int inner_radius)
{
  // dont draw if out of view
  PixelRect rc, bounds;
  SetRect(&rc, 0, 0, canvas.get_width(), canvas.get_height());
  SetRect(&bounds, x - radius, y - radius, x + radius, y + radius);
  if (!IntersectRect(&bounds, &bounds, &rc))
    return false;

  const int istart = NATIVE_TO_INT(start.value_native());
  const int iend = NATIVE_TO_INT(end.value_native());

  int npoly = 0;
  RasterPoint pt[66*2];

  segment_poly(pt, x, y, radius, istart, iend, npoly);
  segment_poly(pt, x, y, inner_radius, iend, istart, npoly);

  assert(npoly <= 66*2);
  if (npoly) {
    canvas.polygon(pt, npoly);
  }

  return true;
}

void
RoundRect(Canvas &canvas, int left, int top, int right, int bottom, int radius)
{
  int npoly = 0;
  RasterPoint pt[66*4];

  segment_poly(pt, left + radius, top + radius, radius, 3072, 4095, npoly);
  segment_poly(pt, right - radius, top + radius, radius, 0, 1023, npoly);
  segment_poly(pt, right - radius, bottom - radius, radius, 1024, 2047, npoly);
  segment_poly(pt, left + radius, bottom - radius, radius, 2048, 3071, npoly);

  assert(npoly <= 66*4);
  if (npoly)
    canvas.polygon(pt, npoly);
}
