#include <assert.h>
#ifdef DO_PRINT
#include <fstream>
#endif

#include "Math/FastMath.h"

#include "test_debug.hpp"
#include "test_aircraft.hpp"
#include "test_airspace.hpp"
#include "test_waypoints.hpp"
#include "test_task.hpp"

bool test_flight(TaskManager &task_manager,
                 Airspaces &airspaces,
                 GlidePolar &glide_polar,
                 int test_num) 
{
  AircraftSim ac(test_num, task_manager);
  unsigned print_counter=0;

#ifdef DO_PRINT
  std::ofstream f4("results/res-sample.txt");
#endif

  bool do_print;

  do {
    do_print = (print_counter++ % 1 ==0);

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

  test_task(waypoints);

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
    
  if (!setup_task(task_manager, waypoints)) {
    return false;
  }

  test_flight(task_manager, airspaces, glide_polar, test_num);

  return 0;
}

#ifndef NEWTASK
int main() {
  ::InitSineTable();
  test_all(2);
//  test_all(3);
}
#endif


