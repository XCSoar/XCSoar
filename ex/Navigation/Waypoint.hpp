#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "GeoPoint.hpp"
#include "FlatGeoPoint.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif

#include "Util/GenericVisitor.hpp"

class TaskProjection;

class Waypoint:
  public BaseVisitable<>
{
 public:
  unsigned id;
  GEOPOINT Location;
  FLAT_GEOPOINT FlatLocation;
  double Altitude;

  // used by kd tree
  struct kd_get_location {
    typedef int result_type;
    int operator() ( const Waypoint &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.FlatLocation.Longitude;
      case 1:
        return d.FlatLocation.Latitude;
      };
      return 0; 
    };
  };
  bool operator==(const Waypoint&wp) const {
    return id == wp.id;
  }

public:
  DEFINE_VISITABLE()

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const Waypoint& wp);
#endif
};


#endif
