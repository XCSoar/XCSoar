#include "TaskClientMap.hpp"
#include "Waypoint/Waypoints.hpp"

bool
TaskClientMap::is_waypoints_empty() const
{
  ScopeLock lock(mutex);
  return m_waypoints.empty();
}

void 
TaskClientMap::waypoints_visit_within_range(const GEOPOINT& location,
                                            const fixed range,
                                            WaypointVisitor& visitor) const
{
  ScopeLock lock(mutex);
  m_waypoints.visit_within_range(location, range, visitor);
}

