#include "TaskStats.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

#define N_AV 6

DistanceStat::DistanceStat():
  distance(0.0),
  speed(0.0),
  av_dist(N_AV),
  dist_lpf(60.0,false),
  df(0.0),
  v_lpf(100.0,false)
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
  /// \todo handle case where dt>1
  if (dt>0) {
    if (av_dist.update(distance)) {
      speed_incremental = 
        -v_lpf.update(
          df.update(
            dist_lpf.update(
              av_dist.average()
              )
            )/(N_AV*dt));
      av_dist.reset();
    }
  } else {
    df.reset(dist_lpf.reset(distance));
    v_lpf.reset(speed);
    speed_incremental = speed;
    av_dist.reset();
  }
}

void DistanceTravelledStat::calc_incremental_speed(const double dt)
{
  /// \todo handle case where dt>1

  // negative of normal
  if (dt>0) {
    if (av_dist.update(distance)) {
      speed_incremental = 
        v_lpf.update(
          df.update(
            dist_lpf.update(
              av_dist.average()
              )
            )/(N_AV*dt));
      av_dist.reset();
    }
  } else {
    df.reset(dist_lpf.reset(distance));
    v_lpf.reset(speed);
    speed_incremental = speed;
    av_dist.reset();
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
