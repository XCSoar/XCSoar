// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct RoutePlannerConfig
{
  enum class Mode : uint8_t {
    NONE,
    TERRAIN,
    AIRSPACE,
    BOTH,
  };

  enum class Polar : uint8_t {
    TASK,
    SAFETY,
  };

  enum class ReachMode : uint8_t {
    OFF,
    STRAIGHT,
    TURNING,
  };

  Mode mode;
  bool allow_climb;
  bool use_ceiling;

  /** Minimum height above terrain for arrival height at non-landable waypoint,
      and for terrain clearance en-route (m) */
  double safety_height_terrain;

  /** Whether to allow turns around obstacles in reach calculations, or just
      straight line */
  ReachMode reach_calc_mode;

  /** Whether reach/abort calculations will use the task or safety polar */
  Polar reach_polar_mode;

  void SetDefaults();

  bool IsTerrainEnabled() const {
    return mode == Mode::TERRAIN || mode == Mode::BOTH;
  }

  bool IsAirspaceEnabled() const {
    return mode == Mode::AIRSPACE || mode== Mode::BOTH;
  }

  bool IsReachEnabled() const {
    return reach_calc_mode != ReachMode::OFF;
  }

  bool IsTurningReachEnabled() const {
    return reach_calc_mode == ReachMode::TURNING;
  }
};
