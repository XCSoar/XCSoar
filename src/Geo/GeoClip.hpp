// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"

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
    :GeoBounds(other), width(GetWidth()) {}

protected:
  /**
   * Imports a longitude value.  To avoid wraparound bugs, all
   * longitudes used internally in this class are relative to the
   * "west" bound.
   */
  [[gnu::pure]]
  Angle ImportLongitude(Angle l) const {
    return (l - GetWest()).AsDelta();
  }

  [[gnu::pure]]
  GeoPoint ImportPoint(GeoPoint pt) const {
    return GeoPoint(ImportLongitude(pt.longitude), pt.latitude);
  }

  [[gnu::pure]]
  GeoPoint ExportPoint(GeoPoint pt) const {
    return GeoPoint(pt.longitude + GetWest(), pt.latitude);
  }

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
  unsigned ClipVertex(const GeoPoint &prev, GeoPoint &pt, GeoPoint *insert,
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
  bool ClipLine(GeoPoint &a, GeoPoint &b) const;

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
  unsigned ClipPolygon(GeoPoint *dest,
                       const GeoPoint *src, unsigned src_length) const;
private:
  [[gnu::pure]] unsigned ClipEncodeX(Angle x) const;
  [[gnu::pure]] unsigned ClipEncodeY(Angle y) const;
  [[gnu::pure]] unsigned ClipEncode(GeoPoint pt) const;

};
