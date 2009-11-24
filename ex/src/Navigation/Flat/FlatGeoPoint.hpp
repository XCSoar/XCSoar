#ifndef FLAT_GEOPOINT_HPP
#define FLAT_GEOPOINT_HPP

#include "Math/FastMath.h"

/**
 * Integer projected (flat-earth) version of Geodetic coordinates
 */
struct FLAT_GEOPOINT {
/** 
 * Constructor (default) at origin
 * 
 * @return Initialised object at origin
 */
  FLAT_GEOPOINT():Longitude(0),Latitude(0) {};

/** 
 * Constructor at specified location (x,y)
 * 
 * @param x x location
 * @param y y location
 *
 * @return Initialised object at origin
 */
  FLAT_GEOPOINT(const int x,
                const int y):
    Longitude(x),Latitude(y) {};

  int Longitude; /**< Projected x (Longitude) value (undefined units) */
  int Latitude; /**< Projected y (Latitude) value (undefined units) */

/** 
 * Find distance from one point to another
 * 
 * @param sp That point
 * 
 * @return Distance in projected units
 */
  unsigned distance_to(const FLAT_GEOPOINT &sp) const;

/** 
 * Subtract one point from another
 * 
 * @param p2 Point to subtract
 * 
 * @return Subtracted value
 */
  FLAT_GEOPOINT operator- (const FLAT_GEOPOINT &p2) const {
    FLAT_GEOPOINT res= *this;
    res.Longitude -= p2.Longitude;
    res.Latitude -= p2.Latitude;
    return res;
  };

/** 
 * Calculate cross product of one point with another
 * 
 * @param other That point
 * 
 * @return Cross product
 */
  int cross(const FLAT_GEOPOINT &other) const {
    return Longitude*other.Latitude-Latitude*other.Longitude;
  }
};

#endif
