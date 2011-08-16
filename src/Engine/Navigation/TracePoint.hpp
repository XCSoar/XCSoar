#ifndef TRACE_POINT_HPP
#define TRACE_POINT_HPP

#include "SearchPoint.hpp"
#include "Aircraft.hpp"
#include "Compiler.h"
#include "FastMath.h"

#include <vector>

/**
 * Class for points used in traces (snail trail, OLC scans)
 * Internally, keeps track of predecessors as a kind of a linked-list
 */
class TracePoint : public SearchPoint
{
public:
  /** Time of sample */
  unsigned time;
  /**
   * Thermal drift factor:
   * 256 indicates drift rate equal to wind speed
   * 0 indicates no drift.
   */
  unsigned short drift_factor;

  /**
   * The NavAltitude [m].  This is a "short" integer to save memory.
   */
  signed short altitude;

  /**
   * The NettoVario value [m/256s].  This is a "short" fixed-point
   * integer to save memory.
   */
  signed short vario;

public:
  /**
   * Dummy constructor for null object
   *
   * @return Null object
   */
  TracePoint():
    time(0 - 1) {};

  /**
   * Constructor for a TracePoint which is only used as parameter to
   * TraceTree::find_within_range().  It initializes only the
   * SearchPoint base class.
   */
  TracePoint(const GeoPoint &location):
    SearchPoint(location) {}

  /**
   * Constructor for actual trace points
   *
   * @param state State of aircraft
   * @param tp Projection used internally
   *
   * @return Initialised object
   */
  TracePoint(const AircraftState &state);

  void Clear() {
    time = (unsigned)(0 - 1);
  }

  bool IsDefined() const {
    return time != (unsigned)(0 - 1);
  }

  fixed GetAltitude() const {
    return fixed(altitude);
  }

  /**
   * Returns the altitude as an integer.  Some calculations may not
   * need the fractional part.
   */
  int GetIntegerAltitude() const {
    return altitude;
  }

  fixed GetVario() const {
    return fixed(vario) / 256;
  }

  /** 
   * Calculate approximate squared (flat projected) distance between this point
   * and another
   * 
   * @param tp Point to calculate distance to
   * 
   * @return Approximate squared distance
   */
  gcc_pure
  unsigned approx_sq_dist(const TracePoint& tp) const {
    return dsqr(get_flatLocation().Longitude - tp.get_flatLocation().Longitude) +
      dsqr(get_flatLocation().Latitude - tp.get_flatLocation().Latitude);
  }

  /** 
   * Calculate approximate (flat projected) distance between this point
   * and another
   * 
   * @param tp Point to calculate distance to
   * 
   * @return Approximate distance
   */
  gcc_pure
  unsigned approx_dist(const TracePoint& tp) const {
    return (unsigned)lhypot(get_flatLocation().Longitude - tp.get_flatLocation().Longitude,
                            get_flatLocation().Latitude - tp.get_flatLocation().Latitude);
  }

  /**
   * Function object used to provide access to coordinate values by kd-tree
   */
  struct kd_get_location {    
    typedef int result_type; /**< type of returned value */
    /**
     * Retrieve coordinate value from object given coordinate index
     * @param d WaypointEnvelope object
     * @param k index of coordinate
     *
     * @return Coordinate value
     */
    int operator() ( const TracePoint &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.get_flatLocation().Longitude;
      case 1:
        return d.get_flatLocation().Latitude;
      };
      return 0; 
    }
  };

  /**
   * Structure for STL sorting by time
   */
  struct time_sort {
    /** 
     * Comparison operator
     * 
     * @return True if s1 is earlier than s2
     */
    bool operator()(const TracePoint& s1, const TracePoint& s2) {
      return s1.time < s2.time;
    }
  };

  /** 
   * Test match based on time (since time of a sample must be unique)
   * 
   * @param a Point to compare to
   * 
   * @return True if time matches
   */
  bool operator==(TracePoint const &a) const {
    return time == a.time; 
  }

private:
  gcc_const
  static inline unsigned dsqr(const int d) {
    return d * d;
  }
};

class TracePointVector : public std::vector<TracePoint> {
};

class TaskProjection;

gcc_pure
TaskProjection get_bounds(const TracePointVector& vec,
                          const GeoPoint &fallback_location);

#endif

