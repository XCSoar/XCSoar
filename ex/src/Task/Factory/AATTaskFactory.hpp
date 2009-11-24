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
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  AATTaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  bool validate();
};

#endif
