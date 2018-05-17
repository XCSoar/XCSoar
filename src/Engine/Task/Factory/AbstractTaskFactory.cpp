/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Constraints.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Task/Ordered/Points/ASTPoint.hpp"
#include "Task/Ordered/Points/FinishPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"

#include <algorithm>

static double
GetOZSize(const ObservationZonePoint &oz)
{
  switch (oz.GetShape()) {
  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    return ((const SectorZone &)oz).GetRadius();

  case ObservationZone::Shape::LINE:
    return ((const LineSectorZone &)oz).GetLength();

  case ObservationZone::Shape::CYLINDER:
  case ObservationZone::Shape::MAT_CYLINDER:
    return ((const CylinderZone &)oz).GetRadius();

  case ObservationZone::Shape::ANNULAR_SECTOR:
    return ((const AnnularSectorZone &)oz).GetRadius();

  default:
    return -1;
  }
}

OrderedTaskPoint*
AbstractTaskFactory::CreateMutatedPoint(const OrderedTaskPoint &tp,
                                        const TaskPointFactoryType newtype) const
{
  auto ozsize = GetOZSize(tp.GetObservationZone());
  return CreatePoint(newtype, tp.GetWaypointPtr(), ozsize, ozsize, ozsize);
}

TaskPointFactoryType
AbstractTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const
{
  const TaskPointFactoryType oldtype = GetType(tp);
  TaskPointFactoryType newtype = oldtype;

  switch (tp.GetType()) {
  case TaskPointType::START:
    if (!IsValidStartType(newtype))
      newtype = GetDefaultStartType();
    break;

  case TaskPointType::AST:
  case TaskPointType::AAT:
    if (!IsValidIntermediateType(newtype))
      newtype = GetDefaultIntermediateType();
    break;

  case TaskPointType::FINISH:
    if (!IsValidFinishType(newtype))
      newtype = GetDefaultFinishType();
    break;

  case TaskPointType::UNORDERED:
    break;
  }
  return newtype;
}

StartPoint*
AbstractTaskFactory::CreateStart(ObservationZonePoint* oz,
                                 WaypointPtr wp) const
{
  assert(wp);

  return new StartPoint(oz, std::move(wp), behaviour,
                        GetOrderedTaskSettings().start_constraints);
}

FinishPoint*
AbstractTaskFactory::CreateFinish(ObservationZonePoint* oz,
                                  WaypointPtr wp) const
{
  assert(wp);

  return new FinishPoint(oz, std::move(wp), behaviour,
                         GetOrderedTaskSettings().finish_constraints);
}

AATPoint*
AbstractTaskFactory::CreateAATPoint(ObservationZonePoint* oz,
                                    WaypointPtr wp) const
{
  assert(wp);

  return new AATPoint(oz, std::move(wp), behaviour);
}

ASTPoint*
AbstractTaskFactory::CreateASTPoint(ObservationZonePoint* oz,
                                    WaypointPtr wp) const
{
  assert(wp);

  return new ASTPoint(oz, std::move(wp), behaviour);
}

StartPoint* 
AbstractTaskFactory::CreateStart(WaypointPtr wp) const
{
  assert(wp);

  return CreateStart(GetDefaultStartType(), std::move(wp));
}

IntermediateTaskPoint* 
AbstractTaskFactory::CreateIntermediate(WaypointPtr wp) const
{
  assert(wp);

  if (constraints.homogeneous_tps && task.TaskSize() > 1) {
    TaskPointFactoryType type = GetType(task.GetPoint(1));
    if (IsValidIntermediateType(type))
      return CreateIntermediate(type, std::move(wp));
  }

  return CreateIntermediate(GetDefaultIntermediateType(), std::move(wp));
}

FinishPoint* 
AbstractTaskFactory::CreateFinish(WaypointPtr wp) const
{
  assert(wp);

  return CreateFinish(GetDefaultFinishType(), std::move(wp));
}

TaskPointFactoryType 
AbstractTaskFactory::GetType(const OrderedTaskPoint &point) const
{
  const ObservationZonePoint &oz = point.GetObservationZone();

  switch (point.GetType()) {
  case TaskPointType::START:
    switch (oz.GetShape()) {
    case ObservationZone::Shape::FAI_SECTOR:
    case ObservationZone::Shape::SYMMETRIC_QUADRANT:
      return TaskPointFactoryType::START_SECTOR;

    case ObservationZone::Shape::LINE:
      return TaskPointFactoryType::START_LINE;

    case ObservationZone::Shape::CYLINDER:
    case ObservationZone::Shape::MAT_CYLINDER:
    case ObservationZone::Shape::SECTOR:
    case ObservationZone::Shape::DAEC_KEYHOLE:
    case ObservationZone::Shape::CUSTOM_KEYHOLE:
    case ObservationZone::Shape::BGAFIXEDCOURSE:
    case ObservationZone::Shape::BGAENHANCEDOPTION:
    case ObservationZone::Shape::ANNULAR_SECTOR:
      return TaskPointFactoryType::START_CYLINDER;

    case ObservationZone::Shape::BGA_START:
      return TaskPointFactoryType::START_BGA;
    }
    break;

  case TaskPointType::AAT:
    switch (oz.GetShape()) {
    case ObservationZone::Shape::SECTOR:
    case ObservationZone::Shape::FAI_SECTOR:
    case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    case ObservationZone::Shape::DAEC_KEYHOLE:
    case ObservationZone::Shape::BGAFIXEDCOURSE:
    case ObservationZone::Shape::BGAENHANCEDOPTION:
    case ObservationZone::Shape::BGA_START:
    case ObservationZone::Shape::LINE:
      return TaskPointFactoryType::AAT_SEGMENT;
    case ObservationZone::Shape::ANNULAR_SECTOR:
      return TaskPointFactoryType::AAT_ANNULAR_SECTOR;
    case ObservationZone::Shape::CYLINDER:
      return TaskPointFactoryType::AAT_CYLINDER;

    case ObservationZone::Shape::CUSTOM_KEYHOLE:
      return TaskPointFactoryType::AAT_KEYHOLE;

    case ObservationZone::Shape::MAT_CYLINDER:
      return TaskPointFactoryType::MAT_CYLINDER;
    }
    break;

  case TaskPointType::AST:
    switch (oz.GetShape()) {
    case ObservationZone::Shape::FAI_SECTOR:
      return TaskPointFactoryType::FAI_SECTOR;

    case ObservationZone::Shape::DAEC_KEYHOLE:
    case ObservationZone::Shape::CUSTOM_KEYHOLE:
      return TaskPointFactoryType::KEYHOLE_SECTOR;

    case ObservationZone::Shape::BGAFIXEDCOURSE:
      return TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR;

    case ObservationZone::Shape::BGAENHANCEDOPTION:
      return TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR;

    case ObservationZone::Shape::BGA_START:
    case ObservationZone::Shape::CYLINDER:
    case ObservationZone::Shape::MAT_CYLINDER:
    case ObservationZone::Shape::SECTOR:
    case ObservationZone::Shape::LINE:
    case ObservationZone::Shape::ANNULAR_SECTOR:
      return TaskPointFactoryType::AST_CYLINDER;

    case ObservationZone::Shape::SYMMETRIC_QUADRANT:
      return TaskPointFactoryType::SYMMETRIC_QUADRANT;
    }
    break;

  case TaskPointType::FINISH:
    switch (oz.GetShape()) {
    case ObservationZone::Shape::BGA_START:
    case ObservationZone::Shape::FAI_SECTOR:
    case ObservationZone::Shape::SYMMETRIC_QUADRANT:
      return TaskPointFactoryType::FINISH_SECTOR;

    case ObservationZone::Shape::LINE:
      return TaskPointFactoryType::FINISH_LINE;

    case ObservationZone::Shape::CYLINDER:
    case ObservationZone::Shape::MAT_CYLINDER:
    case ObservationZone::Shape::SECTOR:
    case ObservationZone::Shape::DAEC_KEYHOLE:
    case ObservationZone::Shape::CUSTOM_KEYHOLE:
    case ObservationZone::Shape::BGAFIXEDCOURSE:
    case ObservationZone::Shape::BGAENHANCEDOPTION:
    case ObservationZone::Shape::ANNULAR_SECTOR:
      return TaskPointFactoryType::FINISH_CYLINDER;
    }
    break;

  case TaskPointType::UNORDERED:
    /* obviously, when we check the type of an OrderedTaskPoint, we
       should never get type==UNORDERED */
    gcc_unreachable();
    break;
  }

  // fail, should never get here
  gcc_unreachable();
}

OrderedTaskPoint* 
AbstractTaskFactory::CreatePoint(const TaskPointFactoryType type,
                                 WaypointPtr wp) const
{
  return CreatePoint(type, std::move(wp), -1, -1, -1);
}

void
AbstractTaskFactory::GetPointDefaultSizes(const TaskPointFactoryType type,
                                          double &start_radius,
                                          double &turnpoint_radius,
                                          double &finish_radius) const
{
  TaskBehaviour ob = this->behaviour;

  if (start_radius < 0)
    start_radius = ob.sector_defaults.start_radius;

  if (turnpoint_radius < 0)
    turnpoint_radius = ob.sector_defaults.turnpoint_radius;

  if (finish_radius < 0)
    finish_radius = ob.sector_defaults.finish_radius;
}

OrderedTaskPoint*
AbstractTaskFactory::CreatePoint(const TaskPointFactoryType type,
                                 WaypointPtr wp,
                                 double start_radius,
                                 double turnpoint_radius,
                                 double finish_radius) const
{
  assert(wp);

  GetPointDefaultSizes(type, start_radius, turnpoint_radius, finish_radius);

  const GeoPoint location = wp->location;
  switch (type) {
  case TaskPointFactoryType::START_SECTOR:
    return CreateStart(SymmetricSectorZone::CreateFAISectorZone(location,
                                                                false),
                       std::move(wp));
  case TaskPointFactoryType::START_LINE:
    return CreateStart(new LineSectorZone(location, start_radius),
                       std::move(wp));
  case TaskPointFactoryType::START_CYLINDER:
    return CreateStart(new CylinderZone(location, start_radius),
                       std::move(wp));
  case TaskPointFactoryType::START_BGA:
    return CreateStart(KeyholeZone::CreateBGAStartSectorZone(location),
                       std::move(wp));
  case TaskPointFactoryType::FAI_SECTOR:
    return CreateASTPoint(SymmetricSectorZone::CreateFAISectorZone(location,
                                                                   true),
                          std::move(wp));
  case TaskPointFactoryType::SYMMETRIC_QUADRANT:
    return CreateASTPoint(new SymmetricSectorZone(location,
                                                  turnpoint_radius),
                          std::move(wp));
  case TaskPointFactoryType::KEYHOLE_SECTOR:
    return CreateASTPoint(KeyholeZone::CreateDAeCKeyholeZone(location),
                          std::move(wp));
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
    return CreateASTPoint(KeyholeZone::CreateBGAFixedCourseZone(location),
                          std::move(wp));
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
    return CreateASTPoint(KeyholeZone::CreateBGAEnhancedOptionZone(location),
                          std::move(wp));
  case TaskPointFactoryType::AST_CYLINDER:
    return CreateASTPoint(new CylinderZone(location, turnpoint_radius),
                          std::move(wp));
  case TaskPointFactoryType::MAT_CYLINDER:
    return CreateAATPoint(CylinderZone::CreateMatCylinderZone(location),
                          std::move(wp));
  case TaskPointFactoryType::AAT_CYLINDER:
    return CreateAATPoint(new CylinderZone(location, turnpoint_radius),
                          std::move(wp));
  case TaskPointFactoryType::AAT_SEGMENT:
    return CreateAATPoint(new SectorZone(location, turnpoint_radius),
                          std::move(wp));
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
    return CreateAATPoint(new AnnularSectorZone(location, turnpoint_radius),
                          std::move(wp));
  case TaskPointFactoryType::AAT_KEYHOLE:
    return CreateAATPoint(KeyholeZone::CreateCustomKeyholeZone(location,
                                                               turnpoint_radius,
                                                               Angle::QuarterCircle()),
                          std::move(wp));
  case TaskPointFactoryType::FINISH_SECTOR:
    return CreateFinish(SymmetricSectorZone::CreateFAISectorZone(location,
                                                                 false),
                        std::move(wp));
  case TaskPointFactoryType::FINISH_LINE:
    return CreateFinish(new LineSectorZone(location, finish_radius),
                        std::move(wp));
  case TaskPointFactoryType::FINISH_CYLINDER:
    return CreateFinish(new CylinderZone(location, finish_radius),
                        std::move(wp));

  case TaskPointFactoryType::COUNT:
    gcc_unreachable();
  }

  gcc_unreachable();
}

StartPoint* 
AbstractTaskFactory::CreateStart(const TaskPointFactoryType type,
                                 WaypointPtr wp) const
{
  assert(wp);

  if (!IsValidStartType(type))
    // error, invalid type!
    return NULL;

  return (StartPoint *)CreatePoint(type, std::move(wp));
}

IntermediateTaskPoint* 
AbstractTaskFactory::CreateIntermediate(const TaskPointFactoryType type,
                                        WaypointPtr wp) const
{
  assert(wp);

  if (!IsValidIntermediateType(type))
    return NULL;

  return (IntermediateTaskPoint *)CreatePoint(type, std::move(wp));
}

FinishPoint* 
AbstractTaskFactory::CreateFinish(const TaskPointFactoryType type,
                                  WaypointPtr wp) const
{
  assert(wp);

  if (!IsValidFinishType(type))
    return NULL;

  return (FinishPoint *)CreatePoint(type, std::move(wp));
}

bool 
AbstractTaskFactory::Append(const OrderedTaskPoint &new_tp,
                            const bool auto_mutate)
{
  if (task.IsFull())
    return false;

  if (auto_mutate) {
    if (!task.TaskSize()) {
      // empty task, so add as a start point
      if (IsValidType(new_tp, task.TaskSize())) {
        // candidate is ok, so add it
        return task.Append(new_tp);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = CreateStart(new_tp.GetWaypointPtr());
        bool success = task.Append(*sp);
        delete sp;
        return success;
      }
    }

    // non-empty task

    if (task.HasFinish()) {
      // old finish must be mutated into an intermediate point
      IntermediateTaskPoint* sp =
        CreateIntermediate(task.GetTaskPoint(task.TaskSize() - 1).GetWaypointPtr());

      task.Replace(*sp, task.TaskSize()-1);
      delete sp;
    }

    if (IsValidType(new_tp, task.TaskSize()))
      // ok to append directly
      return task.Append(new_tp);

    // this point must be mutated into a finish
    FinishPoint* sp = CreateFinish(new_tp.GetWaypointPtr());
    bool success = task.Append(*sp);
    delete sp;
    return success;
  }

  return task.Append(new_tp);
}

bool 
AbstractTaskFactory::Replace(const OrderedTaskPoint &new_tp,
                             const unsigned position,
                             const bool auto_mutate)
{
  if (auto_mutate) {
    if (IsValidType(new_tp, position))
      // ok to replace directly
      return task.Replace(new_tp, position);

    // will need to convert type of candidate
    OrderedTaskPoint *tp;
    if (position == 0) {
      // candidate must be transformed into a startpoint
      tp = CreateStart(new_tp.GetWaypointPtr());
    } else if (IsPositionFinish(position) &&
               position + 1 == task.TaskSize()) {
      // this point must be mutated into a finish
      tp = CreateFinish(new_tp.GetWaypointPtr());
    } else {
      // this point must be mutated into an intermediate
      tp = CreateIntermediate(new_tp.GetWaypointPtr());
    }

    bool success = task.Replace(*tp, position);
    delete tp;
    return success;
  }

  return task.Replace(new_tp, position);
}

bool 
AbstractTaskFactory::Insert(const OrderedTaskPoint &new_tp,
                            const unsigned position,
                            const bool auto_mutate)
{
  if (position >= task.TaskSize())
    return Append(new_tp, auto_mutate);

  if (auto_mutate) {
    if (position == 0) {
      if (task.HasStart()) {
        // old start must be mutated into an intermediate point
        IntermediateTaskPoint* sp =
          CreateIntermediate(task.GetTaskPoint(0).GetWaypointPtr());
        task.Replace(*sp, 0);
        delete sp;
      }

      if (IsValidType(new_tp, 0)) {
        return task.Insert(new_tp, 0);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = CreateStart(new_tp.GetWaypointPtr());
        bool success = task.Insert(*sp, 0);
        delete sp;
        return success;
      }
    } else {
      if (new_tp.IsIntermediatePoint()) {
        // candidate ok for direct insertion
        return task.Insert(new_tp, position);
      } else {
        // candidate must be transformed into a intermediatepoint
        IntermediateTaskPoint* sp = CreateIntermediate(new_tp.GetWaypointPtr());
        bool success = task.Insert(*sp, position);
        delete sp;
        return success;
      }
    }
  }

  return task.Insert(new_tp, position);
}

bool 
AbstractTaskFactory::Remove(const unsigned position, 
                            const bool auto_mutate)
{
  if (position >= task.TaskSize())
    return false;

  if (auto_mutate) {
    if (position == 0) {
      // special case, remove start point..
      if (task.TaskSize() == 1) {
        return task.Remove(0);
      } else {
        // create new start point from next point
        StartPoint* sp = CreateStart(task.GetTaskPoint(1).GetWaypointPtr());
        bool success = task.Remove(0) && task.Replace(*sp, 0);
        delete sp;
        return success;
      }
    } else if (IsPositionFinish(position - 1) &&
               position + 1 == task.TaskSize()) {
      // create new finish from previous point
      FinishPoint *sp =
        CreateFinish(task.GetTaskPoint(position - 1).GetWaypointPtr());
      bool success = task.Remove(position) &&
        task.Replace(*sp, position - 1);
      delete sp;
      return success;
    } else {
      // intermediate point deleted, nothing special to do
      return task.Remove(position);
    }
  }

  return task.Remove(position);
}

bool 
AbstractTaskFactory::Swap(const unsigned position, const bool auto_mutate)
{
  if (task.TaskSize() <= 1)
    return false;
  if (position >= task.TaskSize() - 1)
    return false;

  const OrderedTaskPoint &orig = task.GetTaskPoint(position + 1);
  if (!Insert(orig, position, auto_mutate))
    return false;

  return Remove(position+2, auto_mutate);
}

const OrderedTaskPoint&
AbstractTaskFactory::Relocate(const unsigned position, 
                              WaypointPtr &&waypoint)
{
  task.Relocate(position, std::move(waypoint));
  return task.GetTaskPoint(position);
}

const OrderedTaskSettings &
AbstractTaskFactory::GetOrderedTaskSettings() const
{
  return task.GetOrderedTaskSettings();
}

void
AbstractTaskFactory::UpdateOrderedTaskSettings(OrderedTaskSettings &to)
{
  to.start_constraints.require_arm = constraints.start_requires_arm;
  to.finish_constraints.fai_finish = constraints.fai_finish;
}

bool 
AbstractTaskFactory::IsPositionIntermediate(const unsigned position) const
{
  if (IsPositionStart(position))
    return false;
  if (position >= constraints.max_points)
    return false;
  if (position + 1 < constraints.min_points)
    return true;

  if (constraints.IsFixedSize())
    return (position + 1 < constraints.max_points);
  else if (task.TaskSize() < constraints.min_points)
    return true;
  else
    return (position <= task.TaskSize());
}

bool 
AbstractTaskFactory::IsPositionFinish(const unsigned position) const
{
  if (IsPositionStart(position))
    return false;

  if (position + 1 < constraints.min_points)
    return false;
  if (position + 1 > constraints.max_points)
    return false;

  if (constraints.IsFixedSize())
    return (position + 1 == constraints.max_points);
  else
    return (position + 1 >= task.TaskSize());
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
      (IsValidIntermediateType(TaskPointFactoryType::FAI_SECTOR) 
       || IsValidIntermediateType(TaskPointFactoryType::AST_CYLINDER)
       || IsValidIntermediateType(TaskPointFactoryType::KEYHOLE_SECTOR)
       || IsValidIntermediateType(TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR)
       || IsValidIntermediateType(TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR));
  case POINT_AAT:
    return is_intermediate &&
      (IsValidIntermediateType(TaskPointFactoryType::AAT_CYLINDER)
       || IsValidIntermediateType(TaskPointFactoryType::MAT_CYLINDER)
       || IsValidIntermediateType(TaskPointFactoryType::AAT_SEGMENT)
       || IsValidIntermediateType(TaskPointFactoryType::AAT_ANNULAR_SECTOR));
  };
  return false;
}

bool 
AbstractTaskFactory::IsValidType(const OrderedTaskPoint &new_tp,
                               unsigned position) const
{
  switch (new_tp.GetType()) {
  case TaskPointType::START:
    return ValidAbstractType(POINT_START, position) &&
        IsValidStartType(GetType(new_tp));

  case TaskPointType::AST:
    return ValidAbstractType(POINT_AST, position) &&
        IsValidIntermediateType(GetType(new_tp));

  case TaskPointType::AAT:
    return ValidAbstractType(POINT_AAT, position)&&
        IsValidIntermediateType(GetType(new_tp));

  case TaskPointType::FINISH:
    return ValidAbstractType(POINT_FINISH, position)&&
        IsValidFinishType(GetType(new_tp));

  case TaskPointType::UNORDERED:
    /* obviously, when we check the type of an OrderedTaskPoint, we
       should never get type==UNORDERED */
    gcc_unreachable();
  }

  gcc_unreachable();
}

TaskPointFactoryType
AbstractTaskFactory::GetDefaultStartType() const
{
  TaskPointFactoryType type = behaviour.sector_defaults.start_type;
  if (!IsValidStartType(type) && !start_types.IsEmpty())
    type = start_types.UncheckedFirst();

  return type;
}

TaskPointFactoryType
AbstractTaskFactory::GetDefaultIntermediateType() const
{
  TaskPointFactoryType type = behaviour.sector_defaults.turnpoint_type;
  if (!IsValidIntermediateType(type) && !intermediate_types.IsEmpty())
    type = intermediate_types.UncheckedFirst();

  return type;
}

TaskPointFactoryType
AbstractTaskFactory::GetDefaultFinishType() const
{
  TaskPointFactoryType type = behaviour.sector_defaults.finish_type;
  if (!IsValidFinishType(type) && !finish_types.IsEmpty())
    type = finish_types.UncheckedFirst();

  return type;
}

LegalPointSet
AbstractTaskFactory::GetValidTypes(unsigned position) const
{
  LegalPointSet v;
  if (ValidAbstractType(POINT_START, position))
    v |= start_types;

  v |= GetValidIntermediateTypes(position);

  if (ValidAbstractType(POINT_FINISH, position))
    v |= finish_types;

  return v;
}

bool
AbstractTaskFactory::CheckAddFinish()
{
 if (task.TaskSize() < 2)
   return false;

 if (task.HasFinish())
   return false;

 FinishPoint *fp = CreateFinish(task.GetPoint(task.TaskSize() - 1).GetWaypointPtr());
 assert(fp);
 Remove(task.TaskSize() - 1, false);
 Append(*fp, false);
 delete fp;

 return true;
}

bool
AbstractTaskFactory::ValidateFAIOZs()
{
  ClearValidationErrors();
  bool valid = true;

  for (unsigned i = 0; i < task.TaskSize() && valid; i++) {
    const auto &tp = task.GetPoint(i);
    const auto ozsize = GetOZSize(tp.GetObservationZone());

    switch (GetType(tp)) {
    case TaskPointFactoryType::START_BGA:
    case TaskPointFactoryType::START_CYLINDER:
      valid = false;
      break;

    case TaskPointFactoryType::START_SECTOR:
      if (ozsize > 1000.01)
        valid = false;

      break;
    case TaskPointFactoryType::START_LINE:
      if (ozsize > 2000.01)
        valid = false;

      break;

    case TaskPointFactoryType::FAI_SECTOR:
      break;

    case TaskPointFactoryType::AST_CYLINDER:
      if (ozsize > 500.01)
        valid = false;

      break;

    case TaskPointFactoryType::KEYHOLE_SECTOR:
    case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
    case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
    case TaskPointFactoryType::MAT_CYLINDER:
    case TaskPointFactoryType::AAT_CYLINDER:
    case TaskPointFactoryType::AAT_SEGMENT:
    case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
    case TaskPointFactoryType::AAT_KEYHOLE:
    case TaskPointFactoryType::SYMMETRIC_QUADRANT:
      valid = false;
      break;

    case TaskPointFactoryType::FINISH_SECTOR:
      break;
    case TaskPointFactoryType::FINISH_LINE:
      if (ozsize > 2000.01)
        valid = false;

      break;

    case TaskPointFactoryType::FINISH_CYLINDER:
      valid = false;
      break;

    case TaskPointFactoryType::COUNT:
      gcc_unreachable();
    }
  }

  if (!valid)
    AddValidationError(TaskValidationErrorType::NON_FAI_OZS);

  return valid;
}

bool
AbstractTaskFactory::ValidateMATOZs()
{
  ClearValidationErrors();
  bool valid = true;

  for (unsigned i = 0; i < task.TaskSize() && valid; i++) {
    const OrderedTaskPoint &tp = task.GetPoint(i);

    switch (GetType(tp)) {
    case TaskPointFactoryType::START_CYLINDER:
    case TaskPointFactoryType::START_LINE:
    case TaskPointFactoryType::START_SECTOR:
      break;

    case TaskPointFactoryType::START_BGA:
      valid = false;
      break;

    case TaskPointFactoryType::MAT_CYLINDER:
      break;

    case TaskPointFactoryType::AAT_CYLINDER:
    case TaskPointFactoryType::FAI_SECTOR:
    case TaskPointFactoryType::AST_CYLINDER:
    case TaskPointFactoryType::KEYHOLE_SECTOR:
    case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
    case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
    case TaskPointFactoryType::AAT_SEGMENT:
    case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
    case TaskPointFactoryType::AAT_KEYHOLE:
    case TaskPointFactoryType::FINISH_SECTOR:
    case TaskPointFactoryType::SYMMETRIC_QUADRANT:
      valid = false;
      break;

    case TaskPointFactoryType::FINISH_LINE:
    case TaskPointFactoryType::FINISH_CYLINDER:
      break;

    case TaskPointFactoryType::COUNT:
      gcc_unreachable();
    }
  }

  if (!valid)
    AddValidationError(TaskValidationErrorType::NON_MAT_OZS);

  return valid;
}

bool
AbstractTaskFactory::Validate()
{
  ClearValidationErrors();

  bool valid = true;

  if (!task.HasStart()) {
    AddValidationError(TaskValidationErrorType::NO_VALID_START);
    valid = false;
  }
  if (!task.HasFinish()) {
    AddValidationError(TaskValidationErrorType::NO_VALID_FINISH);
    valid = false;
  }

  if (constraints.is_closed && !IsClosed()) {
    AddValidationError(TaskValidationErrorType::TASK_NOT_CLOSED);
    valid = false;
  }

  if (constraints.IsFixedSize()) {
    if (task.TaskSize() != constraints.max_points) {
      AddValidationError(TaskValidationErrorType::INCORRECT_NUMBER_TURNPOINTS);
      valid = false;
    }
  } else {
    if (task.TaskSize() < constraints.min_points) {
      AddValidationError(TaskValidationErrorType::UNDER_MIN_TURNPOINTS);
      valid = false;
    }
    if (task.TaskSize() > constraints.max_points) {
      AddValidationError(TaskValidationErrorType::EXCEEDS_MAX_TURNPOINTS);
      valid = false;
    }
  }

  if (constraints.homogeneous_tps && !IsHomogeneous()) {
    AddValidationError(TaskValidationErrorType::TASK_NOT_HOMOGENEOUS);
    valid = false;
  }

  return valid;
}

LegalPointSet
AbstractTaskFactory::GetValidIntermediateTypes(unsigned position) const
{
  if (!IsPositionIntermediate(position))
    return LegalPointSet();

  if (constraints.homogeneous_tps &&
      position > 1 && task.TaskSize() > 1) {
    TaskPointFactoryType type = GetType(task.GetPoint(1));
    if (IsValidIntermediateType(type))
      return LegalPointSet(type);
  }

  if (ValidAbstractType(POINT_AAT, position) ||
      ValidAbstractType(POINT_AST, position))
    return intermediate_types;

  return LegalPointSet();
}

bool 
AbstractTaskFactory::IsClosed() const
{
  if (task.TaskSize() < 3)
    return false;

  const auto wp_start = task.GetPoint(0).GetWaypointPtr();
  const auto wp_finish = task.GetPoint(task.TaskSize() - 1).GetWaypointPtr();

  return wp_start->location == wp_finish->location;
}

bool 
AbstractTaskFactory::IsUnique() const
{
  const unsigned size = task.TaskSize();
  for (unsigned i = 0; i + 1 < size; i++) {
    const auto wp_0 = task.GetPoint(i).GetWaypointPtr();

    for (unsigned j = i + 1; j < size; j++) {
      if (i == 0 && j + 1 == size) {
        // start point can be similar to finish point
      } else {
        const auto wp_1 = task.GetPoint(j).GetWaypointPtr();
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

  const unsigned size = task.TaskSize();

  if (size > 2) {
    TaskPointFactoryType homogtype = GetType(task.GetPoint(1));

    for (unsigned i = 2; i < size; i++) {
      const OrderedTaskPoint &tp = task.GetPoint(i);
      if (tp.GetType() == TaskPointType::FINISH) {
        ; // don't check a valid finish point
      } else {
        if (GetType(tp) != homogtype) {
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
  unsigned maxtp = constraints.max_points;
  while (maxtp < task.TaskSize()) {
    Remove(maxtp, false);
    changed = true;
  }
  return changed;
}

bool
AbstractTaskFactory::MutateTPsToTaskType()
{
  bool changed = RemoveExcessTPsPerTaskType();

  for (unsigned int i = 0; i < task.TaskSize(); i++) {
    const OrderedTaskPoint &tp = task.GetPoint(i);
    if (!IsValidType(tp, i) ||
        task.GetFactoryType() == TaskFactoryType::MAT ||
        task.GetFactoryType() == TaskFactoryType::FAI_GENERAL) {

      TaskPointFactoryType newtype = GetMutatedPointType(tp);
      if (IsPositionFinish(i)) {

        if (!IsValidFinishType(newtype))
          newtype = GetDefaultFinishType();

        FinishPoint *fp = (FinishPoint*)CreateMutatedPoint(tp, newtype);
        assert(fp);
        if (Replace(*fp, i, true))
          changed = true;
        delete fp;

      } else if (i == 0) {
        if (!IsValidStartType(newtype))
          newtype = GetDefaultStartType();

        StartPoint *sp = (StartPoint*)CreateMutatedPoint(tp, newtype);
        assert(sp);
        if (Replace(*sp, i, true))
          changed = true;
        delete sp;

      } else {

        if (!IsValidIntermediateType(newtype))
          newtype = GetDefaultIntermediateType();

        OrderedTaskPoint *tpnew = (OrderedTaskPoint*)CreateMutatedPoint(tp, newtype);
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
  if (task.TaskSize() < 2)
    return false;

  if (!IsPositionFinish(task.TaskSize() - 1))
    return false;

  bool changed = false;

  if (constraints.is_closed) {
    if (!IsClosed()) {
      const OrderedTaskPoint &tp = task.GetPoint(task.TaskSize() - 1);
      if (tp.GetType() == TaskPointType::FINISH) {
        FinishPoint *fp = CreateFinish(task.GetPoint(0).GetWaypointPtr());
        assert(fp);
        Remove(task.TaskSize() - 1, false);
        Append(*fp, false);
        delete fp;
        changed = true;
      }
    }
  }
  return changed;
}

bool 
AbstractTaskFactory::AppendOptionalStart(WaypointPtr wp)
{
  OrderedTaskPoint* tp = NULL;
  if (task.TaskSize())
    tp = task.GetPoint(0).Clone(behaviour, GetOrderedTaskSettings(),
                                std::move(wp));
  else
    tp = CreateStart(std::move(wp));

  if (!tp)
    return false; // should never happen

  bool success = task.AppendOptionalStart(*tp);
  delete tp;
  return success;
}

bool
AbstractTaskFactory::AppendOptionalStart(const OrderedTaskPoint &new_tp,
                                           const bool auto_mutate)
{
  if (auto_mutate && !IsValidType(new_tp, 0)) {
    // candidate must be transformed into a startpoint of appropriate type
    StartPoint* sp = CreateStart(new_tp.GetWaypointPtr());
    bool success = task.AppendOptionalStart(*sp);
    delete sp;
    return success;
  }
  // ok to add directly
  return task.AppendOptionalStart(new_tp);
}

void
AbstractTaskFactory::UpdateStatsGeometry()
{
  task.UpdateStatsGeometry();
}

void
AbstractTaskFactory::UpdateGeometry()
{
  task.UpdateGeometry();
}
