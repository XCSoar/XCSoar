#include "harness_flight.hpp"
#include "harness_airspace.hpp"
#include "TaskEventsPrint.hpp"
#include "Util/AircraftStateFilter.hpp"
#ifdef DO_PRINT
#include <fstream>
#endif

Airspaces *airspaces = NULL;
AirspaceWarningManager *airspace_warnings = NULL;
AircraftStateFilter *aircraft_filter = NULL;



double time_elapsed=0.0;
double time_planned=1.0;
double time_remaining=0;
double calc_cruise_efficiency=1.0;

double aat_min_time(int test_num) {
  TaskBehaviour beh;
  switch (test_num) {
  case 2:
    return 3600*3.8;
  default:
    return beh.aat_min_time;
  }
}

int wind_to_mag(int n_wind) {
  if (n_wind) {
    return ((n_wind-1)/4+1)*5;
  }
  return 0;
}

int wind_to_dir(int n_wind) {
  if (n_wind) {
    return 90*((n_wind-1)%4);
  }
  return 0;
}

const char* wind_name(int n_wind) {
  static char buffer[80];
  sprintf(buffer,"%d m/s @ %d",wind_to_mag(n_wind),wind_to_dir(n_wind));
  return buffer;
}


bool run_flight(TaskManager &task_manager,
                GlidePolar &glide_polar,
                bool goto_target,
                double random_mag,
                int n_wind,
                const double speed_factor) 
{
  srand(0);

  AircraftSim ac(0, task_manager, random_mag, goto_target);
  unsigned print_counter=0;

  if (n_wind) {
    ac.set_wind(wind_to_mag(n_wind),wind_to_dir(n_wind));
  }
  ac.set_speed_factor(speed_factor);

#ifdef DO_PRINT
  std::ofstream f4("results/res-sample.txt");
  std::ofstream f5("results/res-sample-filtered.txt");
#endif

  bool do_print = verbose;
  bool first = true;

  time_elapsed=0.0;
  time_planned=1.0;
  time_remaining=0;
  calc_cruise_efficiency=1.0;

  static const fixed fixed_10 =10;

  AirspaceAircraftPerformanceGlide perf(glide_polar);

  if (aircraft_filter) {
    aircraft_filter->reset(ac.get_state());
  }

  if (airspaces) {
    airspace_warnings = new AirspaceWarningManager(*airspaces,
                                                   ac.get_state(),
                                                   glide_polar,
                                                   task_manager);
  }

  do {

    if ((task_manager.getActiveTaskPointIndex()==1) && first 
        && (task_manager.get_stats().total.TimeElapsed>fixed_10)) {
      time_remaining = task_manager.get_stats().total.TimeRemaining;
      first = false;

      time_planned = task_manager.get_stats().total.TimePlanned;

#ifdef DO_PRINT
      if (verbose>1) {
        printf("# time remaining %g\n", time_remaining);
        printf("# time planned %g\n", time_planned);
      }
#endif

    }

#ifdef DO_PRINT
    if (do_print) {
      task_manager.print(ac.get_state());
      ac.print(f4);
      f4.flush();
      if (aircraft_filter) {
        f5 << aircraft_filter->get_speed() << " " 
           << aircraft_filter->get_bearing() << " " 
           << aircraft_filter->get_climb_rate() << "\n"; 
        f5.flush();
      }
    }
#endif

    if (airspaces) {
      scan_airspaces(ac.get_state(), *airspaces, perf,
                     do_print, 
                     ac.target(task_manager));
    }
    if (airspace_warnings) {
#ifdef DO_PRINT
      if (verbose>1) {
        bool warnings_updated = airspace_warnings->update(ac.get_state());
        if (warnings_updated) {
          printf("# airspace warnings updated, size %d\n", (int)airspace_warnings->size());
          print_warnings();
          wait_prompt(ac.get_state().Time);
        }
      }
#endif
    }

    n_samples++;

    do_print = (++print_counter % output_skip ==0) && verbose;

    if (aircraft_filter) {
      aircraft_filter->update(ac.get_state());
    }

  } while (ac.advance(task_manager, glide_polar));

#ifdef DO_PRINT
  if (verbose) {
    task_manager.print(ac.get_state());
    ac.print(f4);
    f4 << "\n";
    f4.flush();
    task_report(task_manager, "end of task\n");
  }
  wait_prompt(0);
#endif

  time_elapsed = task_manager.get_stats().total.TimeElapsed;
  time_planned = task_manager.get_stats().total.TimePlanned;
  calc_cruise_efficiency = task_manager.get_stats().cruise_efficiency;

  if (verbose) {
    distance_counts();
  }

  if (airspace_warnings) {
    delete airspace_warnings;
  }

  return true;
}


bool test_flight(int test_num, int n_wind, const double speed_factor,
                 const bool auto_mc) 
{
  // multipurpose flight test

  GlidePolar glide_polar(2.0,0.0,0.0);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskBehaviour task_behaviour;
  task_behaviour.auto_mc = auto_mc;
  task_behaviour.aat_min_time = aat_min_time(test_num);
  task_behaviour.calc_glide_required = false;

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  bool goto_target = false;

  switch (test_num) {
  case 0:
    goto_target = true;
    break;
  case 2:
    goto_target = true;
    break;
  case 7:
    goto_target = true;
    break;
  default:
    break;
  };

  test_task(task_manager, waypoints, test_num);

  return run_flight(task_manager, glide_polar, goto_target, target_noise, n_wind,
                    speed_factor);
}

bool test_flight_times(int test_num, int n_wind) 
{

  // tests whether elapsed/planned times are consistent
  // there will be small error due to start location

  bool fine = test_flight(test_num, n_wind);
  const double t_rat = fabs(time_elapsed/time_planned-1.0);
  fine &= t_rat<0.02;

  if ((verbose) || !fine) {
    printf("# time remaining %g\n", time_remaining);
    printf("# time elapsed %g\n", time_elapsed);
    printf("# time planned %g\n", time_planned);
    printf("# time ratio error (elapsed/planned) %g\n", t_rat);
  }
  return fine;
}


