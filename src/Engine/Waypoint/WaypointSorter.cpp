#include "WaypointSorter.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#include <string>

void
WaypointSelectInfoVector::push_back(const Waypoint &way_point,
                                    const GEOPOINT &Location,
                                    const fixed distance_factor)
{
  WayPointSelectInfo info;

  info.way_point = &way_point;

  const GeoVector vec(Location, way_point.Location);

  info.Distance = vec.Distance*distance_factor;
  info.Direction = vec.Bearing;

  std::vector<WayPointSelectInfo>::push_back(info);
}
