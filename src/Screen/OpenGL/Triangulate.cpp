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

#include "Screen/OpenGL/Triangulate.hpp"
#include "Screen/Point.hpp"

#include <algorithm>
#include <assert.h>

/**
 * Calculate signed area of the polygon to determine the rotary direction.
 */
static inline bool
polygon_rotates_left(const RasterPoint *points, unsigned num_points)
{
  int area = 0;

  for (unsigned a=num_points-1, b=0; b<num_points; a=b++) {
    area += points[a].x * (int)points[b].y -
            points[a].y * (int)points[b].x;
  }
  return area > 0;  // we actually calculated area*2
}

/**
 * Test whether point p ist left of line (a,b) or not
 */
static inline bool
point_left_of_line(const RasterPoint &p, const RasterPoint &a,
                   const RasterPoint &b)
{
  // normal vector of the line
  int nx = a.y - b.y;
  int ny = b.x - a.x;
  // vector ap
  int apx = p.x - a.x;
  int apy = p.y - a.y;

  // almost distance point from line (normal has to be normalized for that)
  return (nx*apx + ny*apy) >= 0;
}

/**
 * Test whether point p is in inside triangle(a,b,c) or not. Triangle must be
 * counterclockwise, assuming (0,0) is the lower left.
 */
static inline bool
inside_triangle(const RasterPoint &p, const RasterPoint &a,
                const RasterPoint &b, const RasterPoint &c)
{
  return point_left_of_line(p, a, b) &&
         point_left_of_line(p, b, c) &&
         point_left_of_line(p, c, a);
}

/**
 * Test whether the line a,b,c makes a bend to the left or not.
 */
static inline bool
left_bend(const RasterPoint &a, const RasterPoint &b, const RasterPoint &c)
{
  return ((b.x-a.x) * (int)(c.y-b.y) -
          (b.y-a.y) * (int)(c.x-b.x)) > 0;
}

/**
 * cutting ears - simple algorithm, no support for holes
 * Optionally removes all points from a polygon that are too close together.
 *
 * @param points polygon coordinates
 * @param num_points numer of polygon vertices
 * @param triangles triangle indices, size: 3*(num_points-2)
 * @param min_distance minimum distance a point should have from its neighbours
 *
 * @return Returns the number of triangle indices. Possible values:
 *         0: failure,
 *         3 to 3*(num_points-3): success
 */
unsigned
polygon_to_triangle(const RasterPoint *points, unsigned num_points,
                    GLushort *triangles, unsigned min_distance)
{
  //const unsigned orig_num_points = num_points;

  // no redundant start/end please
  if (num_points >= 1 &&
      points[0].x == points[num_points-1].x &&
      points[0].y == points[num_points-1].y) {
    num_points--;
  }

  if (num_points < 3)
    return 0;

  assert(num_points < 65536);
  GLushort *next = new GLushort[num_points];  // next vertex pointer
  GLushort *t = triangles;
  GLushort start = 0;  // index of the first vertex

  // initialize next pointer counterclockwise
  if (polygon_rotates_left(points, num_points)) {
    for (unsigned i = 0; i < num_points-1; i++)
      next[i] = i+1;
    next[num_points-1] = 0;
  } else {
    next[0] = num_points-1;
    for (unsigned i = 1; i < num_points; i++)
      next[i] = i-1;
  }

  // thinning
  if (min_distance > 0) {
    for (unsigned a=start, b=next[a], c=next[b], heat=0;
         num_points > 3 && heat < num_points;
         a=b, b=c, c=next[c], heat++) {
      unsigned distance = manhattan_distance(points[a], points[b]);
      if (distance < min_distance) {
        bool point_removeable = true;
        if (distance != 0) {
          for (unsigned p = next[c]; p != a; p = next[p]) {
            if (inside_triangle(points[p], points[a], points[b], points[c])) {
              point_removeable = false;
              break;
            }
          }
        }
        if (point_removeable) {
          // remove node b from polygon
          if (b == start)
            start = std::min(a, c);  // keep track of the smallest index
          next[a] = c;
          num_points--;
          // 'a' should stay the same in the next loop
          b = a;
          // reset heat
          heat = 0;
        }
      }
    }
    //LogDebug(_T("polygon thinning (%u) removed %u of %u vertices"),
    //         min_distance, orig_num_points-num_points, orig_num_points);
  }

  // triangulation
  const int triangle_idx_count = 3*(num_points-2);
  for (unsigned a=start, b=next[a], c=next[b], heat=0;
       num_points > 2;
       a=b, b=c, c=next[c]) {
    if (left_bend(points[a], points[b], points[c])) {
      bool ear_cuttable = true;
      for (unsigned p = next[c]; p != a; p = next[p]) {
        if (inside_triangle(points[p], points[a], points[b], points[c])) {
          ear_cuttable = false;
          break;
        }
      }
      if (ear_cuttable) {
        // save triangle indices
        *t++ = a; *t++ = b; *t++ = c;
        // remove node b from polygon
        next[a] = c;
        num_points--;
        // 'a' should stay the same in the next loop
        b = a;
        // reset heat
        heat = 0;
      }
    }
    if (heat++ > num_points) {
      // if polygon edges overlap we may loop endlessly
      //LogDebug(_T("polygon_to_triangle: bad polygon"));
      delete next;
      return 0;
    }
  }

  delete next;
  return triangle_idx_count;
}
