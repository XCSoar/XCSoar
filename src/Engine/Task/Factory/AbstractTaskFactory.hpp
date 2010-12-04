/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef ABSTRACT_TASK_FACTORY_HPP
#define ABSTRACT_TASK_FACTORY_HPP

#include "Util/NonCopyable.hpp"
#include "Compiler.h"

#include <vector>

class AATPoint;
class StartPoint;
class IntermediatePoint;
class ASTPoint;
class FinishPoint;
class OrderedTask;
class TaskBehaviour;
class OrderedTaskBehaviour;
class OrderedTaskPoint;
class ObservationZonePoint;
class Waypoint;

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
 * - check TaskBehaviour.task_scored and ask if changes are ok if task/flight is started
 */
class AbstractTaskFactory: private NonCopyable
{
public:
/** 
 * Constructor
 * 
 * @param task Ordered task to be managed by this factory
 * @param behaviour Behaviour (options)
 */  
  AbstractTaskFactory(OrderedTask& task, const TaskBehaviour &behaviour):
    m_task(task),
    m_behaviour(behaviour) {}

  virtual ~AbstractTaskFactory() {};

  /// @todo should be abstract
  virtual void update_ordered_task_behaviour(OrderedTaskBehaviour& to); 

  /**
   * Legal types based on position
   */
  enum LegalAbstractPointType_t {
    POINT_START,
    POINT_AAT,
    POINT_AST,
    POINT_FINISH
  };

  /**
   * Vector of legal abstract point types (non-OZ specific)
   */
  typedef std::vector<LegalAbstractPointType_t> LegalAbstractVector;

  /**
   * Legal types of points with observation zones
   */
  enum LegalPointType_t {
    START_SECTOR = 0,
    START_LINE,
    START_CYLINDER,
    FAI_SECTOR,
    KEYHOLE_SECTOR,
    BGAFIXEDCOURSE_SECTOR,
    BGAENHANCEDOPTION_SECTOR,
    AST_CYLINDER,
    AAT_CYLINDER,
    AAT_SEGMENT,
    FINISH_SECTOR,
    FINISH_LINE,
    FINISH_CYLINDER
  };

  /**
   * Vector of legal point types
   */
  typedef std::vector<LegalPointType_t> LegalPointVector;

  /**
   * Task Validation Error Types
   */
enum TaskValidationErrorType_t {
  NO_VALID_START,
  NO_VALID_FINISH,
  TASK_NOT_CLOSED,
  TASK_NOT_HOMOGENEOUS,
  INCORRECT_NUMBER_TURNPOINTS,
  EXCEEDS_MAX_TURNPOINTS,
  UNDER_MIN_TURNPOINTS,
  TURNPOINTS_NOT_UNIQUE,
  INVALID_FAI_TRIANGLE_GEOMETRY,
  EMPTY_TASK
};

  /**
   * Vector of errors returned by validation routine
   */
typedef std::vector<TaskValidationErrorType_t> TaskValidationErrorVector;

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
  bool replace(OrderedTaskPoint* tp, const unsigned position,
               const bool auto_mutate = true);

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
  bool append(OrderedTaskPoint *new_tp, const bool auto_mutate = true);

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
              const bool auto_mutate = true);

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
  bool remove(const unsigned position, const bool auto_mutate = true);

  /**
   * Swap taskpoint and its successor in ordered task.
   * May fail if the candidate is the wrong type.
   * Does nothing (but returns true) if replacement is equivalent
   * Ownership is transferred to this object.
   *
   * @param position Index in task sequence of task point to replace
   * @param auto_mutate Modify task point types automatically to retain validity
   *
   * @return True on success
   */
  bool swap(const unsigned position, const bool auto_mutate = true);

  /**
   * Relocate a task point to a new location
   *
   * @param position Index in task sequence of task point to replace
   * @param waypoint Waypoint of replacement
   *
   * @return New taskpoint (or old one if failed)
   */
  const OrderedTaskPoint& relocate(const unsigned position,
                                   const Waypoint& waypoint);

  /**
   * Provide list of start types valid for later passing to createStart()
   *
   * @return list of valid start types
   */
  const LegalPointVector& getStartTypes() const {
    return m_start_types;
  }

  /**
   * Provide list of intermediate types valid for later passing to createIntermediate()
   *
   * @return list of valid intermediate types
   */
  const LegalPointVector& getIntermediateTypes() const {
    return m_intermediate_types;
  }

  /**
   * Provide list of finish types valid for later passing to createFinish()
   *
   * @return list of valid finish types
   */
  const LegalPointVector& getFinishTypes() const {
    return m_finish_types;
  }

  /** 
   * Create a point of supplied type
   * 
   * @param type Type of point to be created
   * @param wp Waypoint reference
   * 
   * @return Initialised object.  Transfers ownership to client.
   */
  gcc_pure gcc_malloc
  OrderedTaskPoint* createPoint(const LegalPointType_t type,
                                const Waypoint &wp) const;

  /**
   * Create start point of specified type
   *
   * @param type Type of start point
   * @param wp Waypoint reference
   *
   * @return Initialised StartPoint if valid, otherwise NULL
   */
  gcc_pure gcc_malloc
  StartPoint* createStart(const LegalPointType_t type,
                          const Waypoint &wp) const;

  /**
   * Create intermediate point of specified type
   *
   * @param type Type of intermediate point
   * @param wp Waypoint reference
   *
   * @return Initialised IntermediatePoint if valid, otherwise NULL
   */
  gcc_pure gcc_malloc
  IntermediatePoint* createIntermediate(const LegalPointType_t type,
                                        const Waypoint &wp) const;

  /**
   * Create finish point of specified type
   *
   * @param type Type of finish point
   * @param wp Waypoint reference
   *
   * @return Initialised FinishPoint if valid, otherwise NULL
   */
  gcc_pure gcc_malloc
  FinishPoint* createFinish(const LegalPointType_t type,
                            const Waypoint &wp) const;

  /**
   * Create start point of default type
   *
   * @param wp Waypoint reference
   *
   * @return Initialised StartPoint if valid, otherwise NULL
   */
  gcc_pure gcc_malloc
  StartPoint* createStart(const Waypoint &wp) const;

  /**
   * Create intermediate point of default type
   *
   * @param wp Waypoint reference
   *
   * @return Initialised IntermediatePoint if valid, otherwise NULL
   */
  gcc_pure gcc_malloc
  IntermediatePoint* createIntermediate(const Waypoint &wp) const;

  /**
   * Create finish point of default type
   *
   * @param wp Waypoint reference
   *
   * @return Initialised FinishPoint if valid, otherwise NULL
   */
  gcc_pure gcc_malloc
  FinishPoint* createFinish(const Waypoint &wp) const;

  /**
   * Create start point given an OZ
   *
   * @param pt OZ to be used
   * @param wp Waypoint reference
   *
   * @return Initialised object.  Ownership is transferred to client.
   */
  gcc_pure gcc_malloc
  StartPoint* createStart(ObservationZonePoint* pt,
                                const Waypoint &wp) const;

  /**
   * Create an AST point given an OZ
   *
   * @param pt OZ to be used
   * @param wp Waypoint reference
   *
   * @return Initialised object.  Ownership is transferred to client.
   */
  gcc_pure gcc_malloc
  ASTPoint* createAST(ObservationZonePoint* pt,
                              const Waypoint &wp) const;

  /**
   * Create an AAT point given an OZ
   *
   * @param pt OZ to be used
   * @param wp Waypoint reference
   *
   * @return Initialised object.  Ownership is transferred to client.
   */
  gcc_pure gcc_malloc
  AATPoint* createAAT(ObservationZonePoint* pt,
                                const Waypoint &wp) const;

  /**
   * Create a finish point given an OZ
   *
   * @param pt OZ to be used
   * @param wp Waypoint reference
   *
   * @return Initialised object.  Ownership is transferred to client.
   */
  gcc_pure gcc_malloc
  FinishPoint* createFinish(ObservationZonePoint* pt,
                            const Waypoint &wp) const;

  /**
   * Check whether task is complete and valid according to factory rules
   * Adds error types to m_validation_errors
   *
   * @return True if task is valid according to factory rules
   */
  virtual bool validate();
  /**
   * Retrieve settings from task
   *
   * @return settings from task
   */
  gcc_pure
  const OrderedTaskBehaviour& get_ordered_task_behaviour() const;

  /**
   * Check whether an abstract type is valid in a specified position
   *
   * @param type Type to check
   * @param position Index position in task
   *
   * @return True if type is valid
   */
  gcc_pure
  virtual bool validAbstractType(LegalAbstractPointType_t type, const unsigned position) const;

  /**
   *  FOR TESTING ONLY
   * @param index index of task point sequence
   * @return True if aircraft has previously entered the taskpoint or if index is invalid
   */
  gcc_pure
  bool has_entered(unsigned index) const;

  /**
   * List valid intermediate types for a given position
   *
   * @param position Index position in task
   *
   * @return Vector of valid types in position
   */
  gcc_pure
  LegalPointVector getValidIntermediateTypes(unsigned position) const;

  /**
   * List valid types for a given position
   *
   * @param position Index position in task
   *
   * @return Vector of valid types in position
   */
  gcc_pure
  LegalPointVector getValidTypes(unsigned position) const;

  /**
   * Inspect the type of a point
   *
   * @param point Point to check
   *
   * @return Type of supplied point
   */
  gcc_pure
  LegalPointType_t getType(const OrderedTaskPoint* point) const;

  /**
   * Determines whether task is closed (finish same as start)
   * @return true if task is closed
   */
  gcc_pure
  bool is_closed() const;

  /**
   * Determines whether task is unique 
   * (other than start/finish, no points used more than once)
   * @return true if task is unique
   */
  gcc_pure
  bool is_unique() const;

  /**
   * Determine if a type is valid for a FinishPoint
   *
   * @param type Type to check
   *
   * @return True if type is valid
   */
  gcc_pure
  bool validFinishType(LegalPointType_t type) const;

  /**
   * Determine if a type is valid for a StartPoint
   *
   * @param type Type to check
   *
   * @return True if type is valid
   */
  gcc_pure
  bool validStartType(LegalPointType_t type) const;

  /**
   * Determine if a type is valid for an IntermediatePoint
   *
   * @param type Type to check
   *
   * @return True if type is valid
   */
  gcc_pure
  bool validIntermediateType(LegalPointType_t type) const;


  /**
   * Sets / verifies all tps for the task type.
   * The resultant task is invalid if there are not enough
   * or too many tps for the task type
   * Also checks is_closed property if has Finish
   *
   * Does not ensure tps are unique
   *
   * * @return True if task is changed
   */
  bool mutate_tps_to_task_type();

  /**
  * Call to validate() populates this vector
  * @return returns vector of errors for current task
  */
  gcc_pure
  TaskValidationErrorVector getValidationErrors();


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
  gcc_pure
  virtual bool validType(OrderedTaskPoint *new_tp, unsigned position) const;

  /** task managed by this factory */
  OrderedTask &m_task;
  /** behaviour (settings) */
  const TaskBehaviour &m_behaviour;

  /** list of valid start types, for specialisation */
  LegalPointVector m_start_types;
  /** list of valid intermediate types, for specialisation */
  LegalPointVector m_intermediate_types;
  /** list of valid finish types, for specialisation */
  LegalPointVector m_finish_types;
  /** list of errors returned by task validation */
  TaskValidationErrorVector m_validation_errors;

  /** 
   * Check whether the supplied position can be a StartPoint
   * 
   * @param position index to test
   * 
   * @return True if possible
   */
  bool is_position_start(const unsigned position) const {
    return position == 0;
  }

  /** 
   * Check whether the supplied position can be an IntermediatePoint
   * 
   * @param position index to test
   * 
   * @return True if possible
   */
  gcc_pure
  bool is_position_intermediate(const unsigned position) const;

  /** 
   * Check whether the supplied position can be a FinishPoint
   * 
   * @param position index to test
   * 
   * @return True if possible
   */
  gcc_pure
  bool is_position_finish(const unsigned position) const;

  /**
   * Inserts the validation error type into the vector of validation errors
   *
   * @param e The validation error type to be added
   */
  void addValidationError(TaskValidationErrorType_t e);


private:
  /**
   * Verifies and sets the finish waypoint per the is_closed
   * property of task type.
   *
   * If a finish point exists and the is_closed property
   * is set, sets the finish point waypoint to the waypoint of Start
   *
   * @return True if task is changed
   */
   bool mutate_closed_finish_per_task_type();

  /**
  * Clears the vector of validation errors for the current task
  */
  void clearValidationErrors();


};

#endif
