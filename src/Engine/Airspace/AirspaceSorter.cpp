#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#include <algorithm>


static AirspaceClass MatchClass;
static unsigned char MatchChar = 0;
static const TCHAR *name_prefix;
static Angle Direction;
static fixed MaxDistance;

AirspaceSorter::AirspaceSorter(const Airspaces &airspaces,
                               const GeoPoint &Location,
                               const fixed distance_factor)
{
  m_airspaces_all.reserve(airspaces.size());

  for (Airspaces::AirspaceTree::const_iterator it = airspaces.begin();
       it != airspaces.end(); ++it) {

    AirspaceSelectInfo info;

    const AbstractAirspace &airspace = *it->get_airspace();

    info.airspace = &airspace;

    const GeoPoint closest_loc = airspace.ClosestPoint(Location);
    const GeoVector vec(Location, closest_loc);

    info.Distance = vec.Distance * distance_factor;
    info.Direction = vec.Bearing;

    const TCHAR *name = airspace.GetName();

    info.FourChars = ((name[0] & 0xff) << 24) +
                     ((name[1] & 0xff) << 16) +
                     ((name[2] & 0xff) << 8) +
                     ((name[3] & 0xff));

    m_airspaces_all.push_back(info);
  }

  sort_name(m_airspaces_all);
}

const AirspaceSelectInfoVector&
AirspaceSorter::get_list() const
{
  return m_airspaces_all;
}

static bool
AirspaceClassFilter(const AirspaceSelectInfo& elem1)
{
  return elem1.airspace->GetType() != MatchClass;
}

void
AirspaceSorter::filter_class(AirspaceSelectInfoVector& vec,
                             const AirspaceClass t) const
{
  MatchClass = t;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceClassFilter),
            vec.end());
}

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
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceNameFilter),
            vec.end());
}

static bool
AirspaceNamePrefixFilter(const AirspaceSelectInfo& elem1)
{
  return !elem1.airspace->MatchNamePrefix(name_prefix);
}

void
AirspaceSorter::FilterNamePrefix(AirspaceSelectInfoVector &v,
                                 const TCHAR *prefix) const
{
  name_prefix = prefix;
  v.erase(std::remove_if(v.begin(), v.end(), AirspaceNamePrefixFilter),
          v.end());
}

static bool
AirspaceDirectionFilter(const AirspaceSelectInfo& elem1)
{
  fixed DirectionErr = (elem1.Direction - Direction).as_delta().magnitude_degrees();
  return DirectionErr > fixed_int_constant(18);
}

void
AirspaceSorter::filter_direction(AirspaceSelectInfoVector& vec,
                                 const Angle direction) const
{
  Direction = direction;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceDirectionFilter),
            vec.end());
}

static bool
AirspaceDistanceFilter(const AirspaceSelectInfo& elem1)
{
  return (elem1.Distance > MaxDistance);
}

void
AirspaceSorter::filter_distance(AirspaceSelectInfoVector& vec,
                                const fixed distance) const
{
  MaxDistance = distance;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceDistanceFilter),
            vec.end());
}

static bool
AirspaceDistanceCompare(const AirspaceSelectInfo& elem1,
                        const AirspaceSelectInfo& elem2)
{
  return (elem1.Distance < elem2.Distance);
}

void
AirspaceSorter::sort_distance(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(), vec.end(), AirspaceDistanceCompare);
}

static bool
AirspaceNameCompare(const AirspaceSelectInfo& elem1,
                    const AirspaceSelectInfo& elem2)
{
  return (elem1.FourChars < elem2.FourChars);
}

void
AirspaceSorter::sort_name(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(), vec.end(), AirspaceNameCompare);
}
