#include "WaypointEnvelope.hpp"
#include "Navigation/TaskProjection.hpp"

void 
WaypointEnvelope::project(const TaskProjection& task_projection)
{
  FlatLocation = task_projection.project(waypoint.Location);
}

WaypointEnvelope::WaypointEnvelope(const GeoPoint &location,
                   const TaskProjection &task_projection)
  :waypoint(location)
{
  project(task_projection);
}

