/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_CIRCLING_COMPUTER_HPP
#define XCSOAR_CIRCLING_COMPUTER_HPP

#include "Geo/GeoPoint.hpp"
#include "Time/DeltaTime.hpp"

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
  double turn_start_time;

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

#endif
