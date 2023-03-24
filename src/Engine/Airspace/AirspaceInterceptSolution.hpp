// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/FloatDuration.hxx"

#include <type_traits>

/**
 *  Structure to hold data for intercepts between aircraft and airspace.
 *  (interior or exterior)
 */
struct AirspaceInterceptSolution
{
  /** Location of intercept point */
  GeoPoint location;
  /** Distance from observer to intercept point (m) */
  double distance;
  /** Altitude AMSL (m) of intercept point */
  double altitude;
  /** Estimated time (s) for observer to reach intercept point */
  FloatDuration elapsed_time;

  constexpr void SetInvalid() noexcept {
    distance = -1;
    elapsed_time = FloatDuration{-1};
  }

  static constexpr AirspaceInterceptSolution Invalid() noexcept {
    AirspaceInterceptSolution ais;
    ais.SetInvalid();
    return ais;
  }

  /**
   * Determine whether this solution is valid
   *
   * @return True if solution is valid
   */
  constexpr bool IsValid() const noexcept {
    return elapsed_time.count() >= 0;
  }

  constexpr bool IsEarlierThan(const AirspaceInterceptSolution &other) const noexcept {
    return IsValid() && (!other.IsValid() ||
                         elapsed_time < other.elapsed_time);
  }
};

static_assert(std::is_trivial_v<AirspaceInterceptSolution>,
              "type is not trivial");
