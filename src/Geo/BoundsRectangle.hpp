/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_GEO_BOUNDS_RECTANGLE_HPP
#define XCSOAR_GEO_BOUNDS_RECTANGLE_HPP

#include "Navigation/GeoPoint.hpp"

/**
 * A rectangle on earth's surface with very simple semantics.  Similar
 * to the RECT struct, it is bounded by four orthogonal lines.  Its
 * goal is to perform fast overlap checks, e.g. to determine if an
 * object is visible on the screen.
 */
struct BoundsRectangle {
  Angle west, north, east, south;

  BoundsRectangle() {}
  BoundsRectangle(const GeoPoint pt)
    :west(pt.Longitude), north(pt.Latitude),
     east(pt.Longitude), south(pt.Latitude) {}
  BoundsRectangle(const GeoPoint _north_west, const GeoPoint _south_east)
    :west(_north_west.Longitude), north(_north_west.Latitude),
     east(_south_east.Longitude), south(_south_east.Latitude) {}

  bool empty() const {
    return west == east && north == south;
  }

  void merge(const GeoPoint pt) {
    if (pt.Longitude < west)
      west = pt.Longitude;
    if (pt.Latitude > north)
      north = pt.Latitude;
    if (pt.Longitude > east)
      east = pt.Longitude;
    if (pt.Latitude < south)
      south = pt.Latitude;
  }

  bool inside(Angle longitude, Angle latitude) const {
    return longitude.between(west, east) && latitude.between(south, north);
  }

  bool inside(const GeoPoint pt) const {
    return inside(pt.Longitude, pt.Latitude);
  }

  bool inside(const BoundsRectangle &interior) const {
    return inside(interior.west, interior.north) &&
      inside(interior.east, interior.south);
  }

  GeoPoint center() const {
    return GeoPoint(west.Fraction(east, fixed_half),
                    south.Fraction(north, fixed_half));
  }
};

#endif
