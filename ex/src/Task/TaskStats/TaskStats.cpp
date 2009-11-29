#include "TaskStats.hpp"
#include <assert.h>

TaskStats::TaskStats():
  Time(0.0),
  glide_required(0.0),
  cruise_efficiency(1.0),
  mc_best(0.0),
  distance_nominal(0.0),
  distance_max(0.0),
  distance_min(0.0),
  distance_scored(0.0)
{
  reset();
}

void
TaskStats::reset()
{
  total.reset();
  current_leg.reset();
}
