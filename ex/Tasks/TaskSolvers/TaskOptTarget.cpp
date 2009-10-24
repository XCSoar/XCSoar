#include "TaskOptTarget.hpp"
#include <math.h>
#include <stdio.h>


TaskOptTarget::TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AIRCRAFT_STATE &_aircraft,
                             AATPoint &_tp_current,
                             StartPoint *_ts):
  ZeroFinder(0.02,0.98,0.01),
  tm(tps,activeTaskPoint,1.0),
  aircraft(_aircraft),
  tp_current(_tp_current),
  tp_start(_ts),
  iso(_tp_current)
{

}

double TaskOptTarget::f(const double p) 
{
  // set task targets
  set_target(p);

  res = tm.glide_solution(aircraft);

//  printf(" - %g %g\n",p,res.TimeElapsed);
  return res.TimeElapsed;
}

bool TaskOptTarget::valid(const double tp) 
{
  f(tp);
  return (res.Solution== GLIDE_RESULT::RESULT_OK);
}

double TaskOptTarget::search(const double tp) 
{
  if (iso.valid()) {
    GEOPOINT loc = tp_current.getTargetLocation();
    double t = find_min(tp);
    if (!valid(t)) {
      // invalid, so restore old value
      tp_current.set_target(loc);
      return -1.0;
    } else {
//      printf("%g\n",t);
      return t;
    }
  } else {
    return -1;
  }
}

void TaskOptTarget::set_target(const double p)
{
  GEOPOINT loc = iso.parametric(std::min(xmax,std::max(xmin,p)));
//  printf("%g %g ", loc.Longitude, loc.Latitude);
  tp_current.set_target(loc);
  tp_start->scan_distance_remaining(aircraft.Location);
}
