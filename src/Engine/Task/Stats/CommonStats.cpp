#include "CommonStats.hpp"

void
CommonStats::ResetTask()
{
  start_open_time_span = RoughTimeSpan::Invalid();
  landable_reachable = false;
  TimeUnderStartMaxHeight = fixed(-1);
  aat_time_remaining = fixed(0);
  aat_speed_remaining = fixed(-1);
  aat_speed_max = fixed(-1);
  aat_speed_min = fixed(-1);
  task_type = TaskType::NONE;

  active_has_next = false;
  active_has_previous = false;
  next_is_last = false;
  previous_is_first = false;
  ordered_summary.clear();
}

void
CommonStats::Reset()
{
  vector_home.SetInvalid();

  V_block = fixed(0);
  V_dolphin = fixed(0);

  current_risk_mc = fixed(0);

  ResetTask();
}
