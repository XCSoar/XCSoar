#include "FAITaskFactory.hpp"

FAITaskFactory::FAITaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  start_types.push_back(START_SECTOR);
  start_types.push_back(START_CYLINDER);
  intermediate_types.push_back(FAI_SECTOR);
  intermediate_types.push_back(AST_CYLINDER);
  finish_types.push_back(FINISH_SECTOR);
  finish_types.push_back(FINISH_CYLINDER);
}

bool 
FAITaskFactory::validate()
{
  /**
   * \todo
   * - do checks for closure and triangle leg distances etc
   * - adjustment to finish height if FAI finish height is on
   */

  if (!task.has_start_and_finish()) {
    return false;
  }

  if (task.task_size()==4) {

    // start/finish must be co-located
    if (! (task.getTaskPoint(0)->get_waypoint() == 
           task.getTaskPoint(3)->get_waypoint())) {
      return false;
    }

    const double d1 = task.getTaskPoint(1)->get_vector_planned().Distance/1000.0;
    const double d2 = task.getTaskPoint(2)->get_vector_planned().Distance/1000.0;
    const double d3 = task.getTaskPoint(3)->get_vector_planned().Distance/1000.0;
    const double d_wp = d1+d2+d3;

    // From kflog
    if( ( d_wp < 500.0 ) &&
        ( d1 >= 0.28 * d_wp && d2 >= 0.28 * d_wp && d3 >= 0.28 * d_wp ) )
      // small FAI
      return true;
    else if( d_wp >= 500.0 &&
             ( d1 > 0.25 * d_wp && d2 > 0.25 * d_wp && d3 > 0.25 * d_wp ) &&
             ( d1 <= 0.45 * d_wp && d2 <= 0.45 * d_wp && d3 <= 0.45 * d_wp ) )
      // large FAI
      return true;

    return false;
  }

  // unknown task...
  return true;
}
