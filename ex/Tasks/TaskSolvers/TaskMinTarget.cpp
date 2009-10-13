#include "TaskMinTarget.hpp"
#include <math.h>
#include <stdio.h>


TaskMinTarget::TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AIRCRAFT_STATE &_aircraft,
                             const double _t_remaining,
                             StartPoint *_ts):
  ZeroFinder(0.0,1.0,0.0025),
  tm(tps,activeTaskPoint,1.0),
  aircraft(_aircraft),
  t_remaining(_t_remaining),
  tp_start(_ts)
{

};

double TaskMinTarget::f(const double p) 
{
  // set task targets
  set_range(p);

  res = tm.glide_solution(aircraft);
  const double dt = res.TimeElapsed-t_remaining; 
  if (t_remaining>0) {
    return dt/t_remaining;
  } else {
    return dt;
  }
}

bool TaskMinTarget::valid(const double tp) 
{
  double ff = f(tp);
  return (res.Solution== MacCready::RESULT_OK) && (ff>=-tolerance*2.0);
}

double TaskMinTarget::search(const double tp) 
{
  return find_zero(tp);
}

void TaskMinTarget::set_range(const double p)
{
  // TODO: have various schemes for feeding in p dependent on number of stages
  // from current

  tm.set_range(p);
  tp_start->scan_distance_remaining(aircraft.Location);
}
