#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP

#include "GeoPoint.hpp"

struct AIRCRAFT_STATE {
  GEOPOINT Location;
  double Time;
  double Speed;
  double Altitude;
  double WindSpeed;
  double WindDirection;
};

#endif
