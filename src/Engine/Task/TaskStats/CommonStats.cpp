#include "CommonStats.hpp"

void
CommonStats::reset_task()
{
  landable_reachable = false;
  task_started = false;
  TimeUnderStartMaxHeight = -fixed_one;
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
  ordered_has_targets = false;
  ordered_has_optional_starts = false;

  active_has_next = false;
  active_has_previous = false;
  next_is_last = false;
  previous_is_first = false;
  next_solution.Reset();
  ordered_summary.clear();
}

void
CommonStats::reset()
{
  vector_home.SetInvalid();

  V_block = fixed_zero;
  V_dolphin = fixed_zero;

  current_mc = fixed_zero;
  current_risk_mc = fixed_zero;
  current_bugs = fixed_one;
  current_ballast = fixed_zero;

  reset_task();
}
