/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/BulkPoint.hpp"
#include "Math/Line2D.hpp"
#include "Util/AllocatedArray.hxx"

#include <algorithm>
#include <math.h>
#include <assert.h>

/**
 * Calculate signed area of the polygon to determine the rotary direction.
 */
template <typename PT>
static inline bool
PolygonRotatesLeft(const PT *points, unsigned num_points)
{
  typename PT::product_type area = 0;

  for (unsigned a = num_points - 1, b = 0; b < num_points; a = b++)
    area += CrossProduct(points[a], points[b]);

  // we actually calculated area * 2
  return area > 0;
}

/**
 * Test whether point p ist left of line (a,b) or not
 *
 * @return: positive if p is left of a,b; zero if p is on a,b; else negative
 */
template <typename PT>
static inline typename PT::product_type
PointLeftOfLine(const PT &p, const PT &a, const PT &b)
{
  return Line2D<PT>(a, b).LocatePoint(p);
}

/**
 * Test whether point p is in inside triangle(a,b,c) or not. Triangle must be
 * counterclockwise, assuming (0,0) is the lower left.
 */
template <typename PT>
static inline bool
InsideTriangle(const PT &p, const PT &a, const PT &b, const PT &c)
{
  return PointLeftOfLine(p, a, b) > 0 &&
         PointLeftOfLine(p, b, c) > 0 &&
         PointLeftOfLine(p, c, a) > 0;
}

/**
 * Test whether the line a,b,c makes a bend to the left or not.
 *
 * @return: positive if a,b,c turns left, zero for a spike, negative otherwise
 */
template <typename PT>
static inline typename PT::product_type
LeftBend(const PT &a, const PT &b, const PT &c)
{
  const PT ab = b - a;
  const PT bc = c - b;

  return CrossProduct(ab, bc);
}

/**
 * Test whether the area of a triangle is zero, or not.
 */
template <typename PT>
static inline bool
TriangleEmpty(const PT &a, const PT &b, const PT &c)
{
  return LeftBend(a, b, c) == 0;
}

/**
 * Scale vector v to a given length.
 */
static inline void
Normalize(PixelPoint *v, float length)
{
  // TODO: optimize!
  double squared_length = v->x * (PixelPoint::product_type)v->x +
                          v->y * (PixelPoint::product_type)v->y;
  float scale = length / sqrt(squared_length);
  v->x = lround(v->x * scale);
  v->y = lround(v->y * scale);
}

template <typename PT>
static unsigned
_PolygonToTriangles(const PT *points, unsigned num_points,
                    GLushort *triangles, typename PT::scalar_type min_distance)
{
  // no redundant start/end please
  if (num_points >= 1 && points[0] == points[num_points - 1])
    num_points--;

  if (num_points < 3)
    return 0;

  assert(num_points < 65536);
  // next vertex pointer
  auto next = new GLushort[num_points];
  // index of the first vertex
  GLushort start = 0;

  // initialize next pointer counterclockwise
  if (PolygonRotatesLeft(points, num_points)) {
    for (unsigned i = 0; i < num_points-1; i++)
      next[i] = i + 1;
    next[num_points - 1] = 0;
  } else {
    next[0] = num_points - 1;
    for (unsigned i = 1; i < num_points; i++)
      next[i] = i - 1;
  }

  // thinning
  if (min_distance > 0) {
    for (unsigned a = start, b = next[a], c = next[b], heat = 0;
         num_points > 3 && heat < num_points;
         a = b, b = c, c = next[c], heat++) {
      bool point_removeable = TriangleEmpty(points[a], points[b], points[c]);
      if (!point_removeable) {
        typename PT::scalar_type distance = ManhattanDistance(points[a],
                                                              points[b]);
        if (distance < min_distance) {
          point_removeable = true;
          if (distance > 0) {
            for (unsigned p = next[c]; p != a; p = next[p]) {
              if (InsideTriangle(points[p], points[a], points[b], points[c])) {
                point_removeable = false;
                break;
              }
            }
          }
        }
      }
      if (point_removeable) {
        // remove node b from polygon
        if (b == start)
          // keep track of the smallest index
          start = std::min(a, c);

        next[a] = c;
        num_points--;
        // 'a' should stay the same in the next loop
        b = a;
        // reset heat
        heat = 0;
      }
    }
    //LogDebug(_T("polygon thinning (%u) removed %u of %u vertices"),
    //         min_distance, orig_num_points-num_points, orig_num_points);
  }

  // triangulation
  auto t = triangles;
  for (unsigned a = start, b = next[a], c = next[b], heat = 0;
       num_points > 2; a = b, b = c, c = next[c]) {
    typename PT::product_type bendiness =
      LeftBend(points[a], points[b], points[c]);

    // left bend, spike or line with a redundant point in the middle
    bool ear_cuttable = (bendiness >= 0);

    if (bendiness > 0) {
      // left bend
      for (unsigned prev_p = c, p = next[c]; p != a;
           prev_p = p, p = next[p]) {
        typename PT::product_type ab = PointLeftOfLine(points[p], points[a],
                                                       points[b]);
        typename PT::product_type bc = PointLeftOfLine(points[p], points[b],
                                                       points[c]);
        typename PT::product_type ca = PointLeftOfLine(points[p], points[c],
                                                       points[a]);
        if (ab > 0 && bc > 0 && ca > 0) {
          // p is inside a,b,c
          ear_cuttable = false;
          break;
        } else if (ab >= 0 && bc >= 0 && ca >= 0) {
          // p is on one or two edges of a,b,c
          bool outside_ab = (ab == 0) &&
            PointLeftOfLine(points[prev_p], points[a], points[b]) <= 0;
          bool outside_bc = (bc == 0) &&
            PointLeftOfLine(points[prev_p], points[b], points[c]) <= 0;
          bool outside_ca = (ca == 0) &&
            PointLeftOfLine(points[prev_p], points[c], points[a]) <= 0;
          if (!(outside_ab || outside_bc || outside_ca)) {
            // line p,prev_p intersects with triangle a,b,c
            ear_cuttable = false;
            break;
          }

          outside_ab = (ab == 0) &&
            PointLeftOfLine(points[next[p]], points[a], points[b]) <= 0;
          outside_bc = (bc == 0) &&
            PointLeftOfLine(points[next[p]], points[b], points[c]) <= 0;
          outside_ca = (ca == 0) &&
            PointLeftOfLine(points[next[p]], points[c], points[a]) <= 0;
          if (!(outside_ab || outside_bc || outside_ca)) {
            // line p,next[p] intersects with triangle a,b,c
            ear_cuttable = false;
            break;
          }
        }
      }
      if (ear_cuttable) {
        // save triangle indices
        *t++ = a;
        *t++ = b;
        *t++ = c;
      }
    }

    if (ear_cuttable) {
      // remove node b from polygon
      next[a] = c;
      num_points--;
      // 'a' should stay the same in the next loop
      b = a;
      // reset heat
      heat = 0;
    }

    if (heat++ > num_points) {
      // if polygon edges overlap we may loop endlessly
      //LogDebug(_T("polygon_to_triangle: bad polygon"));
      delete[] next;
      return 0;
    }
  }

  delete[] next;
  return t - triangles;
}

unsigned
PolygonToTriangles(const BulkPixelPoint *points, unsigned num_points,
                   AllocatedArray<GLushort> &triangles, unsigned min_distance)
{
  triangles.GrowDiscard(3 * (num_points - 2));
  return _PolygonToTriangles(points, num_points, triangles.begin(),
                             min_distance);
}

unsigned
PolygonToTriangles(const FloatPoint2D *points, unsigned num_points,
                   GLushort *triangles, float min_distance)
{
  return _PolygonToTriangles(points, num_points, triangles, min_distance);
}

/**
 * Count the occurrences of each value.
 */
static void
AddValueCounts(unsigned *counts, unsigned max_value,
               const GLushort *values, const GLushort *values_end)
{
  for (auto i = values; i != values_end; ++i) {
    const unsigned value = *i;
    assert(value < max_value);
    counts[value]++;
  }
}

gcc_pure
static GLushort *
FindOne(const unsigned *counts, unsigned max_value,
        GLushort *values, const GLushort *values_end)
{
  for (auto i = values; i != values_end; ++i) {
    const unsigned value = *i;
    assert(value < max_value);
    if (counts[value] == 1)
      return i;
  }

  return nullptr;
}

gcc_pure
static GLushort *
FindSharedEdge(const unsigned idx1, const unsigned idx2,
               GLushort *values, const GLushort *values_end)
{
  for (auto v = values; v < values_end; v += 3)
    if ((idx1 == v[0] || idx1 == v[1] || idx1 == v[2]) &&
        (idx2 == v[0] || idx2 == v[1] || idx2 == v[2]))
      return v;

  return nullptr;
}

unsigned
TriangleToStrip(GLushort *triangles, unsigned index_count,
                unsigned vertex_count, unsigned polygon_count)
{
  if (index_count < 3)
    /* bail out, because we didn't get even one triangle; we need to
       catch this special case, because the loop below assumes that
       there must be at least one */
    return 0;

  // count the number of occurrences for each vertex
  auto vcount = new unsigned[vertex_count]();
  auto t = triangles;
  const auto t_end = triangles + index_count;

  AddValueCounts(vcount, vertex_count, t, t_end);

  const unsigned triangle_buffer_size = index_count + 2 * (polygon_count - 1);
  const auto triangle_strip = new GLushort[triangle_buffer_size];
  auto strip = triangle_strip;

  // search a start point with only one reference
  GLushort *v = FindOne(vcount, vertex_count, t, t_end);
  assert(v >= t && v < t_end);

  strip[0] = *v;
  v = t + (v - t) / 3 * 3;
  strip[1] = (v[0] == strip[0]) ? v[1] : v[0];
  strip[2] = (v[2] == strip[0]) ? v[1] : v[2];

  unsigned strip_size = 0;
  unsigned triangles_left = index_count / 3;
  while (triangles_left > 1) {
    assert(v >= t && v < t_end);
    assert((v - t) % 3 == 0);
    assert(v[0] < vertex_count);
    assert(v[1] < vertex_count);
    assert(v[2] < vertex_count);
    assert(vcount[v[0]] > 0);
    assert(vcount[v[1]] > 0);
    assert(vcount[v[2]] > 0);

    vcount[v[0]]--;
    vcount[v[1]]--;
    vcount[v[2]]--;

    // fill hole in triangle array
    if (v != t) {
      v[0] = t[0];
      v[1] = t[1];
      v[2] = t[2];
    }
    t += 3;
    triangles_left--;

    /* TODO: I'm almost sure this is true, but I can't prove it. Maybe there
     *       are polygons, where the degenerated triangle strip is longer
     *       than all triangle indices.
     *       Use a higher polygon_count and bigger triangle buffer if you
     *       hit this one.
     */
    assert(strip + 4 <= triangle_strip + triangle_buffer_size);

    // search for a shared edge
    assert(strip[1] < vertex_count);
    assert(strip[2] < vertex_count);
    if (vcount[strip[1]] > 0 && vcount[strip[2]] > 0) {
      v = FindSharedEdge(strip[1], strip[2], t, t_end);
      if (v != nullptr) {
        // add triangle to strip
        assert(v >= t && v < t_end);

        strip[3] = v[0] != strip[1] && v[0] != strip[2] ? v[0] :
          v[1] != strip[1] && v[1] != strip[2] ? v[1] : v[2];
        strip++;
        strip_size++;
        continue;
      }
    }

    // search for a single shared vertex
    bool found_something = false;
    if (vcount[strip[1]] + vcount[strip[2]] > 0) {
      for (v = t; v < t_end; v++) {
        if (strip[2] == *v) {
          found_something = true;
          break;
        }
        if (strip_size == 0 && strip[1] == *v) {
          // swap the last two indices
          std::swap(strip[1], strip[2]);
          found_something = true;
          break;
        }
      }
    }

    if (!found_something) {
      // search for a vertex that has only one reference left
      v = FindOne(vcount, vertex_count, t, t_end);
      assert(v >= t && v < t_end);

      // add two redundant points
      assert(strip + 5 <= triangle_strip + triangle_buffer_size);
      strip += 2;
      strip[1] = strip[0];
      strip[2] = *v;
    }

    // add triangle to strip
    assert(strip + 6 <= triangle_strip + triangle_buffer_size);
    strip += 3;
    strip[0] = *v;
    v = t + (v - t) / 3 * 3;
    strip[1] = (v[0] == strip[0]) ? v[1] : v[0];
    strip[2] = (v[2] == strip[0]) ? v[1] : v[2];
    strip_size = 0;
  }
  strip += 3;

  // copy strip over triangles
  std::copy(triangle_strip, strip, triangles);

  //LogDebug(_T("triangle_to_strip: indices=%u strip indices=%u (%u%%)"),
  //         index_count, strip - triangle_strip,
  //         (strip - triangle_strip)*100/index_count);

  delete[] triangle_strip;
  delete[] vcount;

  return strip - triangle_strip;
}

/**
 * Append a BulkPixelPoint to the end of an array and advance the array pointer
 */
static void
AppendPoint(BulkPixelPoint *&strip, int x, int y)
{
  strip->x = x;
  strip->y = y;
  strip++;
}

unsigned
LineToTriangles(const BulkPixelPoint *points, unsigned num_points,
                AllocatedArray<BulkPixelPoint> &strip,
                unsigned line_width, bool loop, bool tcap)
{
  // A line has to have at least two points
  if (num_points < 2)
    return 0;

  // allocate memory for triangle vertices
  // max. size: 2*(num_points + (int)(loop || tcap))
  strip.GrowDiscard(2 * (num_points + 1));

  // A closed line path needs to have at least three points
  if (loop && num_points < 3)
    // .. otherwise don't close it
    loop = false;

  float half_line_width = line_width * 0.5f;

  // strip will point to the start of the output array
  // s is the working pointer
  auto *s = strip.begin();

  // a, b and c point to three consecutive points which are used to iterate
  // through the line given in 'points'. Where b is the current position,
  // a the previous point and c the next point.
  const BulkPixelPoint *a, *b, *c;

  // pointer to the end of the original points array
  // used for faster loop conditions
  const auto points_end = points + num_points;

  // initialize a, b and c vertices
  if (loop) {
    b = points + num_points - 1;
    a = b - 1;

    // skip identical points before b
    while (a >= points && *a == *b)
      a--;

    if (a < points)
      // all points in the array are identical
      return 0;

    c = points;
  } else  {
    a = points;
    b = a + 1;

    // skip identical points after a
    while (b != points_end && *a == *b)
      b++;

    if (b == points_end)
      // all points in the array are identical
      return 0;

    c = b + 1;
  }

  // skip identical points after b
  while (c != points_end && *b == *c)
    c++;

  if (!loop) {
    // add flat or triangle cap at beginning of line
    PixelPoint ba = *a - *b;
    Normalize(&ba, half_line_width);

    if (tcap)
      // add triangle cap coordinate to the output array
      AppendPoint(s, a->x + ba.x, a->y + ba.y);

    // add flat cap coordinates to the output array
    PixelPoint p;
    p.x = ba.y;
    p.y = -ba.x;
    AppendPoint(s, a->x - p.x, a->y - p.y);
    AppendPoint(s, a->x + p.x, a->y + p.y);
  }

  // add points by calculating the angle bisector of ab and bc
  int sign = 1;
  if (num_points >= 3) {
    while (c != points_end) {
      // skip zero or 180 degree bends
      // TODO: support 180 degree bends!
      if (!TriangleEmpty(*a, *b, *c)) {
        PixelPoint g = *b - *a, h = *c - *b;
        Normalize(&g, 1000.);
        Normalize(&h, 1000.);
        int bisector_x = -g.y - h.y;
        int bisector_y = g.x + h.x;

        float projected_length = (-g.y * bisector_x + g.x * bisector_y) *
                                 (1.f / 1000.f);
        if (projected_length < 400.f) {
          // acute angle, use the normal of the bisector instead
          projected_length = (g.x * bisector_x + g.y * bisector_y) *
                             (1.f / 1000.f);
          std::swap(bisector_x, bisector_y);
          bisector_y *= -1;
          // the order of the triangles switches. keep track with 'sign'
          sign *= -1;
        }

        float scale = half_line_width / projected_length;
        bisector_x = sign * (int)lround(bisector_x * scale);
        bisector_y = sign * (int)lround(bisector_y * scale);

        AppendPoint(s, b->x - bisector_x, b->y - bisector_y);
        AppendPoint(s, b->x + bisector_x, b->y + bisector_y);
      }

      a = b;
      b = c;
      c++;

      while (c != points_end && *b == *c)
        // skip identical points
        c++;
    }
  }

  if (loop) {
    // repeat first two points at the end
    if (sign == 1) {
      AppendPoint(s, strip[0].x, strip[0].y);
      AppendPoint(s, strip[1].x, strip[1].y);
    } else {
      AppendPoint(s, strip[1].x, strip[1].y);
      AppendPoint(s, strip[0].x, strip[0].y);
    }
  } else {
    // add flat or triangle cap at end of line
    PixelPoint ab = *b - *a;
    Normalize(&ab, half_line_width);

    PixelPoint p;
    p.x = sign * -ab.y;
    p.y = sign * ab.x;
    AppendPoint(s, b->x - p.x, b->y - p.y);
    AppendPoint(s, b->x + p.x, b->y + p.y);

    if (tcap)
      AppendPoint(s, b->x + ab.x, b->y + ab.y);
  }

  return s - strip.begin();
}
