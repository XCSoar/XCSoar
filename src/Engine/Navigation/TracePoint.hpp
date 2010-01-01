#ifndef TRACE_POINT_HPP
#define TRACE_POINT_HPP

#include "SearchPoint.hpp"
#include "Aircraft.hpp"
#include <vector>

class TracePoint:
  public SearchPoint
{
public:
/** 
 * Dummy constructor for null object
 * 
 * @return Null object
 */
  TracePoint():time(0-1),last_time(0-1) {};

  TracePoint(const AIRCRAFT_STATE &state, const TaskProjection& tp):
    SearchPoint(state.Location, tp, true),
    altitude(state.NavAltitude.as_int()),
    time(state.Time.as_int()),
    Vario(state.Vario),
    rank(0) {};

  unsigned altitude;
  unsigned time;
  fixed Vario;
  unsigned rank;
  unsigned last_time;

  void set_rank(const unsigned d) {
    if (d>rank) {
      rank = d;
    }
  }

  unsigned approx_dist(const TracePoint& tp) const {
    return std::max(abs(get_flatLocation().Longitude-tp.get_flatLocation().Longitude),
                    abs(get_flatLocation().Latitude-tp.get_flatLocation().Latitude));
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
};

typedef std::vector<TracePoint> TracePointVector;

void reset_rank(TracePointVector& vec);

#endif

