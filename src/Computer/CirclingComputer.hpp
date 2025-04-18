// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/DeltaTime.hpp"
#include "time/Stamp.hpp"

struct CirclingInfo;
struct NMEAInfo;
struct MoreData;
struct CirclingSettings;
struct FlyingState;

/**
 * Detect when the aircraft begins or ends circling.
 *
 * Dependencies: #FlyingComputer.
 */
class CirclingComputer {
  DeltaTime turn_rate_delta_time;

  Angle last_track, last_heading;

  DeltaTime turning_delta_time;

  /**
   * Start/end time of the turn.
   */
  TimeStamp turn_start_time;

  /**
   * Start/end location of the turn.
   */
  GeoPoint turn_start_location;

  /**
   * Start/end energy height of the turn.
   */
  double turn_start_energy_height;

  /**
   * Start/end altitude of the turn.
   */
  double turn_start_altitude;

  DeltaTime percent_delta_time;

  /**
   * Minimum altitude since start of task.
   */
  double min_altitude;

public:
  /**
   * Reset this computer, as if a new flight starts.
   */
  void Reset();

  /**
   * Reset only statistics, to be called when the task starts, to
   * eliminate the portion of the flight that is irrelevant to the
   * task.
   */
  void ResetStats();

  /**
   * Calculates the turn rate
   */
  void TurnRate(CirclingInfo &circling_info,
                const NMEAInfo &basic,
                const FlyingState &flight);

  /**
   * Calculates the turn rate and the derived features.
   * Determines the current flight mode (cruise/circling).
   */
  void Turning(CirclingInfo &circling_info,
               const MoreData &basic,
               const FlyingState &flight,
               const CirclingSettings &settings);

  /**
   * Calculate the circling time and percentage
   */
  void PercentCircling(const MoreData &basic, const FlyingState &flight,
                       CirclingInfo &circling_info);

  void MaxHeightGain(const MoreData &basic, const FlyingState &flight,
                     CirclingInfo &circling_info);
};
