#ifndef TRACE_POINT_HPP
#define TRACE_POINT_HPP

#include "SearchPoint.hpp"
#include "Aircraft.hpp"
#include <vector>

/**
 * Class for points used in traces (snail trail, OLC scans)
 * Internally, keeps track of predecessors as a kind of a linked-list
 */
class TracePoint:
  public SearchPoint,
  public ALTITUDE_STATE,
  public VARIO_STATE
{
public:
/** 
 * Dummy constructor for null object
 * 
 * @return Null object
 */
  TracePoint():time(0-1),last_time(0-1) {};

/** 
 * Constructor for actual trace points
 * 
 * @param state State of aircraft
 * @param tp Projection used internally
 * 
 * @return Initialised object
 */  
  TracePoint(const AIRCRAFT_STATE &state, const TaskProjection& tp);

  unsigned time; /**< Time of sample */
  unsigned last_time; /**< Time of sample prior to this */
  fixed drift_factor; /**< Thermal drift factor */

  unsigned dsqr(const int d) const {
    return d*d;
  }

  unsigned approx_sq_dist(const TracePoint& tp) const {
    return dsqr(get_flatLocation().Longitude-tp.get_flatLocation().Longitude)+
      dsqr(get_flatLocation().Latitude-tp.get_flatLocation().Latitude);
  }
  unsigned approx_dist(const TracePoint& tp) const {
    return (unsigned)sqrt(approx_sq_dist(tp));
  }

  unsigned dt() const {
    return time-last_time;
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
    };
  };

  /**
   * Structure for STL sorting by time
   */
  struct time_sort {
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
  bool operator==(TracePoint const& a) { 
    return time == a.time; 
  }

};

typedef std::vector<TracePoint> TracePointVector;

void reset_rank(TracePointVector& vec);

#endif

