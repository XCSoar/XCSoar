#include "Waypoint.hpp"
#include "Math/FastMath.h"
#include "BaseTask/TaskProjection.h"

unsigned 
FLAT_GEOPOINT::distance_to(const FLAT_GEOPOINT &sp) const
{
  const long dx = (Longitude-sp.Longitude);
  const long dy = (Latitude-sp.Latitude);
  return isqrt4(dx*dx+dy*dy);
}

void 
WAYPOINT::print(std::ostream &f, const TaskProjection &task_projection) const
{
  GEOPOINT g = task_projection.unproject(FlatLocation);
  f << g.Longitude << " " << g.Latitude << "\n";
}
