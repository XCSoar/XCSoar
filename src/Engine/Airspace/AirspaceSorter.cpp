#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#include <algorithm>

static AirspaceClass match_class;
static unsigned char match_char = 0;
static const TCHAR *name_prefix;
static Angle direction;
static fixed max_distance;

AirspaceSorter::AirspaceSorter(const Airspaces &airspaces,
                               const GeoPoint &Location,
                               const fixed distance_factor)
{
  m_airspaces_all.reserve(airspaces.size());

  for (auto it = airspaces.begin(); it != airspaces.end(); ++it) {
    AirspaceSelectInfo info;

    const AbstractAirspace &airspace = *it->get_airspace();

    info.airspace = &airspace;

    const GeoPoint closest_loc =
      airspace.ClosestPoint(Location, airspaces.GetProjection());
    const GeoVector vec(Location, closest_loc);

    info.distance = vec.distance * distance_factor;
    info.direction = vec.bearing;

    const TCHAR *name = airspace.GetName();

    info.four_chars = ((name[0] & 0xff) << 24) +
                     ((name[1] & 0xff) << 16) +
                     ((name[2] & 0xff) << 8) +
                     ((name[3] & 0xff));

    m_airspaces_all.push_back(info);
  }

  SortByName(m_airspaces_all);
}

const AirspaceSelectInfoVector&
AirspaceSorter::GetList() const
{
  return m_airspaces_all;
}

static bool
AirspaceClassFilter(const AirspaceSelectInfo& elem1)
{
  return elem1.airspace->GetType() != match_class;
}

void
AirspaceSorter::FilterByClass(AirspaceSelectInfoVector& vec,
                             const AirspaceClass t) const
{
  match_class = t;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceClassFilter),
            vec.end());
}

static bool
AirspaceNameFilter(const AirspaceSelectInfo& elem1)
{
  return (((elem1.four_chars & 0xff000000) >> 24) != match_char);
}

void
AirspaceSorter::FilterByName(AirspaceSelectInfoVector& vec,
                            const unsigned char c) const
{
  match_char = c;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceNameFilter),
            vec.end());
}

static bool
AirspaceNamePrefixFilter(const AirspaceSelectInfo& elem1)
{
  return !elem1.airspace->MatchNamePrefix(name_prefix);
}

void
AirspaceSorter::FilterByNamePrefix(AirspaceSelectInfoVector &v,
                                   const TCHAR *prefix) const
{
  name_prefix = prefix;
  v.erase(std::remove_if(v.begin(), v.end(), AirspaceNamePrefixFilter),
          v.end());
}

static bool
AirspaceDirectionFilter(const AirspaceSelectInfo& elem1)
{
  fixed DirectionErr = (elem1.direction - direction).AsDelta().AbsoluteDegrees();
  return DirectionErr > fixed_int_constant(18);
}

void
AirspaceSorter::FilterByDirection(AirspaceSelectInfoVector& vec,
                                 const Angle _direction) const
{
  direction = _direction;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceDirectionFilter),
            vec.end());
}

static bool
AirspaceDistanceFilter(const AirspaceSelectInfo& elem1)
{
  return (elem1.distance > max_distance);
}

void
AirspaceSorter::FilterByDistance(AirspaceSelectInfoVector& vec,
                                const fixed distance) const
{
  max_distance = distance;
  vec.erase(std::remove_if(vec.begin(), vec.end(), AirspaceDistanceFilter),
            vec.end());
}

static bool
AirspaceDistanceCompare(const AirspaceSelectInfo& elem1,
                        const AirspaceSelectInfo& elem2)
{
  return (elem1.distance < elem2.distance);
}

void
AirspaceSorter::SortByDistance(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(), vec.end(), AirspaceDistanceCompare);
}

static bool
AirspaceNameCompare(const AirspaceSelectInfo& elem1,
                    const AirspaceSelectInfo& elem2)
{
  return (elem1.four_chars < elem2.four_chars);
}

void
AirspaceSorter::SortByName(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(), vec.end(), AirspaceNameCompare);
}
