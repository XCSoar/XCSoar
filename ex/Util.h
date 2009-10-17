#ifndef UTIL_H
#define UTIL_H

#include "GeoPoint.hpp"

GEOPOINT FindLocation(const GEOPOINT&, double, double);

GEOPOINT InterpolateLocation(const GEOPOINT& p1,
                             const GEOPOINT& p2, const double t);

double AngleToGradient(const double d);

#endif
