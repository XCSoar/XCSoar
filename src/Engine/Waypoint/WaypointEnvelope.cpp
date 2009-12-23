#include "WaypointEnvelope.hpp"
#include "Navigation/TaskProjection.hpp"

void 
WaypointEnvelope::project(const TaskProjection& task_projection)
{
  FlatLocation = task_projection.project(waypoint.Location);
}

WaypointEnvelope::WaypointEnvelope(const GEOPOINT &location,
                   const TaskProjection &task_projection)
{
  waypoint.Location = location;
  project(task_projection);
}

