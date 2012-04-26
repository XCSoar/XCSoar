#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

#include <algorithm>

AirspaceSorter::AirspaceSorter(const Airspaces &airspaces,
                               const GeoPoint &Location)
{
  m_airspaces_all.reserve(airspaces.size());

  for (auto it = airspaces.begin(); it != airspaces.end(); ++it) {
    AirspaceSelectInfo info;

    const AbstractAirspace &airspace = *it->get_airspace();

    info.airspace = &airspace;

    const GeoPoint closest_loc =
      airspace.ClosestPoint(Location, airspaces.GetProjection());
    const GeoVector vec(Location, closest_loc);

    info.distance = vec.distance;
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

class AirspaceClassFilter
{
  AirspaceClass match_class;

public:
  AirspaceClassFilter(AirspaceClass _match_class): match_class(_match_class) {}

  bool operator()(const AirspaceSelectInfo &info) {
    return info.airspace->GetType() != match_class;
  }
};

void
AirspaceSorter::FilterByClass(AirspaceSelectInfoVector& vec,
                             const AirspaceClass match_class) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(),
                           AirspaceClassFilter(match_class)), vec.end());
}

class AirspaceNameFilter
{
  unsigned char match_char;

public:
  AirspaceNameFilter(unsigned char _match_char): match_char(_match_char) {}

  bool operator()(const AirspaceSelectInfo &info) {
    return (((info.four_chars & 0xff000000) >> 24) != match_char);
  }
};

void
AirspaceSorter::FilterByName(AirspaceSelectInfoVector& vec,
                             const unsigned char match_char) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(),
                           AirspaceNameFilter(match_char)), vec.end());
}

class AirspaceNamePrefixFilter
{
  const TCHAR *prefix;

public:
  AirspaceNamePrefixFilter(const TCHAR *_prefix): prefix(_prefix) {}

  bool operator()(const AirspaceSelectInfo &info) {
    return !info.airspace->MatchNamePrefix(prefix);
  }
};

void
AirspaceSorter::FilterByNamePrefix(AirspaceSelectInfoVector &v,
                                   const TCHAR *prefix) const
{
  v.erase(std::remove_if(v.begin(), v.end(),
                         AirspaceNamePrefixFilter(prefix)), v.end());
}

class AirspaceDirectionFilter
{
  Angle direction;

public:
  AirspaceDirectionFilter(Angle _direction): direction(_direction) {}

  bool operator()(const AirspaceSelectInfo &info) {
    fixed direction_error = (info.direction - direction).AsDelta().AbsoluteDegrees();
    return direction_error > fixed_int_constant(18);
  }
};

void
AirspaceSorter::FilterByDirection(AirspaceSelectInfoVector& vec,
                                 const Angle direction) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(),
                           AirspaceDirectionFilter(direction)), vec.end());
}

class AirspaceDistanceFilter
{
  fixed min_distance;

public:
  AirspaceDistanceFilter(fixed _min_distance): min_distance(_min_distance) {}

  bool operator()(const AirspaceSelectInfo &info) {
    return info.distance > min_distance;
  }
};

void
AirspaceSorter::FilterByDistance(AirspaceSelectInfoVector& vec,
                                const fixed distance) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(),
                           AirspaceDistanceFilter(distance)), vec.end());
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
