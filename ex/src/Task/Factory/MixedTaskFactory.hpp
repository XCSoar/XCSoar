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
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  MixedTaskFactory(OrderedTask& _task,
                   const TaskBehaviour &tb);

    virtual ~MixedTaskFactory() {};

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  bool validate();
};

#endif
