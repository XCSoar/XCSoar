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

  info.FourChars =
    ((way_point.Name.c_str()[0] & 0xff) << 24)
    +((way_point.Name.c_str()[1] & 0xff) << 16)
    +((way_point.Name.c_str()[2] & 0xff) << 8)
    +((way_point.Name.c_str()[3] & 0xff));

  std::vector<WayPointSelectInfo>::push_back(info);
}
