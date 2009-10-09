#ifndef UTIL_H
#define UTIL_H

#include "Navigation/Waypoint.hpp"

#define DEG2RAD(x) (x*3.1415926/180.0)
#define RAD2DEG(x) (x/3.1415926*180.0)

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
