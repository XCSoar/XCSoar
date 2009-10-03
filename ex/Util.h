#ifndef UTIL_H
#define UTIL_H

struct GEOPOINT {
  double Longitude;
  double Latitude;
};

struct FLAT_GEOPOINT {
  int Longitude;
  int Latitude;
};


struct WAYPOINT {
  GEOPOINT Location;
  double Altitude;
};

struct AIRCRAFT_STATE {
  GEOPOINT Location;
  double Time;
  double Speed;
  double Altitude;
};

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
