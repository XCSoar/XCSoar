/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Math/Constants.h"
#include "Math/Geometry.hpp"
#include "Math/FastMath.h"
#include "Screen/shapelib/mapprimitive.h"
#include "InfoBoxLayout.h"
#include "Asset.hpp" // for needclipping

#include <tchar.h>

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
OutputToInput(unsigned int *inLength, POINT *inVertexArray,
    unsigned int *outLength, POINT *outVertexArray)
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
           (*outLength) * sizeof(POINT));
  }
}

/*
 * The "Inside" function returns TRUE if the vertex tested is on the
 * inside of the clipping boundary. "Inside" is defined as "to the
 * left of clipping boundary when one looks from the first vertex to
 * the second vertex of the clipping boundary".
 * @param testVertex Vertex to be tested
 * @param clipBoundary Clipping boundary
 * @return True if the vertex tested is on the inside of the
 * clipping boundary
 */
/*
static bool Inside (const POINT *testVertex, const POINT *clipBoundary)
{
  if (clipBoundary[1].x > clipBoundary[0].x)              // bottom edge
    if (testVertex->y <= clipBoundary[0].y) return TRUE;
  if (clipBoundary[1].x < clipBoundary[0].x)              // top edge
   if (testVertex->y >= clipBoundary[0].y) return TRUE;
  if (clipBoundary[1].y < clipBoundary[0].y)              // right edge
    if (testVertex->x <= clipBoundary[1].x) return TRUE;
  if (clipBoundary[1].y > clipBoundary[0].y)              // left edge
    if (testVertex->x >= clipBoundary[1].x) return TRUE;
  return FALSE;
}
*/

#define INSIDE_LEFT_EDGE(a,b)   (a->x >= b[1].x)
#define INSIDE_BOTTOM_EDGE(a,b) (a->y <= b[0].y)
#define INSIDE_RIGHT_EDGE(a,b)  (a->x <= b[1].x)
#define INSIDE_TOP_EDGE(a,b)    (a->y >= b[0].y)

/**
 * The "Intersect" function calculates the intersection of the polygon
 * edge (vertex s to p) with the clipping boundary.
 * @param first First point of the polygon edge
 * @param second Second point of the polygon edge
 * @param clipBoundary Clipping Boundary (2 POINTs)
 * @param intersectPt Intersection point of the clipping boundary
 * and the polygon edge
 * @return True if intersection occurs, False otherwise
 */
static bool
Intersect (const POINT &first, const POINT &second,
           const POINT *clipBoundary, POINT *intersectPt)
{
  float f;

  // Horizontal
  if (clipBoundary[0].y == clipBoundary[1].y) {
    intersectPt->y = clipBoundary[0].y;
    if (second.y != first.y) {
      f = ((float)(second.x - first.x)) / ((float)(second.y - first.y));
      intersectPt->x = first.x + (long)(((clipBoundary[0].y - first.y) * f));
      return true;
    }

  // Vertical
  } else {
    intersectPt->x = clipBoundary[0].x;
    if (second.x != first.x) {
      f = ((float)(second.y - first.y)) / ((float)(second.x - first.x));
      intersectPt->y = first.y + (long)(((clipBoundary[0].x - first.x) * f));
      return true;
    }
  }

  // no need to add point!
  return false;
}


// The "Output" function moves "newVertex" to "outVertexArray" and
// updates "outLength".

static void
Output(const POINT *newVertex, unsigned int *outLength,
    POINT *outVertexArray)
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
         const POINT *clipBoundary,
         POINT *outVertexArray,
         const POINT *s,
         const POINT *p,
         unsigned int *outLength,
         const bool fill)
{
  if (fill) {
    if (p_inside && s_inside) {
      // case 1, save endpoint p
      return true;
    } else if (p_inside != s_inside) {
      POINT i;
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
SutherlandHodgmanPolygoClip (POINT* inVertexArray,
                             POINT* outVertexArray,
                             const unsigned int inLength,
                             const POINT *clipBoundary,
                             const bool fill,
                             const int mode)
{
  // Start, end point of current polygon edge
  POINT *s, *p;
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

static POINT clip_ptout[MAXCLIPPOLYGON];
static POINT clip_ptin[MAXCLIPPOLYGON];

/**
 * Clips a polygon (m_ptin) to the given rect (rc)
 * @param canvas
 * @param m_ptin
 * @param inLength
 * @param rc
 * @param fill
 */
void
ClipPolygon(Canvas &canvas, const POINT *m_ptin, unsigned int inLength,
    RECT rc, bool fill)
{
  unsigned int outLength = 0;

  if (inLength >= MAXCLIPPOLYGON - 1) {
    inLength = MAXCLIPPOLYGON - 2;
  }
  if (inLength < 2) {
    return;
  }

  memcpy((void*)clip_ptin, (void*)m_ptin, inLength * sizeof(POINT));

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

  // PAOLO NOTE: IF CLIPPING WITH N>2 DOESN'T WORK,
  // TRY IFDEF'ing out THE FOLLOWING ADJUSTMENT TO THE CLIPPING RECTANGLE
  rc.top--;
  rc.bottom++;
  rc.left--;
  rc.right++;

  // OK, do the clipping
  POINT edge[5] = {{rc.left, rc.top},
                   {rc.left, rc.bottom},
                   {rc.right, rc.bottom},
                   {rc.right, rc.top},
                   {rc.left, rc.top}};
  //steps left_edge, bottom_edge, right_edge, top_edge
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

// QUESTION TB: what about fast(co)sine?! produces the same data i think...
/**
 * Coordinates of the sine-function (x-Coordinates of a circle)
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

#ifdef ENABLE_UNUSED_CODE
void StartArc(HDC hdc,
	      double longitude0, double latitude0,
	      double longitude1, double latitude1,
	      double arclength) {

  double radius, bearing;
  DistanceBearing(latitude0, longitude0,
                  latitude1, longitude1,
                  &radius,
                  &bearing);
  double angle = 360*min(1, arclength/(2.0*M_PI*radius));
  int i0 = (int)(bearing+angle/2);
  int i1 = (int)(bearing-angle/2);
  int i;
  if (i0<0) { i1+= 360; }
  if (i1<0) { i1+= 360; }
  if (i0>360) {i0-= 360; }
  if (i1>360) {i1-= 360; }
  i0 = i0*64/360;
  i1 = i1*64/360;
  POINT pt[2];
//  double lat, lon;
  int x=0;
  int y=0;

  if (i1<i0) {
    for (i=i0; i<64-1; i++) {
      //      MapWindow::LatLon2Screen(lon, lat, &scx, &scy);
      pt[0].x = x + (long) (radius * xcoords[i]);
      pt[0].y = y + (long) (radius * ycoords[i]);
      pt[1].x = x + (long) (radius * xcoords[i+1]);
      pt[1].y = y + (long) (radius * ycoords[i+1]);
      Polygon(hdc,pt,2);
    }
    for (i=0; i<i1-1; i++) {
      pt[0].x = x + (long) (radius * xcoords[i]);
      pt[0].y = y + (long) (radius * ycoords[i]);
      pt[1].x = x + (long) (radius * xcoords[i+1]);
      pt[1].y = y + (long) (radius * ycoords[i+1]);
      Polygon(hdc,pt,2);
    }
  } else {
    for (i=i0; i<i1-1; i++) {
      pt[0].x = x + (long) (radius * xcoords[i]);
      pt[0].y = y + (long) (radius * ycoords[i]);
      pt[1].x = x + (long) (radius * xcoords[i+1]);
      pt[1].y = y + (long) (radius * ycoords[i+1]);
      Polygon(hdc,pt,2);
    }
  }

}
#endif /* ENABLE_UNUSED_CODE */

/**
 * Paints a circle to the canvas
 * @param canvas Painting canvas
 * @param x x-Coordinate of the circle's center
 * @param y y-Coordinate of the circle's center
 * @param radius Radius of the circle
 * @param rc Clipping bounds
 * @param fill Whether the circle will be filled (closed polygon) 
 * @return
 */
int
ClippedCircle(Canvas &canvas, long x, long y, int radius, RECT rc, bool fill)
{
  POINT pt[65];
  unsigned int i;

  rectObj rect;
  rect.minx = x-radius;
  rect.maxx = x+radius;
  rect.miny = y-radius;
  rect.maxy = y+radius;
  rectObj rcrect;
  rcrect.minx = rc.left;
  rcrect.maxx = rc.right;
  rcrect.miny = rc.top;
  rcrect.maxy = rc.bottom;

  if (msRectOverlap(&rect, &rcrect) != MS_TRUE) {
    return FALSE;
  }
  // JMW added faster checking...

  unsigned int step = 1;
  if (radius < 20) {
    step = 2;
  }
  for (i = 64 / step; i--;) {
    pt[i].x = x + (long)(radius * xcoords[i * step]);
    pt[i].y = y + (long)(radius * ycoords[i * step]);
  }
  step = 64 / step;
  pt[step].x = x + (long)(radius * xcoords[0]);
  pt[step].y = y + (long)(radius * ycoords[0]);

  ClipPolygon(canvas, pt, step + 1, rc, fill);
  return TRUE;
}

int
Segment(Canvas &canvas, long x, long y, int radius, RECT rc, double start,
    double end, bool horizon)
{
  POINT pt[66];
  int i;
  int istart;
  int iend;

  rectObj rect;
  rect.minx = x - radius;
  rect.maxx = x + radius;
  rect.miny = y - radius;
  rect.maxy = y + radius;
  rectObj rcrect;
  rcrect.minx = rc.left;
  rcrect.maxx = rc.right;
  rcrect.miny = rc.top;
  rcrect.maxy = rc.bottom;

  if (msRectOverlap(&rect, &rcrect) != MS_TRUE) {
    return FALSE;
  }

  // JMW added faster checking...

  start = AngleLimit360(start);
  end = AngleLimit360(end);

  istart = iround(start / 360.0 * 64);
  iend = iround(end / 360.0 * 64);

  int npoly = 0;

  if (istart > iend) {
    iend+= 64;
  }
  istart++;
  iend--;

  if (!horizon) {
    pt[0].x = x;
    pt[0].y = y;
    npoly = 1;
  }
  pt[npoly].x = x + (long)(radius * fastsine(start));
  pt[npoly].y = y - (long)(radius * fastcosine(start));
  npoly++;

  for (i = 0; i < 64; i++) {
    if (i <= iend - istart) {
      pt[npoly].x = x + (long)(radius * xcoords[(i + istart) % 64]);
      pt[npoly].y = y - (long)(radius * ycoords[(i + istart) % 64]);
      npoly++;
    }
  }
  pt[npoly].x = x + (long)(radius * fastsine(end));
  pt[npoly].y = y - (long)(radius * fastcosine(end));
  npoly++;

  if (!horizon) {
    pt[npoly].x = x;
    pt[npoly].y = y;
    npoly++;
  } else {
    pt[npoly].x = pt[0].x;
    pt[npoly].y = pt[0].y;
    npoly++;
  }
  if (npoly) {
    canvas.polygon(pt, npoly);
  }

  return TRUE;
}

/*
 * VENTA3 This is a modified Segment()
 */
int
DrawArc(Canvas &canvas, long x, long y, int radius, RECT rc,
    double start, double end)
{
  POINT pt[66];
  int i;
  int istart;
  int iend;

  rectObj rect;
  rect.minx = x - radius;
  rect.maxx = x + radius;
  rect.miny = y - radius;
  rect.maxy = y + radius;
  rectObj rcrect;
  rcrect.minx = rc.left;
  rcrect.maxx = rc.right;
  rcrect.miny = rc.top;
  rcrect.maxy = rc.bottom;

  if (msRectOverlap(&rect, &rcrect) != MS_TRUE) {
    return FALSE;
  }

  // JMW added faster checking...

  start = AngleLimit360(start);
  end = AngleLimit360(end);

  istart = iround(start / 360.0 * 64);
  iend = iround(end / 360.0 * 64);

  int npoly = 0;

  if (istart > iend) {
    iend += 64;
  }
  istart++;
  iend--;

  pt[npoly].x = x + (long)(radius * fastsine(start));
  pt[npoly].y = y - (long)(radius * fastcosine(start));
  npoly++;

  for (i = 0; i < 64; i++) {
    if (i <= iend - istart) {
      pt[npoly].x = x + (long)(radius * xcoords[(i + istart) % 64]);
      pt[npoly].y = y - (long)(radius * ycoords[(i + istart) % 64]);
      npoly++;
    }
  }
  pt[npoly].x = x + (long)(radius * fastsine(end));
  pt[npoly].y = y - (long)(radius * fastcosine(end));
  npoly++;
  if (npoly) {
    canvas.polyline(pt, npoly); // TODO check ClipPolygon for HP31X
  }

  return TRUE;
}

/* Not used
   void DrawDotLine(HDC hdc, POINT ptStart, POINT ptEnd, COLORREF cr,
   const RECT rc)
   {
   HPEN hpDot, hpOld;
   LOGPEN dashLogPen;
   POINT pt[2];
   //Create a dot pen
   dashLogPen.lopnColor = cr;
   dashLogPen.lopnStyle = PS_DOT;
   dashLogPen.lopnWidth.x = 0;
   dashLogPen.lopnWidth.y = 0;

   hpDot = (HPEN)CreatePenIndirect(&dashLogPen);
   hpOld = (HPEN)SelectObject(hdc, hpDot);

   pt[0].x = ptStart.x;
   pt[0].y = ptStart.y;
   pt[1].x = ptEnd.x;
   pt[1].y = ptEnd.y;

   Polyline(hdc, pt, 2);

   SelectObject(hdc, hpOld);
   DeleteObject((HPEN)hpDot);
   }

*/
