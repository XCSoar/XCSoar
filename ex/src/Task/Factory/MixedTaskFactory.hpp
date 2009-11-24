#ifndef MIXED_TASK_FACTORY_HPP
#define MIXED_TASK_FACTORY_HPP

#include "AbstractTaskFactory.hpp"

/**
 * Factory for mixed tasks (mixture of AST and AAT sectors)
 * This is the most general of the factories.
 */
class MixedTaskFactory: public AbstractTaskFactory 
{
public:
  MixedTaskFactory(OrderedTask& _task,
                   const TaskBehaviour &tb);

  bool validate();
};

#endif
