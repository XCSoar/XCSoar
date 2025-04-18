// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SoonestAirspace.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceInterceptSolution.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "Minimum.hpp"
#include "Predicate/AirspacePredicate.hpp"
#include "Navigation/Aircraft.hpp"

struct SoonestAirspace {
  ConstAirspacePtr airspace;
  FloatDuration time{-1};

  SoonestAirspace() = default;
  SoonestAirspace(ConstAirspacePtr &&_airspace,
                  FloatDuration _time)
    :airspace(std::move(_airspace)), time(_time) {}


  bool IsDefined() const {
    return airspace != nullptr;
  }
};

[[gnu::pure,gnu::always_inline]]
static inline SoonestAirspace
CalculateSoonestAirspace(const AircraftState &state,
                         const AirspaceAircraftPerformance &perf,
                         const FloatDuration max_time,
                         const FlatProjection &projection,
                         ConstAirspacePtr &&airspace)
{
  const auto closest = airspace->ClosestPoint(state.location, projection);
  assert(closest.IsValid());

  const auto solution = airspace->Intercept(state, perf, closest, closest);
  if (!solution.IsValid() ||
      solution.elapsed_time > max_time)
    return SoonestAirspace();

  return SoonestAirspace(std::move(airspace), solution.elapsed_time);
}

struct CompareSoonestAirspace {
  [[gnu::pure]]
  bool operator()(const SoonestAirspace &a, const SoonestAirspace &b) const {
    return a.IsDefined() && (!b.IsDefined() || a.time < b.time);
  }
};

ConstAirspacePtr
FindSoonestAirspace(const Airspaces &airspaces,
                    const AircraftState &state,
                    const AirspaceAircraftPerformance &perf,
                    AirspacePredicate predicate,
                    const FloatDuration max_time)
{
  const auto &projection = airspaces.GetProjection();
  const auto range = perf.GetMaxSpeed() * max_time;
  return std::move(FindMinimum(airspaces, state.location, range.count(), predicate,
                               [&state, &perf, max_time,
                                &projection](ConstAirspacePtr &&airspace){
                                 return CalculateSoonestAirspace(state, perf, max_time,
                                                                 projection,
                                                                 std::move(airspace));
                               },
                               CompareSoonestAirspace()).airspace);
}
