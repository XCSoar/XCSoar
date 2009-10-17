#include "TaskGlideRequired.hpp"
#include <math.h>
#include <stdio.h>


TaskGlideRequired::TaskGlideRequired(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AIRCRAFT_STATE &_aircraft):
  ZeroFinder(-10.0,10.0,0.001),
  tm(tps,activeTaskPoint,0.0), // Vopt at mc=0
  aircraft(_aircraft) 
{
}

TaskGlideRequired::TaskGlideRequired(TaskPoint* tp,
                                     const AIRCRAFT_STATE &_aircraft):
  ZeroFinder(-10.0,10.0,0.001),
  tm(tp,0.0), // Vopt at mc=0
  aircraft(_aircraft) 
{

}

double TaskGlideRequired::f(const double S) 
{
  res = tm.glide_sink(aircraft, S);
  // TODO: this fails if Mc too low for wind, need to 
  // account for failed solution
  // && (fabs(res.AltitudeDifference)<tolerance)
  if (res.Distance>0) {
    return res.AltitudeDifference/res.Distance;
  } else {
    return res.AltitudeDifference;
  }
}

bool TaskGlideRequired::valid(const double S) 
{
  double ff = f(S);
  return (res.Solution== MacCready::RESULT_OK) && (fabs(ff)>=-tolerance*2.0);
}

double TaskGlideRequired::search(const double S) 
{
  double a = find_zero(S);
  return a;
}
