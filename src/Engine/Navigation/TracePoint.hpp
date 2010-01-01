#ifndef TRACE_POINT_HPP
#define TRACE_POINT_HPP

#include "SearchPoint.hpp"
#include "Aircraft.hpp"
#include <vector>

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

  TracePoint(const AIRCRAFT_STATE &state, const TaskProjection& tp);

  unsigned time;
  unsigned rank;
  unsigned last_time;
  fixed drift_factor;

  void set_rank(const unsigned d) {
    if (d>rank) {
      rank = d;
    }
  }

  unsigned dsqr(const int d) const {
    return d*d;
  }

  unsigned approx_sq_dist(const TracePoint& tp) const {
    return dsqr(get_flatLocation().Longitude-tp.get_flatLocation().Longitude)+
      dsqr(get_flatLocation().Latitude-tp.get_flatLocation().Latitude);
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

  struct time_sort {
    bool operator()(const TracePoint& s1, const TracePoint& s2) {
      return s1.time < s2.time;
    }
  };
};

typedef std::vector<TracePoint> TracePointVector;

void reset_rank(TracePointVector& vec);

#endif

