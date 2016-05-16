#include "CommonStats.hpp"

void
CommonStats::ResetTask()
{
  start_open_time_span = RoughTimeSpan::Invalid();
  landable_reachable = false;
  TimeUnderStartMaxHeight = -1;
  aat_time_remaining = 0;
  aat_speed_target = -1;
  aat_speed_max = -1;
  aat_speed_min = -1;
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

  V_block = 0;
  V_dolphin = 0;

  current_risk_mc = 0;
  height_min_working = 0;
  height_max_working = 0;
  height_fraction_working = 1;

  vario_scale_positive = 0;
  vario_scale_negative = 0;

  ResetTask();
}
