#ifndef FLATRAY_HPP
#define FLATRAY_HPP

#include "FlatGeoPoint.hpp"

/**
 * Projected ray (a point and vector) in 2-d cartesian integer coordinates
 */
class FlatRay {
public:
/** 
 * Constructor given start/end locations
 * 
 * @param from Origin of ray
 * @param to End point of ray
 */
  FlatRay(const FLAT_GEOPOINT& from,
          const FLAT_GEOPOINT& to):
    point(from),vector(to-from),
    fx(vector.Longitude!=0? 1.0/vector.Longitude:0),
    fy(vector.Latitude!=0? 1.0/vector.Latitude:0) {};

  const FLAT_GEOPOINT point; /**< Origin of ray */
  const FLAT_GEOPOINT vector; /**< Vector representing ray direction and length */
  const double fx; /**< speedups for box intersection test */
  const double fy; /**< speedups for box intersection test */

/** 
 * Test whether two rays intersect
 * 
 * @param oray Other ray to test intersection with
 * 
 * @return Parameter [0,1] of vector on this ray that intersection occurs (or -1 if fail)
 */
  double intersects(const FlatRay &oray) const;

/** 
 * Parametric form of ray
 * 
 * @param t Parameter [0,1] of ray
 * 
 * @return Location of end point
 */
  FLAT_GEOPOINT parametric(const double t) const;
};

#endif
