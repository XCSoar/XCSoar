#include "AATTaskFactory.hpp"

AATTaskFactory::AATTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  m_start_types.push_back(START_CYLINDER);
  m_intermediate_types.push_back(AAT_CYLINDER);
  m_intermediate_types.push_back(AAT_SEGMENT);
  m_finish_types.push_back(FINISH_LINE);
}

bool 
AATTaskFactory::validate()
{
  /**
   * \todo
   * maybe warn on overlap?
   */

  if (!m_task.has_start() || !m_task.has_finish()) {
    return false;
  }
  if (m_task.task_size()<3) {
    // not enough turnpoints!
    return false;
  }

  return true;
}
