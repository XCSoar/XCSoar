#ifndef TASKBESTMC_HPP
#define TASKBESTMC_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "ZeroFinder.hpp"

class TaskBestMc: 
  public ZeroFinder
{
public:
  TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
             const unsigned activeTaskPoint,
             const AIRCRAFT_STATE &_aircraft):
    ZeroFinder(0.1,10.0,0.05),
    tm(tps,activeTaskPoint,1.0),
    aircraft(_aircraft) 
    {
    };
  virtual double f(double mc) {
    tm.set_mc(mc);
    res = tm.glide_solution(aircraft);
    // TODO: this fails if Mc too low for wind, need to 
    // account for failed solution
    return res.AltitudeDifference;
  }
  virtual bool valid(double mc) {
    tm.set_mc(mc);
    res = tm.glide_solution(aircraft);
    return (res.Solution== MacCready::RESULT_OK);
  }
  virtual double search(double mc) {
    double a = find_zero(mc);
    if (fabs(f(a))>tolerance*2.0) {
      return find_min(mc);
    } else {
      return a;
    }
  }
protected:
  TaskMacCreadyRemaining tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

