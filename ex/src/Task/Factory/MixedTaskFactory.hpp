#ifndef MIXED_TASK_FACTORY_HPP
#define MIXED_TASK_FACTORY_HPP

#include "AbstractTaskFactory.hpp"

class MixedTaskFactory: public AbstractTaskFactory 
{
public:
  MixedTaskFactory(OrderedTask& _task,
                   const TaskBehaviour &tb);

  bool validate();
};

#endif
