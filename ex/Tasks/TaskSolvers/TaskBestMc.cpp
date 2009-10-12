#include "TaskBestMc.hpp"
#include <math.h>
#include <stdio.h>


TaskBestMc::TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AIRCRAFT_STATE &_aircraft):
  ZeroFinder(0.1,10.0,0.001),
  tm(tps,activeTaskPoint,1.0),
  aircraft(_aircraft) 
{
};

double TaskBestMc::f(const double mc) 
{
  tm.set_mc(mc);
  res = tm.glide_solution(aircraft);
  // TODO: this fails if Mc too low for wind, need to 
  // account for failed solution
  // && (fabs(res.AltitudeDifference)<tolerance)
  if (res.Distance>0) {
    return res.AltitudeDifference/res.Distance;
  } else {
    return res.AltitudeDifference;
  }
}

bool TaskBestMc::valid(const double mc) 
{
  double ff = f(mc);
  return (res.Solution== MacCready::RESULT_OK) && (ff>=-tolerance*2.0);
}

double TaskBestMc::search(const double mc) 
{
  double a = find_zero(mc);
  if (!valid(a)) {
    return mc;
  } 
  return a;
}
