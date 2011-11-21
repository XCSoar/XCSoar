/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "harness_airspace.hpp"
#include "harness_waypoints.hpp"
#include "harness_task.hpp"

bool run_flight(TaskManager &task_manager,
                const AutopilotParameters &parms,
                const int n_wind,
                const double speed_factor=1.0);

bool test_flight(int test_num, int n_wind, const double speed_factor=1.0,
                 const bool auto_mc=false);

#define NUM_WIND 9

const char* wind_name(int n_wind);

bool test_flight_times(int test_num, int n_wind);
bool test_aat(int test_num, int n_wind);
bool test_speed_factor(int test_num, int n_wind);
bool test_cruise_efficiency(int test_num, int n_wind);
bool test_effective_mc(int test_num, int n_wind);
bool test_automc(int test_num, int n_wind);
bool test_bestcruisetrack(int test_num, int n_wind);
bool test_abort(int n_wind);
bool test_goto(int n_wind, unsigned id, bool auto_mc=false);
bool test_null();
bool test_airspace(const unsigned n_airspaces);
bool test_olc(int n_wind, Contests id);

fixed
aat_min_time(int test_num);

#endif
