#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "Predicate/AirspacePredicate.hpp"
#include "Geo/GeoVector.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>

const GeoVector &
AirspaceSelectInfo::GetVector(const GeoPoint &location,
                              const FlatProjection &projection) const noexcept
{
  if (!vec.IsValid()) {
    const auto closest_loc = airspace->ClosestPoint(location, projection);
    vec = GeoVector(location, closest_loc);
  }

  return vec;
}

inline bool
AirspaceFilterData::Match(const GeoPoint &location,
                          const FlatProjection &projection,
                          const AbstractAirspace &as) const noexcept
{
  if (cls != AirspaceClass::AIRSPACECLASSCOUNT && as.GetType() != cls)
    return false;

  if (name_prefix != nullptr && !as.MatchNamePrefix(name_prefix))
    return false;

  if (!direction.IsNegative()) {
    const auto closest = as.ClosestPoint(location, projection);
    const auto bearing = location.Bearing(closest);
    auto direction_error = (bearing - direction).AsDelta().Absolute();
    if (direction_error > Angle::Degrees(18))
      return false;
  }

  if (distance >= 0) {
    const auto closest = as.ClosestPoint(location, projection);
    const auto distance = location.Distance(closest);
    if (distance > distance)
      return false;
  }

  return true;
}

class AirspaceFilterPredicate final {
  const GeoPoint location;
  const FlatProjection &projection;
  const AirspaceFilterData &filter;

public:
  AirspaceFilterPredicate(const GeoPoint &_location,
                          const FlatProjection &_projection,
                          const AirspaceFilterData &_filter) noexcept
    :location(_location), projection(_projection), filter(_filter) {}

  [[gnu::pure]]
  bool operator()(const AbstractAirspace &as) const noexcept {
    return filter.Match(location, projection, as);
  }
};

static void
SortByDistance(AirspaceSelectInfoVector &vec, const GeoPoint &location,
               const FlatProjection &projection) noexcept
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return elem1.GetVector(location, projection).distance <
           elem2.GetVector(location, projection).distance;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

static void
SortByName(AirspaceSelectInfoVector &vec) noexcept
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return StringCollate(elem1.GetAirspace().GetName(),
                         elem2.GetAirspace().GetName()) < 0;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

AirspaceSelectInfoVector
FilterAirspaces(const Airspaces &airspaces, const GeoPoint &location,
                const AirspaceFilterData &filter) noexcept
{
  const AirspaceFilterPredicate predicate(location, airspaces.GetProjection(),
                                          filter);
  AirspaceSelectInfoVector result;

  auto range = filter.distance < 0
    ? airspaces.QueryAll()
    : airspaces.QueryWithinRange(location, filter.distance);
  for (const auto &i : range)
    if (predicate(i.GetAirspace()))
      result.emplace_back(i.GetAirspacePtr());

  if (filter.direction.IsNegative() && filter.distance < 0)
    SortByName(result);
  else
    SortByDistance(result, location, airspaces.GetProjection());

  return result;
}
