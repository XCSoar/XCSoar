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

#ifndef XCSOAR_GEO_RECT_HPP
#define XCSOAR_GEO_RECT_HPP

#include "Math/ARange.hpp"
#include "GeoPoint.hpp"
#include "Compiler.h"

/**
 * A rectangle on earth's surface with very simple semantics.  Similar
 * to the RECT struct, it is bounded by four orthogonal lines.  Its
 * goal is to perform fast overlap checks, e.g. to determine if an
 * object is visible on the screen.
 */
class GeoBounds {
  /**
   * The range from west to east.
   */
  AngleRange longitude;

  /**
   * The range from south to north.
   */
  AngleRange latitude;

public:
  GeoBounds() = default;

  constexpr
  GeoBounds(const GeoPoint pt)
    :longitude(pt.longitude, pt.longitude),
     latitude(pt.latitude, pt.latitude) {}

  constexpr
  GeoBounds(const GeoPoint north_west, const GeoPoint south_east)
    :longitude(north_west.longitude, south_east.longitude),
     latitude(south_east.latitude, north_west.latitude) {}

  /**
   * Construct an instance that is "invalid", i.e. IsValid() will
   * return false.  The return value must not be used in any
   * calculation.
   */
  constexpr
  static GeoBounds Invalid() {
    return GeoBounds(GeoPoint::Invalid());
  }

  /**
   * Set this instance to "invalid", i.e. IsValid() will return false.
   * The return value must not be used in any calculation.
   */
  void SetInvalid() {
    latitude.end = Angle::FullCircle();
  }

  constexpr Angle GetWest() const {
    return longitude.start;
  }

  constexpr Angle GetEast() const {
    return longitude.end;
  }

  constexpr Angle GetSouth() const {
    return latitude.start;
  }

  constexpr Angle GetNorth() const {
    return latitude.end;
  }

  constexpr GeoPoint GetNorthWest() const {
    return GeoPoint(GetWest(), GetNorth());
  }

  constexpr GeoPoint GetNorthEast() const {
    return GeoPoint(GetEast(), GetNorth());
  }

  constexpr GeoPoint GetSouthWest() const {
    return GeoPoint(GetWest(), GetSouth());
  }

  constexpr GeoPoint GetSouthEast() const {
    return GeoPoint(GetEast(), GetSouth());
  }

  /**
   * Check if this object is "valid".  Returns false when it was
   * constructed by Invalid().  This is not an extensive plausibility
   * check; it is only designed to catch instances created by
   * Invalid().  If you want a real check, call Check().
   */
  constexpr
  bool IsValid() const {
    return latitude.end <= Angle::HalfCircle();
  }

  /**
   * Check if this object is "valid".  This is an extensive test; if
   * all you want is check if this object was constructed by
   * Invalid(), then call the cheaper method IsValid().
   */
  constexpr bool Check() const {
    return GetSouthWest().Check() && GetNorthEast().Check() &&
      GetNorth() >= GetSouth();
  }

  constexpr bool IsEmpty() const {
    return longitude.IsEmpty() || latitude.IsEmpty();
  }

  Angle GetWidth() const {
    return longitude.GetLength();
  }

  Angle GetHeight() const {
    return latitude.GetLength();
  }

  /**
   * Returns the geographic width of the object (west to east) at its
   * center in metres.
   */
  gcc_pure
  double GetGeoWidth() const {
    const Angle middle_latitude = latitude.GetMiddle();
    return GeoPoint(GetWest(), middle_latitude)
      .Distance(GeoPoint(GetEast(), middle_latitude));
  }

  /**
   * Returns the geographic height of the object (south to north) in
   * metres.
   */
  gcc_pure
  double GetGeoHeight() const {
    return GetNorthWest().Distance(GetSouthWest());
  }

  /**
   * Extend the bounds so the given point is inside.
   *
   * @return true if the bounds have been modified
   */
  bool Extend(const GeoPoint pt);

  bool IsInside(Angle _longitude, Angle _latitude) const {
    return longitude.IsInside(_longitude) && latitude.IsInside(_latitude);
  }

  bool IsInside(const GeoPoint pt) const {
    return IsInside(pt.longitude, pt.latitude);
  }

  bool IsInside(const GeoBounds &interior) const {
    return longitude.IsInside(interior.longitude) &&
      latitude.IsInside(interior.latitude);
  }

  /**
   * Does this GeoBounds instance overlap with the specified one?
   */
  gcc_pure
  bool Overlaps(const GeoBounds &other) const {
    return longitude.Overlaps(other.longitude) &&
      latitude.Overlaps(other.latitude);
  }

  /**
   * Set this object to the intersection of this and the other object.
   *
   * @return false if the two objects do not overlap; in this case,
   * the object is left in an undefined state
   */
  bool IntersectWith(const GeoBounds &other);

  gcc_pure
  GeoPoint GetCenter() const;

  /**
   * Returns a scaled version of the GeoBounds.
   * The bounds are scaled around the center by the given factor.
   * @param factor The scaling factor
   * @return A scaled version of the GeoBounds
   */
  gcc_pure
  GeoBounds Scale(double factor) const;
};

#endif
