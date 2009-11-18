#ifndef AAT_TASK_FACTORY_HPP
#define AAT_TASK_FACTORY_HPP

#include "AbstractTaskFactory.hpp"

class AATTaskFactory: public AbstractTaskFactory 
{
public:
  AATTaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

  bool validate();
};

#endif
