#ifdef DO_PRINT
#include <fstream>
#endif

#include "Math/FastMath.h"
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
                int n_wind) 
{
  AircraftSim ac(0, task_manager, random_mag, goto_target);
  unsigned print_counter=0;

  ac.set_wind(n_wind*5.0,0.0);

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
    n_samples++;

    do_print = (print_counter++ % output_skip ==0) && verbose;

  } while (ac.advance(task_manager, glide_polar));

  task_manager.print(ac.get_state());
  ac.print(f4);
  f4.flush();

  double time_elapsed = task_manager.get_stats().total.TimeElapsed;
  double time_planned = task_manager.get_stats().total.TimePlanned;

  bool time_ok = fabs(time_elapsed/time_planned-1.0)<0.02;
  if ((verbose>1) || !time_ok) {
    printf("# time remaining %g\n", time_remaining);
    printf("# time elapsed %g\n", time_elapsed);
    printf("# time planned %g\n", time_planned);
  }
  if (verbose) {
    distance_counts();
  }
  return time_ok;
}


bool test_flight(int test_num, int n_wind) {

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

  return run_flight(task_manager, glide_polar, test_num, goto_target, target_noise, n_wind);
}

int main(int argc, char** argv) 
{
  ::InitSineTable();

  if (!parse_args(argc,argv)) {
    return 0;
  }

  int n_wind = 3;

  plan_tests(5*n_wind);

  for (int i=0; i<n_wind; i++) {
    ok (test_flight(3,i), "flight or",0);
    ok (test_flight(4,i), "flight dash",0);
    ok (test_flight(1,i), "flight fai",0);
    ok (test_flight(2,i), "flight aat",0);
    ok (test_flight(0,i), "flight mixed",0);
  }
  return exit_status();
}


