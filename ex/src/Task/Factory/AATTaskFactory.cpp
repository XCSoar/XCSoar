#include "AATTaskFactory.hpp"

AATTaskFactory::AATTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  start_types.push_back(START_CYLINDER);
  intermediate_types.push_back(AAT_CYLINDER);
  intermediate_types.push_back(AAT_SEGMENT);
  finish_types.push_back(FINISH_LINE);
}

bool 
AATTaskFactory::validate()
{
  /**
   * \todo
   * maybe warn on overlap?
   */

  if (!task.has_start_and_finish()) {
    return false;
  }

  return true;
}
