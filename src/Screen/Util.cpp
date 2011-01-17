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

#include "Screen/Util.hpp"
#include "Screen/Canvas.hpp"
#include "Math/FastMath.h"

bool
Segment(Canvas &canvas, long x, long y, int radius,
        Angle start, Angle end, bool horizon)
{
  // dont draw if out of view
  RECT rc, bounds;
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

  // add start node
  pt[npoly].x = x + ISINETABLE[istart] * radius / 1024;
  pt[npoly].y = y - ICOSTABLE[istart] * radius / 1024;
  npoly++;

  // add intermediate nodes (if any)
  int ilast = istart < iend ? iend : iend + 4096;
  for (int i = istart + 4096 / 64; i < ilast; i += 4096 / 64) {
    int angle = i & 0xfff;
    pt[npoly].x = x + ISINETABLE[angle] * radius / 1024;
    pt[npoly].y = y - ICOSTABLE[angle] * radius / 1024;

    if ((pt[npoly].x != pt[npoly-1].x) || (pt[npoly].y != pt[npoly-1].y)) {
      npoly++;
    }
  }

  // and end node
  pt[npoly].x = x + ISINETABLE[iend] * radius / 1024;
  pt[npoly].y = y - ICOSTABLE[iend] * radius / 1024;
  npoly++;

  assert(npoly <= 66);
  if (npoly) {
    canvas.polygon(pt, npoly);
  }

  return true;
}
