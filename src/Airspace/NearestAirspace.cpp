// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NearestAirspace.hpp"
#include "ProtectedAirspaceWarningManager.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicateHeightRange.hpp"
#include "Engine/Airspace/Predicate/OutsideAirspacePredicate.hpp"
#include "Engine/Airspace/Minimum.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

#include <concepts>

[[gnu::pure]] [[gnu::always_inline]]
static inline NearestAirspace
CalculateNearestAirspaceHorizontal(const GeoPoint &location,
                                   const FlatProjection &projection,
                                   const AbstractAirspace &airspace) noexcept
{
  const auto closest = airspace.ClosestPoint(location, projection);
  assert(closest.IsValid());

  return NearestAirspace(airspace, closest.DistanceS(location));
}

struct CompareNearestAirspace {
  [[gnu::pure]]
  bool operator()(const NearestAirspace &a, const NearestAirspace &b) const {
    return !b.IsDefined() || a.distance < b.distance;
  }
};

[[gnu::pure]]
static NearestAirspace
FindHorizontal(const GeoPoint &location,
               const Airspaces &airspace_database,
               std::predicate<const AbstractAirspace &> auto predicate) noexcept
{
  const auto &projection = airspace_database.GetProjection();
  return FindMinimum(airspace_database, location, 30000, predicate,
                     [&location, &projection](ConstAirspacePtr &&airspace){
                       return CalculateNearestAirspaceHorizontal(location, projection, *airspace);
                     },
                     CompareNearestAirspace());
}

[[gnu::pure]]
NearestAirspace
NearestAirspace::FindHorizontal(const MoreData &basic,
                                const ProtectedAirspaceWarningManager *airspace_warnings,
                                const Airspaces &airspace_database) noexcept
{
  if (!basic.location_available)
    /* can't check for airspaces without a GPS fix */
    return NearestAirspace();

  /* find the nearest airspace */
  //consider only active airspaces
  auto outside_and_active =
    MakeAndPredicate(ActiveAirspacePredicate(airspace_warnings),
                     OutsideAirspacePredicate(AGeoPoint(basic.location, 0)));

  //if altitude is available, filter airspaces in same height as airplane
  if (basic.NavAltitudeAvailable()) {
    /* check altitude; hard-coded margin of 50m (for now) */
    auto outside_and_active_and_height =
      MakeAndPredicate(outside_and_active,
                       AirspacePredicateHeightRange(basic.nav_altitude - 50,
                                                    basic.nav_altitude + 50));
    return ::FindHorizontal(basic.location, airspace_database,
                            std::move(outside_and_active_and_height));
  } else {
    /* only filter outside and active */
    return ::FindHorizontal(basic.location, airspace_database,
                            std::move(outside_and_active));
  }
}

[[gnu::pure]]
NearestAirspace
NearestAirspace::FindVertical(const MoreData &basic,
                              const DerivedInfo &calculated,
                              const ProtectedAirspaceWarningManager *airspace_warnings,
                              const Airspaces &airspace_database) noexcept
{
  if (!basic.location_available ||
      (!basic.baro_altitude_available && !basic.gps_altitude_available))
    /* can't check for airspaces without a GPS fix and altitude
       value */
    return NearestAirspace();

  /* find the nearest airspace */

  AltitudeState altitude;
  altitude.altitude = basic.nav_altitude;
  altitude.altitude_agl = calculated.altitude_agl;

  const AbstractAirspace *nearest = nullptr;
  double nearest_delta = 100000;
  const ActiveAirspacePredicate active_predicate(airspace_warnings);

  for (const auto &i : airspace_database.QueryInside(basic.location)) {
    const AbstractAirspace &airspace = i.GetAirspace();

    if (!active_predicate(airspace))
      continue;

    /* check delta below */
    auto base = airspace.GetBase().GetAltitude(altitude);
    auto base_delta = base - altitude.altitude;
    if (base_delta >= 0 && base_delta < fabs(nearest_delta)) {
      nearest = &airspace;
      nearest_delta = base_delta;
    }

    /* check delta above */
    auto top = airspace.GetTop().GetAltitude(altitude);
    auto top_delta = altitude.altitude - top;
    if (top_delta >= 0 && top_delta < fabs(nearest_delta)) {
      nearest = &airspace;
      nearest_delta = -top_delta;
    }
  }

  if (nearest == nullptr)
    return NearestAirspace();

  return NearestAirspace(*nearest, nearest_delta);
}
