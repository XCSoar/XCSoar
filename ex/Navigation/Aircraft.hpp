#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP

#include "Navigation/Waypoint.hpp"

struct GLIDE_RESULT;

struct AIRCRAFT_STATE {
  GEOPOINT Location;
  double Time;
  double Speed;
  double Altitude;
  double WindSpeed;
  double WindDirection;
  void back_predict(const GLIDE_RESULT &res);
  void forward_predict(const GLIDE_RESULT &res);
};

#endif
