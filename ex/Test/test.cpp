#include <assert.h>
#ifdef DO_PRINT
#include <fstream>
#endif

#include "Math/FastMath.h"

#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "harness_airspace.hpp"
#include "harness_waypoints.hpp"
#include "harness_task.hpp"

bool test_flight(TaskManager &task_manager,
                 Airspaces &airspaces,
                 GlidePolar &glide_polar,
                 int test_num,
                 bool goto_target) 
{
  AircraftSim ac(test_num, task_manager, 1.0, goto_target);
  unsigned print_counter=0;

#ifdef DO_PRINT
  std::ofstream f4("results/res-sample.txt");
#endif

  bool do_print;

  do {
    do_print = (print_counter++ % output_skip ==0);

    if (do_print) {
      // task_manager.Accept(tv);
#ifdef DO_PRINT
      task_manager.print(ac.get_state());
      ac.print(f4);
      f4.flush();
#endif
    }
    n_samples++;

    scan_airspaces(ac.get_state(), airspaces, do_print, 
                   ac.get_next());

  } while (ac.advance(task_manager, glide_polar));

  distance_counts();
  return true;
}


bool test_all(int test_num) {

  //// aircraft
  GlidePolar glide_polar(2.0,0.0,0.0);

  ////////////////////////// Waypoints //////
  Waypoints waypoints;
  setup_waypoints(waypoints);

  ////////////////////////// AIRSPACES //////
  Airspaces airspaces;
  setup_airspaces(airspaces);

  ////////////////////////// TASK //////

  distance_counts();

  TaskBehaviour task_behaviour;

  if (test_num==0) {
    return 0;
  }
  if (test_num==3) {
    task_behaviour.all_off();
  }

  TaskEvents default_events;

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  test_task_fai(task_manager, waypoints);
  test_flight(task_manager, airspaces, glide_polar, test_num, true);

  return 0;
}

int main(int argc, char** argv) {
  ::InitSineTable();

  if (!parse_args(argc,argv)) {
    return 0;
  }
  test_all(2);
}

