#ifndef WAYPOINT_SORTER_HPP
#define WAYPOINT_SORTER_HPP

#include "Waypoint/Waypoints.hpp"
#include <vector>

struct WayPointSelectInfo {
  const Waypoint* way_point;
  fixed Distance;
  fixed Direction;
  int    DirectionErr;
  unsigned int FourChars;
};

typedef std::vector<WayPointSelectInfo> WaypointSelectInfoVector;

class WaypointSorter
{
public:
  WaypointSorter(const Waypoints &_waypoints, 
                 const GEOPOINT &Location,
                 const fixed distance_factor);

  const WaypointSelectInfoVector& get_list();

  void filter_airport(WaypointSelectInfoVector&) const;
  void filter_landable(WaypointSelectInfoVector&) const;
  void filter_turnpoint(WaypointSelectInfoVector&) const;
  void filter_file(WaypointSelectInfoVector&, const int idx) const;

private:
  WaypointSelectInfoVector m_waypoints_all;
};

#endif
