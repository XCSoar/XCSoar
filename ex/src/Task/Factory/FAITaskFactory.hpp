#ifndef FAI_TASK_FACTORY_HPP
#define FAI_TASK_FACTORY_HPP

#include "AbstractTaskFactory.hpp"

class FAITaskFactory: public AbstractTaskFactory 
{
public:
  FAITaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

  bool validate();
};

#endif
