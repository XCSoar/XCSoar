/* Copyright_License {

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

#ifndef HARNESS_FLIGHT_HPP
#define HARNESS_FLIGHT_HPP

#include "harness_aircraft.hpp"
#include "harness_airspace.hpp"
#include "harness_waypoints.hpp"
#include "harness_task.hpp"

struct AutopilotParameters;

struct TestFlightComponents
{
  AircraftStateFilter *aircraft_filter;
  Airspaces *airspaces;

  TestFlightComponents():aircraft_filter(NULL), airspaces(NULL) {}
};

struct TestFlightResult
{
  bool result;
  double time_elapsed;
  double time_planned;
  double time_remaining;
  double calc_cruise_efficiency;
  double calc_effective_mc;

  TestFlightResult()
    :result(false),
     time_elapsed(0.0), time_planned(1.0), time_remaining(0.0),
     calc_cruise_efficiency(1.0), calc_effective_mc(1.0) {}

  operator bool() {
    return result;
  }
};

TestFlightResult run_flight(TestFlightComponents components,
                            TaskManager &task_manager,
                            const AutopilotParameters &parms, const int n_wind,
                            const double speed_factor = 1.0);

TestFlightResult run_flight(TaskManager &task_manager,
                            const AutopilotParameters &parms, const int n_wind,
                            const double speed_factor = 1.0);

TestFlightResult test_flight(TestFlightComponents components,
                             int test_num, int n_wind,
                             const double speed_factor = 1.0,
                             const bool auto_mc = false);

TestFlightResult test_flight(int test_num, int n_wind,
                             const double speed_factor = 1.0,
                             const bool auto_mc = false);

bool test_flight_times(int test_num, int n_wind);

double
aat_min_time(int test_num);

#endif
