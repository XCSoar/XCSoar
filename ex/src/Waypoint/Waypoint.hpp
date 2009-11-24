#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "Navigation/GeoPoint.hpp"
#include "Navigation/Flat/FlatGeoPoint.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif

#include "Util/GenericVisitor.hpp"

class TaskProjection;

/**
 * Class for waypoints.  
 * This is small enough currently to be used with local copies (e.g. in a TaskPoint),
 * but this may change if we include airfield details inside.
 */
class Waypoint:
  public BaseVisitable<>
{
 public:

  unsigned id;
  GEOPOINT Location;
  FLAT_GEOPOINT FlatLocation;
  double Altitude;

  /**
   * Function object used to provide access to coordinate values by kd-tree
   */
  struct kd_get_location {
    typedef int result_type;
    /**
     * Retrieve coordinate value from object given coordinate index
     * @param d Waypoint object
     * @param k index of coordinate
     *
     * @return Coordinate value
     */
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

  /**
   * Equality operator (by id)
   * 
   * @param d Waypoint object to match against
   *
   * @return true if ids match
   */
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
