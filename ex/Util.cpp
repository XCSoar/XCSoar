#include <math.h>
#include "Util.h"

double Bearing(const GEOPOINT& p1, const GEOPOINT& p2) {
  return atan2(p1.Latitude-p2.Latitude,p1.Longitude-p2.Longitude);
}

#define sqr(x) ((x)*(x))

double Distance(const GEOPOINT& p1, const GEOPOINT& p2) {
  return sqrt(sqr(p1.Latitude-p2.Latitude)+sqr(p1.Longitude-p2.Longitude));
}

double ProjectedDistance(const GEOPOINT& p1, const GEOPOINT& p2,
  const GEOPOINT& p3) {
  Distance(p1,p3);
};

double AngleLimit360(double x) {
  return x;
}

GEOPOINT FindLocation(const GEOPOINT& p1, double a, double b) {
  return p1;
}

double HalfAngle(double a, double b) {
  return (a+b)/2;
}
