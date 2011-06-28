/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "AbstractTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/BGAStartSectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"

#include <algorithm>

/**
 * Utility class to read the user-definable radius or length of and observation zone
 * returns radius or -1 if oz type has no user-definable radius or length
 */
class UserSizeObservationZone: public ObservationZoneConstVisitor
{
public:
  UserSizeObservationZone() :
    ozUserSize(fixed_minus_one)
  {
  }
  void
  Visit(const FAISectorZone& oz)
  {
  }
  void
  Visit(const KeyholeZone& oz)
  {
  }
  void
  Visit(const BGAFixedCourseZone& oz)
  {
  }
  void
  Visit(const BGAEnhancedOptionZone& oz)
  {
  }
  void
  Visit(const BGAStartSectorZone& oz)
  {
  }
  void
  Visit(const SectorZone& oz)
  {
    ozUserSize = oz.getRadius();
  }
  void
  Visit(const AnnularSectorZone& oz)
  {
    ozUserSize = oz.getRadius();
  }
  void
  Visit(const LineSectorZone& oz)
  {
    ozUserSize = oz.getLength();
  }
  void
  Visit(const CylinderZone& oz)
  {
    ozUserSize = oz.getRadius();
  }
public:
  fixed
  get_user_size()
  {
    return ozUserSize;
  }
private:
  fixed ozUserSize;
};

OrderedTaskPoint*
AbstractTaskFactory::createMutatedPoint(const OrderedTaskPoint &tp,
                                        const LegalPointType_t newtype) const
{
  const ObservationZonePoint* oz = tp.get_oz();
  UserSizeObservationZone soz;
  ObservationZoneConstVisitor &oz_visitor = soz;
  oz_visitor.Visit(*oz);
  const fixed ozsize = soz.get_user_size();

  return (OrderedTaskPoint*)createPoint(newtype,
     tp.get_waypoint(), ozsize, ozsize, ozsize);
}

AbstractTaskFactory::LegalPointType_t
AbstractTaskFactory::getMutatedPointType(const OrderedTaskPoint &tp) const
{
  const LegalPointType_t oldtype = getType(tp);
  LegalPointType_t newtype = oldtype;

  switch (tp.GetType()) {
  case TaskPoint::START:
    if (!validStartType(newtype)) {
      newtype = m_behaviour.sector_defaults.start_type;
      if (!validStartType(newtype))
        newtype = *m_start_types.begin();
    }
    break;

  case TaskPoint::AST:
  case TaskPoint::AAT:
    if (!validIntermediateType(newtype)) {
      newtype = m_behaviour.sector_defaults.turnpoint_type;
      if (!validIntermediateType(newtype)) {
        newtype = *m_intermediate_types.begin();
      }
    }
    break;

  case TaskPoint::FINISH:
    if (!validFinishType(newtype)) {
      newtype = m_behaviour.sector_defaults.finish_type;
      if (!validFinishType(newtype))
        newtype = *m_finish_types.begin();
    }
    break;

  case TaskPoint::UNORDERED:
  case TaskPoint::ROUTE:
    break;
  }
  return newtype;
}

StartPoint* 
AbstractTaskFactory::createStart(ObservationZonePoint* oz,
                                 const Waypoint& wp) const
{
  return new StartPoint(oz, wp, m_behaviour,
                        get_ordered_task_behaviour());
}

FinishPoint* 
AbstractTaskFactory::createFinish(ObservationZonePoint* oz,
                                 const Waypoint& wp) const
{
  return new FinishPoint(oz, wp, m_behaviour,
                         get_ordered_task_behaviour());
}

AATPoint* 
AbstractTaskFactory::createAAT(ObservationZonePoint* oz,
                               const Waypoint& wp) const
{
  return new AATPoint(oz, wp, m_behaviour,
                      get_ordered_task_behaviour());
}

ASTPoint* 
AbstractTaskFactory::createAST(ObservationZonePoint* oz,
                               const Waypoint& wp) const
{
  return new ASTPoint(oz, wp, m_behaviour,
                      get_ordered_task_behaviour());
}

StartPoint* 
AbstractTaskFactory::createStart(const Waypoint &wp) const
{
  LegalPointType_t type = m_behaviour.sector_defaults.start_type;
  if (!validStartType(type))
    type = *m_start_types.begin();

  return createStart(type, wp);
}

IntermediateTaskPoint* 
AbstractTaskFactory::createIntermediate(const Waypoint &wp) const
{
  if (get_ordered_task_behaviour().homogeneous_tps && (m_task.task_size()>1)) {
    LegalPointType_t type = getType(*m_task.get_tp(1));
    if (validIntermediateType(type))
      return createIntermediate(type, wp);
  }

  LegalPointType_t type = m_behaviour.sector_defaults.turnpoint_type;
    if (!validIntermediateType(type))
      type = *m_intermediate_types.begin();

  return createIntermediate(type, wp);
}

FinishPoint* 
AbstractTaskFactory::createFinish(const Waypoint &wp) const
{
  LegalPointType_t type = m_behaviour.sector_defaults.finish_type;
    if (!validFinishType(type))
      type = *m_finish_types.begin();

    return createFinish(type, wp);
}

AbstractTaskFactory::LegalPointType_t 
AbstractTaskFactory::getType(const OrderedTaskPoint &point) const
{
  const ObservationZonePoint* oz = point.get_oz();

  switch (point.GetType()) {
  case TaskPoint::START:
    switch (oz->shape) {
    case ObservationZonePoint::FAI_SECTOR:
      return START_SECTOR;

    case ObservationZonePoint::LINE:
      return START_LINE;

    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
    case ObservationZonePoint::ANNULAR_SECTOR:
      return START_CYLINDER;

    case ObservationZonePoint::BGA_START:
      return START_BGA;
    }
    break;

  case TaskPoint::AAT:
    switch (oz->shape) {
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::FAI_SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
    case ObservationZonePoint::BGA_START:
    case ObservationZonePoint::LINE:
      return AAT_SEGMENT;
    case ObservationZonePoint::ANNULAR_SECTOR:
      return AAT_ANNULAR_SECTOR;
    case ObservationZonePoint::CYLINDER:
      return AAT_CYLINDER;
    } 
    break;

  case TaskPoint::AST:
    switch (oz->shape) {
    case ObservationZonePoint::FAI_SECTOR:
      return FAI_SECTOR;

    case ObservationZonePoint::KEYHOLE:
      return KEYHOLE_SECTOR;

    case ObservationZonePoint::BGAFIXEDCOURSE:
      return BGAFIXEDCOURSE_SECTOR;

    case ObservationZonePoint::BGAENHANCEDOPTION:
      return BGAENHANCEDOPTION_SECTOR;

    case ObservationZonePoint::BGA_START:
    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::LINE:
    case ObservationZonePoint::ANNULAR_SECTOR:
      return AST_CYLINDER;
    } 
    break;

  case TaskPoint::FINISH:
    switch (oz->shape) {
    case ObservationZonePoint::BGA_START:
    case ObservationZonePoint::FAI_SECTOR:
      return FINISH_SECTOR;

    case ObservationZonePoint::LINE:
      return FINISH_LINE;

    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
    case ObservationZonePoint::ANNULAR_SECTOR:
      return FINISH_CYLINDER;
    } 
    break;

  case TaskPoint::UNORDERED:
  case TaskPoint::ROUTE:
    /* obviously, when we check the type of an OrderedTaskPoint, we
       should never get type==UNORDERED or ROUTE. */
    assert(false);
    break;
  } 

  // fail, should never get here
  assert(1);
  return START_LINE;
}

OrderedTaskPoint* 
AbstractTaskFactory::createPoint(const LegalPointType_t type,
                                 const Waypoint &wp) const
{
  return createPoint(type, wp,
      fixed_minus_one,
      fixed_minus_one,
      fixed_minus_one);
}

void
AbstractTaskFactory::getPointDefaultSizes(const LegalPointType_t type,
                                          fixed &start_radius,
                                          fixed &turnpoint_radius,
                                          fixed &finish_radius) const
{
  TaskBehaviour ob =this->m_behaviour;

  if (start_radius < fixed_zero)
    start_radius = ob.sector_defaults.start_radius;

  if (turnpoint_radius < fixed_zero)
    turnpoint_radius = ob.sector_defaults.turnpoint_radius;

  if (finish_radius < fixed_zero)
    finish_radius = ob.sector_defaults.finish_radius;
}

OrderedTaskPoint*
AbstractTaskFactory::createPoint(const LegalPointType_t type,
                                 const Waypoint &wp,
                                 const fixed _start_radius,
                                 const fixed _turnpoint_radius,
                                 const fixed _finish_radius) const
{
  fixed start_radius = _start_radius;
  fixed turnpoint_radius = _turnpoint_radius;
  fixed finish_radius = _finish_radius;

  getPointDefaultSizes(type, start_radius, turnpoint_radius, finish_radius);

  switch (type) {
  case START_SECTOR:
    return createStart(new FAISectorZone(wp.Location, false), wp);
  case START_LINE:
    return createStart(new LineSectorZone(wp.Location, start_radius), wp);
  case START_CYLINDER:
    return createStart(new CylinderZone(wp.Location, start_radius), wp);
  case START_BGA:
    return createStart(new BGAStartSectorZone(wp.Location), wp);
  case FAI_SECTOR:
    return createAST(new FAISectorZone(wp.Location, true), wp);
  case KEYHOLE_SECTOR:
    return createAST(new KeyholeZone(wp.Location), wp);
  case BGAFIXEDCOURSE_SECTOR:
    return createAST(new BGAFixedCourseZone(wp.Location), wp);
  case BGAENHANCEDOPTION_SECTOR:
    return createAST(new BGAEnhancedOptionZone(wp.Location), wp);
  case AST_CYLINDER:
    return createAST(new CylinderZone(wp.Location, turnpoint_radius), wp);
  case AAT_CYLINDER:
    return createAAT(new CylinderZone(wp.Location, turnpoint_radius), wp);
  case AAT_SEGMENT:
    return createAAT(new SectorZone(wp.Location, turnpoint_radius), wp);
  case AAT_ANNULAR_SECTOR:
    return createAAT(new AnnularSectorZone(wp.Location, turnpoint_radius), wp);
  case FINISH_SECTOR:
    return createFinish(new FAISectorZone(wp.Location, false), wp);
  case FINISH_LINE:
    return createFinish(new LineSectorZone(wp.Location, finish_radius), wp);
  case FINISH_CYLINDER:
    return createFinish(new CylinderZone(wp.Location, finish_radius), wp);
  };
  assert(1);
  return NULL;
}

StartPoint* 
AbstractTaskFactory::createStart(const LegalPointType_t type,
                                 const Waypoint &wp) const
{
  if (!validStartType(type))
    // error, invalid type!
    return NULL;

  return (StartPoint*)createPoint(type, wp);
}

IntermediateTaskPoint* 
AbstractTaskFactory::createIntermediate(const LegalPointType_t type,
                                        const Waypoint &wp) const
{
  if (!validIntermediateType(type))
    return NULL;

  return (IntermediateTaskPoint*)createPoint(type, wp);
}

FinishPoint* 
AbstractTaskFactory::createFinish(const LegalPointType_t type,
                                  const Waypoint &wp) const
{
  if (!validFinishType(type))
    return NULL;

  return (FinishPoint*)createPoint(type, wp);
}

bool 
AbstractTaskFactory::append(const OrderedTaskPoint &new_tp,
                            const bool auto_mutate)
{
  if (m_task.is_max_size())
    return false;

  if (auto_mutate) {
    if (!m_task.task_size()) {
      // empty task, so add as a start point
      if (validType(new_tp, m_task.task_size())) {
        // candidate is ok, so add it
        return m_task.append(new_tp);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp.get_waypoint());
        bool success = m_task.append(*sp);
        delete sp;
        return success;
      }
    }

    // non-empty task

    if (m_task.has_finish()) {
      // old finish must be mutated into an intermediate point
      IntermediateTaskPoint* sp = createIntermediate(m_task.getTaskPoint(
          m_task.task_size() - 1)->get_waypoint());

      m_task.replace(*sp, m_task.task_size()-1);
      delete sp;
    }

    if (validType(new_tp, m_task.task_size()))
      // ok to append directly
      return m_task.append(new_tp);

    // this point must be mutated into a finish
    FinishPoint* sp = createFinish(new_tp.get_waypoint());
    bool success = m_task.append(*sp);
    delete sp;
    return success;
  }

  return m_task.append(new_tp);
}

bool 
AbstractTaskFactory::replace(const OrderedTaskPoint &new_tp,
                             const unsigned position,
                             const bool auto_mutate)
{
  if (auto_mutate) {
    if (validType(new_tp, position))
      // ok to replace directly
      return m_task.replace(new_tp, position);

    // will need to convert type of candidate
    OrderedTaskPoint *tp;
    if (position == 0) {
      // candidate must be transformed into a startpoint
      tp = createStart(new_tp.get_waypoint());
    } else if (is_position_finish(position) && (position + 1 == m_task.task_size())) {
      // this point must be mutated into a finish
      tp = createFinish(new_tp.get_waypoint());
    } else {
      // this point must be mutated into an intermediate
      tp = createIntermediate(new_tp.get_waypoint());
    }

    bool success = m_task.replace(*tp, position);
    delete tp;
    return success;
  }

  return m_task.replace(new_tp, position);
}

bool 
AbstractTaskFactory::insert(const OrderedTaskPoint &new_tp,
                            const unsigned position,
                            const bool auto_mutate)
{
  if (position >= m_task.task_size())
    return append(new_tp, auto_mutate);

  if (auto_mutate) {
    if (position == 0) {
      if (m_task.has_start()) {
        // old start must be mutated into an intermediate point
        IntermediateTaskPoint* sp =
            createIntermediate(m_task.getTaskPoint(0)->get_waypoint());
        m_task.replace(*sp, 0);
        delete sp;
      }
      if (validType(new_tp, 0)) {
        return m_task.insert(new_tp, 0);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp.get_waypoint());
        bool success = m_task.insert(*sp, 0);
        delete sp;
        return success;
      }
    } else {
      if (new_tp.is_intermediate()) {
        // candidate ok for direct insertion
        return m_task.insert(new_tp, position);
      } else {
        // candidate must be transformed into a intermediatepoint
        IntermediateTaskPoint* sp = createIntermediate(new_tp.get_waypoint());
        bool success = m_task.insert(*sp, position);
        delete sp;
        return success;
      }
    }
  }

  return m_task.insert(new_tp, position);
}

bool 
AbstractTaskFactory::remove(const unsigned position, 
                            const bool auto_mutate)
{
  if (position >= m_task.task_size())
    return false;

  if (auto_mutate) {
    if (position == 0) {
      // special case, remove start point..
      if (m_task.task_size() == 1) {
        return m_task.remove(0);
      } else {
        // create new start point from next point
        StartPoint* sp = createStart(m_task.getTaskPoint(1)->get_waypoint());
        bool success = m_task.remove(0) && m_task.replace(*sp, 0);
        delete sp;
        return success;
      }
    } else if (is_position_finish(position - 1) && (position + 1 == m_task.task_size())) {
      // create new finish from previous point
      FinishPoint* sp = createFinish(
          m_task.getTaskPoint(position - 1)->get_waypoint());
      bool success = m_task.remove(position) &&
        m_task.replace(*sp, position - 1);
      delete sp;
      return success;
    } else {
      // intermediate point deleted, nothing special to do
      return m_task.remove(position);
    }
  }

  return m_task.remove(position);
}

bool 
AbstractTaskFactory::has_entered(unsigned position) const
{
  if (m_task.getTaskPoint(position))
    return m_task.getTaskPoint(position)->has_entered();

  return true;
}

bool 
AbstractTaskFactory::swap(const unsigned position, const bool auto_mutate)
{
  if (m_task.task_size() <= 1)
    return false;
  if (position >= m_task.task_size() - 1)
    return false;

  const OrderedTaskPoint* orig = m_task.getTaskPoint(position+1);
  bool retval = insert(*orig, position, auto_mutate);
  if (!retval)
    return false;

  return remove(position+2, auto_mutate);
}

const OrderedTaskPoint&
AbstractTaskFactory::relocate(const unsigned position, 
                              const Waypoint& waypoint)
{
  m_task.relocate(position, waypoint);  
  return *m_task.getTaskPoint(position);
}

const OrderedTaskBehaviour& 
AbstractTaskFactory::get_ordered_task_behaviour() const {
  return m_task.get_ordered_task_behaviour();
}

void 
AbstractTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
}

bool 
AbstractTaskFactory::is_position_intermediate(const unsigned position) const
{
  if (is_position_start(position))
    return false;
  if (position >= get_ordered_task_behaviour().max_points)
    return false;
  if (position + 1 < get_ordered_task_behaviour().min_points)
    return true;

  if (get_ordered_task_behaviour().is_fixed_size())
    return (position + 1 < get_ordered_task_behaviour().max_points);
  else if (m_task.task_size() < get_ordered_task_behaviour().min_points)
    return true;
  else
    return (position <= m_task.task_size());
}

bool 
AbstractTaskFactory::is_position_finish(const unsigned position) const
{
  if (is_position_start(position))
    return false;

  if (position + 1 < get_ordered_task_behaviour().min_points)
    return false;
  if (position + 1 > get_ordered_task_behaviour().max_points)
    return false;

  if (get_ordered_task_behaviour().is_fixed_size())
    return (position + 1 == get_ordered_task_behaviour().max_points);
  else
    return (position + 1 >= m_task.task_size());
}

bool
AbstractTaskFactory::validAbstractType(LegalAbstractPointType_t type, 
                                       const unsigned position) const
{
  const bool is_start = is_position_start(position);
  const bool is_finish = is_position_finish(position);
  const bool is_intermediate = is_position_intermediate(position);

  switch (type) {
  case POINT_START:
    return is_start;
  case POINT_FINISH:
    return is_finish;
  case POINT_AST:
    return is_intermediate &&
      (validIntermediateType(FAI_SECTOR) 
       || validIntermediateType(AST_CYLINDER)
       || validIntermediateType(KEYHOLE_SECTOR)
       || validIntermediateType(BGAFIXEDCOURSE_SECTOR)
       || validIntermediateType(BGAENHANCEDOPTION_SECTOR));
  case POINT_AAT:
    return is_intermediate &&
      (validIntermediateType(AAT_CYLINDER)
       || validIntermediateType(AAT_SEGMENT)
       || validIntermediateType(AAT_ANNULAR_SECTOR));
  };
  return false;
}

bool 
AbstractTaskFactory::validType(const OrderedTaskPoint &new_tp,
                               unsigned position) const
{
  switch (new_tp.GetType()) {
  case TaskPoint::START:
    return validAbstractType(POINT_START, position) &&
        validStartType(getType(new_tp));

  case TaskPoint::AST:
    return validAbstractType(POINT_AST, position) &&
        validIntermediateType(getType(new_tp));

  case TaskPoint::AAT:
    return validAbstractType(POINT_AAT, position)&&
        validIntermediateType(getType(new_tp));

  case TaskPoint::FINISH:
    return validAbstractType(POINT_FINISH, position)&&
        validFinishType(getType(new_tp));

  case TaskPoint::UNORDERED:
  case TaskPoint::ROUTE:
    /* obviously, when we check the type of an OrderedTaskPoint, we
       should never get type==UNORDERED or ROUTE */
    assert(false);
    break;
  }

  return false;
}

bool
AbstractTaskFactory::validIntermediateType(LegalPointType_t type) const 
{
  return (std::find(m_intermediate_types.begin(), m_intermediate_types.end(), type) 
          != m_intermediate_types.end());
}

bool
AbstractTaskFactory::validStartType(LegalPointType_t type) const 
{
  return (std::find(m_start_types.begin(), m_start_types.end(), type) 
          != m_start_types.end());
}

bool
AbstractTaskFactory::validFinishType(LegalPointType_t type) const 
{
  return (std::find(m_finish_types.begin(), m_finish_types.end(), type) 
          != m_finish_types.end());
}

AbstractTaskFactory::LegalPointVector 
AbstractTaskFactory::getValidTypes(unsigned position) const
{
  LegalPointVector v;
  if (validAbstractType(POINT_START, position))
    v.insert(v.end(), m_start_types.begin(), m_start_types.end());

  LegalPointVector i = getValidIntermediateTypes(position);
  if (!i.empty())
    v.insert(v.end(), i.begin(), i.end());

  if (validAbstractType(POINT_FINISH, position))
    v.insert(v.end(), m_finish_types.begin(), m_finish_types.end());

  return v;
}

void
AbstractTaskFactory::addValidationError(TaskValidationErrorType_t e)
{
  m_validation_errors.push_back(e);
}

void
AbstractTaskFactory::clearValidationErrors()
{
  m_validation_errors.clear();
}

AbstractTaskFactory::TaskValidationErrorVector
AbstractTaskFactory::getValidationErrors()
{
  return m_validation_errors;
}

bool
AbstractTaskFactory::CheckAddFinish()
{
 if (m_task.task_size() < 2)
   return false;

 if (m_task.has_finish())
   return false;

 FinishPoint *fp = createFinish(m_task.get_tp(m_task.task_size() - 1)->get_waypoint());
 assert(fp);
 remove(m_task.task_size() - 1, false);
 append(*fp, false);
 delete fp;

 return true;
}

bool
AbstractTaskFactory::validateFAIOZs()
{
  clearValidationErrors();
  bool valid = true;

  for (unsigned i = 0; i < m_task.task_size() && valid; i++) {
    const OrderedTaskPoint *tp = m_task.get_tp(i);
    const ObservationZonePoint* oz = tp->get_oz();
    UserSizeObservationZone soz;
    ObservationZoneConstVisitor &oz_visitor = soz;
    oz_visitor.Visit(*oz);
    const fixed ozsize = soz.get_user_size();

    switch (getType(*tp)) {
    case  START_BGA:
    case  START_CYLINDER:
      valid = false;
      break;

    case  START_SECTOR:
      if (ozsize > fixed(1000.01)) {
        valid = false;
      }
      break;
    case  START_LINE:
      if (ozsize > fixed(2000.01)) {
        valid = false;
      }
      break;

    case  FAI_SECTOR:
      break;

    case  AST_CYLINDER:
      if (ozsize > fixed(500.01)) {
        valid = false;
      }
      break;

    case  KEYHOLE_SECTOR:
    case  BGAFIXEDCOURSE_SECTOR:
    case  BGAENHANCEDOPTION_SECTOR:
    case  AAT_CYLINDER:
    case  AAT_SEGMENT:
    case  AAT_ANNULAR_SECTOR:
      valid = false;
      break;

    case  FINISH_SECTOR:
      break;
    case  FINISH_LINE:
      if (ozsize > fixed(2000.01)) {
        valid = false;
      }
      break;

    case  FINISH_CYLINDER:
      valid = false;
      break;
    }
  }
  if (!valid)
    addValidationError(NON_FAI_OZS);

  return valid;
}

bool
AbstractTaskFactory::validate()
{
  clearValidationErrors();

  bool valid = true;

  if (!m_task.has_start()) {
    addValidationError(NO_VALID_START);
    valid = false;
  }
  if (!m_task.has_finish()) {
    addValidationError(NO_VALID_FINISH);
    valid = false;
  }

  if (get_ordered_task_behaviour().is_closed && !is_closed()) {
    addValidationError(TASK_NOT_CLOSED);
    valid = false;
  }

  if (get_ordered_task_behaviour().is_fixed_size()) {
    if (m_task.task_size() != get_ordered_task_behaviour().max_points) {
      addValidationError(INCORRECT_NUMBER_TURNPOINTS);
      valid = false;
    }
  } else {
    if (m_task.task_size() < get_ordered_task_behaviour().min_points) {
      addValidationError(UNDER_MIN_TURNPOINTS);
      valid = false;
    }
    if (m_task.task_size() > get_ordered_task_behaviour().max_points) {
      addValidationError(EXCEEDS_MAX_TURNPOINTS);
      valid = false;
    }
  }

   if (get_ordered_task_behaviour().homogeneous_tps && !is_homogeneous()) {
    addValidationError(TASK_NOT_HOMOGENEOUS);
    valid = false;
  }
  return valid;
}

AbstractTaskFactory::LegalPointVector 
AbstractTaskFactory::getValidIntermediateTypes(unsigned position) const
{
  LegalPointVector v;

  if (!is_position_intermediate(position))
    return v;

  if (get_ordered_task_behaviour().homogeneous_tps 
      && (position>1) && (m_task.task_size()>1)) {
    LegalPointType_t type = getType(*m_task.get_tp(1));
    if (validIntermediateType(type)) {
      v.push_back(type);
      return v;
    }
  }
  if (validAbstractType(POINT_AAT, position) ||
      validAbstractType(POINT_AST, position))
    v.insert(v.end(), m_intermediate_types.begin(), m_intermediate_types.end());
  return v;
}

AbstractTaskFactory::LegalPointVector
AbstractTaskFactory::getValidStartTypes() const
{
  LegalPointVector v;
  v.insert(v.end(), m_start_types.begin(), m_start_types.end());
  return v;
}

AbstractTaskFactory::LegalPointVector
AbstractTaskFactory::getValidIntermediateTypes() const
{
  LegalPointVector v;
  v.insert(v.end(), m_intermediate_types.begin(), m_intermediate_types.end());
  return v;
}

AbstractTaskFactory::LegalPointVector
AbstractTaskFactory::getValidFinishTypes() const
{
  LegalPointVector v;
  v.insert(v.end(), m_finish_types.begin(), m_finish_types.end());
  return v;
}

bool 
AbstractTaskFactory::is_closed() const
{
  if (m_task.task_size() < 3)
    return false;

  const Waypoint& wp_start = m_task.get_tp(0)->get_waypoint();
  const Waypoint& wp_finish =
      m_task.get_tp(m_task.task_size() - 1)->get_waypoint();

  return (wp_start.Location == wp_finish.Location);
}

bool 
AbstractTaskFactory::is_unique() const
{
  const unsigned size = m_task.task_size();
  for (unsigned i = 0; i + 1 < size; i++) {
    const Waypoint& wp_0 = m_task.get_tp(i)->get_waypoint();

    for (unsigned j = i + 1; j < size; j++) {
      if ((i == 0) && (j + 1 == size)) {
        // ok to be the same
      } else {
        const Waypoint& wp_1 = m_task.get_tp(j)->get_waypoint();
        if (wp_1 == wp_0)
          return false;
      }
    }
  }
  return true;
}

bool
AbstractTaskFactory::is_homogeneous() const
{
  bool valid = true;

  const unsigned size = m_task.task_size();

  if (size > 2) {
    LegalPointType_t homogtype = getType(*m_task.get_tp(1));

    for (unsigned i = 2; i < size; i++) {
      OrderedTaskPoint *tp = m_task.get_tp(i);
      if ((tp->GetType() == TaskPoint::FINISH)) {
        ; // don't check a valid finish point
      } else {
        if (getType(*tp) != homogtype) {
          valid = false;
          break;
        }
      }
    }
  }
  return valid;
}

bool
AbstractTaskFactory::remove_excess_tps_per_task_type()
{
  bool changed = false;
  unsigned maxtp = get_ordered_task_behaviour().max_points;
  while (maxtp < m_task.task_size()) {
    remove(maxtp, false);
    changed = true;
  }
  return changed;
}

bool
AbstractTaskFactory::mutate_tps_to_task_type()
{
  bool changed = remove_excess_tps_per_task_type();

  for (unsigned int i = 0; i < m_task.task_size(); i++) {
    OrderedTaskPoint *tp = m_task.get_tp(i);
    if (!validType(*tp, i) ||
        (m_task.get_factory_type() == TaskBehaviour::FACTORY_FAI_GENERAL)) {

      LegalPointType_t newtype = getMutatedPointType(*tp);
      if (is_position_finish(i)) {

        if (!validFinishType(newtype)) {
          newtype = m_behaviour.sector_defaults.finish_type;
          if (!validFinishType(newtype))
            newtype = *m_finish_types.begin();
        }

        FinishPoint *fp = (FinishPoint*)createMutatedPoint(*tp, newtype);
        assert(fp);
        if (replace(*fp, i, true))
          changed = true;
        delete fp;

      } else if (i == 0) {
        if (!validStartType(newtype)) {
          newtype = m_behaviour.sector_defaults.start_type;
          if (!validStartType(newtype))
            newtype = *m_start_types.begin();
        }

        StartPoint *sp = (StartPoint*)createMutatedPoint(*tp, newtype);
        assert(sp);
        if (replace(*sp, i, true))
          changed = true;
        delete sp;

      } else {

        if (!validIntermediateType(newtype)) {
          newtype = m_behaviour.sector_defaults.turnpoint_type;
          if (!validIntermediateType(newtype))
            newtype = *m_intermediate_types.begin();
        }
        OrderedTaskPoint *tpnew = (OrderedTaskPoint*)createMutatedPoint(*tp, newtype);
        if (replace(*tpnew, i, true))
          changed = true;
        delete tpnew;
      }
    }
  }

  changed |= mutate_closed_finish_per_task_type();
  return changed;
}

bool
AbstractTaskFactory::mutate_closed_finish_per_task_type()
{
  if (m_task.task_size() < 2)
    return false;

  if (!is_position_finish(m_task.task_size() - 1))
    return false;

  bool changed = false;

  if (get_ordered_task_behaviour().is_closed) {
    if (!is_closed()) {
      OrderedTaskPoint *tp = m_task.get_tp(m_task.task_size() - 1);
      assert(tp);
      if (tp->GetType() == TaskPoint::FINISH) {
        FinishPoint *fp = createFinish(m_task.get_tp(0)->get_waypoint());
        assert(fp);
        remove(m_task.task_size() - 1, false);
        append(*fp, false);
        delete fp;
        changed = true;
      }
    }
  }
  return changed;
}

bool 
AbstractTaskFactory::append_optional_start(const Waypoint& wp)
{
  OrderedTaskPoint* tp = NULL;
  if (m_task.task_size()) {
    tp = m_task.get_tp(0)->clone(m_behaviour, m_task.get_ordered_task_behaviour(), &wp);
  } else {
    tp = createStart(wp);
  }
  if (!tp)
    return false; // should never happen

  bool success = m_task.append_optional_start(*tp);
  delete tp;
  return success;
}

bool
AbstractTaskFactory::append_optional_start(const OrderedTaskPoint &new_tp,
                                           const bool auto_mutate)
{
  if (auto_mutate && !validType(new_tp, 0)) {
    // candidate must be transformed into a startpoint of appropriate type
    StartPoint* sp = createStart(new_tp.get_waypoint());
    bool success = m_task.append_optional_start(*sp);
    delete sp;
    return success;
  }
  // ok to add directly
  return m_task.append_optional_start(new_tp);
}

bool
AbstractTaskFactory::TestFAITriangle(const fixed d1, const fixed d2,
                                     const fixed d3)
{
  const fixed d_wp = d1 + d2 + d3;

  /**
   * A triangle is a valid FAI-triangle, if no side is less than
   * 28% of the total length (total length less than 750 km), or no
   * side is less than 25% or larger than 45% of the total length
   * (totallength >= 750km).
   */
  bool geometryok = false;

  if (d_wp < fixed(750000) && d1 >= fixed(0.28) * d_wp && d2 >= fixed(0.28)
      * d_wp && d3 >= fixed(0.28) * d_wp)
    // small FAI
    geometryok = true;
  else if (d_wp >= fixed(750000) && d1 > d_wp / 4 && d2 > d_wp / 4 && d3 > d_wp
      / 4 && d1 <= fixed(0.45) * d_wp && d2 <= fixed(0.45) * d_wp && d3
      <= fixed(0.45) * d_wp)
    // large FAI
    geometryok = true;

  return geometryok;
}

bool
AbstractTaskFactory::TestFAITriangle()
{
  bool valid = is_unique();

  if (m_task.task_size()==4) {
    const fixed d1 = m_task.getTaskPoint(1)->get_vector_planned().Distance;
    const fixed d2 = m_task.getTaskPoint(2)->get_vector_planned().Distance;
    const fixed d3 = m_task.getTaskPoint(3)->get_vector_planned().Distance;

    valid = TestFAITriangle(d1, d2, d3);
  }
  else
    valid = false;

  return valid;
}

