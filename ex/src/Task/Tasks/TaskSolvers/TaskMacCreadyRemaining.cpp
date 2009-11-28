#include <algorithm>
#include "TaskMacCreadyRemaining.hpp"

TaskMacCreadyRemaining::TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const GlidePolar _gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
  m_start = m_activeTaskPoint;
}

TaskMacCreadyRemaining::TaskMacCreadyRemaining(TaskPoint* tp,
                                               const GlidePolar _gp):
  TaskMacCready(tp,_gp)
{
}

GlideResult 
TaskMacCreadyRemaining::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    double minH) const
{
  return m_tps[i]->glide_solution_remaining(aircraft, m_glide_polar, minH);
}


const AIRCRAFT_STATE 
TaskMacCreadyRemaining::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  return aircraft;
}

void 
TaskMacCreadyRemaining::set_range(const double tp, const bool force_current)
{
  // first try to modify targets without regard to current inside (unless forced)
  bool modified = force_current;
  for (int i=m_start; i<=m_end; i++) {
    modified |= m_tps[i]->set_range(tp,false);
  }
  if (!force_current && !modified) {
    // couldn't modify remaining targets, so force move even if inside
    for (int i=m_start; i<=m_end; i++) {
      if (m_tps[i]->set_range(tp,true)) {
        // quick exit
        return;
      }
    }
  }
}


void 
TaskMacCreadyRemaining::target_save()
{
  for (int i=m_start; i<=m_end; i++) {
      m_tps[i]->target_save();
  }
}

void 
TaskMacCreadyRemaining::target_restore()
{
  for (int i=m_start; i<=m_end; i++) {
      m_tps[i]->target_restore();
  }
}
