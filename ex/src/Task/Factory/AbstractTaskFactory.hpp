#ifndef ABSTRACT_TASK_FACTORY_HPP
#define ABSTRACT_TASK_FACTORY_HPP

#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/Tasks/BaseTask/IntermediatePoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"

#include <vector>

/**
 * \todo move orderedtask manipulation methods (insert, replace, append, remove) to here?
 *
 */
class AbstractTaskFactory
{
public:
  AbstractTaskFactory(OrderedTask& _task,
                      const TaskBehaviour &tb):
    task(_task),
    behaviour(tb) 
    {
    }

  enum LegalStartType_t {
    START_SECTOR = 0,
    START_LINE,
    START_CYLINDER
  };

  enum LegalIntermediateType_t {
    FAI_SECTOR = 0,
    AST_CYLINDER,
    AAT_CYLINDER,
    AAT_SEGMENT
  };

  enum LegalFinishType_t {
    FINISH_SECTOR = 0,
    FINISH_LINE,
    FINISH_CYLINDER
  };

/** 
 * Provide list of start types valid for later passing to createStart()
 * 
 * @return list of valid start types
 */
  const std::vector<LegalStartType_t>& getStartTypes() const {
    return start_types;
  }

/** 
 * Provide list of intermediate types valid for later passing to createIntermediate()
 * 
 * @return list of valid intermediate types
 */
  const std::vector<LegalIntermediateType_t>& getIntermediateTypes() const {
    return intermediate_types;
  }

/** 
 * Provide list of finish types valid for later passing to createFinish()
 * 
 * @return list of valid finish types
 */
  const std::vector<LegalFinishType_t>& getFinishTypes() const {
    return finish_types;
  }

/** 
 * Create start point of specified type
 * 
 * @param type Type of start point
 * @param wp Waypoint reference
 * 
 * @return Initialised StartPoint if valid, otherwise NULL
 */
  virtual StartPoint* createStart(const LegalStartType_t type,
                                 const Waypoint &wp) const;

/** 
 * Create intermediate point of specified type
 * 
 * @param type Type of intermediate point
 * @param wp Waypoint reference
 * 
 * @return Initialised IntermediatePoint if valid, otherwise NULL
 */
  virtual IntermediatePoint* createIntermediate(const LegalIntermediateType_t type,
                                 const Waypoint &wp) const;

/** 
 * Create finish point of specified type
 * 
 * @param type Type of finish point
 * @param wp Waypoint reference
 * 
 * @return Initialised FinishPoint if valid, otherwise NULL
 */
  virtual FinishPoint* createFinish(const LegalFinishType_t type,
                                 const Waypoint &wp) const;

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  virtual bool validate() = 0;

protected:
  OrderedTask &task;
  const TaskBehaviour &behaviour;
  std::vector<LegalStartType_t> start_types;
  std::vector<LegalIntermediateType_t> intermediate_types;
  std::vector<LegalFinishType_t> finish_types;
};

#endif
