#include "TaskStats.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>


DistanceStat::DistanceStat():
  distance(0.0),
  distance_last(0.0),
  speed(0.0),
  counter(0),
  lpf(60.0)
{

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
    if (counter++ % 5 == 0) {
      double d = lpf.update(distance);
      double v = (distance_last-d)/(5*dt);
      distance_last = d;
      speed_incremental = v;
    }
  } else {
    distance_last = lpf.reset(distance);
    speed_incremental = speed;
    counter=0;
  }
}

void DistanceTravelledStat::calc_incremental_speed(const double dt)
{
  // negative of normal
  if (dt>0) {
    if (counter++ % 5 == 0) {
      double d = lpf.update(distance);
      double v = (d-distance_last)/(5*dt);
      distance_last = d;
      speed_incremental = v;
    }
  } else {
    distance_last = lpf.reset(distance);
    speed_incremental = speed;
    counter=0;
  }
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
