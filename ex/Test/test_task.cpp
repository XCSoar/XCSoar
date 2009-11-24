#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "test_debug.hpp"

int main(int argc, char** argv)
{
  ::InitSineTable();

  if (!parse_args(argc,argv)) {
    return 0;
  }

  #define NUM_RANDOM 50

  plan_tests(NUM_TASKS+2+NUM_RANDOM+6);

  TaskBehaviour task_behaviour;
  TaskEvents default_events;
  GlidePolar glide_polar(2.0,0.0,0.0);

  Waypoints waypoints;
  setup_waypoints(waypoints);

  {
    TaskManager task_manager(default_events,
                             task_behaviour,
                             glide_polar,
                             waypoints);
    test_task_bad(task_manager,waypoints);
  }

  for (int i=0; i<NUM_TASKS+2; i++) {
    TaskManager task_manager(default_events,
                             task_behaviour,
                             glide_polar,
                             waypoints);
    ok(test_task(task_manager, waypoints, i),test_name("construction",i,0),0);
  }

  for (int i=0; i<NUM_RANDOM; i++) {
    TaskManager task_manager(default_events,
                             task_behaviour,
                             glide_polar,
                             waypoints);
    ok(test_task(task_manager, waypoints, 7),test_name("construction",7,0),0);
  }

  return exit_status();
}

