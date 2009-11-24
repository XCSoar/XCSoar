#ifndef FAI_TASK_FACTORY_HPP
#define FAI_TASK_FACTORY_HPP

#include "AbstractTaskFactory.hpp"

/**
 * Factory for construction of legal FAI tasks
 * Currently the validate() method will check 4-point tasks as to whether they
 * satisfy short and long-distance FAI triangle rules.
 */
class FAITaskFactory: public AbstractTaskFactory 
{
public:
  FAITaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

  bool validate();
};

#endif
