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

#include "Printing.hpp"
#include "harness_flight.hpp"
#include "harness_airspace.hpp"
#include "harness_wind.hpp"
#include "TaskEventsPrint.hpp"
#include "Util/AircraftStateFilter.hpp"
#include "Replay/TaskAutoPilot.hpp"
#include "Replay/AircraftSim.hpp"
#include "Replay/TaskAccessor.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Airspace/AirspaceAircraftPerformance.hpp"
#include "OS/FileUtil.hpp"
#include "test_debug.hpp"

#include <fstream>

double
aat_min_time(int test_num)
{
  OrderedTaskSettings beh;
  beh.SetDefaults();
  switch (test_num) {
  case 2:
    return 3600 * 3.8;
  default:
    return beh.aat_min_time;
  }
}

#include "Task/TaskManager.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"

class PrintTaskAutoPilot: public TaskAutoPilot
{
public:
  PrintTaskAutoPilot(const AutopilotParameters &_parms):
    TaskAutoPilot(_parms) {};

protected:
  virtual void OnManualAdvance() {
    if (verbose>1) {
      printf("# manual advance to %d\n",awp);
    }
  }
  virtual void OnModeChange() {
    if (verbose>1) {
      switch (acstate) {
      case Cruise:
        puts("# mode cruise");
        break;
      case Climb:
        puts("# mode climb");
        break;
      case FinalGlide:
        puts("# mode fg");
        break;
      }
    }
  }
  virtual void OnClose() {
    WaitPrompt();
  }
};

TestFlightResult
run_flight(TaskManager &task_manager, const AutopilotParameters &parms,
           const int n_wind, const double speed_factor)
{
  return run_flight(TestFlightComponents(), task_manager, parms, n_wind,
                    speed_factor);
}

TestFlightResult
run_flight(TestFlightComponents components, TaskManager &task_manager,
           const AutopilotParameters &parms, const int n_wind,
           const double speed_factor)
{
  AircraftStateFilter *aircraft_filter = components.aircraft_filter;
  Airspaces *airspaces = components.airspaces;

  TestFlightResult result;

  TaskAccessor ta(task_manager, 300);
  PrintTaskAutoPilot autopilot(parms);
  AircraftSim aircraft;

  autopilot.SetDefaultLocation(GeoPoint(Angle::Degrees(1), Angle::Degrees(0)));

  unsigned print_counter=0;
  if (n_wind)
    aircraft.SetWind(wind_to_mag(n_wind), wind_to_dir(n_wind));

  autopilot.SetSpeedFactor(speed_factor);

  Directory::Create(Path(_T("output/results")));
  std::ofstream f4("output/results/res-sample.txt");
  std::ofstream f5("output/results/res-sample-filtered.txt");

  bool do_print = verbose;
  bool first = true;

  const AirspaceAircraftPerformance perf(task_manager.GetGlidePolar());

  if (aircraft_filter)
    aircraft_filter->Reset(aircraft.GetState());

  autopilot.Start(ta);
  aircraft.Start(autopilot.location_start, autopilot.location_previous,
                 parms.start_alt);

  AirspaceWarningManager *airspace_warnings;
  if (airspaces) {
    AirspaceWarningConfig airspace_warning_config;
    airspace_warning_config.SetDefaults();
    airspace_warnings = new AirspaceWarningManager(airspace_warning_config,
                                                   *airspaces);
    airspace_warnings->Reset(aircraft.GetState());
  } else {
    airspace_warnings = NULL;
  }

  do {

    if ((task_manager.GetActiveTaskPointIndex() == 1) &&
        first && (task_manager.GetStats().total.time_elapsed > 10)) {
      result.time_remaining = (double)task_manager.GetStats().total.time_remaining_now;
      first = false;

      result.time_planned = (double)task_manager.GetStats().total.time_planned;

      if (verbose > 1) {
        printf("# time remaining %g\n", result.time_remaining);
        printf("# time planned %g\n", result.time_planned);
      }
    }

    if (do_print) {
      PrintHelper::taskmanager_print(task_manager, aircraft.GetState());

      const AircraftState state = aircraft.GetState();
      f4 << state.time << " "
         <<  state.location.longitude << " "
         <<  state.location.latitude << " "
         <<  state.altitude << "\n";

      f4.flush();
      if (aircraft_filter) {
        f5 << aircraft_filter->GetSpeed() << " " 
           << aircraft_filter->GetBearing() << " " 
           << aircraft_filter->GetClimbRate() << "\n"; 
        f5.flush();
      }
    }

    if (airspaces) {
      scan_airspaces(aircraft.GetState(), *airspaces, perf,
                     do_print, 
                     autopilot.GetTarget(ta));
    }
    if (airspace_warnings) {
      if (verbose > 1) {
        bool warnings_updated = airspace_warnings->Update(aircraft.GetState(),
                                                          task_manager.GetGlidePolar(),
                                                          task_manager.GetStats(),
                                                          false, 1);
        if (warnings_updated) {
          printf("# airspace warnings updated, size %d\n",
                 (int)airspace_warnings->size());
          print_warnings(*airspace_warnings);
          WaitPrompt();
        }
      }
    }

    n_samples++;

    do_print = (++print_counter % output_skip == 0) && verbose;

    if (aircraft_filter)
      aircraft_filter->Update(aircraft.GetState());

    autopilot.UpdateState(ta, aircraft.GetState());
    aircraft.Update(autopilot.heading);

    {
      const AircraftState state = aircraft.GetState();
      const AircraftState state_last = aircraft.GetLastState();
      task_manager.Update(state, state_last);
      task_manager.UpdateIdle(state);
      task_manager.UpdateAutoMC(state, 0);
    }

  } while (autopilot.UpdateAutopilot(ta, aircraft.GetState()));

  if (verbose) {
    PrintHelper::taskmanager_print(task_manager, aircraft.GetState());

    const AircraftState state = aircraft.GetState();
    f4 << state.time << " "
       <<  state.location.longitude << " "
       <<  state.location.latitude << " "
       <<  state.altitude << "\n";
    f4 << "\n";
    f4.flush();
    task_report(task_manager, "end of task\n");
  }
  WaitPrompt();

  result.time_elapsed = (double)task_manager.GetStats().total.time_elapsed;
  result.time_planned = (double)task_manager.GetStats().total.time_planned;
  result.calc_cruise_efficiency = (double)task_manager.GetStats().cruise_efficiency;
  result.calc_effective_mc = (double)task_manager.GetStats().effective_mc;

  if (verbose)
    PrintDistanceCounts();

  if (airspace_warnings)
    delete airspace_warnings;

  result.result = true;
  return result;
}

TestFlightResult
test_flight(int test_num, int n_wind, const double speed_factor,
            const bool auto_mc)
{
  return test_flight(TestFlightComponents(), test_num, n_wind, speed_factor,
                     auto_mc);
}

TestFlightResult
test_flight(TestFlightComponents components, int test_num, int n_wind,
            const double speed_factor, const bool auto_mc)
{
  // multipurpose flight test

  GlidePolar glide_polar(2);
  Waypoints waypoints;
  SetupWaypoints(waypoints);

  if (verbose)
    PrintDistanceCounts();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  task_behaviour.auto_mc = auto_mc;
  task_behaviour.calc_glide_required = false;
  if ((test_num == 0) || (test_num == 2))
    task_behaviour.optimise_targets_bearing = false;

  TaskManager task_manager(task_behaviour, waypoints);

  TaskEventsPrint default_events(verbose);
  task_manager.SetTaskEvents(default_events);
  task_manager.SetGlidePolar(glide_polar);

  OrderedTaskSettings otb = task_manager.GetOrderedTask().GetOrderedTaskSettings();
  otb.aat_min_time = aat_min_time(test_num);
  task_manager.SetOrderedTaskSettings(otb);

  bool goto_target = false;

  switch (test_num) {
  case 0:
  case 2:
  case 7:
    goto_target = true;
    break;
  };
  autopilot_parms.goto_target = goto_target;
  test_task(task_manager, waypoints, test_num);

  waypoints.Clear(); // clear waypoints so abort wont do anything

  return run_flight(components, task_manager, autopilot_parms, n_wind,
                    speed_factor);
}

bool
test_flight_times(int test_num, int n_wind)
{
  // tests whether elapsed/planned times are consistent
  // there will be small error due to start location

  TestFlightResult result = test_flight(test_num, n_wind);
  bool fine = result.result;
  const double t_rat = fabs(result.time_elapsed / result.time_planned - 1.0);
  fine &= t_rat < 0.02;

  if ((verbose) || !fine) {
    printf("# time remaining %g\n", result.time_remaining);
    printf("# time elapsed %g\n", result.time_elapsed);
    printf("# time planned %g\n", result.time_planned);
    printf("# time ratio error (elapsed/planned) %g\n", t_rat);
  }

  return fine;
}
