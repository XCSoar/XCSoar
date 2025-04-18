// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "FlightStatistics.hpp"
#include "time/GPSClock.hpp"
#include "time/Stamp.hpp"

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;

class StatsComputer {
  static constexpr std::chrono::steady_clock::duration PERIOD = std::chrono::minutes(1);

  GeoPoint last_location;

  TimeStamp last_climb_start_time, last_cruise_start_time;
  TimeStamp last_thermal_end_time;

  FlightStatistics flightstats;
  GPSClock stats_clock;

public:
  /** Returns the FlightStatistics object */
  FlightStatistics &GetFlightStats() { return flightstats; }
  const FlightStatistics &GetFlightStats() const { return flightstats; }

  void ResetFlight(const bool full = true);
  void StartTask(const NMEAInfo &basic);
  bool DoLogging(const MoreData &basic, const DerivedInfo &calculated);

private:
  void OnClimbBase(const DerivedInfo &calculated);
  void OnClimbCeiling(const DerivedInfo &calculated);
  void OnDepartedThermal(const DerivedInfo &calculated);

public:
  /**
   * Check of climbing has started or ended, and collect statistics
   * about that.
   */
  void ProcessClimbEvents(const DerivedInfo &calculated);
};
