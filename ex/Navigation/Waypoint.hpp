#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "GeoPoint.hpp"
#include "FlatGeoPoint.hpp"
#include <iostream>

class TaskProjection;

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
