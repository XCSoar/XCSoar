#include "Tasks/TaskStats.hpp"
#include "Navigation/Aircraft.hpp"


void DistanceRemainingStat::calc_speed(const ElementStat* es) 
{
  if (es->TimeRemaining>0) {
    speed = distance/es->TimeRemaining;
  } else {
    speed = 0;
  }
}

void DistancePlannedStat::calc_speed(const ElementStat* es) 
{
  if (es->TimePlanned>0) {
    speed = distance/es->TimePlanned;
  } else {
    speed = 0;
  }
}

void DistanceTravelledStat::calc_speed(const ElementStat* es) 
{
  if (es->TimeElapsed>0) {
    speed = distance/es->TimeElapsed;
  } else {
    speed = 0;
  }
}

void DistanceStat::calc_incremental_speed(const double dt)
{
  speed_incremental = (distance_last-distance)/dt;
  distance_last = distance;
}

void DistanceTravelledStat::calc_incremental_speed(const double dt)
{
  // negative of normal
  speed_incremental = (distance-distance_last)/dt;
  distance_last = distance;
}


void DistanceStat::print(std::ostream &f) const
{
  f << "#    Distance " << distance << "\n";
  f << "#    Speed " << speed << "\n";
  f << "#    Speed incremental " << speed_incremental << "\n";
}


void ElementStat::set_times(const double ts, 
                            const AIRCRAFT_STATE& state,
                            const double dt)
{
  TimeStarted = ts;
  TimeElapsed = std::max(state.Time-ts,0.0);
  TimeRemaining = solution_remaining.TimeElapsed;
  TimePlanned = TimeElapsed+TimeRemaining;
  
  remaining_effective.calc_speed(this);
  remaining.calc_speed(this);
  planned.calc_speed(this);
  travelled.calc_speed(this);
  
  if (dt>0) {
    remaining_effective.calc_incremental_speed(dt);
    remaining.calc_incremental_speed(dt);
    planned.calc_incremental_speed(dt);
    travelled.calc_incremental_speed(dt);
  }
}

void ElementStat::print(std::ostream &f) const
{
  f << "#  Time started " << TimeStarted << "\n";
  f << "#  Time elapsed " << TimeElapsed << "\n";
  f << "#  Time remaining " << TimeRemaining << "\n";
  f << "#  Time planned " << TimePlanned << "\n";
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
  f << "# Total -- \n";
  total.print(f);
  f << "# Leg -- \n";
  current_leg.print(f);
}
