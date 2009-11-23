#include "harness_flight.hpp"
#ifdef DO_PRINT
#include <fstream>
#endif

double time_elapsed=0.0;
double time_planned=1.0;
double cruise_efficiency=1.0;

bool run_flight(TaskManager &task_manager,
                GlidePolar &glide_polar,
                int test_num,
                bool goto_target,
                double random_mag,
                int n_wind,
                const double speed_factor) 
{
  AircraftSim ac(0, task_manager, random_mag, goto_target);
  unsigned print_counter=0;

  ac.set_wind(n_wind*5.0,0.0);
  ac.set_speed_factor(speed_factor);

#ifdef DO_PRINT
  std::ofstream f4("results/res-sample.txt");
#endif

  bool do_print = verbose;
  bool first = true;

  double time_remaining = 0;

  do {

    if ((task_manager.getActiveTaskPointIndex()==1) && first 
        && (task_manager.get_stats().total.TimeElapsed>10)) {
      time_remaining = task_manager.get_stats().total.TimeRemaining;
      first = false;

      double time_planned = task_manager.get_stats().total.TimePlanned;
      if (verbose>1) {
        printf("# time remaining %g\n", time_remaining);
        printf("# time planned %g\n", time_planned);
      }

    }

    if (do_print) {
      task_manager.print(ac.get_state());
      ac.print(f4);
      f4.flush();
    }

/*
    scan_airspaces(ac.get_state(), airspaces, do_print, 
                   ac.get_next());
*/

    n_samples++;

    do_print = (print_counter++ % output_skip ==0) && verbose;

  } while (ac.advance(task_manager, glide_polar));

  if (verbose) {
    task_manager.print(ac.get_state());
    ac.print(f4);
    f4.flush();
  }

  time_elapsed = task_manager.get_stats().total.TimeElapsed;
  time_planned = task_manager.get_stats().total.TimePlanned;
  cruise_efficiency = task_manager.get_stats().cruise_efficiency;

  bool time_ok = fabs(time_elapsed/time_planned-1.0)<0.02;
  if ((verbose) || !time_ok) {
    printf("# time remaining %g\n", time_remaining);
    printf("# time elapsed %g\n", time_elapsed);
    printf("# time planned %g\n", time_planned);
    printf("# cruise efficiency %g\n", cruise_efficiency);
  }
  if (verbose) {
    distance_counts();
  }
  return time_ok;
}


bool test_flight(int test_num, int n_wind, const double speed_factor) 
{
  //// aircraft
  GlidePolar glide_polar(2.0,0.0,0.0);

  ////////////////////////// Waypoints //////
  Waypoints waypoints;
  setup_waypoints(waypoints);

  ////////////////////////// TASK //////

  if (verbose) {
    distance_counts();
  }

  TaskBehaviour task_behaviour;

  TaskEvents default_events;  default_events.verbose = verbose;

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  bool goto_target = false;
  switch (test_num) {
  case 0:
    goto_target = true;
    test_task_mixed(task_manager, waypoints);
    break;
  case 1:
    test_task_fai(task_manager, waypoints);
    break;
  case 2:
    task_behaviour.aat_min_time = 3600*3.8;
    goto_target = true;
    test_task_aat(task_manager, waypoints);
    break;
  case 3:
    test_task_or(task_manager, waypoints);
    break;
  case 4:
    test_task_dash(task_manager, waypoints);
    break;
  default:
    break;
  };

  return run_flight(task_manager, glide_polar, test_num, goto_target, target_noise, n_wind,
    speed_factor);
}


bool test_speed_factor(int test_num, int n_wind) 
{
  // flying at opt speed should be minimum time flight!

  double te0, te1, te2;

  srand(0);
  test_flight(test_num, n_wind, 1.0);
  te0 = time_elapsed;

  srand(0);
  test_flight(test_num, n_wind, 0.8);
  te1 = time_elapsed;

  srand(0);
  test_flight(test_num, n_wind, 1.2);
  te2 = time_elapsed;

  bool retval = (te0<=te1) && (te0<=te2);
  if (verbose || !retval) {
    printf("# sf 0.8 time_elapsed_rat %g\n",te1/te0);
    printf("# sf 1.2 time_elapsed_rat %g\n",te2/te0);
  }
  return retval;
}


bool test_cruise_efficiency(int test_num, int n_wind) 
{
  double ce0, ce1, ce2, ce3, ce4, ce5, ce6;

  bearing_noise = 0.0;
  target_noise = 0.1;

  srand(0);
  test_flight(test_num, n_wind);
  ce0 = cruise_efficiency;

  // wandering
  bearing_noise = 40.0;
  srand(0);
  test_flight(test_num, n_wind);
  ce1 = cruise_efficiency;
  ok (ce0>ce1, "ce wandering",0);

  // flying too slow
  bearing_noise = 0.0;
  srand(0);
  test_flight(test_num, n_wind, 0.8);
  ce2 = cruise_efficiency;
  ok (ce0>ce2, "ce speed slow",0);

  // flying too fast
  bearing_noise = 0.0;
  srand(0);
  test_flight(test_num, n_wind, 1.2);
  ce3 = cruise_efficiency;
  ok (ce0>ce3, "ce speed fast",0);

  // higher than expected cruise sink
  sink_factor = 1.2;
  srand(0);
  test_flight(test_num, n_wind);
  ce4 = cruise_efficiency;
  ok (ce0>ce4, "ce high sink",0);
  sink_factor = 1.0;

  // slower than expected climb
  climb_factor = 0.8;
  srand(0);
  test_flight(test_num, n_wind);
  ce5 = cruise_efficiency;
  ok (ce0>ce5, "ce slow climb",0);
  climb_factor = 1.0;

  // lower than expected cruise sink
  sink_factor = 0.8;
  srand(0);
  test_flight(test_num, n_wind);
  ce6 = cruise_efficiency;
  ok (ce0<ce6, "ce low sink",0);
  sink_factor = 1.0;

  bool retval = (ce0>ce1) && (ce0>ce2) && (ce0>ce3) && (ce0>ce4) && (ce0>ce5)
    && (ce0<ce6);
  if (verbose || !retval) {
    printf("# ce nominal %g\n",ce0);
    printf("# ce wandering %g\n",ce1);
    printf("# ce speed slow %g\n",ce2);
    printf("# ce speed fast %g\n",ce3);
    printf("# ce high sink %g\n",ce4);
    printf("# ce slow climb %g\n",ce5);
    printf("# ce low sink %g\n",ce6);
  }
  return retval;
}
