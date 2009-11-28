#include "FAITaskFactory.hpp"

FAITaskFactory::FAITaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  m_start_types.push_back(START_SECTOR);
  m_start_types.push_back(START_CYLINDER);
  m_intermediate_types.push_back(FAI_SECTOR);
  m_intermediate_types.push_back(AST_CYLINDER);
  m_finish_types.push_back(FINISH_SECTOR);
  m_finish_types.push_back(FINISH_CYLINDER);
}

bool 
FAITaskFactory::validate()
{
  /**
   * \todo
   * - adjustment to finish height if FAI finish height is on
   */

  if (!m_task.has_start() || !m_task.has_finish()) {
    return false;
  }

  if (m_task.task_size()==4) {

    // start/finish must be co-located
    if (! (m_task.getTaskPoint(0)->get_waypoint() == 
           m_task.getTaskPoint(3)->get_waypoint())) {
      return false;
    }

    const double d1 = m_task.getTaskPoint(1)->get_vector_planned().Distance/1000.0;
    const double d2 = m_task.getTaskPoint(2)->get_vector_planned().Distance/1000.0;
    const double d3 = m_task.getTaskPoint(3)->get_vector_planned().Distance/1000.0;
    const double d_wp = d1+d2+d3;


    /**
     * From kflog:
     * A triangle is a valig FAI-triangle, if no side is less than
     * 28% of the total length (totallength less than 500 km), or no
     * side is less than 25% or larger than 45% of the total length
     * (totallength >= 500km).
     */
 
    if( ( d_wp < 500.0 ) &&
        ( d1 >= 0.28 * d_wp && d2 >= 0.28 * d_wp && d3 >= 0.28 * d_wp ) )
      // small FAI
      return true;
    else if( d_wp >= 500.0 &&
             ( d1 > 0.25 * d_wp && d2 > 0.25 * d_wp && d3 > 0.25 * d_wp ) &&
             ( d1 <= 0.45 * d_wp && d2 <= 0.45 * d_wp && d3 <= 0.45 * d_wp ) )
      // large FAI
      return true;

    // distances out of limits
    return false;
  }

  // unknown task...
  return true;
}
