/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "GeoPoint.hpp"
#include "Compiler.h"

/**
 * A rectangle on earth's surface with very simple semantics.  Similar
 * to the RECT struct, it is bounded by four orthogonal lines.  Its
 * goal is to perform fast overlap checks, e.g. to determine if an
 * object is visible on the screen.
 */
struct GeoBounds {
  Angle west, north, east, south;

  GeoBounds() = default;

  constexpr
  GeoBounds(const GeoPoint pt)
    :west(pt.longitude), north(pt.latitude),
     east(pt.longitude), south(pt.latitude) {}

  constexpr
  GeoBounds(const GeoPoint _north_west, const GeoPoint _south_east)
    :west(_north_west.longitude), north(_north_west.latitude),
     east(_south_east.longitude), south(_south_east.latitude) {}

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
    north = Angle::FullCircle();
  }

  /**
   * Check if this object is "valid".  Returns false when it was
   * constructed by Invalid().  This is not an extensive plausibility
   * check; it is only designed to catch instances created by
   * Invalid().
   */
  constexpr
  bool IsValid() const {
    return north <= Angle::HalfCircle();
  }

  bool IsEmpty() const {
    return west == east && north == south;
  }

  void Extend(const GeoPoint pt);

  bool IsInside(Angle longitude, Angle latitude) const {
    return longitude.Between(west, east) && latitude.Between(south, north);
  }

  bool IsInside(const GeoPoint pt) const {
    return IsInside(pt.longitude, pt.latitude);
  }

  bool IsInside(const GeoBounds &interior) const {
    return IsInside(interior.west, interior.north) &&
      IsInside(interior.east, interior.south);
  }

protected:
  /**
   * Does the range a1..a2 overlap with b1..b2?
   */
  gcc_const
  static bool Overlaps(Angle a1, Angle a2, Angle b1, Angle b2) {
    return a1.Between(b1, b2) || b1.Between(a1, a2);
  }

public:
  /**
   * Does this GeoBounds instance overlap with the specified one?
   */
  gcc_pure
  bool Overlaps(const GeoBounds &other) const {
    return Overlaps(west, east, other.west, other.east) &&
      Overlaps(south, north, other.south, other.north);
  }

  gcc_pure
  GeoPoint GetCenter() const;

  /**
   * Returns a scaled version of the GeoBounds.
   * The bounds are scaled around the center by the given factor.
   * @param factor The scaling factor
   * @return A scaled version of the GeoBounds
   */
  gcc_pure
  GeoBounds Scale(fixed factor) const;
};

#endif
