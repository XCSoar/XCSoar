/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_BOOST_GEO_POINT_HPP
#define XCSOAR_BOOST_GEO_POINT_HPP

#include "Geo/GeoPoint.hpp"

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/core/cs.hpp>

namespace boost {
namespace geometry {
namespace traits {

/* these declarations allow using GeoPoint as a boost::geometry
   point */

BOOST_GEOMETRY_DETAIL_SPECIALIZE_POINT_TRAITS(GeoPoint, 2, double,
                                              boost::geometry::cs::geographic<boost::geometry::radian>)

template<> struct access<GeoPoint, 0> {
  static inline double get(const GeoPoint &p) {
    return p.longitude.Native();
  }

  static inline void set(GeoPoint &p, double value) {
    p.longitude = Angle::Native(value);
  }
};

template<> struct access<GeoPoint, 1> {
  static inline double get(const GeoPoint &p) {
    return p.latitude.Native();
  }

  static inline void set(GeoPoint &p, double value) {
    p.latitude = Angle::Native(value);
  }
};

}
}
}

#endif
