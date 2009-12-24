#include "AirspaceSorter.hpp"
#include <algorithm>
#include "Navigation/Geometry/GeoVector.hpp"
#include "Math/Geometry.hpp"


AirspaceSorter::AirspaceSorter(const Airspaces &airspaces,
                               const GEOPOINT &Location,
                               const fixed distance_factor)
{
  m_airspaces_all.reserve(airspaces.size());

  for (Airspaces::AirspaceTree::const_iterator it = airspaces.begin();
       it != airspaces.end(); ++it) {

    AirspaceSelectInfo info;
    
    const AbstractAirspace &airspace = *it->get_airspace();

    info.airspace = &airspace;

    const GEOPOINT closest_loc = airspace.closest_point(Location);
    const GeoVector vec(Location, closest_loc);

    info.Distance = vec.Distance*distance_factor;
    info.Direction = vec.Bearing;

    tstring Name = airspace.get_name_text(true);

    info.FourChars = 
      ((Name.c_str()[0] & 0xff) << 24)
      +((Name.c_str()[1] & 0xff) << 16)
      +((Name.c_str()[2] & 0xff) << 8)
      +((Name.c_str()[3] & 0xff));

    m_airspaces_all.push_back(info);
  }
  sort_name(m_airspaces_all);
}

const AirspaceSelectInfoVector& 
AirspaceSorter::get_list()
{
  return m_airspaces_all;
}

static AirspaceClass_t MatchClass;

static bool 
AirspaceClassFilter(const AirspaceSelectInfo& elem1) 
{
  return elem1.airspace->get_type() != MatchClass;
}

void
AirspaceSorter::filter_class(AirspaceSelectInfoVector& vec, const AirspaceClass_t t) const
{
  MatchClass = t;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceClassFilter), vec.end());
}


static unsigned char MatchChar = 0;

static bool
AirspaceNameFilter(const AirspaceSelectInfo& elem1)
{
  return (((elem1.FourChars & 0xff000000) >> 24) != MatchChar);
}

void
AirspaceSorter::filter_name(AirspaceSelectInfoVector& vec,
                            const unsigned char c) const
{
  MatchChar = c;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceNameFilter), vec.end());
}

static const fixed fixed_18 = 18;
static fixed Direction;

static bool
AirspaceDirectionFilter(const AirspaceSelectInfo& elem1) 
{
  fixed DirectionErr = fabs(AngleLimit180(elem1.Direction-Direction));
  return (DirectionErr > fixed_18);
}

void 
AirspaceSorter::filter_direction(AirspaceSelectInfoVector& vec, const fixed direction) const
{
  Direction = direction;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceDirectionFilter), vec.end());
}

static fixed MaxDistance;

static bool
AirspaceDistanceFilter(const AirspaceSelectInfo& elem1)
{
  return (elem1.Distance > MaxDistance);
}

void 
AirspaceSorter::filter_distance(AirspaceSelectInfoVector& vec, const fixed distance) const
{
  MaxDistance = distance;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceDistanceFilter), vec.end());
}


static bool
AirspaceDistanceCompare(const AirspaceSelectInfo& elem1, 
                        const AirspaceSelectInfo& elem2 ) 
{
  return (elem1.Distance < elem2.Distance);
}

void 
AirspaceSorter::sort_distance(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(),
            vec.end(),
            AirspaceDistanceCompare);
}


static bool
AirspaceNameCompare(const AirspaceSelectInfo& elem1, 
                    const AirspaceSelectInfo& elem2 ) 
{
  return (elem1.FourChars < elem2.FourChars);
}


void 
AirspaceSorter::sort_name(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(),
            vec.end(),
            AirspaceNameCompare);
}
