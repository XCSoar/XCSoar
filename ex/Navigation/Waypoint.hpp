#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include <iostream>
#include "GeoPoint.hpp"

class TaskProjection;

struct FLAT_GEOPOINT {
  FLAT_GEOPOINT():Longitude(0),Latitude(0) {};
  FLAT_GEOPOINT(const int x,
                const int y):
    Longitude(x),Latitude(y) {};
  int Longitude;
  int Latitude;

  unsigned distance_to(const FLAT_GEOPOINT &sp) const;
};

struct WAYPOINT {
  unsigned id;
  GEOPOINT Location;
  FLAT_GEOPOINT FlatLocation;
  double Altitude;

  void print(std::ostream &f, const TaskProjection &task_projection) const;

  // used by kd tree
  struct kd_get_location {
    typedef int result_type;
    int operator() ( const WAYPOINT &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.FlatLocation.Longitude;
      case 1:
        return d.FlatLocation.Latitude;
      };
      return 0; 
    };
  };
  bool operator==(const WAYPOINT&wp) const {
    return id == wp.id;
  }
};


#endif
