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
    time(state.Time.as_int()) {};

  unsigned altitude;
  unsigned time;
};

typedef std::vector<TracePoint> TracePointVector;

#endif

