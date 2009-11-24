#ifndef AAT_TASK_FACTORY_HPP
#define AAT_TASK_FACTORY_HPP

#include "AbstractTaskFactory.hpp"

/**
 * Factory for construction of legal AAT tasks
 * Currently the validate() method simply checks that there is at least one
 * AAT turnpoint.
 */
class AATTaskFactory: public AbstractTaskFactory 
{
public:
  AATTaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

  bool validate();
};

#endif
