#ifndef WAYPOINT_SORTER_HPP
#define WAYPOINT_SORTER_HPP

#include "Waypoint/Waypoints.hpp"
#include <vector>

#define NAMEFILTERLEN 10

/**
 * Structure to hold Waypoint sorting information
 */
struct WayPointSelectInfo {
  const Waypoint* way_point; /**< Pointer to actual waypoint (unprotected!) */
  fixed Distance; /**< Distance in user units from observer to waypoint */
  Angle Direction; /**< Bearing (deg true north) from observer to waypoint */
  unsigned int FourChars; /**< Fast access of first four characters of name */
};

class WaypointSelectInfoVector : public std::vector<WayPointSelectInfo> {
public:
  void push_back(const Waypoint &way_point, const GEOPOINT &Location,
                 const fixed distance_factor);
};

#endif
