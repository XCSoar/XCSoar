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
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  FAITaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

    virtual ~FAITaskFactory() {};

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  bool validate();
};

#endif
