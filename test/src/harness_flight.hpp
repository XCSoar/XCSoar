// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  FloatDuration time_elapsed;
  FloatDuration time_planned;
  FloatDuration time_remaining;
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

std::chrono::duration<unsigned>
aat_min_time(int test_num) noexcept;
