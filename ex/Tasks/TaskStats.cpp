#include "Tasks/TaskStats.hpp"


void DistanceRemainingStat::calc_speed(const ElementStat* es) {
  if (es->TimeRemaining>0) {
    speed = distance/es->TimeRemaining;
  } else {
    speed = 0;
  }
}

void DistancePlannedStat::calc_speed(const ElementStat* es) {
  if (es->TimePlanned>0) {
    speed = distance/es->TimePlanned;
  } else {
    speed = 0;
  }
}

void DistanceTravelledStat::calc_speed(const ElementStat* es) {
  if (es->TimeElapsed>0) {
    speed = distance/es->TimeElapsed;
  } else {
    speed = 0;
  }
}

void DistanceStat::print(std::ostream &f)
{
  f << "#    Distance " << distance << "\n";
  f << "#    Speed " << speed << "\n";
}


void ElementStat::print(std::ostream &f) 
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

void TaskStats::print(std::ostream &f) 
{
  f << "# Total -- \n";
  total.print(f);
  f << "# Leg -- \n";
  current_leg.print(f);
}
