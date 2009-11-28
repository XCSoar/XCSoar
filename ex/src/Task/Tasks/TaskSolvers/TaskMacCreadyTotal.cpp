#include "TaskMacCreadyTotal.hpp"

TaskMacCreadyTotal::TaskMacCreadyTotal(const std::vector<OrderedTaskPoint*> &_tps,
                                       const unsigned _activeTaskPoint,
                                       const GlidePolar &_gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
}


GlideResult 
TaskMacCreadyTotal::tp_solution(const unsigned i,
                                const AIRCRAFT_STATE &aircraft, 
                                double minH) const
{
  return m_tps[i]->glide_solution_planned(aircraft, m_glide_polar, minH);
}

const AIRCRAFT_STATE 
TaskMacCreadyTotal::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  if (m_tps[0]->has_entered()) {
    return m_tps[0]->get_state_entered();
  } else {
    return aircraft;
  }
}

double 
TaskMacCreadyTotal::effective_distance(const double time_remaining) const
{

  double t_total = 0.0;
  double d_total = 0.0;
  for (int i=m_end; i>=m_start; i--) {
    if (m_gs[i].TimeElapsed>0) {
      double p = (time_remaining-t_total)/m_gs[i].TimeElapsed;
      if ((p>=0.0) && (p<=1.0)) {
        return d_total+p*m_gs[i].Vector.Distance;
      }
      d_total += m_gs[i].Vector.Distance;
      t_total += m_gs[i].TimeElapsed;
    }
  }
  return d_total;
}

double 
TaskMacCreadyTotal::effective_leg_distance(const double time_remaining) const
{
  double p = (time_remaining)/m_gs[m_activeTaskPoint].TimeElapsed;
  return p*m_gs[m_activeTaskPoint].Vector.Distance;
}

