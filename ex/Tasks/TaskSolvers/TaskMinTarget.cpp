#include "TaskMinTarget.hpp"
#include <math.h>
#include <stdio.h>


TaskMinTarget::TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AIRCRAFT_STATE &_aircraft,
                             const double _t_remaining):
  ZeroFinder(0.0,1.0,0.0025),
  tm(tps,activeTaskPoint,1.0),
  aircraft(_aircraft),
  t_remaining(_t_remaining)
{

};

double TaskMinTarget::f(const double tp) 
{
  // set task targets
  tm.set_range(tp);

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
