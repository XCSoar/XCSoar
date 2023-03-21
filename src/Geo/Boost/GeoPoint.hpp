// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

}  // namespace traits
}  // namespace geometry
}  // namespace boost
