#include "CommonStats.hpp"

CommonStats::CommonStats():
  vector_home(0,0)
{
  reset();
}

void
CommonStats::reset()
{
  landable_reachable = false;
  task_finished = false;
  aat_time_remaining = fixed_zero;
  aat_remaining_speed = -fixed_one;
  task_time_remaining = fixed_zero;
  task_time_elapsed = fixed_zero;
}
