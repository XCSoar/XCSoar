#include "WaypointSorter.hpp"
#include <algorithm>
#include "Navigation/Geometry/GeoVector.hpp"
#include "UtilsText.hpp"
#include "Compatibility/string.h" /* _tcsnicmp -> _strnicmp -> strncasecmp */

#include <string>

static int SelectedWayPointFileIdx=0;


WaypointSorter::WaypointSorter(const Waypoints &way_points,
                               const GEOPOINT &Location,
                               const fixed distance_factor)
{
  m_waypoints_all.reserve(way_points.size());

  for (Waypoints::WaypointTree::const_iterator it = way_points.begin();
       it != way_points.end(); ++it) {

    WayPointSelectInfo info;
    
    const Waypoint &way_point = it->get_waypoint();

    info.way_point = &way_point;

    const GeoVector vec(Location, way_point.Location);

    info.Distance = vec.Distance*distance_factor;
    info.Direction = vec.Bearing;

    info.FourChars = 
      ((way_point.Name.c_str()[0] & 0xff) << 24)
      +((way_point.Name.c_str()[1] & 0xff) << 16)
      +((way_point.Name.c_str()[2] & 0xff) << 8)
      +((way_point.Name.c_str()[3] & 0xff));

    m_waypoints_all.push_back(info);
  }
  sort_name(m_waypoints_all);
}

const WaypointSelectInfoVector& 
WaypointSorter::get_list()
{
  return m_waypoints_all;
}

static bool 
WaypointAirportFilter(const WayPointSelectInfo& elem1) 
{
  return !elem1.way_point->Flags.Airport;
}

void
WaypointSorter::filter_airport(WaypointSelectInfoVector& vec) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointAirportFilter), vec.end());
}

static bool 
WaypointLandableFilter(const WayPointSelectInfo& elem1) 
{
  return !elem1.way_point->is_landable();
}

void
WaypointSorter::filter_landable(WaypointSelectInfoVector& vec) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointLandableFilter), vec.end());
}

static bool 
WaypointTurnPointFilter(const WayPointSelectInfo& elem1) 
{
  return !elem1.way_point->Flags.TurnPoint;
}

void
WaypointSorter::filter_turnpoint(WaypointSelectInfoVector& vec) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointTurnPointFilter), vec.end());
}

static bool 
WaypointFileIdxFilter(const WayPointSelectInfo& elem1) 
{
  return elem1.way_point->FileNum != SelectedWayPointFileIdx;
}

void
WaypointSorter::filter_file(WaypointSelectInfoVector& vec,
                            const int file_idx) const
{
  SelectedWayPointFileIdx = file_idx;
  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointFileIdxFilter), vec.end());
}

static unsigned char MatchChar = 0;
static TCHAR MatchString[NAMEFILTERLEN+1];

static bool
WaypointNameFilter(const WayPointSelectInfo& elem1)
{
  if (((elem1.FourChars & 0xff000000) >> 24) != MatchChar) {
    return true;
  }
  else if (_tcsnicmp( elem1.way_point->Name.c_str(),MatchString,_tcslen(MatchString)) != 0) {
    return true;
  }

  return false;

}

void
WaypointSorter::filter_name(WaypointSelectInfoVector& vec,
                            const TCHAR * s) const
{
  MatchChar = s[0] &0xff;
  _tcscpy(MatchString, s);

  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointNameFilter), vec.end());
}

static const fixed fixed_18(18);
static Angle Direction;

static bool
WaypointDirectionFilter(const WayPointSelectInfo& elem1) 
{
  fixed DirectionErr = (elem1.Direction-Direction).AngleLimit180().magnitude();
  return (DirectionErr > fixed_18);
}

void 
WaypointSorter::filter_direction(WaypointSelectInfoVector& vec, const Angle direction) const
{
  Direction = direction;
  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointDirectionFilter), vec.end());
}

static fixed MaxDistance;

static bool
WaypointDistanceFilter(const WayPointSelectInfo& elem1)
{
  return (elem1.Distance > MaxDistance);
}

void 
WaypointSorter::filter_distance(WaypointSelectInfoVector& vec, const fixed distance) const
{
  MaxDistance = distance;
  vec.erase(std::remove_if(vec.begin(), vec.end(), WaypointDistanceFilter), vec.end());
}


static bool
WaypointDistanceCompare(const WayPointSelectInfo& elem1, 
                        const WayPointSelectInfo& elem2 ) 
{
  return (elem1.Distance < elem2.Distance);
}

void 
WaypointSorter::sort_distance(WaypointSelectInfoVector& vec) const
{
  std::sort(vec.begin(),
            vec.end(),
            WaypointDistanceCompare);
}


static bool
WaypointNameCompare(const WayPointSelectInfo& elem1, 
                    const WayPointSelectInfo& elem2 ) 
{
  if (elem1.FourChars != elem2.FourChars) {
    return (elem1.FourChars < elem2.FourChars);
  }
  else {
    // if they're equal to 4 chars, then do full name comparison
    return elem1.way_point->Name.compare(elem2.way_point->Name) < 0;
  }
}


void 
WaypointSorter::sort_name(WaypointSelectInfoVector& vec) const
{
  std::sort(vec.begin(),
            vec.end(),
            WaypointNameCompare);
}
