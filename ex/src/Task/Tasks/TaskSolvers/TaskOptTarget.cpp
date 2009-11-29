#include "TaskOptTarget.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"

TaskOptTarget::TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AIRCRAFT_STATE &_aircraft,
                             const GlidePolar &_gp,
                             AATPoint &_tp_current,
                             StartPoint *_ts):
  ZeroFinder(0.02,0.98,TOLERANCE_OPT_TARGET),
  tm(tps,activeTaskPoint,_gp),
  aircraft(_aircraft),
  tp_start(_ts),
  tp_current(_tp_current),
  iso(_tp_current)
{

}

double 
TaskOptTarget::f(const double p) 
{
  // set task targets
  set_target(p);

  res = tm.glide_solution(aircraft);

  return res.TimeElapsed;
}

bool 
TaskOptTarget::valid(const double tp) 
{
  f(tp);
  return (res.Solution== GlideResult::RESULT_OK);
}

double 
TaskOptTarget::search(const double tp) 
{
  if (iso.valid()) {
      tm.target_save();
    const double t = find_min(tp);
    if (!valid(t)) {
      // invalid, so restore old value
      tm.target_restore();
      return -1.0;
    } else {
      return t;
    }
  } else {
    return -1;
  }
}

void 
TaskOptTarget::set_target(const double p)
{
  const GEOPOINT loc = iso.parametric(std::min(xmax,std::max(xmin,p)));
  tp_current.set_target(loc);
  tp_start->scan_distance_remaining(aircraft.Location);
}
