#ifndef UTIL_H
#define UTIL_H

#include "Navigation/Waypoint.hpp"


double Bearing(const GEOPOINT& p1, const GEOPOINT& p2);
double Distance(const GEOPOINT& p1, const GEOPOINT& p2);
double ProjectedDistance(const GEOPOINT& p1, const GEOPOINT& p2,
  const GEOPOINT& p3);
double AngleLimit360(double x);
GEOPOINT FindLocation(const GEOPOINT&, double, double);
double HalfAngle(double, double);
double BiSector(double InBound, double OutBound);
double Reciprocal(double InBound);

unsigned int isqrt4(unsigned long val);

#endif
