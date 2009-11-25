#include "AbstractTaskFactory.hpp"

#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include <algorithm>

StartPoint* 
AbstractTaskFactory::createStart(const Waypoint &wp) const
{
  const LegalStartType_t type = *start_types.begin();
  return createStart(type, wp);
}

IntermediatePoint* 
AbstractTaskFactory::createIntermediate(const Waypoint &wp) const
{
  return createIntermediate(*intermediate_types.begin(), wp);
}

FinishPoint* 
AbstractTaskFactory::createFinish(const Waypoint &wp) const
{
  return createFinish(*finish_types.begin(), wp);
}

StartPoint* 
AbstractTaskFactory::createStart(const LegalStartType_t type,
                                 const Waypoint &wp) const
{
  if (std::find(start_types.begin(), start_types.end(),type) 
      == start_types.end()) {
    // error, invalid type!
    return NULL;
  }
  switch (type) {
  case START_SECTOR:
    return new StartPoint(new FAISectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case START_LINE:
    return new StartPoint(new LineSectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case START_CYLINDER:
    return new StartPoint(new CylinderZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  default:
    assert(1);
  };
  return NULL;
}

IntermediatePoint* 
AbstractTaskFactory::createIntermediate(const LegalIntermediateType_t type,
                                        const Waypoint &wp) const
{
  if (std::find(intermediate_types.begin(), intermediate_types.end(),type) 
      == intermediate_types.end()) {
    return NULL;
  }
  switch (type) {
  case FAI_SECTOR:
    return new ASTPoint(new FAISectorZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  case AST_CYLINDER:
    return new ASTPoint(new CylinderZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  case AAT_CYLINDER:
    return new AATPoint(new CylinderZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  case AAT_SEGMENT:
    return new AATPoint(new SectorZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  default:
    assert(1);
  };
  return NULL;
}

FinishPoint* 
AbstractTaskFactory::createFinish(const LegalFinishType_t type,
                                 const Waypoint &wp) const
{
  if (std::find(finish_types.begin(), finish_types.end(),type) 
      == finish_types.end()) {
    return NULL;
  }
  switch (type) {
  case FINISH_SECTOR:
    return new FinishPoint(new FAISectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case FINISH_LINE:
    return new FinishPoint(new LineSectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case FINISH_CYLINDER:
    return new FinishPoint(new CylinderZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  default:
    assert(1);
  };
  return NULL;
}

bool 
AbstractTaskFactory::validType(OrderedTaskPoint *new_tp, unsigned position) const
{
  if (position==0) {
    return (NULL != dynamic_cast<StartPoint*>(new_tp));
  } else if (position+1>=task.task_size()) {
    return (NULL != dynamic_cast<FinishPoint*>(new_tp));
  } else {
    return (NULL != dynamic_cast<IntermediatePoint*>(new_tp));
  }
}


bool 
AbstractTaskFactory::append(OrderedTaskPoint *new_tp, const bool auto_mutate)
{
  if (!new_tp) return false;

  if (auto_mutate) {
    if (!task.task_size()) {
      // empty task, so add as a start point
      if (validType(new_tp, task.task_size())) {
        // candidate is ok, so add it
        return task.append(new_tp);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return task.append(sp);
      }
    }

    // non-empty task

    if (task.has_finish()) {
      // old finish must be mutated into an intermediate point
      IntermediatePoint* sp = createIntermediate(task.getTaskPoint(task.task_size()-1)
                                                 ->get_waypoint());
      task.replace(sp, task.task_size()-1);
    }

    if (validType(new_tp, task.task_size())) {
      // ok to append directly
      return task.append(new_tp);
    } else {
      // this point must be mutated into a finish
      FinishPoint* sp = createFinish(new_tp->get_waypoint());
      // delete original since we own it now
      delete new_tp;
      return task.append(sp);
    }
  } else {
    return task.append(new_tp);
  }
}

bool 
AbstractTaskFactory::replace(OrderedTaskPoint *new_tp, const unsigned position,
                             const bool auto_mutate)
{
  if (!new_tp) return false;

  if (auto_mutate) {
    if (validType(new_tp, position)) {
      // ok to replace directly
      return task.replace(new_tp, position);
    } else {
      // will need to convert type of candidate

      if (position==0) {

        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return task.replace(sp, position);

      } else if (position+1==task.task_size()) {

        // this point must be mutated into a finish
        FinishPoint* sp = createFinish(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return task.replace(sp, position);

      } else {

        // this point must be mutated into an intermediate
        IntermediatePoint* sp = createIntermediate(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return task.replace(sp, position);
      }
    }
  } else {
    return task.replace(new_tp, position);
  }
}

bool 
AbstractTaskFactory::insert(OrderedTaskPoint *new_tp, const unsigned position,
                            const bool auto_mutate)
{
  if (!new_tp) return false;

  if (position>= task.task_size()) {
    return append(new_tp, auto_mutate);
  }

  if (auto_mutate) {

    if (position==0) {
      if (task.has_start()) {
        // old start must be mutated into an intermediate point
        IntermediatePoint* sp = createIntermediate(task.getTaskPoint(0)->get_waypoint());
        task.replace(sp, 0);
      }
      if (validType(new_tp, 0)) {
        return task.insert(new_tp, 0);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return task.insert(sp, 0);
      }
    } else {
      if (dynamic_cast<IntermediatePoint*>(new_tp) != NULL) {
        // candidate ok for direct insertion
        return task.insert(new_tp, position);
      } else {
        // candidate must be transformed into a intermediatepoint
        IntermediatePoint* sp = createIntermediate(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return task.insert(sp, position);
      }
    }
  } else {
    return task.insert(new_tp, position);
  }
}

bool 
AbstractTaskFactory::remove(const unsigned position, 
                            const bool auto_mutate)
{
  if (position>= task.task_size()) {
    return false;
  }

  if (auto_mutate) {

    if (position==0) {
      // special case, remove start point..
      if (task.task_size()==1) {
        return task.remove(0);
      } else {
        // create new start point from next point
        StartPoint* sp = createStart(task.getTaskPoint(1)->get_waypoint());
        return task.remove(0) && task.replace(sp,0);
      }
    } else if (position+1 == task.task_size()) {
      // create new finish from previous point
      FinishPoint* sp = createFinish(task.getTaskPoint(position-1)->get_waypoint());
      return task.remove(position) && task.replace(sp,position-1);
    } else {
      // intermediate point deleted, nothing special to do
      return task.remove(position);
    }
  } else {
    return task.remove(position);
  }

}

bool 
AbstractTaskFactory::has_entered(unsigned position) const
{
  if (task.getTaskPoint(position)) {
    return task.getTaskPoint(position)->has_entered();
  } else {
    return true;
  }
}
