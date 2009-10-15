#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "GeoPoint.hpp"

struct FLAT_GEOPOINT {
  int Longitude;
  int Latitude;
};

struct WAYPOINT {
  GEOPOINT Location;
  double Altitude;
};


#endif
