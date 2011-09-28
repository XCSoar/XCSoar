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

#ifndef XCSOAR_GEO_RECT_HPP
#define XCSOAR_GEO_RECT_HPP

#include "Navigation/GeoPoint.hpp"
#include "Compiler.h"

/**
 * A rectangle on earth's surface with very simple semantics.  Similar
 * to the RECT struct, it is bounded by four orthogonal lines.  Its
 * goal is to perform fast overlap checks, e.g. to determine if an
 * object is visible on the screen.
 */
struct GeoBounds {
  Angle west, north, east, south;

  GeoBounds() {}
  GeoBounds(const GeoPoint pt)
    :west(pt.longitude), north(pt.latitude),
     east(pt.longitude), south(pt.latitude) {}
  GeoBounds(const GeoPoint _north_west, const GeoPoint _south_east)
    :west(_north_west.longitude), north(_north_west.latitude),
     east(_south_east.longitude), south(_south_east.latitude) {}

  bool empty() const {
    return west == east && north == south;
  }

  void extend(const GeoPoint pt) {
    if (pt.longitude < west)
      west = pt.longitude;
    if (pt.latitude > north)
      north = pt.latitude;
    if (pt.longitude > east)
      east = pt.longitude;
    if (pt.latitude < south)
      south = pt.latitude;
  }

  bool inside(Angle longitude, Angle latitude) const {
    return longitude.Between(west, east) && latitude.Between(south, north);
  }

  bool inside(const GeoPoint pt) const {
    return inside(pt.longitude, pt.latitude);
  }

  bool inside(const GeoBounds &interior) const {
    return inside(interior.west, interior.north) &&
      inside(interior.east, interior.south);
  }

protected:
  /**
   * Does the range a1..a2 overlap with b1..b2?
   */
  gcc_const
  static bool overlaps(Angle a1, Angle a2, Angle b1, Angle b2) {
    return a1.Between(b1, b2) || b1.Between(a1, a2);
  }

public:
  /**
   * Does this GeoBounds instance overlap with the specified one?
   */
  gcc_pure
  bool overlaps(const GeoBounds &other) const {
    return overlaps(west, east, other.west, other.east) &&
      overlaps(south, north, other.south, other.north);
  }

  GeoPoint center() const {
    return GeoPoint(west.Fraction(east, fixed_half),
                    south.Fraction(north, fixed_half));
  }

  /**
   * Returns a scaled version of the GeoBounds.
   * The bounds are scaled around the center by the given factor.
   * @param factor The scaling factor
   * @return A scaled version of the GeoBounds
   */
  GeoBounds scale(fixed factor) const {
    Angle diff_lat_half =
        (north - south).AsBearing() / fixed_two * (factor - fixed_one);
    Angle diff_lon_half =
        (east - west).AsBearing() / fixed_two * (factor - fixed_one);

    GeoBounds br = *this;
    br.east += diff_lon_half;
    br.west -= diff_lon_half;
    br.north += diff_lat_half;
    br.south -= diff_lat_half;

    return br;
  }
};

#endif
