#include "WaypointSorter.hpp"
#include <algorithm>
#include "Navigation/Geometry/GeoVector.hpp"

static int SelectedWayPointFileIdx=0;

static bool WaypointNameCompare(const WayPointSelectInfo& elem1, 
                                const WayPointSelectInfo& elem2 ) 
{
  return (elem1.FourChars < elem2.FourChars);
}

static bool WaypointDistanceCompare(const WayPointSelectInfo& elem1, 
                                    const WayPointSelectInfo& elem2 ) 
{
  return (elem1.Distance < elem2.Distance);
}


static bool WaypointDirectionCompare(const WayPointSelectInfo& elem1, 
                                     const WayPointSelectInfo& elem2 ) 
{
  return (elem1.DirectionErr < elem2.DirectionErr);
}

WaypointSorter::WaypointSorter(const Waypoints &way_points,
                               const GEOPOINT &Location,
                               const fixed distance_factor)
{
  m_waypoints_all.reserve(way_points.size());

  for (Waypoints::WaypointTree::const_iterator it = way_points.begin();
       it != way_points.end(); it++) {

    WayPointSelectInfo info;
    
    const Waypoint &way_point = it->get_waypoint();

    info.way_point = &way_point;

    GeoVector vec(Location, way_point.Location);

    info.Distance = vec.Distance*distance_factor;
    info.Direction = vec.Bearing;

    info.FourChars = 
      ((way_point.Name.c_str()[0] & 0xff) << 24)
      +((way_point.Name.c_str()[1] & 0xff) << 16)
      +((way_point.Name.c_str()[2] & 0xff) << 8)
      +((way_point.Name.c_str()[3] & 0xff));

    m_waypoints_all.push_back(info);
  }

  std::sort(m_waypoints_all.begin(),
            m_waypoints_all.end(),
            WaypointNameCompare); // TODO UpLimit
}

const WaypointSelectInfoVector& 
WaypointSorter::get_list()
{
  return m_waypoints_all;
}

static bool WaypointAirportFilter(const WayPointSelectInfo& elem1) 
{
  return !elem1.way_point->Flags.Airport;
}

void
WaypointSorter::filter_airport(WaypointSelectInfoVector& inv) const
{

  inv.erase(std::remove_if(inv.begin(), inv.end(), WaypointAirportFilter), inv.end());
}

static bool WaypointLandableFilter(const WayPointSelectInfo& elem1) 
{
  return !elem1.way_point->is_landable();
}

void
WaypointSorter::filter_landable(WaypointSelectInfoVector& inv) const
{
  inv.erase(std::remove_if(inv.begin(), inv.end(), WaypointLandableFilter), inv.end());
}

static bool WaypointTurnPointFilter(const WayPointSelectInfo& elem1) 
{
  return !elem1.way_point->Flags.TurnPoint;
}

void
WaypointSorter::filter_turnpoint(WaypointSelectInfoVector& inv) const
{
  inv.erase(std::remove_if(inv.begin(), inv.end(), WaypointTurnPointFilter), inv.end());
}

static bool WaypointFileIdxFilter(const WayPointSelectInfo& elem1) 
{
  return elem1.way_point->FileNum != SelectedWayPointFileIdx;
}

void
WaypointSorter::filter_file(WaypointSelectInfoVector& inv,
                            const int file_idx) const
{
  SelectedWayPointFileIdx = file_idx;
  inv.erase(std::remove_if(inv.begin(), inv.end(), WaypointFileIdxFilter), inv.end());
}

