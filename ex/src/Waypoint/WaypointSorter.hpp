#ifndef WAYPOINT_SORTER_HPP
#define WAYPOINT_SORTER_HPP

#include "Waypoint/Waypoints.hpp"
#include <vector>

/**
 * Structure to hold Waypoint sorting information
 */
struct WayPointSelectInfo {
  const Waypoint* way_point; /**< Pointer to actual waypoint (unprotected!) */
  fixed Distance; /**< Distance in user units from observer to waypoint */
  fixed Direction; /**< Bearing (deg true north) from observer to waypoint */
  unsigned int FourChars; /**< Fast access of first four characters of name */
};

typedef std::vector<WayPointSelectInfo> WaypointSelectInfoVector;

/**
 * Utility class to manage sorting of waypoints (e.g. for dlgWayPointSelect)
 * Note that distance queries do not yet make use of the kdtree, but this system
 * keeps a local master list so won't need to lock the waypoints for long.
 */

class WaypointSorter
{
public:
/** 
 * Constructor.  Sorts master list of waypoints by name 
 * 
 * @param _waypoints Waypoints store
 * @param Location Location of aircraft at time of query
 * @param distance_factor Units factor to apply to distance calculations
 */
  WaypointSorter(const Waypoints &_waypoints, 
                 const GEOPOINT &Location,
                 const fixed distance_factor);

/** 
 * Return master list
 * 
 * @return Master list
 */
  const WaypointSelectInfoVector& get_list();

/** 
 * Remove non-airport waypoints
 * 
 * @param vec List of waypoints to filter (read-write)
 */
  void filter_airport(WaypointSelectInfoVector& vec) const;

/** 
 * Remove non-landable waypoints
 * 
 * @param vec List of waypoints to filter (read-write)
 */
  void filter_landable(WaypointSelectInfoVector& vec) const;

/** 
 * Remove non-turnpoint waypoints
 * 
 * @param vec List of waypoints to filter (read-write)
 */
  void filter_turnpoint(WaypointSelectInfoVector& vec) const;

/** 
 * Remove waypoints not from file index 
 * 
 * @param vec List of waypoints to filter (read-write)
 * @param idx File index (0 or 1) to search for
 */
  void filter_file(WaypointSelectInfoVector& vec, const int idx) const;

/** 
 * Remove waypoints not matching supplied initial character 
 * 
 * @param vec List of waypoints to filter (read-write)
 * @param c Character to match
 */
  void filter_name(WaypointSelectInfoVector& vec, const unsigned char c) const;

/** 
 * Remove waypoints bearing greater than 18 degrees from supplied direction 
 * 
 * @param vec List of waypoints to filter (read-write)
 * @param direction Bearing (degrees) of desired direction
 */
  void filter_direction(WaypointSelectInfoVector& vec, const fixed direction) const;

/** 
 * Remove waypoints further than desired direction
 * 
 * @param vec List of waypoints to filter (read-write)
 * @param distance Distance (user units) of limit
 */
  void filter_distance(WaypointSelectInfoVector& vec, const fixed distance) const;

/** 
 * Sort waypoints by distance
 * 
 * @param vec List of waypoints to sort (read-write)
 */
  void sort_distance(WaypointSelectInfoVector& vec) const;

/** 
 * Sort waypoints alphabetically
 * 
 * @param vec List of waypoints to sort (read-write)
 */
  void sort_name(WaypointSelectInfoVector& vec) const;

private:
  WaypointSelectInfoVector m_waypoints_all;
};

#endif
