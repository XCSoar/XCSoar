#ifndef FLATRAY_HPP
#define FLATRAY_HPP

#include "FlatGeoPoint.hpp"

class FlatRay {
public:
  FlatRay(const FLAT_GEOPOINT& from,
          const FLAT_GEOPOINT& to):
    point(from),vector(to-from),
    fx(vector.Longitude!=0? 1.0/vector.Longitude:0),
    fy(vector.Latitude!=0? 1.0/vector.Latitude:0) {};

  const FLAT_GEOPOINT point;
  const FLAT_GEOPOINT vector;
  const double fx; // speedups for box intersection test
  const double fy;

  double intersects(const FlatRay &oray) const;

  FLAT_GEOPOINT parametric(const double t) const;
};

#endif
