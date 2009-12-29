#include "CommonStats.hpp"
#include "Waypoint/Waypoint.hpp"

CommonStats::CommonStats():
  vector_home(fixed_zero, fixed_zero)
{
  reset();
}

void
CommonStats::reset()
{
  landable_reachable = false;
  task_finished = false;
  aat_time_remaining = fixed_zero;
  aat_speed_remaining = -fixed_one;
  aat_speed_max = -fixed_one;
  aat_speed_min = -fixed_one;
  task_time_remaining = fixed_zero;
  task_time_elapsed = fixed_zero;
  mode_abort = false;
  mode_goto = false;
  mode_ordered = false;
  ordered_valid = false;

  active_has_next = false;
  active_has_previous = false; 
  next_is_last = false;
  previous_is_first = false;

  V_block = fixed_zero;
  V_dolphin = fixed_zero;

  current_mc = fixed_zero;
  current_risk_mc = fixed_zero;
  current_bugs = fixed_one;
  current_ballast = fixed_zero;

  distance_olc = fixed_zero;

  clear_waypoints_in_task();
}

void
CommonStats::clear_waypoints_in_task()
{
  waypoints_in_task.clear();
}

void
CommonStats::append_waypoint_in_task(const Waypoint& wp)
{
  waypoints_in_task.insert(wp.id);
}

bool
CommonStats::is_waypoint_in_task(const Waypoint& wp) const
{
  return waypoints_in_task.find(wp.id) != waypoints_in_task.end();
}
