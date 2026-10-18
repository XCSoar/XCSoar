// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceWarning.hpp"
#include "util/StaticArray.hxx"

/**
 * What an airspace warning says.
 *
 * Near and inside match the warning list. Above is the vertical
 * case: no horizontal distance, and the intercept is above the
 * aircraft.
 */
struct SpokenAirspaceAlert {
  enum class Kind : uint8_t {
    NEAR,
    INSIDE,
    ABOVE,
  };

  Kind kind;

  constexpr bool
  operator==(const SpokenAirspaceAlert &) const noexcept = default;

  static constexpr SpokenAirspaceAlert
  From(AirspaceWarning::State state,
       const AirspaceInterceptSolution &solution,
       double nav_altitude) noexcept {
    if (state == AirspaceWarning::WARNING_INSIDE)
      return {Kind::INSIDE};

    /* Same test as the warning list: distance > 0 is horizontal. */
    if (solution.IsValid() && !(solution.distance > 0) &&
        solution.altitude > nav_altitude)
      return {Kind::ABOVE};

    return {Kind::NEAR};
  }

  /**
   * The sound resource to play.
   */
  [[gnu::pure]]
  StaticArray<const char *, 1> GetClips() const noexcept {
    StaticArray<const char *, 1> clips;
    const char *id = "IDR_WAV_AIRSPACE_NEAR";
    if (kind == Kind::INSIDE)
      id = "IDR_WAV_AIRSPACE_INSIDE";
    else if (kind == Kind::ABOVE)
      id = "IDR_WAV_AIRSPACE_ABOVE";
    clips.push_back(id);
    return clips;
  }
};
