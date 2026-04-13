// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/Ptr.hpp"
#include "Geo/GeoPoint.hpp"
#include "Projection/WindowProjection.hpp"

#include <span>

/**
 * Invoke f(const AbstractAirspace &) for each airspace from the R-tree
 * within range of query_center, then each external airspace, applying
 * visible() to both sources.
 */
template<typename F>
inline void
ForEachAirspaceInRange(const Airspaces *airspaces,
                       std::span<const ConstAirspacePtr> external,
                       const GeoPoint &query_center, double range_m,
                       const AirspacePredicate &visible, F &&f)
{
  if (airspaces != nullptr) {
    for (const auto &i : airspaces->QueryWithinRange(query_center, range_m)) {
      const AbstractAirspace &airspace = i.GetAirspace();
      if (visible(airspace))
        f(airspace);
    }
  }

  for (const auto &ea : external) {
    if (visible(*ea))
      f(*ea);
  }
}

/**
 * Same as ForEachAirspaceInRange() using the projection viewport center
 * and radius in metres.
 */
template<typename F>
inline void
ForEachAirspaceInView(const Airspaces *airspaces,
                      std::span<const ConstAirspacePtr> external,
                      const WindowProjection &projection,
                      const AirspacePredicate &visible, F &&f)
{
  ForEachAirspaceInRange(airspaces, external,
                         projection.GetGeoScreenCenter(),
                         projection.GetScreenDistanceMeters(),
                         visible, std::forward<F>(f));
}
