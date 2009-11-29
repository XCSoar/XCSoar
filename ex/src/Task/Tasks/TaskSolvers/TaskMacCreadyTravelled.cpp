#include "TaskMacCreadyTravelled.hpp"
#include "TaskSolution.hpp"

TaskMacCreadyTravelled::TaskMacCreadyTravelled(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const GlidePolar &_gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
  m_end = m_activeTaskPoint;
}


GlideResult 
TaskMacCreadyTravelled::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    double minH) const
{
  return TaskSolution::glide_solution_travelled(*m_tps[i],aircraft, m_glide_polar, minH);
}

const AIRCRAFT_STATE 
TaskMacCreadyTravelled::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  if (m_tps[0]->has_entered()) {
    return m_tps[0]->get_state_entered();
  } else {
    return aircraft;
  }
}


