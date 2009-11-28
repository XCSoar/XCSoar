#include "MixedTaskFactory.hpp"

MixedTaskFactory::MixedTaskFactory(OrderedTask& _task,
                                   const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  m_start_types.push_back(START_SECTOR);
  m_start_types.push_back(START_LINE);
  m_start_types.push_back(START_CYLINDER);
  m_intermediate_types.push_back(FAI_SECTOR);
  m_intermediate_types.push_back(AST_CYLINDER);
  m_intermediate_types.push_back(AAT_CYLINDER);
  m_intermediate_types.push_back(AAT_SEGMENT);
  m_finish_types.push_back(FINISH_SECTOR);
  m_finish_types.push_back(FINISH_LINE);
  m_finish_types.push_back(FINISH_CYLINDER);
}

bool 
MixedTaskFactory::validate()
{
  /**
   * \todo
   * maybe warn on overlap?
   */

  if (!m_task.has_start() || !m_task.has_finish()) {
    return false;
  }

  return true;
}
