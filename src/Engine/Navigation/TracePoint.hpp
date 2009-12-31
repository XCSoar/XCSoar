#ifndef TRACE_POINT_HPP
#define TRACE_POINT_HPP

#include "SearchPoint.hpp"
#include "Aircraft.hpp"
#include <vector>

class TracePoint:
  public SearchPoint
{
public:
  TracePoint(const AIRCRAFT_STATE &state, const TaskProjection& tp):
    SearchPoint(state.Location, tp, true),
    altitude(state.NavAltitude.as_int()),
    time(state.Time.as_int()),
    rank(0) {};

  unsigned altitude;
  unsigned time;
  unsigned rank;

  void set_rank(const unsigned d) {
    if (d>rank) {
      rank = d;
    }
  }
};

typedef std::vector<TracePoint> TracePointVector;

void reset_rank(TracePointVector& vec);

#endif

