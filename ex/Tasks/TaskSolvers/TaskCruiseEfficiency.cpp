#include "TaskCruiseEfficiency.hpp"

TaskCruiseEfficiency::TaskCruiseEfficiency(const std::vector<OrderedTaskPoint*>& tps,
                                           const unsigned activeTaskPoint,
                                           const AIRCRAFT_STATE &_aircraft,
                                           const double mc):
  ZeroFinder(0.1,2.0,0.01),
  tm(tps,activeTaskPoint,mc),
  aircraft(_aircraft) 
{
  dt = aircraft.Time-tps[0]->get_state_entered().Time;
}

double TaskCruiseEfficiency::f(const double ce) 
{
  tm.set_cruise_efficiency(ce);
  res = tm.glide_solution(aircraft);
  double d = fabs(res.TimeElapsed-dt);
  if (!res.Solution==GLIDE_RESULT::RESULT_OK) {
    d += res.TimeVirtual;
  }
  if (dt>0) {
    d/= dt;
  }
  return d;
}

bool TaskCruiseEfficiency::valid(const double ce) 
{
  tm.set_cruise_efficiency(ce);
  res = tm.glide_solution(aircraft);
  return (res.Solution== GLIDE_RESULT::RESULT_OK);
}

double TaskCruiseEfficiency::search(const double ce) 
{
  return find_min(ce);
}
