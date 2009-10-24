#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "GeoPoint.hpp"

struct FLAT_GEOPOINT {
  FLAT_GEOPOINT():Longitude(0),Latitude(0) {};
  FLAT_GEOPOINT(const int x,
                const int y):
    Longitude(x),Latitude(y) {};
  int Longitude;
  int Latitude;

  unsigned distance_to(const FLAT_GEOPOINT &sp) const;

  inline int operator[](unsigned const N) const 
    { 
      switch(N) {
      case 0:
        return Longitude;
      case 1:
        return Latitude;
      };
      return 0;
    }
};

struct WAYPOINT {
  GEOPOINT Location;
  FLAT_GEOPOINT FlatLocation;
  double Altitude;
};


#endif
