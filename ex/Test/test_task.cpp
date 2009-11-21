#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "test_debug.hpp"

int main()
{
  interactive = false;

  plan_tests(3);

  TaskBehaviour task_behaviour;
  TaskEvents default_events;
  GlidePolar glide_polar(2.0,0.0,0.0);

  Waypoints waypoints;
  setup_waypoints(waypoints);

  TaskManager task_manager1(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  ok (test_task_mixed(task_manager1, waypoints),"mixed task",0);

  TaskManager task_manager2(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  ok (test_task_fai(task_manager2, waypoints),"fai task construction",0);

  TaskManager task_manager3(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);

  ok (test_task_aat(task_manager3, waypoints),"aat task construction",0);

  return exit_status();
}

