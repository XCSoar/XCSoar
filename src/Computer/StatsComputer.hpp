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

#ifndef XCSOAR_STATS_COMPUTER_HPP
#define XCSOAR_STATS_COMPUTER_HPP

#include "Geo/GeoPoint.hpp"
#include "FlightStatistics.hpp"
#include "Time/GPSClock.hpp"

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;

class StatsComputer {
  static constexpr unsigned PERIOD = 60;

  GeoPoint last_location;

  double last_climb_start_time, last_cruise_start_time;
  double last_thermal_end_time;

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

#endif
