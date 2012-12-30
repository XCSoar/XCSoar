#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "Geo/GeoVector.hpp"

#include <string.h>
#include <algorithm>

AirspaceSelectInfo::AirspaceSelectInfo(const AbstractAirspace &_airspace)
  :airspace(&_airspace), vec(GeoVector::Invalid())
{
  const TCHAR *name = airspace->GetName();
  four_chars = ((name[0] & 0xff) << 24) +
               ((name[1] & 0xff) << 16) +
               ((name[2] & 0xff) << 8) +
               ((name[3] & 0xff));
}

void
AirspaceSelectInfo::ResetVector()
{
  vec.SetInvalid();
}

const GeoVector &
AirspaceSelectInfo::GetVector(const GeoPoint &location,
                              const TaskProjection &projection) const
{
  if (!vec.IsValid()) {
    const GeoPoint closest_loc = airspace->ClosestPoint(location, projection);
    vec = GeoVector(location, closest_loc);
  }

  return vec;
}

AirspaceSorter::AirspaceSorter(const Airspaces &airspaces,
                               const GeoPoint &_location)
  :projection(airspaces.GetProjection()), location(_location)
{
  m_airspaces_all.reserve(airspaces.size());

  for (auto it = airspaces.begin(); it != airspaces.end(); ++it) {
    const AbstractAirspace &airspace = *it->GetAirspace();
    AirspaceSelectInfo info(airspace);
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
  const TaskProjection &projection;
  const GeoPoint &location;

public:
  AirspaceDirectionFilter(Angle _direction,
      const TaskProjection &_projection, const GeoPoint &_location)
    :direction(_direction), projection(_projection), location(_location) {}

  bool operator()(const AirspaceSelectInfo &info) {
    Angle bearing = info.GetVector(location, projection).bearing;
    fixed direction_error = (bearing - direction).AsDelta().AbsoluteDegrees();
    return direction_error > fixed_int_constant(18);
  }
};

void
AirspaceSorter::FilterByDirection(AirspaceSelectInfoVector& vec,
                                 const Angle direction) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(),
      AirspaceDirectionFilter(direction, projection, location)), vec.end());
}

class AirspaceDistanceFilter
{
  fixed min_distance;
  const TaskProjection &projection;
  const GeoPoint &location;

public:
  AirspaceDistanceFilter(fixed _min_distance,
      const TaskProjection &_projection, const GeoPoint &_location)
    :min_distance(_min_distance), projection(_projection), location(_location) {}

  bool operator()(const AirspaceSelectInfo &info) {
    return info.GetVector(location, projection).distance > min_distance;
  }
};

void
AirspaceSorter::FilterByDistance(AirspaceSelectInfoVector& vec,
                                const fixed distance) const
{
  vec.erase(std::remove_if(vec.begin(), vec.end(),
      AirspaceDistanceFilter(distance, projection, location)), vec.end());
}

class AirspaceDistanceCompare
{
  const TaskProjection &projection;
  const GeoPoint &location;

public:
  AirspaceDistanceCompare(
      const TaskProjection &_projection, const GeoPoint &_location)
    :projection(_projection), location(_location) {}

  bool operator()(const AirspaceSelectInfo &elem1,
                  const AirspaceSelectInfo &elem2) {
    return elem1.GetVector(location, projection).distance <
           elem2.GetVector(location, projection).distance;
  }
};

void
AirspaceSorter::SortByDistance(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(), vec.end(), AirspaceDistanceCompare(projection, location));
}

static bool
AirspaceNameCompare(const AirspaceSelectInfo& elem1,
                    const AirspaceSelectInfo& elem2)
{
  return _tcscmp(elem1.airspace->GetName(), elem2.airspace->GetName()) < 0;
}

void
AirspaceSorter::SortByName(AirspaceSelectInfoVector& vec) const
{
  std::sort(vec.begin(), vec.end(), AirspaceNameCompare);
}
