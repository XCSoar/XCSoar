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

/**
 * Coordinates of the sine-function (x-Coordinates of a circle)
 * Even though this data is available from the fast(co)sine, it is faster to have a local small array since we will
 * be iterating through this directly.  Iterating through fast(co)sine requires more operations.
 */
static const double xcoords[64] = {
  0,			0.09801714,		0.195090322,	0.290284677,	0.382683432,	0.471396737,	0.555570233,	0.634393284,
  0.707106781,	0.773010453,	0.831469612,	0.881921264,	0.923879533,	0.956940336,	0.98078528,		0.995184727,
  1,			0.995184727,	0.98078528,		0.956940336,	0.923879533,	0.881921264,	0.831469612,	0.773010453,
  0.707106781,	0.634393284,	0.555570233,	0.471396737,	0.382683432,	0.290284677,	0.195090322,	0.09801714,
  0,			-0.09801714,	-0.195090322,	-0.290284677,	-0.382683432,	-0.471396737,	-0.555570233,	-0.634393284,
  -0.707106781,	-0.773010453,	-0.831469612,	-0.881921264,	-0.923879533,	-0.956940336,	-0.98078528,	-0.995184727,
  -1,			-0.995184727,	-0.98078528,	-0.956940336,	-0.923879533,	-0.881921264,	-0.831469612,	-0.773010453,
  -0.707106781,	-0.634393284,	-0.555570233,	-0.471396737,	-0.382683432,	-0.290284677,	-0.195090322,	-0.09801714
};

/**
 * Coordinates of the cosine-function (y-Coordinates of a circle)
 */
static const double ycoords[64] = {
  1,			0.995184727,	0.98078528,		0.956940336,	0.923879533,	0.881921264,	0.831469612,	0.773010453,
  0.707106781,	0.634393284,	0.555570233,	0.471396737,	0.382683432,	0.290284677,	0.195090322,	0.09801714,
  0,			-0.09801714,	-0.195090322,	-0.290284677,	-0.382683432,	-0.471396737,	-0.555570233,	-0.634393284,
  -0.707106781,	-0.773010453,	-0.831469612,	-0.881921264,	-0.923879533,	-0.956940336,	-0.98078528,	-0.995184727,
  -1,			-0.995184727,	-0.98078528,	-0.956940336,	-0.923879533,	-0.881921264,	-0.831469612,	-0.773010453,
  -0.707106781,	-0.634393284,	-0.555570233,	-0.471396737,	-0.382683432,	-0.290284677,	-0.195090322,	-0.09801714,
  0,			0.09801714,		0.195090322,	0.290284677,	0.382683432,	0.471396737,	0.555570233,	0.634393284,
  0.707106781,	0.773010453,	0.831469612,	0.881921264,	0.923879533,	0.956940336,	0.98078528,		0.995184727
};

static const fixed seg_steps_degrees(64/ 360.0);

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

  start = start.as_bearing();
  end = end.as_bearing();

  int istart = iround(start.value_degrees()*seg_steps_degrees);
  int iend = iround(end.value_degrees()*seg_steps_degrees);

  // adjust for wraparound
  if (istart > iend) {
    iend+= 64;
  }
  // we want accurate placement of start/end, so we draw those with
  // accurate calculation, the rest use the table
  // skip to next 
  istart++;
  iend--;

  int npoly = 0;
  RasterPoint pt[66];

  // add center point
  if (!horizon) {
    pt[0].x = x;
    pt[0].y = y;
    npoly = 1;
  }

  // add start node
  pt[npoly].x = x + (long)(radius * start.fastsine());
  pt[npoly].y = y - (long)(radius * start.fastcosine());
  npoly++;
    
  if (istart<iend) {
    // add intermediate nodes (if any)
    for (int i = istart; i <= iend; ++i) {
      pt[npoly].x = x + (long)(radius * xcoords[i % 64]);
      pt[npoly].y = y - (long)(radius * ycoords[i % 64]);
      if ((pt[npoly].x != pt[npoly-1].x) || (pt[npoly].y != pt[npoly-1].y)) {
        npoly++;
      }
    }
  }

  // and end node
  pt[npoly].x = x + (long)(radius * end.fastsine());
  pt[npoly].y = y - (long)(radius * end.fastcosine());
  npoly++;

  // add start point to close
  if (npoly) {
    pt[npoly].x = pt[0].x;
    pt[npoly].y = pt[0].y;
    npoly++;
  }

  assert(npoly <= 66);
  if (npoly) {
    canvas.polygon(pt, npoly);
  }

  return true;
}
