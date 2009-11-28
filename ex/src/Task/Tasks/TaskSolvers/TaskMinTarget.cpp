#include "TaskMinTarget.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"


TaskMinTarget::TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AIRCRAFT_STATE &_aircraft,
                             const GlidePolar &_gp,
                             const double _t_remaining,
                             StartPoint *_ts):
  ZeroFinder(0.0,1.0, TOLERANCE_MIN_TARGET),
  tm(tps,activeTaskPoint,_gp),
  aircraft(_aircraft),
  t_remaining(_t_remaining),
  tp_start(_ts),
  force_current(false)
{

}

double 
TaskMinTarget::f(const double p) 
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

bool 
TaskMinTarget::valid(const double tp) 
{
  const double ff = f(tp);
  return (res.Solution== GlideResult::RESULT_OK) && (ff>= -tolerance*2.0);
}

double 
TaskMinTarget::search(const double tp) 
{
  force_current = false;
  /// \todo if search fails, force current
  const double p = find_zero(tp);
  if (valid(p)) {
    return p;
  } else {
    force_current = true;
    return find_zero(tp);
  }
}

void 
TaskMinTarget::set_range(const double p)
{
  tm.set_range(p, force_current);
  tp_start->scan_distance_remaining(aircraft.Location);
}
