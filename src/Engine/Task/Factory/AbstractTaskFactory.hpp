/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
 * - check TaskBehaviour.task_scored and ask if changes are ok if task/flight is started
 */
class AbstractTaskFactory: 
  private NonCopyable
{
public:
/** 
 * Constructor
 * 
 * @param task Ordered task to be managed by this factory
 * @param behaviour Behaviour (options)
 */  
  AbstractTaskFactory(OrderedTask& task,
                      const TaskBehaviour &behaviour):
    m_task(task),
    m_behaviour(behaviour)
    {
    }

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

  typedef std::vector<LegalAbstractPointType_t> LegalAbstractVector;

  /**
   * Legal types of points with observation zones
   */
  enum LegalPointType_t {
    START_SECTOR = 0,
    START_LINE,
    START_CYLINDER,
    FAI_SECTOR,
    AST_CYLINDER,
    AAT_CYLINDER,
    AAT_SEGMENT,
    FINISH_SECTOR,
    FINISH_LINE,
    FINISH_CYLINDER
  };
  
  typedef std::vector<LegalPointType_t> LegalPointVector;

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
  bool swap(const unsigned position, const bool auto_mutate=true);

/** 
 * Relocate a task point to a new location
 * 
 * @param position Index in task sequence of task point to replace
 * @param waypoint Waypoint of replacement
 * 
 * @return New taskpoint (or old one if failed)
 */
  const OrderedTaskPoint& relocate(const unsigned position, const Waypoint& waypoint);

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
  FinishPoint* createFinish(const LegalPointType_t type,
                            const Waypoint &wp) const;

/** 
 * Create start point of default type
 * 
 * @param wp Waypoint reference
 * 
 * @return Initialised StartPoint if valid, otherwise NULL
 */
  StartPoint* createStart(const Waypoint &wp) const;

/** 
 * Create intermediate point of default type
 * 
 * @param wp Waypoint reference
 * 
 * @return Initialised IntermediatePoint if valid, otherwise NULL
 */
  IntermediatePoint* createIntermediate(const Waypoint &wp) const;

/** 
 * Create finish point of default type
 * 
 * @param wp Waypoint reference
 * 
 * @return Initialised FinishPoint if valid, otherwise NULL
 */
  FinishPoint* createFinish(const Waypoint &wp) const;

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  virtual bool validate() = 0;

  const OrderedTaskBehaviour& get_ordered_task_behaviour() const;

  virtual bool validAbstractType(LegalAbstractPointType_t type, const unsigned position) const;

  /**
   *  FOR TESTING ONLY
   * @param index index of task point sequence
   * @return True if aircraft has previously entered the taskpoint or if index is invalid
   */
  bool has_entered(unsigned index) const;

  LegalPointVector getValidTypes(unsigned position) const;
  LegalPointType_t getType(const OrderedTaskPoint* point) const;

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

  bool validFinishType(LegalPointType_t type) const;
  bool validStartType(LegalPointType_t type) const;
  bool validIntermediateType(LegalPointType_t type) const;

  OrderedTask &m_task; /**< task managed by this factory */
  const TaskBehaviour &m_behaviour; /**< behaviour (settings) */

  LegalPointVector m_start_types; /**< list of valid start types, for specialisation */
  LegalPointVector m_intermediate_types; /**< list of valid intermediate types, for specialisation */
  LegalPointVector m_finish_types; /**< list of valid finish types, for specialisation */

  bool is_position_start(const unsigned position) const;
  bool is_position_intermediate(const unsigned position) const;
  bool is_position_finish(const unsigned position) const;
};

#endif
