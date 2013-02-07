#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceVisitor.hpp"
#include "Geo/GeoVector.hpp"

#include <string.h>
#include <algorithm>

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

bool
AirspaceFilterData::Match(const GeoPoint &location,
                          const TaskProjection &projection,
                          const AbstractAirspace &as) const
{
  if (cls != AirspaceClass::AIRSPACECLASSCOUNT && as.GetType() != cls)
    return false;

  if (name_prefix != nullptr && !as.MatchNamePrefix(name_prefix))
    return false;

  if (!negative(direction.Native())) {
    const GeoPoint closest = as.ClosestPoint(location, projection);
    const Angle bearing = location.Bearing(closest);
    fixed direction_error = (bearing - direction).AsDelta().AbsoluteDegrees();
    if (direction_error > fixed(18))
      return false;
  }

  if (!negative(distance)) {
    const GeoPoint closest = as.ClosestPoint(location, projection);
    const fixed distance = location.Distance(closest);
    if (distance > distance)
      return false;
  }

  return true;
}

class AirspaceFilterVisitor final : public AirspaceVisitor {
  GeoPoint location;
  const TaskProjection &projection;
  const AirspaceFilterData &filter;

public:
  AirspaceSelectInfoVector result;

  AirspaceFilterVisitor(const GeoPoint &_location,
                        const TaskProjection &_projection,
                        const AirspaceFilterData &_filter)
    :location(_location), projection(_projection), filter(_filter) {}

  virtual void Visit(const AbstractAirspace &as) {
    if (filter.Match(location, projection, as))
      result.emplace_back(as);
  }
};

static void
SortByDistance(AirspaceSelectInfoVector &vec, const GeoPoint &location,
               const TaskProjection &projection)
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return elem1.GetVector(location, projection).distance <
           elem2.GetVector(location, projection).distance;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

static void
SortByName(AirspaceSelectInfoVector &vec)
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return _tcscmp(elem1.GetAirspace().GetName(),
                   elem2.GetAirspace().GetName()) < 0;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

AirspaceSelectInfoVector
FilterAirspaces(const Airspaces &airspaces, const GeoPoint &location,
                const AirspaceFilterData &filter)
{
  AirspaceFilterVisitor visitor(location, airspaces.GetProjection(), filter);

  if (!negative(filter.distance))
    airspaces.VisitWithinRange(location, filter.distance, visitor);
  else
    for (const auto &i : airspaces)
      visitor.Visit(*i.GetAirspace());

  if (negative(filter.direction.Native()) && negative(filter.distance))
    SortByName(visitor.result);
  else
    SortByDistance(visitor.result, location, airspaces.GetProjection());

  return visitor.result;
}
