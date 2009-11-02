#include "TaskStats.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

double lpf(const double y_this, const double y_last) {
  return y_this*0.1+y_last*0.9;
}

void DistanceRemainingStat::calc_speed(const ElementStat* es) 
{
  if (es->TimeRemaining>0) {
    speed = (distance/es->TimeRemaining);
  } else {
    speed = 0;
  }
}

void DistancePlannedStat::calc_speed(const ElementStat* es) 
{
  if (es->TimePlanned>0) {
    speed = (distance/es->TimePlanned);
  } else {
    speed = 0;
  }
}

void DistanceTravelledStat::calc_speed(const ElementStat* es) 
{
  if (es->TimeElapsed>0) {
    speed = (distance/es->TimeElapsed);
  } else {
    speed = 0;
  }
}

void DistanceStat::calc_incremental_speed(const double dt)
{  
  if (dt>0) {
    speed_incremental = lpf((distance_last-distance)/dt,speed_incremental);
  } else {
    speed_incremental = speed;
  }
  distance_last = distance;
}

void DistanceTravelledStat::calc_incremental_speed(const double dt)
{
  // negative of normal
  if (dt>0) {
    speed_incremental = lpf(-(distance_last-distance)/dt,speed_incremental);
  } else {
    speed_incremental = speed;
  }
  distance_last = distance;
}


void ElementStat::set_times(const double ts, 
                            const AIRCRAFT_STATE& state)
{
  TimeStarted = ts;
  TimeElapsed = std::max(state.Time-ts,0.0);
  TimeRemaining = solution_remaining.TimeElapsed;
  TimePlanned = TimeElapsed+TimeRemaining;
}

void
ElementStat::reset()
{
  initialised = false;
}

void ElementStat::calc_speeds(const double dt)
{
  remaining_effective.calc_speed(this);
  remaining.calc_speed(this);
  planned.calc_speed(this);
  travelled.calc_speed(this);

  if (!initialised || (TimeElapsed<60.0)) {
    initialised = true;
    remaining_effective.calc_incremental_speed(0.0);
    remaining.calc_incremental_speed(0.0);
    planned.calc_incremental_speed(0.0);
    travelled.calc_incremental_speed(0.0);
  } else if (dt>0) {
    remaining_effective.calc_incremental_speed(dt);
    remaining.calc_incremental_speed(dt);
    planned.calc_incremental_speed(dt);
    travelled.calc_incremental_speed(dt);
  }
}


void
TaskStats::reset()
{
  total.reset();
  current_leg.reset();
}
