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

#ifndef XCSOAR_GEO_CLIP_HPP
#define XCSOAR_GEO_CLIP_HPP

#include "Geo/GeoBounds.hpp"
#include "Compiler.h"

/**
 * A rectangle on earth's surface with very simple semantics.  Similar
 * to the RECT struct, it is bounded by four orthogonal lines.  Its
 * goal is to perform fast overlap checks, e.g. to determine if an
 * object is visible on the screen.
 */
class GeoClip : protected GeoBounds {
  Angle width;

public:
  GeoClip() = default;
  GeoClip(const GeoBounds &other)
    :GeoBounds(other), width((east - west).AsBearing()) {}

protected:
  /**
   * Imports a longitude value.  To avoid wraparound bugs, all
   * longitudes used internally in this class are relative to the
   * "west" bound.
   */
  gcc_pure
  Angle import_longitude(Angle l) const {
    return (l - west).AsDelta();
  }

  gcc_pure
  GeoPoint import_point(GeoPoint pt) const {
    return GeoPoint(import_longitude(pt.longitude), pt.latitude);
  }

  gcc_pure
  GeoPoint export_point(GeoPoint pt) const {
    return GeoPoint(pt.longitude + west, pt.latitude);
  }

  /**
   * Clips a point to be bounds.
   *
   * @param pt the point to be clipped (in and out)
   * @return false if neither origin nor pt are visible
   */
  bool clip_point(const GeoPoint &origin, GeoPoint &pt) const;

  /**
   * Clips a vertex.
   *
   * @param prev the previous vertex
   * @param pt the vertex to be clipped (in and out)
   * @param insert an array of up to two vertices to be inserted after pt
   * @param next the next vertex
   * @return the new number of vertices between prev and next (e.g. 0
   * deletes, 1 preserves, 2 or 3 insert new ones)
   */
  unsigned clip_vertex(const GeoPoint &prev, GeoPoint &pt, GeoPoint *insert,
                       const GeoPoint &next) const;

public:
  /**
   * Makes sure that the line does not exceed the bounds.  This method
   * is not designed to perform strict clipping, it is just here to
   * avoid integer overflows in the graphics drivers.
   *
   * @return false if the line is definitely outside the bounds
   * rectangle
   */
  bool clip_line(GeoPoint &a, GeoPoint &b) const;

  /**
   * Makes sure that the polygon does not exceed the bounds.  This
   * method is not designed to perform strict clipping, it is just
   * here to avoid integer overflows in the graphics drivers.
   *
   * The implementation is a specialization of the Sutherland-Hodgman
   * algorithm, with only horizontal and vertical bound lines.
   *
   * @param dest a GeoPoint array with enough space for three times
   * src_length
   * @return the number of vertices written to dest; if less than 3,
   * then the polygon can not be drawn
   */
  unsigned clip_polygon(GeoPoint *dest,
                        const GeoPoint *src, unsigned src_length) const;
};

#endif
