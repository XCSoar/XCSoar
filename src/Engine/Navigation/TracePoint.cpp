#include "TracePoint.hpp"
#include "TaskProjection.hpp"
#include "Aircraft.hpp"

#include <limits.h>

TracePoint::TracePoint(const AircraftState &state):
  SearchPoint(state.location),
  time((int)state.time),
  drift_factor(sigmoid(state.altitude_agl / 100) * 256),
  altitude(state.altitude),
  vario(state.netto_vario)
{
}

TaskProjection get_bounds(const TracePointVector& trace,
                          const GeoPoint &fallback_location) 
{
  TaskProjection task_projection;

  task_projection.reset(fallback_location);
  for (auto it = trace.begin(); it != trace.end(); ++it)
    task_projection.scan_location(it->get_location());

  task_projection.update_fast();
  return task_projection;
}
