#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP

#include "GeoPoint.hpp"

/**
 * Structure containing basic aircraft data used for navigation.
 * Typically this will be updated only by the GPS devices or by
 * a simulator.
 *
 * Copies of the state are made in some TaskPoint types so this should be
 * kept fairly small.  If extra data is required to be saved by the GPS
 * device, then this structure itself should be contained within a larger one.
 */
struct AIRCRAFT_STATE {
  GEOPOINT Location;
  double Time;
  double Speed;
  double Altitude;
  double WindSpeed;
  double WindDirection;
};

#endif
