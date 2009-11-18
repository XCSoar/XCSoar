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
  return true;
}
