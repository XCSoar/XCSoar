// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Id.hpp"
#include "util/StaticString.hxx"
#include "TeamCode.hpp"

#include <type_traits>

/**
 * Settings for teamcode calculations
 */
struct TeamCodeSettings {
  /** Reference waypoint id for code origin */
  int team_code_reference_waypoint;

  /** CN of the glider to track */
  StaticString<4> team_flarm_callsign;

  /**
   * Check TeamCode::IsDefined() before using this attribute.
   */
  TeamCode team_code;

  /**
   * FlarmId of the glider to track.  Check FlarmId::IsDefined()
   * before using this attribute.
   */
  FlarmId team_flarm_id;

  void SetDefaults();

  /**
   * Track a specific FLARM.
   *
   * Don't use this method directory, use TeamActions::TrackFlarm()
   * instead.
   */
  void TrackFlarm(FlarmId id, const char *callsign);
};

static_assert(std::is_trivial<TeamCodeSettings>::value, "type is not trivial");
