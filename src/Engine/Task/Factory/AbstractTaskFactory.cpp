/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include <algorithm>

static fixed
GetOZSize(ObservationZonePoint *oz)
{
  switch (oz->shape) {
  case ObservationZonePoint::SECTOR:
    return ((SectorZone *)oz)->GetRadius();

  case ObservationZonePoint::LINE:
    return ((LineSectorZone *)oz)->GetLength();

  case ObservationZonePoint::CYLINDER:
    return ((CylinderZone *)oz)->GetRadius();

  case ObservationZonePoint::ANNULAR_SECTOR:
    return ((AnnularSectorZone *)oz)->GetRadius();

  default:
    return fixed_minus_one;
  }
}

OrderedTaskPoint*
AbstractTaskFactory::CreateMutatedPoint(const OrderedTaskPoint &tp,
                                        const LegalPointType newtype) const
{
  fixed ozsize = GetOZSize(tp.GetOZPoint());
  return CreatePoint(newtype, tp.GetWaypoint(), ozsize, ozsize, ozsize);
}

AbstractTaskFactory::LegalPointType
AbstractTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const
{
  const LegalPointType oldtype = GetType(tp);
  LegalPointType newtype = oldtype;

  switch (tp.GetType()) {
  case TaskPoint::START:
    if (!IsValidStartType(newtype)) {
      newtype = m_behaviour.sector_defaults.start_type;
      if (!IsValidStartType(newtype))
        newtype = *m_start_types.begin();
    }
    break;

  case TaskPoint::AST:
  case TaskPoint::AAT:
    if (!IsValidIntermediateType(newtype)) {
      newtype = m_behaviour.sector_defaults.turnpoint_type;
      if (!IsValidIntermediateType(newtype)) {
        newtype = *m_intermediate_types.begin();
      }
    }
    break;

  case TaskPoint::FINISH:
    if (!IsValidFinishType(newtype)) {
      newtype = m_behaviour.sector_defaults.finish_type;
      if (!IsValidFinishType(newtype))
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
AbstractTaskFactory::CreateStart(ObservationZonePoint* oz,
                                 const Waypoint& wp) const
{
  return new StartPoint(oz, wp, m_behaviour, GetOrderedTaskBehaviour());
}

FinishPoint*
AbstractTaskFactory::CreateFinish(ObservationZonePoint* oz,
                                  const Waypoint& wp) const
{
  return new FinishPoint(oz, wp, m_behaviour, GetOrderedTaskBehaviour());
}

AATPoint*
AbstractTaskFactory::CreateAATPoint(ObservationZonePoint* oz,
                               const Waypoint& wp) const
{
  return new AATPoint(oz, wp, m_behaviour, GetOrderedTaskBehaviour());
}

ASTPoint*
AbstractTaskFactory::CreateASTPoint(ObservationZonePoint* oz,
                               const Waypoint& wp) const
{
  return new ASTPoint(oz, wp, m_behaviour, GetOrderedTaskBehaviour());
}

StartPoint* 
AbstractTaskFactory::CreateStart(const Waypoint &wp) const
{
  LegalPointType type = m_behaviour.sector_defaults.start_type;
  if (!IsValidStartType(type))
    type = *m_start_types.begin();

  return CreateStart(type, wp);
}

IntermediateTaskPoint* 
AbstractTaskFactory::CreateIntermediate(const Waypoint &wp) const
{
  if (GetOrderedTaskBehaviour().homogeneous_tps && m_task.TaskSize() > 1) {
    LegalPointType type = GetType(*m_task.get_tp(1));
    if (IsValidIntermediateType(type))
      return CreateIntermediate(type, wp);
  }

  LegalPointType type = m_behaviour.sector_defaults.turnpoint_type;
  if (!IsValidIntermediateType(type))
    type = *m_intermediate_types.begin();

  return CreateIntermediate(type, wp);
}

FinishPoint* 
AbstractTaskFactory::CreateFinish(const Waypoint &wp) const
{
  LegalPointType type = m_behaviour.sector_defaults.finish_type;
  if (!IsValidFinishType(type))
    type = *m_finish_types.begin();

  return CreateFinish(type, wp);
}

AbstractTaskFactory::LegalPointType 
AbstractTaskFactory::GetType(const OrderedTaskPoint &point) const
{
  const ObservationZonePoint* oz = point.GetOZPoint();

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
AbstractTaskFactory::CreatePoint(const LegalPointType type,
                                 const Waypoint &wp) const
{
  return CreatePoint(type, wp, fixed_minus_one, fixed_minus_one, fixed_minus_one);
}

void
AbstractTaskFactory::GetPointDefaultSizes(const LegalPointType type,
                                          fixed &start_radius,
                                          fixed &turnpoint_radius,
                                          fixed &finish_radius) const
{
  TaskBehaviour ob = this->m_behaviour;

  if (start_radius < fixed_zero)
    start_radius = ob.sector_defaults.start_radius;

  if (turnpoint_radius < fixed_zero)
    turnpoint_radius = ob.sector_defaults.turnpoint_radius;

  if (finish_radius < fixed_zero)
    finish_radius = ob.sector_defaults.finish_radius;
}

OrderedTaskPoint*
AbstractTaskFactory::CreatePoint(const LegalPointType type,
                                 const Waypoint &wp,
                                 fixed start_radius,
                                 fixed turnpoint_radius,
                                 fixed finish_radius) const
{
  GetPointDefaultSizes(type, start_radius, turnpoint_radius, finish_radius);

  switch (type) {
  case START_SECTOR:
    return CreateStart(new FAISectorZone(wp.location, false), wp);
  case START_LINE:
    return CreateStart(new LineSectorZone(wp.location, start_radius), wp);
  case START_CYLINDER:
    return CreateStart(new CylinderZone(wp.location, start_radius), wp);
  case START_BGA:
    return CreateStart(new BGAStartSectorZone(wp.location), wp);
  case FAI_SECTOR:
    return CreateASTPoint(new FAISectorZone(wp.location, true), wp);
  case KEYHOLE_SECTOR:
    return CreateASTPoint(new KeyholeZone(wp.location), wp);
  case BGAFIXEDCOURSE_SECTOR:
    return CreateASTPoint(new BGAFixedCourseZone(wp.location), wp);
  case BGAENHANCEDOPTION_SECTOR:
    return CreateASTPoint(new BGAEnhancedOptionZone(wp.location), wp);
  case AST_CYLINDER:
    return CreateASTPoint(new CylinderZone(wp.location, turnpoint_radius), wp);
  case AAT_CYLINDER:
    return CreateAATPoint(new CylinderZone(wp.location, turnpoint_radius), wp);
  case AAT_SEGMENT:
    return CreateAATPoint(new SectorZone(wp.location, turnpoint_radius), wp);
  case AAT_ANNULAR_SECTOR:
    return CreateAATPoint(new AnnularSectorZone(wp.location, turnpoint_radius), wp);
  case FINISH_SECTOR:
    return CreateFinish(new FAISectorZone(wp.location, false), wp);
  case FINISH_LINE:
    return CreateFinish(new LineSectorZone(wp.location, finish_radius), wp);
  case FINISH_CYLINDER:
    return CreateFinish(new CylinderZone(wp.location, finish_radius), wp);
  }

  assert(1);
  return NULL;
}

StartPoint* 
AbstractTaskFactory::CreateStart(const LegalPointType type,
                                 const Waypoint &wp) const
{
  if (!IsValidStartType(type))
    // error, invalid type!
    return NULL;

  return (StartPoint*)CreatePoint(type, wp);
}

IntermediateTaskPoint* 
AbstractTaskFactory::CreateIntermediate(const LegalPointType type,
                                        const Waypoint &wp) const
{
  if (!IsValidIntermediateType(type))
    return NULL;

  return (IntermediateTaskPoint*)CreatePoint(type, wp);
}

FinishPoint* 
AbstractTaskFactory::CreateFinish(const LegalPointType type,
                                  const Waypoint &wp) const
{
  if (!IsValidFinishType(type))
    return NULL;

  return (FinishPoint*)CreatePoint(type, wp);
}

bool 
AbstractTaskFactory::Append(const OrderedTaskPoint &new_tp,
                            const bool auto_mutate)
{
  if (m_task.is_max_size())
    return false;

  if (auto_mutate) {
    if (!m_task.TaskSize()) {
      // empty task, so add as a start point
      if (IsValidType(new_tp, m_task.TaskSize())) {
        // candidate is ok, so add it
        return m_task.Append(new_tp);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = CreateStart(new_tp.GetWaypoint());
        bool success = m_task.Append(*sp);
        delete sp;
        return success;
      }
    }

    // non-empty task

    if (m_task.HasFinish()) {
      // old finish must be mutated into an intermediate point
      IntermediateTaskPoint* sp =
        CreateIntermediate(m_task.GetTaskPoint(m_task.TaskSize() - 1).GetWaypoint());

      m_task.Replace(*sp, m_task.TaskSize()-1);
      delete sp;
    }

    if (IsValidType(new_tp, m_task.TaskSize()))
      // ok to append directly
      return m_task.Append(new_tp);

    // this point must be mutated into a finish
    FinishPoint* sp = CreateFinish(new_tp.GetWaypoint());
    bool success = m_task.Append(*sp);
    delete sp;
    return success;
  }

  return m_task.Append(new_tp);
}

bool 
AbstractTaskFactory::Replace(const OrderedTaskPoint &new_tp,
                             const unsigned position,
                             const bool auto_mutate)
{
  if (auto_mutate) {
    if (IsValidType(new_tp, position))
      // ok to replace directly
      return m_task.Replace(new_tp, position);

    // will need to convert type of candidate
    OrderedTaskPoint *tp;
    if (position == 0) {
      // candidate must be transformed into a startpoint
      tp = CreateStart(new_tp.GetWaypoint());
    } else if (IsPositionFinish(position) &&
               position + 1 == m_task.TaskSize()) {
      // this point must be mutated into a finish
      tp = CreateFinish(new_tp.GetWaypoint());
    } else {
      // this point must be mutated into an intermediate
      tp = CreateIntermediate(new_tp.GetWaypoint());
    }

    bool success = m_task.Replace(*tp, position);
    delete tp;
    return success;
  }

  return m_task.Replace(new_tp, position);
}

bool 
AbstractTaskFactory::Insert(const OrderedTaskPoint &new_tp,
                            const unsigned position,
                            const bool auto_mutate)
{
  if (position >= m_task.TaskSize())
    return Append(new_tp, auto_mutate);

  if (auto_mutate) {
    if (position == 0) {
      if (m_task.HasStart()) {
        // old start must be mutated into an intermediate point
        IntermediateTaskPoint* sp =
          CreateIntermediate(m_task.GetTaskPoint(0).GetWaypoint());
        m_task.Replace(*sp, 0);
        delete sp;
      }

      if (IsValidType(new_tp, 0)) {
        return m_task.Insert(new_tp, 0);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = CreateStart(new_tp.GetWaypoint());
        bool success = m_task.Insert(*sp, 0);
        delete sp;
        return success;
      }
    } else {
      if (new_tp.IsIntermediatePoint()) {
        // candidate ok for direct insertion
        return m_task.Insert(new_tp, position);
      } else {
        // candidate must be transformed into a intermediatepoint
        IntermediateTaskPoint* sp = CreateIntermediate(new_tp.GetWaypoint());
        bool success = m_task.Insert(*sp, position);
        delete sp;
        return success;
      }
    }
  }

  return m_task.Insert(new_tp, position);
}

bool 
AbstractTaskFactory::Remove(const unsigned position, 
                            const bool auto_mutate)
{
  if (position >= m_task.TaskSize())
    return false;

  if (auto_mutate) {
    if (position == 0) {
      // special case, remove start point..
      if (m_task.TaskSize() == 1) {
        return m_task.Remove(0);
      } else {
        // create new start point from next point
        StartPoint* sp = CreateStart(m_task.GetTaskPoint(1).GetWaypoint());
        bool success = m_task.Remove(0) && m_task.Replace(*sp, 0);
        delete sp;
        return success;
      }
    } else if (IsPositionFinish(position - 1) && (position + 1 == m_task.TaskSize())) {
      // create new finish from previous point
      FinishPoint *sp =
        CreateFinish(m_task.GetTaskPoint(position - 1).GetWaypoint());
      bool success = m_task.Remove(position) &&
        m_task.Replace(*sp, position - 1);
      delete sp;
      return success;
    } else {
      // intermediate point deleted, nothing special to do
      return m_task.Remove(position);
    }
  }

  return m_task.Remove(position);
}

bool 
AbstractTaskFactory::Swap(const unsigned position, const bool auto_mutate)
{
  if (m_task.TaskSize() <= 1)
    return false;
  if (position >= m_task.TaskSize() - 1)
    return false;

  const OrderedTaskPoint &orig = m_task.GetTaskPoint(position + 1);
  if (!Insert(orig, position, auto_mutate))
    return false;

  return Remove(position+2, auto_mutate);
}

const OrderedTaskPoint&
AbstractTaskFactory::Relocate(const unsigned position, 
                              const Waypoint& waypoint)
{
  m_task.Relocate(position, waypoint);
  return m_task.GetTaskPoint(position);
}

const OrderedTaskBehaviour& 
AbstractTaskFactory::GetOrderedTaskBehaviour() const
{
  return m_task.get_ordered_task_behaviour();
}

void 
AbstractTaskFactory::UpdateOrderedTaskBehaviour(OrderedTaskBehaviour& to)
{
}

bool 
AbstractTaskFactory::IsPositionIntermediate(const unsigned position) const
{
  if (IsPositionStart(position))
    return false;
  if (position >= GetOrderedTaskBehaviour().max_points)
    return false;
  if (position + 1 < GetOrderedTaskBehaviour().min_points)
    return true;

  if (GetOrderedTaskBehaviour().IsFixedSize())
    return (position + 1 < GetOrderedTaskBehaviour().max_points);
  else if (m_task.TaskSize() < GetOrderedTaskBehaviour().min_points)
    return true;
  else
    return (position <= m_task.TaskSize());
}

bool 
AbstractTaskFactory::IsPositionFinish(const unsigned position) const
{
  if (IsPositionStart(position))
    return false;

  if (position + 1 < GetOrderedTaskBehaviour().min_points)
    return false;
  if (position + 1 > GetOrderedTaskBehaviour().max_points)
    return false;

  if (GetOrderedTaskBehaviour().IsFixedSize())
    return (position + 1 == GetOrderedTaskBehaviour().max_points);
  else
    return (position + 1 >= m_task.TaskSize());
}

bool
AbstractTaskFactory::ValidAbstractType(LegalAbstractPointType type, 
                                       const unsigned position) const
{
  const bool is_start = IsPositionStart(position);
  const bool is_finish = IsPositionFinish(position);
  const bool is_intermediate = IsPositionIntermediate(position);

  switch (type) {
  case POINT_START:
    return is_start;
  case POINT_FINISH:
    return is_finish;
  case POINT_AST:
    return is_intermediate &&
      (IsValidIntermediateType(FAI_SECTOR) 
       || IsValidIntermediateType(AST_CYLINDER)
       || IsValidIntermediateType(KEYHOLE_SECTOR)
       || IsValidIntermediateType(BGAFIXEDCOURSE_SECTOR)
       || IsValidIntermediateType(BGAENHANCEDOPTION_SECTOR));
  case POINT_AAT:
    return is_intermediate &&
      (IsValidIntermediateType(AAT_CYLINDER)
       || IsValidIntermediateType(AAT_SEGMENT)
       || IsValidIntermediateType(AAT_ANNULAR_SECTOR));
  };
  return false;
}

bool 
AbstractTaskFactory::IsValidType(const OrderedTaskPoint &new_tp,
                               unsigned position) const
{
  switch (new_tp.GetType()) {
  case TaskPoint::START:
    return ValidAbstractType(POINT_START, position) &&
        IsValidStartType(GetType(new_tp));

  case TaskPoint::AST:
    return ValidAbstractType(POINT_AST, position) &&
        IsValidIntermediateType(GetType(new_tp));

  case TaskPoint::AAT:
    return ValidAbstractType(POINT_AAT, position)&&
        IsValidIntermediateType(GetType(new_tp));

  case TaskPoint::FINISH:
    return ValidAbstractType(POINT_FINISH, position)&&
        IsValidFinishType(GetType(new_tp));

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
AbstractTaskFactory::IsValidIntermediateType(LegalPointType type) const 
{
  return (std::find(m_intermediate_types.begin(), m_intermediate_types.end(), type) 
          != m_intermediate_types.end());
}

bool
AbstractTaskFactory::IsValidStartType(LegalPointType type) const 
{
  return (std::find(m_start_types.begin(), m_start_types.end(), type) 
          != m_start_types.end());
}

bool
AbstractTaskFactory::IsValidFinishType(LegalPointType type) const 
{
  return (std::find(m_finish_types.begin(), m_finish_types.end(), type) 
          != m_finish_types.end());
}

AbstractTaskFactory::LegalPointVector 
AbstractTaskFactory::GetValidTypes(unsigned position) const
{
  LegalPointVector v;
  if (ValidAbstractType(POINT_START, position))
    v.insert(v.end(), m_start_types.begin(), m_start_types.end());

  LegalPointVector i = GetValidIntermediateTypes(position);
  if (!i.empty())
    v.insert(v.end(), i.begin(), i.end());

  if (ValidAbstractType(POINT_FINISH, position))
    v.insert(v.end(), m_finish_types.begin(), m_finish_types.end());

  return v;
}

void
AbstractTaskFactory::AddValidationError(TaskValidationErrorType e)
{
  m_validation_errors.push_back(e);
}

void
AbstractTaskFactory::ClearValidationErrors()
{
  m_validation_errors.clear();
}

AbstractTaskFactory::TaskValidationErrorVector
AbstractTaskFactory::GetValidationErrors()
{
  return m_validation_errors;
}

bool
AbstractTaskFactory::CheckAddFinish()
{
 if (m_task.TaskSize() < 2)
   return false;

 if (m_task.HasFinish())
   return false;

 FinishPoint *fp = CreateFinish(m_task.get_tp(m_task.TaskSize() - 1)->GetWaypoint());
 assert(fp);
 Remove(m_task.TaskSize() - 1, false);
 Append(*fp, false);
 delete fp;

 return true;
}

bool
AbstractTaskFactory::ValidateFAIOZs()
{
  ClearValidationErrors();
  bool valid = true;

  for (unsigned i = 0; i < m_task.TaskSize() && valid; i++) {
    const OrderedTaskPoint *tp = m_task.get_tp(i);
    const fixed ozsize = GetOZSize(tp->GetOZPoint());

    switch (GetType(*tp)) {
    case  START_BGA:
    case  START_CYLINDER:
      valid = false;
      break;

    case  START_SECTOR:
      if (ozsize > fixed(1000.01))
        valid = false;

      break;
    case  START_LINE:
      if (ozsize > fixed(2000.01))
        valid = false;

      break;

    case  FAI_SECTOR:
      break;

    case  AST_CYLINDER:
      if (ozsize > fixed(500.01))
        valid = false;

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
      if (ozsize > fixed(2000.01))
        valid = false;

      break;

    case  FINISH_CYLINDER:
      valid = false;
      break;
    }
  }

  if (!valid)
    AddValidationError(NON_FAI_OZS);

  return valid;
}

bool
AbstractTaskFactory::Validate()
{
  ClearValidationErrors();

  bool valid = true;

  if (!m_task.HasStart()) {
    AddValidationError(NO_VALID_START);
    valid = false;
  }
  if (!m_task.HasFinish()) {
    AddValidationError(NO_VALID_FINISH);
    valid = false;
  }

  if (GetOrderedTaskBehaviour().is_closed && !IsClosed()) {
    AddValidationError(TASK_NOT_CLOSED);
    valid = false;
  }

  if (GetOrderedTaskBehaviour().IsFixedSize()) {
    if (m_task.TaskSize() != GetOrderedTaskBehaviour().max_points) {
      AddValidationError(INCORRECT_NUMBER_TURNPOINTS);
      valid = false;
    }
  } else {
    if (m_task.TaskSize() < GetOrderedTaskBehaviour().min_points) {
      AddValidationError(UNDER_MIN_TURNPOINTS);
      valid = false;
    }
    if (m_task.TaskSize() > GetOrderedTaskBehaviour().max_points) {
      AddValidationError(EXCEEDS_MAX_TURNPOINTS);
      valid = false;
    }
  }

  if (GetOrderedTaskBehaviour().homogeneous_tps && !IsHomogeneous()) {
    AddValidationError(TASK_NOT_HOMOGENEOUS);
    valid = false;
  }

  return valid;
}

AbstractTaskFactory::LegalPointVector 
AbstractTaskFactory::GetValidIntermediateTypes(unsigned position) const
{
  LegalPointVector v;

  if (!IsPositionIntermediate(position))
    return v;

  if (GetOrderedTaskBehaviour().homogeneous_tps &&
      position > 1 && m_task.TaskSize() > 1) {
    LegalPointType type = GetType(*m_task.get_tp(1));
    if (IsValidIntermediateType(type)) {
      v.push_back(type);
      return v;
    }
  }

  if (ValidAbstractType(POINT_AAT, position) ||
      ValidAbstractType(POINT_AST, position))
    v.insert(v.end(), m_intermediate_types.begin(), m_intermediate_types.end());

  return v;
}

AbstractTaskFactory::LegalPointVector
AbstractTaskFactory::GetValidStartTypes() const
{
  LegalPointVector v;
  v.insert(v.end(), m_start_types.begin(), m_start_types.end());
  return v;
}

AbstractTaskFactory::LegalPointVector
AbstractTaskFactory::GetValidIntermediateTypes() const
{
  LegalPointVector v;
  v.insert(v.end(), m_intermediate_types.begin(), m_intermediate_types.end());
  return v;
}

AbstractTaskFactory::LegalPointVector
AbstractTaskFactory::GetValidFinishTypes() const
{
  LegalPointVector v;
  v.insert(v.end(), m_finish_types.begin(), m_finish_types.end());
  return v;
}

bool 
AbstractTaskFactory::IsClosed() const
{
  if (m_task.TaskSize() < 3)
    return false;

  const Waypoint& wp_start = m_task.get_tp(0)->GetWaypoint();
  const Waypoint& wp_finish =
      m_task.get_tp(m_task.TaskSize() - 1)->GetWaypoint();

  return (wp_start.location == wp_finish.location);
}

bool 
AbstractTaskFactory::IsUnique() const
{
  const unsigned size = m_task.TaskSize();
  for (unsigned i = 0; i + 1 < size; i++) {
    const Waypoint& wp_0 = m_task.get_tp(i)->GetWaypoint();

    for (unsigned j = i + 1; j < size; j++) {
      if (i == 0 && j + 1 == size) {
        // start point can be similar to finish point
      } else {
        const Waypoint& wp_1 = m_task.get_tp(j)->GetWaypoint();
        if (wp_1 == wp_0)
          return false;
      }
    }
  }
  return true;
}

bool
AbstractTaskFactory::IsHomogeneous() const
{
  bool valid = true;

  const unsigned size = m_task.TaskSize();

  if (size > 2) {
    LegalPointType homogtype = GetType(*m_task.get_tp(1));

    for (unsigned i = 2; i < size; i++) {
      OrderedTaskPoint *tp = m_task.get_tp(i);
      if ((tp->GetType() == TaskPoint::FINISH)) {
        ; // don't check a valid finish point
      } else {
        if (GetType(*tp) != homogtype) {
          valid = false;
          break;
        }
      }
    }
  }

  return valid;
}

bool
AbstractTaskFactory::RemoveExcessTPsPerTaskType()
{
  bool changed = false;
  unsigned maxtp = GetOrderedTaskBehaviour().max_points;
  while (maxtp < m_task.TaskSize()) {
    Remove(maxtp, false);
    changed = true;
  }
  return changed;
}

bool
AbstractTaskFactory::MutateTPsToTaskType()
{
  bool changed = RemoveExcessTPsPerTaskType();

  for (unsigned int i = 0; i < m_task.TaskSize(); i++) {
    OrderedTaskPoint *tp = m_task.get_tp(i);
    if (!IsValidType(*tp, i) ||
        (m_task.get_factory_type() == TaskFactoryType::FAI_GENERAL)) {

      LegalPointType newtype = GetMutatedPointType(*tp);
      if (IsPositionFinish(i)) {

        if (!IsValidFinishType(newtype)) {
          newtype = m_behaviour.sector_defaults.finish_type;
          if (!IsValidFinishType(newtype))
            newtype = *m_finish_types.begin();
        }

        FinishPoint *fp = (FinishPoint*)CreateMutatedPoint(*tp, newtype);
        assert(fp);
        if (Replace(*fp, i, true))
          changed = true;
        delete fp;

      } else if (i == 0) {
        if (!IsValidStartType(newtype)) {
          newtype = m_behaviour.sector_defaults.start_type;
          if (!IsValidStartType(newtype))
            newtype = *m_start_types.begin();
        }

        StartPoint *sp = (StartPoint*)CreateMutatedPoint(*tp, newtype);
        assert(sp);
        if (Replace(*sp, i, true))
          changed = true;
        delete sp;

      } else {

        if (!IsValidIntermediateType(newtype)) {
          newtype = m_behaviour.sector_defaults.turnpoint_type;
          if (!IsValidIntermediateType(newtype))
            newtype = *m_intermediate_types.begin();
        }
        OrderedTaskPoint *tpnew = (OrderedTaskPoint*)CreateMutatedPoint(*tp, newtype);
        if (Replace(*tpnew, i, true))
          changed = true;
        delete tpnew;
      }
    }
  }

  changed |= MutateClosedFinishPerTaskType();
  return changed;
}

bool
AbstractTaskFactory::MutateClosedFinishPerTaskType()
{
  if (m_task.TaskSize() < 2)
    return false;

  if (!IsPositionFinish(m_task.TaskSize() - 1))
    return false;

  bool changed = false;

  if (GetOrderedTaskBehaviour().is_closed) {
    if (!IsClosed()) {
      OrderedTaskPoint *tp = m_task.get_tp(m_task.TaskSize() - 1);
      assert(tp);
      if (tp->GetType() == TaskPoint::FINISH) {
        FinishPoint *fp = CreateFinish(m_task.get_tp(0)->GetWaypoint());
        assert(fp);
        Remove(m_task.TaskSize() - 1, false);
        Append(*fp, false);
        delete fp;
        changed = true;
      }
    }
  }
  return changed;
}

bool 
AbstractTaskFactory::AppendOptionalStart(const Waypoint& wp)
{
  OrderedTaskPoint* tp = NULL;
  if (m_task.TaskSize())
    tp = m_task.get_tp(0)->clone(m_behaviour, m_task.get_ordered_task_behaviour(), &wp);
  else
    tp = CreateStart(wp);

  if (!tp)
    return false; // should never happen

  bool success = m_task.AppendOptionalStart(*tp);
  delete tp;
  return success;
}

bool
AbstractTaskFactory::AppendOptionalStart(const OrderedTaskPoint &new_tp,
                                           const bool auto_mutate)
{
  if (auto_mutate && !IsValidType(new_tp, 0)) {
    // candidate must be transformed into a startpoint of appropriate type
    StartPoint* sp = CreateStart(new_tp.GetWaypoint());
    bool success = m_task.AppendOptionalStart(*sp);
    delete sp;
    return success;
  }
  // ok to add directly
  return m_task.AppendOptionalStart(new_tp);
}
