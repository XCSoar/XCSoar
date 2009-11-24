#ifndef ABSTRACT_TASK_FACTORY_HPP
#define ABSTRACT_TASK_FACTORY_HPP

#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/Tasks/BaseTask/IntermediatePoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"

#include <vector>

/**
 * AbstractTaskFactory is a class used as a sort of wizard
 * to assist clients to create valid tasks of particular types.
 *
 * This serves to constrain the TaskPoint types to valid ones
 * and also contains a method to validate a constructed or partially
 * constructed task according to rules.
 *
 * \todo 
 * - add descriptor field so validate() methods can return feedback about
 *   validity of task etc.  This can also be added to saved files.
 *
 */
class AbstractTaskFactory
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  AbstractTaskFactory(OrderedTask& _task,
                      const TaskBehaviour &tb):
    task(_task),
    behaviour(tb) 
    {
    }

  /**
   * Legal types of StartPoint observation zones
   */
  enum LegalStartType_t {
    START_SECTOR = 0,
    START_LINE,
    START_CYLINDER
  };

  /**
   * Legal types of IntermediatePoint observation zones (and type AAT/AST)
   */
  enum LegalIntermediateType_t {
    FAI_SECTOR = 0,
    AST_CYLINDER,
    AAT_CYLINDER,
    AAT_SEGMENT
  };

  /**
   * Legal types of FinishPoint observation zones
   */
  enum LegalFinishType_t {
    FINISH_SECTOR = 0,
    FINISH_LINE,
    FINISH_CYLINDER
  };

/** 
 * Replace taskpoint in ordered task.
 * May fail if the candidate is the wrong type.
 * Does nothing (but returns true) if replacement is equivalent
 * Ownership is transferred to this object.
 * 
 * @param tp Taskpoint to become replacement
 * @param position Index in task sequence of task point to replace
 * @param auto_mutate Modify task point types automatically to retain validity
 * 
 * @return True on success
 */
  bool replace(OrderedTaskPoint* tp, const unsigned position, const bool auto_mutate=true);

/** 
 * Add taskpoint to ordered task.  It is the
 * user's responsibility to ensure the task is
 * valid (has a start/intermediate/finish).
 * 
 * @param new_tp New taskpoint to add
 * @param auto_mutate Modify task point types automatically to retain validity
 * 
 * @return True if operation successful
 */
  bool append(OrderedTaskPoint *new_tp, const bool auto_mutate=true);

/** 
 * Insert taskpoint to ordered task.  It is the
 * user's responsibility to ensure the task is
 * valid (has a start/intermediate/finish).
 * 
 * @param new_tp New taskpoint to insert
 * @param position Sequence before which to insert new task point
 * @param auto_mutate Modify task point types automatically to retain validity
 * 
 * @return True if operation successful
 */
  bool insert(OrderedTaskPoint *new_tp, const unsigned position, 
              const bool auto_mutate=true);

/** 
 * Remove taskpoint from ordered task.  It is the
 * user's responsibility to ensure the task is
 * valid (has a start/intermediate/finish).
 * 
 * @param position Sequence number of taskpoint to remove
 * @param auto_mutate Modify task point types automatically to retain validity
 * 
 * @return True if operation successful
 */
  bool remove(const unsigned position, 
              const bool auto_mutate=true);

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
 * Create start point of default type
 * 
 * @param wp Waypoint reference
 * 
 * @return Initialised StartPoint if valid, otherwise NULL
 */
  virtual StartPoint* createStart(const Waypoint &wp) const;

/** 
 * Create intermediate point of default type
 * 
 * @param wp Waypoint reference
 * 
 * @return Initialised IntermediatePoint if valid, otherwise NULL
 */
  virtual IntermediatePoint* createIntermediate(const Waypoint &wp) const;

/** 
 * Create finish point of default type
 * 
 * @param wp Waypoint reference
 * 
 * @return Initialised FinishPoint if valid, otherwise NULL
 */
  virtual FinishPoint* createFinish(const Waypoint &wp) const;

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  virtual bool validate() = 0;

  /**
   *  FOR TESTING ONLY
   * @param index index of task point sequence
   * @return True if aircraft has previously entered the taskpoint
   */
  bool has_entered(unsigned index) const;

protected:
/** 
 * Test whether a candidate object is of correct type to be added/replaced/etc
 * in the task.
 * 
 * @param new_tp Candidate object
 * @param position Desired task sequence index of candidate
 * 
 * @return True if candidate is valid at the position
 */
  virtual bool validType(OrderedTaskPoint *new_tp, unsigned position) const;

  OrderedTask &task; /**< task managed by this factory */
  const TaskBehaviour &behaviour; /**< behaviour (settings) */

  std::vector<LegalStartType_t> start_types; /**< list of valid start types, for specialisation */
  std::vector<LegalIntermediateType_t> intermediate_types; /**< list of valid intermediate types, for specialisation */
  std::vector<LegalFinishType_t> finish_types; /**< list of valid finish types, for specialisation */
};

#endif
