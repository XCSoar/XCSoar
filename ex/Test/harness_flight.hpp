#ifndef HARNESS_FLIGHT_HPP
#define HARNESS_FLIGHT_HPP

#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "harness_airspace.hpp"
#include "harness_waypoints.hpp"
#include "harness_task.hpp"

bool run_flight(TaskManager &task_manager,
                GlidePolar &glide_polar,
                int test_num,
                bool goto_target,
                double random_mag,
                int n_wind,
                const double speed_factor=1.0);

bool test_flight(int test_num, int n_wind, const double speed_factor=1.0,
                 const bool auto_mc=false);

#define NUM_WIND 9

const char* wind_name(int n_wind);

bool test_flight_times(int test_num, int n_wind);
bool test_aat(int test_num, int n_wind);
bool test_speed_factor(int test_num, int n_wind);
bool test_cruise_efficiency(int test_num, int n_wind);
bool test_automc(int test_num, int n_wind);
bool test_bestcruisetrack(int test_num, int n_wind);

#endif
