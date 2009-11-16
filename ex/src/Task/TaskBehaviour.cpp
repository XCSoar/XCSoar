#include "Navigation/Aircraft.hpp"
#include "TaskBehaviour.hpp"

void
TaskBehaviour::all_off()
{
  optimise_targets_range=false;
  optimise_targets_bearing=false;
  auto_mc=false;
  calc_cruise_efficiency=false;
}

bool 
TaskBehaviour::check_start_speed(const AIRCRAFT_STATE &state) const
{
  if ((start_max_speed>0) && (state.Speed>start_max_speed)) {
    return false;
  } else {
    return true;
  }
}

TaskBehaviour::TaskBehaviour():
    optimise_targets_range(true),
    optimise_targets_bearing(true),
    auto_mc(true),
    calc_cruise_efficiency(true),
    aat_min_time(3600*4.9),
    safety_height_terrain(150.0),
    safety_height_arrival(300.0),
    start_max_speed(30.0)
{
}

