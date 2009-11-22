#include "MixedTaskFactory.hpp"

MixedTaskFactory::MixedTaskFactory(OrderedTask& _task,
                                   const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  start_types.push_back(START_SECTOR);
  start_types.push_back(START_LINE);
  start_types.push_back(START_CYLINDER);
  intermediate_types.push_back(FAI_SECTOR);
  intermediate_types.push_back(AST_CYLINDER);
  intermediate_types.push_back(AAT_CYLINDER);
  intermediate_types.push_back(AAT_SEGMENT);
  finish_types.push_back(FINISH_SECTOR);
  finish_types.push_back(FINISH_LINE);
  finish_types.push_back(FINISH_CYLINDER);
}

bool 
MixedTaskFactory::validate()
{
  /**
   * \todo
   * maybe warn on overlap?
   */

  if (!task.has_start() || !task.has_finish()) {
    return false;
  }

  return true;
}
