#ifndef FLAT_GEOPOINT_HPP
#define FLAT_GEOPOINT_HPP

#include "Math/FastMath.h"

/**
 * Integer projected (flat-earth) version of Geodetic coordinates
 */
struct FLAT_GEOPOINT {
  FLAT_GEOPOINT():Longitude(0),Latitude(0) {};
  FLAT_GEOPOINT(const int x,
                const int y):
    Longitude(x),Latitude(y) {};

  int Longitude;
  int Latitude;

  unsigned distance_to(const FLAT_GEOPOINT &sp) const;
  FLAT_GEOPOINT operator- (const FLAT_GEOPOINT &p2) const {
    FLAT_GEOPOINT res= *this;
    res.Longitude -= p2.Longitude;
    res.Latitude -= p2.Latitude;
    return res;
  };

  int cross(const FLAT_GEOPOINT &other) const {
    return Longitude*other.Latitude-Latitude*other.Longitude;
  }
};

#endif
