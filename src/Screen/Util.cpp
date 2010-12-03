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
 * The "OutputToInput" function sets the resulting polygon of this
 * step, up to be the input polygon for next step of the clipping
 * algorithm. As the Sutherland-Hodgman algorithm is a polygon
 * clipping algorithm, it does not handle line clipping very well. The
 * modification so that lines may be clipped as well as polygons is
 * included in this function.
 * @param inLength Length of the inVertexArray
 * @param inVertexArray
 * @param outLength Length of the outVertexArray
 * @param outVertexArray
 */
static void
OutputToInput(unsigned int *inLength, RasterPoint *inVertexArray,
              unsigned int *outLength, RasterPoint *outVertexArray)
{
  // linefix
  if ((*inLength == 2) && (*outLength == 3)) {
    inVertexArray[0].x = outVertexArray[0].x;
    inVertexArray[0].y = outVertexArray[0].y;

    // First two vertices are same
    if ((outVertexArray[0].x == outVertexArray[1].x)
        && (outVertexArray[0].y == outVertexArray[1].y)) {
      inVertexArray[1].x = outVertexArray[2].x;
      inVertexArray[1].y = outVertexArray[2].y;

    // First vertex is same as third vertex
    } else {
      inVertexArray[1].x = outVertexArray[1].x;
      inVertexArray[1].y = outVertexArray[1].y;
    }

    *inLength = 2;

  // set the outVertexArray as inVertexArray for next step*/
  } else {
    *inLength = *outLength;
    memcpy((void*)inVertexArray, (void*)outVertexArray,
           (*outLength) * sizeof(RasterPoint));
  }
}

#define INSIDE_LEFT_EDGE(a,b)   (a->x >= b[1].x)
#define INSIDE_BOTTOM_EDGE(a,b) (a->y <= b[0].y)
#define INSIDE_RIGHT_EDGE(a,b)  (a->x <= b[1].x)
#define INSIDE_TOP_EDGE(a,b)    (a->y >= b[0].y)

/**
 * The "Intersect" function calculates the intersection of the polygon
 * edge (vertex s to p) with the clipping boundary.
 * @param first First point of the polygon edge
 * @param second Second point of the polygon edge
 * @param clipBoundary Clipping Boundary (2 RasterPoints)
 * @param intersectPt Intersection point of the clipping boundary
 * and the polygon edge
 * @return True if intersection occurs, False otherwise
 */
static bool
Intersect (const RasterPoint &first, const RasterPoint &second,
           const RasterPoint *clipBoundary, RasterPoint *intersectPt)
{
  // Horizontal
  if (clipBoundary[0].y == clipBoundary[1].y) {
    intersectPt->y = clipBoundary[0].y;
    if (second.y != first.y) {
      long dx = second.x - first.x, dy = second.y - first.y;
      long cy = clipBoundary[0].y - first.y;
      intersectPt->x = first.x + cy * dx / dy;
      return true;
    }

  // Vertical
  } else {
    intersectPt->x = clipBoundary[0].x;
    if (second.x != first.x) {
      long dx = second.x - first.x, dy = second.y - first.y;
      long cx = clipBoundary[0].x - first.x;
      intersectPt->y = first.y + cx * dy / dx;
      return true;
    }
  }

  // no need to add point!
  return false;
}


// The "Output" function moves "newVertex" to "outVertexArray" and
// updates "outLength".

static void
Output(const RasterPoint *newVertex, unsigned int *outLength,
       RasterPoint *outVertexArray)
{
  if (*outLength) {
    if ((newVertex->x == outVertexArray[*outLength - 1].x)
        && (newVertex->y == outVertexArray[*outLength - 1].y)) {
      // no need for duplicates
      return;
    }
  }

  outVertexArray[*outLength].x= newVertex->x;
  outVertexArray[*outLength].y= newVertex->y;
  (*outLength)++;
}

static bool
ClipEdge(const bool &s_inside,
         const bool &p_inside,
         const RasterPoint *clipBoundary,
         RasterPoint *outVertexArray,
         const RasterPoint *s,
         const RasterPoint *p,
         unsigned int *outLength,
         const bool fill)
{
  if (fill) {
    if (p_inside && s_inside) {
      // case 1, save endpoint p
      return true;
    } else if (p_inside != s_inside) {
      RasterPoint i;
      if (Intersect(*s, *p, clipBoundary, &i)) {
        Output(&i, outLength, outVertexArray);
      }
      // case 4, save intersection and endpoint
      // case 2, exit visible, save intersection i
      return p_inside;
    } else {
      // case 3, both outside, save nothing
      return false;
    }
  } else {
    if (p_inside) {
      return true;
    }
  }
  return false;
}

/**
 * Clips a polygon to the clipping boundary with the
 * Sutherland-Hodgman algorithm
 * @param inVertexArray Polygon to be clipped
 * @param outVertexArray
 * @param inLength Number of points in the inVertexArray
 * @param clipBoundary Clipping Boundary
 * @param fill
 * @param mode
 * @return
 * @see http://en.wikipedia.org/wiki/Sutherland-Hodgman_clipping_algorithm
 */
static unsigned int
SutherlandHodgmanPolygoClip (RasterPoint* inVertexArray,
                             RasterPoint* outVertexArray,
                             const unsigned int inLength,
                             const RasterPoint *clipBoundary,
                             const bool fill,
                             const int mode)
{
  // Start, end point of current polygon edge
  RasterPoint *s, *p;
  // Vertex loop counter
  unsigned int j;
  unsigned int outLength = 0;

  if (inLength<1) return 0;

  s = inVertexArray + inLength-1;
  p = inVertexArray;

  bool s_inside, p_inside;

  // Start with the last vertex in inVertexArray
  switch (mode) {
  case 0:
    for (j = inLength; j--;) {
      s_inside = INSIDE_LEFT_EDGE(s,clipBoundary);
      p_inside = INSIDE_LEFT_EDGE(p,clipBoundary);
      // Now s and p correspond to the vertices
      if (ClipEdge(s_inside, p_inside, clipBoundary, outVertexArray, s, p,
          &outLength, fill || (p != inVertexArray))) {
        Output(p, &outLength, outVertexArray);
      }
      // Advance to next pair of vertices
      s = p;
      p++;
    }
    break;
  case 1:
    for (j = inLength; j--;) {
      s_inside = INSIDE_BOTTOM_EDGE(s,clipBoundary);
      p_inside = INSIDE_BOTTOM_EDGE(p,clipBoundary);
      if (ClipEdge(s_inside, p_inside, clipBoundary, outVertexArray, s, p,
          &outLength, fill || (p != inVertexArray))) {
        Output(p, &outLength, outVertexArray);
      }
      s = p;
      p++;
    }
    break;
  case 2:
    for (j = inLength; j--;) {
      s_inside = INSIDE_RIGHT_EDGE(s,clipBoundary);
      p_inside = INSIDE_RIGHT_EDGE(p,clipBoundary);
      if (ClipEdge(s_inside, p_inside, clipBoundary, outVertexArray, s, p,
          &outLength, fill || (p != inVertexArray))) {
        Output(p, &outLength, outVertexArray);
      }
      s = p;
      p++;
    }
    break;
  case 3:
    for (j = inLength; j--;) {
      s_inside = INSIDE_TOP_EDGE(s,clipBoundary);
      p_inside = INSIDE_TOP_EDGE(p,clipBoundary);
      if (ClipEdge(s_inside, p_inside, clipBoundary, outVertexArray, s, p,
          &outLength, fill || (p != inVertexArray))) {
        Output(p, &outLength, outVertexArray);
      }
      s = p;
      p++;
    }
    break;
  }

  return outLength;
}

/**
 * Clips a polygon (m_ptin) to the given rect (rc)
 * @param canvas
 * @param m_ptin
 * @param inLength
 * @param rc
 * @param fill
 */
void
ClipPolygon(Canvas &canvas, const RasterPoint *m_ptin, unsigned int inLength,
            bool fill)
{
  unsigned int outLength = 0;

  if (inLength >= MAXCLIPPOLYGON - 1) {
    inLength = MAXCLIPPOLYGON - 2;
  }
  if (inLength < 2) {
    return;
  }

  RasterPoint clip_ptin[inLength + 1];
  memcpy((void*)clip_ptin, (void*)m_ptin, inLength * sizeof(RasterPoint));

  // add extra point for final point if it doesn't equal the first
  // this is required to close some airspace areas that have missing
  // final point
  if (fill) {
    if ((m_ptin[inLength - 1].x != m_ptin[0].x)
        && (m_ptin[inLength - 1].y != m_ptin[0].y)) {
      clip_ptin[inLength] = clip_ptin[0];
      inLength++;
    }
  }

  RECT rc;
  SetRect(&rc, 0, 0, canvas.get_width(), canvas.get_height());

  // OK, do the clipping
  RasterPoint edge[5] = {{rc.left, rc.top},
                   {rc.left, rc.bottom},
                   {rc.right, rc.bottom},
                   {rc.right, rc.top},
                   {rc.left, rc.top}};
  //steps left_edge, bottom_edge, right_edge, top_edge
  RasterPoint clip_ptout[inLength * 16];
  for (int step = 0; step < 4; step++) {
    outLength = SutherlandHodgmanPolygoClip(clip_ptin, clip_ptout, inLength,
        edge + step, fill, step);
    OutputToInput(&inLength, clip_ptin, &outLength, clip_ptout);
  }

  if (fill) {
    if (outLength > 2) {
      canvas.polygon(clip_ptout, outLength);
    }
  } else {
    if (outLength > 1) {
      canvas.polyline(clip_ptout, outLength);
    }
  }
}

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

  assert(npoly<66);
  if (npoly) {
    canvas.polygon(pt, npoly);
  }

  return true;
}
