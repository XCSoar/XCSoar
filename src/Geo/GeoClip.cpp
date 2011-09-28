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

#include "Geo/GeoClip.hpp"
#include "Compiler.h"

#include <assert.h>

gcc_const
static GeoPoint
clip_longitude(const GeoPoint origin, const GeoPoint pt, Angle at)
{
  Angle dx = pt.longitude - origin.longitude;
  Angle dy = pt.latitude - origin.latitude;

  Angle ex = at - origin.longitude;
  Angle ey = ex * (dy.Native() / dx.Native());

  return GeoPoint(at, origin.latitude + ey);
}

gcc_const
static GeoPoint
clip_latitude(const GeoPoint origin, const GeoPoint pt, Angle at)
{
  Angle dx = pt.longitude - origin.longitude;
  Angle dy = pt.latitude - origin.latitude;

  Angle ey = at - origin.latitude;
  Angle ex = ey * (dx.Native() / dy.Native());

  return GeoPoint(origin.longitude + ex, at);
}

bool
GeoClip::clip_point(const GeoPoint &origin, GeoPoint &pt) const
{
  const Angle zero = Angle::Zero();

  if (pt.longitude < zero) {
    if (origin.longitude <= zero)
      return false;

    pt = clip_longitude(origin, pt, zero);
  } else if (pt.longitude > width) {
    if (origin.longitude >= width)
      return false;

    pt = clip_longitude(origin, pt, width);
  }

  if (pt.latitude < south) {
    if (origin.latitude <= south)
      return false;

    pt = clip_latitude(origin, pt, south);
  } else if (pt.latitude > north) {
    if (origin.latitude >= north)
      return false;

    pt = clip_latitude(origin, pt, north);
  }

  return true;
}

bool
GeoClip::clip_line(GeoPoint &a, GeoPoint &b) const
{
  GeoPoint a2 = import_point(a);
  GeoPoint b2 = import_point(b);

  if (!clip_point(a2, b2) || !clip_point(b2, a2))
    return false;

  a = export_point(a2);
  b = export_point(b2);
  return true;
}

static unsigned
clip_vertex_longitude(const Angle west, const Angle east,
                      const GeoPoint &prev, GeoPoint &pt, GeoPoint &insert,
                      const GeoPoint &next)
{
  unsigned num_insert = 0;

  if (pt.longitude < west) {
    if (prev.longitude <= west) {
      if (next.longitude <= west)
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_longitude(next, pt, west);
    } else {
      if (next.longitude > west) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_longitude(next, pt, west);
        ++num_insert;
      }

      pt = clip_longitude(prev, pt, west);
    }
  } else if (pt.longitude > east) {
    if (prev.longitude >= east) {
      if (next.longitude >= east)
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_longitude(next, pt, east);
    } else {
      if (next.longitude < east) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_longitude(next, pt, east);
        ++num_insert;
      }

      pt = clip_longitude(prev, pt, east);
    }
  }

  return 1 + num_insert;
}

static unsigned
clip_vertex_latitude(const Angle south, const Angle north,
                     const GeoPoint &prev, GeoPoint &pt, GeoPoint &insert,
                     const GeoPoint &next)
{
  unsigned num_insert = 0;

  if (pt.latitude < south) {
    if (prev.latitude <= south) {
      if (next.latitude <= south)
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_latitude(next, pt, south);
    } else {
      if (next.latitude > south) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_latitude(next, pt, south);
        ++num_insert;
      }

      pt = clip_latitude(prev, pt, south);
    }
  } else if (pt.latitude > north) {
    if (prev.latitude >= north) {
      if (next.latitude >= north)
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_latitude(next, pt, north);
    } else {
      if (next.latitude < north) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_latitude(next, pt, north);
        ++num_insert;
      }

      pt = clip_latitude(prev, pt, north);
    }
  }

  return 1 + num_insert;
}

static unsigned
clip_polygon_longitude(const Angle west, const Angle east, GeoPoint *dest,
                       const GeoPoint *src, unsigned src_length)
{
  /* this array always holds the current vertex and its two neighbors;
     it is filled in advance with the last two points, because that
     avoids range checking inside the loop */
  GeoPoint three[3];
  three[0] = src[src_length - 2];
  three[1] = src[src_length - 1];

  unsigned dest_length = 0;
  for (unsigned i = 0; i < src_length; ++i) {
    if (i < src_length - 1)
      three[2] = src[i];
    else {
      /* the last vertex may have been removed in the first iteration,
         so use the first element of the "dest" buffer instead */

      if (dest_length == 0)
        return 0;

      three[2] = dest[0];
    }

    GeoPoint insert;
    unsigned n = clip_vertex_longitude(west, east,
                                       three[0], three[1], insert, three[2]);
    assert(n <= 2);

    if (n == 0) {
      /* thee points outside, the middle one can be deleted */
      three[1] = three[2];
    } else if (n == 1) {
      /* no change in vertex count, but the current vertex may have
         been edited */
      dest[dest_length++] = three[1];
      three[0] = three[1];
      three[1] = three[2];
    } else if (n == 2) {
      /* one vertex has been inserted */
      dest[dest_length++] = three[1];
      three[0] = three[1];
      three[1] = insert;

      /* backtrack, inspect inserted vertex in the next iteration */
      --i;
    }
  }

  return dest_length;
}

static unsigned
clip_polygon_latitude(const Angle south, const Angle north, GeoPoint *dest,
                      const GeoPoint *src, unsigned src_length)
{
  /* this array always holds the current vertex and its two neighbors;
     it is filled in advance with the last two points, because that
     avoids range checking inside the loop */
  GeoPoint three[3];
  three[0] = src[src_length - 2];
  three[1] = src[src_length - 1];

  unsigned dest_length = 0;
  for (unsigned i = 0; i < src_length; ++i) {
    if (i < src_length - 1)
      three[2] = src[i];
    else {
      /* the last vertex may have been removed in the first iteration,
         so use the first element of the "dest" buffer instead */

      if (dest_length == 0)
        return 0;

      three[2] = dest[0];
    }

    GeoPoint insert;
    unsigned n = clip_vertex_latitude(south, north,
                                      three[0], three[1], insert, three[2]);
    assert(n <= 2);

    if (n == 0) {
      /* thee points outside, the middle one can be deleted */
      three[1] = three[2];
    } else if (n == 1) {
      /* no change in vertex count, but the current vertex may have
         been edited */
      dest[dest_length++] = three[1];
      three[0] = three[1];
      three[1] = three[2];
    } else if (n == 2) {
      /* one vertex has been inserted */
      dest[dest_length++] = three[1];
      three[0] = three[1];
      three[1] = insert;

      /* backtrack, inspect inserted vertex in the next iteration */
      --i;
    }
  }

  return dest_length;
}

unsigned
GeoClip::clip_polygon(GeoPoint *dest,
                      const GeoPoint *src, unsigned src_length) const
{
  if (src_length < 3)
    return 0;

  GeoPoint *imported = dest + src_length * 2;
  for (unsigned i = 0; i < src_length; ++i)
    imported[i] = import_point(src[i]);

  GeoPoint *first_stage = dest + src_length;
  unsigned n = clip_polygon_longitude(Angle::Zero(), width,
                                      first_stage, imported, src_length);
  if (n < 3)
    return 0;

  n = clip_polygon_latitude(south, north, dest, first_stage, n);
  if (n < 3)
    return 0;

  for (unsigned i = 0; i < n; ++i)
    dest[i] = export_point(dest[i]);
  return n;
}
