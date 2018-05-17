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

static constexpr unsigned CLIP_LEFT_EDGE = 0x1;
static constexpr unsigned CLIP_RIGHT_EDGE = 0x2;
static constexpr unsigned CLIP_BOTTOM_EDGE = 0x4;
static constexpr unsigned CLIP_TOP_EDGE = 0x8;
static constexpr unsigned CLIP_LEFT_EQUALS = 0x10;
static constexpr unsigned CLIP_RIGHT_EQUALS = 0x20;
static constexpr unsigned CLIP_BOTTOM_EQUALS = 0x40;
static constexpr unsigned CLIP_TOP_EQUALS = 0x80;

static constexpr bool CLIP_INSIDE(unsigned a) {
  return !a;
}

static constexpr bool CLIP_REJECT(unsigned a, unsigned b) {
  return a & b;
}

static constexpr bool CLIP_ACCEPT(unsigned a, unsigned b) {
  return !(a | b);
}

unsigned
GeoClip::ClipEncodeX(const Angle x) const
{
  if (x< Angle::Zero())
    return CLIP_LEFT_EDGE;
  if (x> width)
    return CLIP_RIGHT_EDGE;
  return 0;
}

unsigned
GeoClip::ClipEncodeY(const Angle y) const
{
  if (y< GetSouth())
    return CLIP_BOTTOM_EDGE;
  if (y> GetNorth())
    return CLIP_TOP_EDGE;
  return 0;
}

unsigned
GeoClip::ClipEncode(const GeoPoint pt) const
{
  return ClipEncodeX(pt.longitude) | ClipEncodeY(pt.latitude);
}

bool
GeoClip::ClipLine(GeoPoint &a, GeoPoint &b) const
{
  const Angle zero = Angle::Zero();

  GeoPoint a2 = ImportPoint(a);
  GeoPoint b2 = ImportPoint(b);

  unsigned code1 = ClipEncode(a2);
  unsigned code2 = ClipEncode(b2);

  bool swapped = false;

  while (true) {

    if (CLIP_ACCEPT(code1, code2)) {
      if (swapped) 
        std::swap(a2, b2);      
      a = ExportPoint(a2);
      b = ExportPoint(b2);
      return true;
    }

    if (CLIP_REJECT(code1, code2))
      return false;

    if (CLIP_INSIDE(code1)) {
      swapped = !swapped;
      std::swap(a2, b2);
      std::swap(code1, code2);
    }

    if (code1 & CLIP_LEFT_EDGE) {
      a2 = clip_longitude(b2, a2, zero);
      code1 = ClipEncodeY(a2.latitude);
    } else if (code1 & CLIP_RIGHT_EDGE) {
      a2 = clip_longitude(b2, a2, width);
      code1 = ClipEncodeY(a2.latitude);
    } else if (code1 & CLIP_BOTTOM_EDGE) {
      a2 = clip_latitude(b2, a2, GetSouth());
      code1 = ClipEncodeX(a2.longitude);
    } else if (code1 & CLIP_TOP_EDGE) {
      a2 = clip_latitude(b2, a2, GetNorth());
      code1 = ClipEncodeX(a2.longitude);
    }
  }
}

class ClipGeoPoint: public GeoPoint {
public:
  ClipGeoPoint(const GeoPoint& _p): GeoPoint(_p), clip_code(0)
  {
  };

  ClipGeoPoint(): clip_code(0) {}

  unsigned clip_code;

  void ClipEncodeY(const Angle& south, const Angle& north) {
    if (latitude< south)
      clip_code = CLIP_BOTTOM_EDGE;
    else if (latitude == south) 
      clip_code = CLIP_BOTTOM_EQUALS;
    else if (latitude> north)
      clip_code = CLIP_TOP_EDGE;
    else if (latitude == north)
      clip_code = CLIP_TOP_EQUALS;
    else
      clip_code = 0;
  }

  void ClipEncodeX(const Angle& west, const Angle& east) {
    if (longitude< west)
      clip_code = CLIP_LEFT_EDGE;
    else if (longitude == west)
      clip_code = CLIP_LEFT_EQUALS;
    else if (longitude> east)
      clip_code = CLIP_RIGHT_EDGE;
    else if (longitude == east)
      clip_code = CLIP_RIGHT_EQUALS;
    else
      clip_code = 0;
  }

};

static unsigned
ClipVertexLongitude(const Angle west, const Angle east,
                    const ClipGeoPoint &prev, ClipGeoPoint &pt, ClipGeoPoint &insert,
                    const ClipGeoPoint &next)
{
  unsigned num_insert = 0;

  if (pt.clip_code & CLIP_LEFT_EDGE) {
    if (prev.clip_code & (CLIP_LEFT_EDGE | CLIP_LEFT_EQUALS)) {
      if (next.clip_code & (CLIP_LEFT_EDGE | CLIP_LEFT_EQUALS))
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_longitude(next, pt, west);
      pt.clip_code = CLIP_LEFT_EQUALS;
    } else {
      if (! (next.clip_code & (CLIP_LEFT_EDGE | CLIP_LEFT_EQUALS) )) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_longitude(next, pt, west);
        ++num_insert;
      }

      pt = clip_longitude(prev, pt, west);
      pt.clip_code = CLIP_LEFT_EQUALS;
    }
  } else if (pt.clip_code & CLIP_RIGHT_EDGE) {
    if (prev.clip_code & (CLIP_RIGHT_EDGE | CLIP_RIGHT_EQUALS)) {
      if (next.clip_code & (CLIP_RIGHT_EDGE | CLIP_RIGHT_EQUALS))
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_longitude(next, pt, east);
      pt.clip_code = CLIP_RIGHT_EQUALS;
    } else {
      if (! (next.clip_code & (CLIP_RIGHT_EDGE | CLIP_RIGHT_EQUALS) )) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_longitude(next, pt, east);
        ++num_insert;
      }

      pt = clip_longitude(prev, pt, east);
      pt.clip_code = CLIP_RIGHT_EQUALS;
    }
  }

  return 1 + num_insert;
}

static unsigned
ClipVertex_latitude(const Angle south, const Angle north,
                    const ClipGeoPoint &prev, ClipGeoPoint &pt, ClipGeoPoint &insert,
                    const ClipGeoPoint &next)
{
  unsigned num_insert = 0;

  if (pt.clip_code & CLIP_BOTTOM_EDGE) {
    if (prev.clip_code & (CLIP_BOTTOM_EDGE | CLIP_BOTTOM_EQUALS)) {
      if (next.clip_code & (CLIP_BOTTOM_EDGE | CLIP_BOTTOM_EQUALS))
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_latitude(next, pt, south);
      pt.clip_code = CLIP_BOTTOM_EQUALS;

    } else {
      if (! (next.clip_code & (CLIP_BOTTOM_EDGE | CLIP_BOTTOM_EQUALS) )) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_latitude(next, pt, south);
        ++num_insert;
      }

      pt = clip_latitude(prev, pt, south);
      pt.clip_code = CLIP_BOTTOM_EQUALS;
    }
  } else if (pt.clip_code & CLIP_TOP_EDGE) {
    if (prev.clip_code & (CLIP_TOP_EDGE | CLIP_TOP_EQUALS)) {
      if (next.clip_code & (CLIP_TOP_EDGE | CLIP_TOP_EQUALS))
        /* all three outside, middle one can be deleted */
        return 0;

      pt = clip_latitude(next, pt, north);
      pt.clip_code = CLIP_TOP_EQUALS;
    } else {
      if (! (next.clip_code & (CLIP_TOP_EDGE | CLIP_TOP_EQUALS) )) {
        /* both neighbours are inside, clip both lines and insert a
           new vertex */
        insert = clip_latitude(next, pt, north);

        ++num_insert;
      }

      pt = clip_latitude(prev, pt, north);
      pt.clip_code = CLIP_TOP_EQUALS;
    }
  }

  return 1 + num_insert;
}

static unsigned
ClipPolygonLongitude(const Angle west, const Angle east, GeoPoint *dest,
                     const GeoPoint *src, unsigned src_length)
{
  /* this array always holds the current vertex and its two neighbors;
     it is filled in advance with the last two points, because that
     avoids range checking inside the loop */
  ClipGeoPoint three[3];
  three[0] = src[src_length - 2];
  three[0].ClipEncodeX(west, east);
  three[1] = src[src_length - 1];
  three[1].ClipEncodeX(west, east);

  unsigned dest_length = 0;
  for (unsigned i = 0; i < src_length; ++i) {
    if (i < src_length - 1) {
      three[2] = src[i];
      three[2].ClipEncodeX(west, east);
    } else {
      /* the last vertex may have been removed in the first iteration,
         so use the first element of the "dest" buffer instead */

      if (dest_length == 0)
        return 0;

      three[2] = dest[0];
      three[2].ClipEncodeX(west, east);
    }

    ClipGeoPoint insert;
    unsigned n = ClipVertexLongitude(west, east,
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
ClipPolygonLatitude(const Angle south, const Angle north, GeoPoint *dest,
                    const GeoPoint *src, unsigned src_length)
{
  /* this array always holds the current vertex and its two neighbors;
     it is filled in advance with the last two points, because that
     avoids range checking inside the loop */
  ClipGeoPoint three[3];
  three[0] = src[src_length - 2]; 
  three[0].ClipEncodeY(south, north);
  three[1] = src[src_length - 1]; 
  three[1].ClipEncodeY(south, north);

  unsigned dest_length = 0;
  for (unsigned i = 0; i < src_length; ++i) {
    if (i < src_length - 1) {
      three[2] = src[i]; 
      three[2].ClipEncodeY(south, north);
    }
    else {
      /* the last vertex may have been removed in the first iteration,
         so use the first element of the "dest" buffer instead */

      if (dest_length == 0)
        return 0;

      three[2] = dest[0];
      three[2].ClipEncodeY(south, north);
    }

    ClipGeoPoint insert;
    unsigned n = ClipVertex_latitude(south, north,
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
GeoClip::ClipPolygon(GeoPoint *dest,
                     const GeoPoint *src, unsigned src_length) const
{
  if (src_length < 3)
    return 0;

  GeoPoint *imported = dest + src_length * 2;
  for (unsigned i = 0; i < src_length; ++i)
    imported[i] = ImportPoint(src[i]);

  GeoPoint *first_stage = dest + src_length;
  unsigned n = ClipPolygonLongitude(Angle::Zero(), width,
                                    first_stage, imported, src_length);
  if (n < 3)
    return 0;

  n = ClipPolygonLatitude(GetSouth(), GetNorth(), dest, first_stage, n);
  if (n < 3)
    return 0;

  for (unsigned i = 0; i < n; ++i)
    dest[i] = ExportPoint(dest[i]);
  return n;
}
