// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NearestAirspace.hpp"
#include "ProtectedAirspaceWarningManager.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicateHeightRange.hpp"
#include "Engine/Airspace/Predicate/OutsideAirspacePredicate.hpp"
#include "Engine/Airspace/Minimum.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "util/StaticArray.hxx"

#include <algorithm>
#include <concepts>
#include <cstddef>

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

namespace {

/* Altitude band of one airspace at aircraft position */
struct Band {
  const AbstractAirspace *airspace;
  double base, top;
  bool warning_capable;  // would trigger a warning when entered
  bool cleared;          // suppresses warnings from overlapping bands
};

using BandList = StaticArray<Band, 64>;

struct Edge {
  double z;
  std::size_t idx;
  /* True if crossing this edge in the sweep direction enters the
     band; false if it exits the band. */
  bool entering;
};

/* Sweep band edges (dir = +1: upward, -1: downward) starting at
   start_z. Returns the first altitude transitioning from "no
   warning" to "warning" (along with one warning-capable
   airspace at that altitude), or {nullptr, 0} if no such transition
   exists in this direction. */
[[gnu::pure]]
static std::pair<const AbstractAirspace *, double>
SweepForWarningEntry(const BandList &bands, double start_z, int dir) noexcept
{
  StaticArray<Edge, 128> edges;
  for (std::size_t i = 0; i < bands.size(); ++i) {
    const auto &b = bands[i];
    if (dir > 0) {
      // going up: base is entering, top is exiting
      if (b.base > start_z) edges.checked_append({b.base, i, true});
      if (b.top > start_z) edges.checked_append({b.top, i, false});
    } else {
      // going down: top is entering, base is exiting
      if (b.top < start_z) edges.checked_append({b.top, i, true});
      if (b.base < start_z) edges.checked_append({b.base, i, false});
    }
  }

  std::sort(edges.begin(), edges.end(),
            [dir](const Edge &a, const Edge &b) noexcept {
              return dir > 0 ? a.z < b.z : a.z > b.z;
            });

  /* Initial membership at start_z. */
  int cleared = 0, warning = 0;
  for (const auto &b : bands) {
    if (b.base <= start_z && start_z <= b.top) {
      if (b.cleared) ++cleared;
      if (b.warning_capable) ++warning;
    }
  }

  bool prev = warning > 0 && cleared == 0;

  std::size_t j = 0;
  while (j < edges.size()) {
    const double z = edges[j].z;
    /* Apply all edges at this altitude to the running membership
       counts, so f(z + dir*epsilon) is reflected after the inner loop. */
    while (j < edges.size() && edges[j].z == z) {
      const auto &b = bands[edges[j].idx];
      const int delta = edges[j].entering ? +1 : -1;
      if (b.cleared) cleared += delta;
      if (b.warning_capable) warning += delta;
      ++j;
    }

    const bool cur = warning > 0 && cleared == 0;
    if (!prev && cur) {
      /* Pick a representative airspace whose altitude band actually
         contains the probe interval just past z. */
      for (const auto &b : bands) {
        if (!b.warning_capable || b.cleared) continue;
        const bool in = dir > 0
          ? (b.base <= z && b.top > z)
          : (b.base < z && b.top >= z);
        if (in)
          return {b.airspace, std::abs(z - start_z)};
      }
    }
    prev = cur;
  }

  return {nullptr, 0.0};
}

} // anonymous namespace

[[gnu::pure]]
NearestAirspace
NearestAirspace::FindNextWarningEntry(const MoreData &basic,
                                      const DerivedInfo &calculated,
                                      const ProtectedAirspaceWarningManager *airspace_warnings,
                                      const Airspaces &airspace_database) noexcept
{
  if (!basic.location_available ||
      (!basic.baro_altitude_available && !basic.gps_altitude_available))
    return NearestAirspace();

  AltitudeState altitude;
  altitude.altitude = basic.nav_altitude;
  altitude.altitude_agl = calculated.altitude_agl;

  AirspaceWarningCopy warnings;
  if (airspace_warnings != nullptr)
    warnings.Visit(*airspace_warnings);

  BandList bands;
  for (const auto &i : airspace_database.QueryInside(basic.location)) {
    const AbstractAirspace &airspace = i.GetAirspace();

    const bool is_cleared = warnings.IsCleared(airspace);
    const bool is_warning =
      !is_cleared &&
      airspace.IsActive() &&
      warnings.IsWarningCapable(airspace) &&
      !warnings.IsAcked(airspace);

    if (!is_cleared && !is_warning)
      continue;

    Band band;
    band.airspace = &airspace;
    band.base = airspace.GetBase().GetAltitude(altitude);
    band.top = airspace.GetTop().GetAltitude(altitude);
    band.warning_capable = is_warning;
    band.cleared = is_cleared;
    if (!bands.checked_append(band))
      break;
  }

  if (bands.empty())
    return NearestAirspace();

  const auto up = SweepForWarningEntry(bands, altitude.altitude, +1);
  const auto down = SweepForWarningEntry(bands, altitude.altitude, -1);

  if (up.first == nullptr && down.first == nullptr)
    return NearestAirspace();
  if (up.first == nullptr)
    return NearestAirspace(*down.first, -down.second);
  if (down.first == nullptr)
    return NearestAirspace(*up.first, up.second);
  return up.second <= down.second
    ? NearestAirspace(*up.first, up.second)
    : NearestAirspace(*down.first, -down.second);
}
