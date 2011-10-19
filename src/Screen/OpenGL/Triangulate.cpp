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
#include <math.h>
#include <assert.h>

/**
 * Calculate signed area of the polygon to determine the rotary direction.
 */
template <typename PT>
static inline bool
polygon_rotates_left(const PT *points, unsigned num_points)
{
  int area = 0;

  for (unsigned a=num_points-1, b=0; b<num_points; a=b++) {
    area += points[a].x * (int)points[b].y -
            points[a].y * (int)points[b].x;
  }
  return area > 0;  // we actually calculated area*2
}

/**
 * Test whether the area of a triangle is zero, or not.
 */
template <typename PT>
static inline bool
triangle_empty(const PT &a, const PT &b, const PT &c)
{
  return ((b.x-a.x) * (int)(c.y-b.y) -
          (b.y-a.y) * (int)(c.x-b.x)) == 0;
}

/**
 * Test whether point p ist left of line (a,b) or not
 */
template <typename PT>
static inline bool
point_left_of_line(const PT &p, const PT &a, const PT &b)
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
template <typename PT>
static inline bool
inside_triangle(const PT &p, const PT &a, const PT &b, const PT &c)
{
  return point_left_of_line(p, a, b) &&
         point_left_of_line(p, b, c) &&
         point_left_of_line(p, c, a);
}

/**
 * Test whether the line a,b,c makes a bend to the left or not.
 */
template <typename PT>
static inline bool
left_bend(const PT &a, const PT &b, const PT &c)
{
  return ((b.x-a.x) * (int)(c.y-b.y) -
          (b.y-a.y) * (int)(c.x-b.x)) > 0;
}

/**
 * Scale vector v to a given length.
 */
static inline void
normalize(RasterPoint *v, float length)
{
  // TODO: optimize!
  float scale = length / sqrt(v->x*(float)v->x + v->y*(float)v->y);
  v->x = floor(v->x*scale + 0.5f);
  v->y = floor(v->y*scale + 0.5f);
}

#if RASTER_POINT_SIZE == SHAPE_POINT_SIZE
unsigned
polygon_to_triangle(const RasterPoint *points, unsigned num_points,
                    GLushort *triangles, unsigned min_distance)
#else
template <typename PT>
static inline unsigned
_polygon_to_triangle(const PT *points, unsigned num_points,
                     GLushort *triangles, unsigned min_distance)
#endif
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
      bool point_removeable = triangle_empty(points[a], points[b], points[c]);
      if (!point_removeable) {
        unsigned distance = manhattan_distance(points[a], points[b]);
        if (distance < min_distance) {
          point_removeable = true;
          if (distance != 0) {
            for (unsigned p = next[c]; p != a; p = next[p]) {
              if (inside_triangle(points[p], points[a], points[b], points[c])) {
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
          start = std::min(a, c);  // keep track of the smallest index
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
  GLushort *t = triangles;
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
      delete[] next;
      return 0;
    }
  }

  delete[] next;
  return t - triangles;
}

#if RASTER_POINT_SIZE != SHAPE_POINT_SIZE
unsigned
polygon_to_triangle(const RasterPoint *points, unsigned num_points,
                    GLushort *triangles, unsigned min_distance)
{
  return _polygon_to_triangle(points, num_points, triangles, min_distance);
}

unsigned
polygon_to_triangle(const ShapePoint *points, unsigned num_points,
                    GLushort *triangles, unsigned min_distance)
{
  return _polygon_to_triangle(points, num_points, triangles, min_distance);
}
#endif

unsigned
triangle_to_strip(GLushort *triangles, unsigned index_count,
                  unsigned vertex_count, unsigned polygon_count)
{
  // count the number of occurrences for each vertex
  GLushort *vcount = new GLushort[vertex_count]();
  GLushort *t = triangles;
  GLushort *v;
  const GLushort * const t_end = triangles + index_count;
  for (v = t; v < t_end; v++)
    vcount[*v]++;

  const unsigned triangle_buffer_size = index_count + 2*(polygon_count-1);
  GLushort *triangle_strip = new GLushort[triangle_buffer_size];
  GLushort *strip = triangle_strip;

  // search a start point with only one reference
  for (v = t; v < t_end; v++)
    if (vcount[*v] == 1)
      break;
  strip[0] = *v;
  v = t + (v-t)/3*3;
  strip[1] = (v[0] == strip[0]) ? v[1] : v[0];
  strip[2] = (v[2] == strip[0]) ? v[1] : v[2];

  unsigned strip_size = 0;
  unsigned triangles_left = index_count/3;
  while (triangles_left > 1) {
    bool found_something = false;

    vcount[v[0]]--;  vcount[v[1]]--;  vcount[v[2]]--;
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
    assert(strip+4 <= triangle_strip+triangle_buffer_size);

    // search for a shared edge
    if (vcount[strip[1]] > 0 && vcount[strip[2]] > 0) {
      for (v = t; v < t_end; v += 3) {
        if ((strip[1] == v[0] || strip[1] == v[1] || strip[1] == v[2]) &&
            (strip[2] == v[0] || strip[2] == v[1] || strip[2] == v[2])) {
          // add triangle to strip
          strip[3] = v[0] != strip[1] && v[0] != strip[2] ? v[0] :
                     v[1] != strip[1] && v[1] != strip[2] ? v[1] : v[2];
          strip++;
          strip_size++;
          found_something = true;
          break;
        }
      }
    }
    if (found_something)
      continue;

    // search for a single shared vertex
    if (vcount[strip[1]] + vcount[strip[2]] > 0) {
      for (v = t; v < t_end; v++) {
        if (strip[2] == *v) {
          found_something = true;
          break;
        }
        if (strip_size == 0 && strip[1] == *v) {
          // swap the last two indices
          GLushort tmp = strip[1];
          strip[1] = strip[2];
          strip[2] = tmp;
          found_something = true;
          break;
        }
      }
    }

    if (!found_something) {
      // search for a vertex that has only one reference left
      for (v = t; v < t_end; v++)
        if (vcount[*v] == 1)
          break;
      assert(v != t_end);

      // add two redundant points
      assert(strip+5 <= triangle_strip+triangle_buffer_size);
      strip += 2;
      strip[1] = strip[0];
      strip[2] = *v;
    }

    // add triangle to strip
    assert(strip+6 <= triangle_strip+triangle_buffer_size);
    strip += 3;
    strip[0] = *v;
    v = t + (v-t)/3*3;
    strip[1] = (v[0] == strip[0]) ? v[1] : v[0];
    strip[2] = (v[2] == strip[0]) ? v[1] : v[2];
    strip_size = 0;
  }
  strip += 3;

  // copy strip over triangles
  for (t=triangles, v=triangle_strip; v < strip; v++, t++)
    *t = *v;

  //LogDebug(_T("triangle_to_strip: indices=%u strip indices=%u (%u%%)"),
  //         index_count, strip - triangle_strip,
  //         (strip - triangle_strip)*100/index_count);

  delete[] triangle_strip;
  delete[] vcount;

  return strip - triangle_strip;
}

unsigned
line_to_triangle(const RasterPoint *points, unsigned num_points,
                 RasterPoint *strip, unsigned line_width,
                 bool loop, bool tcap)
{
  if (num_points < 2)
    return 0;
  if (loop && num_points < 3)
    loop = false;

  float half_line_width = line_width*0.5f;
  RasterPoint *s = strip;
  const RasterPoint *a, *b, *c;
  const RasterPoint * const points_end = points + num_points;

  // initialize a, b and c vertices
  if (loop) {
    b = points + num_points - 1;
    a = b-1;
    while (a >= points && a->x == b->x && a->y == b->y)
      a--;  // skip identical points
    if (a < points)
      return 0;  // no two different points found
    c = points;
  } else  {
    a = points;
    b = a+1;
    while (b != points_end && a->x == b->x && a->y == b->y)
      b++;  // skip identical points
    if (b == points_end)
      return 0;  // no two different points found
    c = b+1;
  }
  while (c != points_end && b->x == c->x && b->y == c->y)
    c++;  // skip identical points

  if (!loop) {
    // add flat or triangle cap at beginning of line
    RasterPoint p;
    if (tcap) {
      p.x = a->x - b->x;
      p.y = a->y - b->y;
      normalize(&p, half_line_width);

      s->x = a->x + p.x;
      s->y = a->y + p.y;
      s++;
    }
    p.x = a->y - b->y;
    p.y = b->x - a->x;
    normalize(&p, half_line_width);

    s->x = a->x - p.x;
    s->y = a->y - p.y;
    s++;
    s->x = a->x + p.x;
    s->y = a->y + p.y;
    s++;
  }

  // add points by calculating the angle bisector of ab and bc
  if (num_points >= 3) {
    while (c != points_end) {
      if (!triangle_empty(*a, *b, *c)) {  // skip zero or 180 degree bends
        // TODO: support 180 degree bends!

        RasterPoint g, h;
        g.x = b->x - a->x;
        g.y = b->y - a->y;
        h.x = c->x - b->x;
        h.y = c->y - b->y;
        normalize(&g, 1000.);
        normalize(&h, 1000.);
        int bisector_x = -g.y - h.y;
        int bisector_y = g.x + h.x;

        float projected_length = (-g.y*bisector_x + g.x*bisector_y) * (1.f/1000.f);

        // HACK: reduce artefacts for acute angles
        if (projected_length < 400.f)
          projected_length = 400.f;

        float scale = half_line_width / projected_length;
        bisector_x = floor(bisector_x*scale + 0.5f);
        bisector_y = floor(bisector_y*scale + 0.5f);

        s->x = b->x - bisector_x;
        s->y = b->y - bisector_y;
        s++;
        s->x = b->x + bisector_x;
        s->y = b->y + bisector_y;
        s++;
      }

      a=b;  b=c;  c++;
      while (c != points_end && b->x == c->x && b->y == c->y)
        c++;  // skip identical points
    }
  }

  if (loop) {
    // repeat first two points at the end
    s->x = strip[0].x;
    s->y = strip[0].y;
    s++;
    s->x = strip[1].x;
    s->y = strip[1].y;
    s++;
  } else {
    // add flat or triangle cap at end of line
    RasterPoint p;
    p.x = a->y - b->y;
    p.y = b->x - a->x;
    normalize(&p, half_line_width);

    s->x = b->x - p.x;
    s->y = b->y - p.y;
    s++;
    s->x = b->x + p.x;
    s->y = b->y + p.y;
    s++;
    if (tcap) {
      p.x = b->x - a->x;
      p.y = b->y - a->y;
      normalize(&p, half_line_width);

      s->x = b->x + p.x;
      s->y = b->y + p.y;
      s++;
    }
  }

  return s - strip;
}
