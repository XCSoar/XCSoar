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

