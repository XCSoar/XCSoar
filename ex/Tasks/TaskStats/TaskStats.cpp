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
  }
  distance_last = distance;
}

void DistanceTravelledStat::calc_incremental_speed(const double dt)
{
  // negative of normal
  if (dt>0) {
    speed_incremental = lpf(-(distance_last-distance)/dt,speed_incremental);
  }
  distance_last = distance;
}


void DistanceStat::print(std::ostream &f) const
{
  f << "#    Distance " << distance << " (m)\n";
  f << "#    Speed " << speed << " (m/s)\n";
  f << "#    Speed incremental " << speed_incremental << " (m/s)\n";
}


void ElementStat::set_times(const double ts, 
                            const AIRCRAFT_STATE& state)
{
  TimeStarted = ts;
  TimeElapsed = std::max(state.Time-ts,0.0);
  TimeRemaining = solution_remaining.TimeElapsed;
  TimePlanned = TimeElapsed+TimeRemaining;
}

void ElementStat::calc_speeds(const double dt)
{
  remaining_effective.calc_speed(this);
  remaining.calc_speed(this);
  planned.calc_speed(this);
  travelled.calc_speed(this);

  if (!initialised) {
    initialised = true;
    remaining_effective.calc_incremental_speed(0.0);
    remaining.calc_incremental_speed(0.0);
    planned.calc_incremental_speed(0.0);
    travelled.calc_incremental_speed(0.0);
  } else {
    remaining_effective.calc_incremental_speed(dt);
    remaining.calc_incremental_speed(dt);
    planned.calc_incremental_speed(dt);
    travelled.calc_incremental_speed(dt);
  }
}


void ElementStat::print(std::ostream &f) const
{
  f << "#  Time started " << TimeStarted << " (s)\n";
  f << "#  Time elapsed " << TimeElapsed << " (s)\n";
  f << "#  Time remaining " << TimeRemaining << " (s)\n";
  f << "#  Time planned " << TimePlanned << " (s)\n";
  f << "#  Remaining: \n";
  remaining.print(f);
  solution_remaining.print(f);
  f << "#  Remaining effective: \n";
  remaining_effective.print(f);
  f << "#  Planned: \n";
  planned.print(f);
  solution_planned.print(f);
  f << "#  Travelled: \n";
  travelled.print(f);
  solution_travelled.print(f);
}

void TaskStats::print(std::ostream &f) const
{
  f << "#### Task Stats\n";
  f << "# dist nominal " << distance_nominal << " (m)\n";
  f << "# min dist after achieving max " << distance_min << " (?)\n";
  f << "# max dist after achieving max " << distance_max << " (?)\n";
  f << "# dist scored " << distance_scored << " (m)\n";
  f << "# mc best " << mc_best << " (m/s)\n";
  f << "# cruise efficiency " << cruise_efficiency << "\n";
  f << "# glide required " << glide_required << "\n";
  f << "#\n";
  f << "# Total -- \n";
  total.print(f);
  f << "# Leg -- \n";
  current_leg.print(f);
}
