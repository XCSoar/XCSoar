#include "TaskGlideRequired.hpp"
#include <math.h>
#include <stdio.h>


TaskGlideRequired::TaskGlideRequired(const std::vector<OrderedTaskPoint*>& tps,
                                     const unsigned activeTaskPoint,
                                     const AIRCRAFT_STATE &_aircraft,
                                     const GlidePolar &_gp):
  ZeroFinder(-10.0,10.0,0.001),
  tm(tps,activeTaskPoint,_gp), 
  aircraft(_aircraft) 
{
  // Vopt at mc=0
  tm.set_mc(0.0);
}

TaskGlideRequired::TaskGlideRequired(TaskPoint* tp,
                                     const AIRCRAFT_STATE &_aircraft,
                                     const GlidePolar &_gp):
  ZeroFinder(-10.0,10.0,0.001),
  tm(tp,_gp), // Vopt at mc=0
  aircraft(_aircraft) 
{
  tm.set_mc(0.0);
}

double TaskGlideRequired::f(const double S) 
{
  res = tm.glide_sink(aircraft, S);
  // TODO: this fails if Mc too low for wind, need to 
  // account for failed solution
  // && (fabs(res.AltitudeDifference)<tolerance)
  if (res.Vector.Distance>0) {
    return res.AltitudeDifference/res.Vector.Distance;
  } else {
    return res.AltitudeDifference;
  }
}

bool TaskGlideRequired::valid(const double S) 
{
  double ff = f(S);
  return (res.Solution== GLIDE_RESULT::RESULT_OK) && (fabs(ff)>=-tolerance*2.0);
}

double TaskGlideRequired::search(const double S) 
{
  double a = find_zero(S);
  return a/res.VOpt;
}
