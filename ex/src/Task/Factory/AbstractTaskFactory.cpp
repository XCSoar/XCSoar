#include "AbstractTaskFactory.hpp"

#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include <algorithm>


#include "Task/Visitors/TaskPointVisitor.hpp"

class TaskPointCloneVisitor: 
  public BaseVisitor,
  public Visitor<StartPoint>,
  public Visitor<ASTPoint>,
  public Visitor<AATPoint>,
  public Visitor<FinishPoint>
{
public:
  TaskPointCloneVisitor(const TaskBehaviour& tb,
                        const TaskProjection &tp,
                        const Waypoint *_wp):
    m_task_behaviour(tb),
    m_task_projection(tp),
    m_retval(NULL),
    m_waypoint(_wp)
    {}
  OrderedTaskPoint* Visit(const OrderedTaskPoint &taskpoint) {
    if (!m_waypoint) {
      m_waypoint = &taskpoint.get_waypoint();
    }
    taskpoint.Accept(*this);
    return m_retval;
  }
  
private:
  virtual void Visit(const FinishPoint& tp) {
    m_retval= new FinishPoint(tp.get_oz()->clone(&m_waypoint->Location),
                              m_task_projection,*m_waypoint,m_task_behaviour);
  }
  virtual void Visit(const StartPoint& tp) {
    m_retval= new StartPoint(tp.get_oz()->clone(&m_waypoint->Location),
                             m_task_projection,*m_waypoint,m_task_behaviour);
  }
  virtual void Visit(const AATPoint& tp) {
    m_retval= new AATPoint(tp.get_oz()->clone(&m_waypoint->Location),
                           m_task_projection,*m_waypoint,m_task_behaviour);
  }
  virtual void Visit(const ASTPoint& tp) {
    m_retval= new ASTPoint(tp.get_oz()->clone(&m_waypoint->Location),
                           m_task_projection,*m_waypoint,m_task_behaviour);
  }
  const TaskBehaviour &m_task_behaviour;
  const TaskProjection &m_task_projection;
  OrderedTaskPoint* m_retval;
  const Waypoint* m_waypoint;
};


OrderedTaskPoint* 
AbstractTaskFactory::clone(const OrderedTaskPoint& tp, const Waypoint* waypoint) const
{
  TaskPointCloneVisitor tpcv(m_behaviour, m_task.get_task_projection(), waypoint);
  return tpcv.Visit(tp);
}

StartPoint* 
AbstractTaskFactory::createStart(const Waypoint &wp) const
{
  return createStart(*m_start_types.begin(), wp);
}

IntermediatePoint* 
AbstractTaskFactory::createIntermediate(const Waypoint &wp) const
{
  return createIntermediate(*m_intermediate_types.begin(), wp);
}

FinishPoint* 
AbstractTaskFactory::createFinish(const Waypoint &wp) const
{
  return createFinish(*m_finish_types.begin(), wp);
}

StartPoint* 
AbstractTaskFactory::createStart(const LegalStartType_t type,
                                 const Waypoint &wp) const
{
  if (std::find(m_start_types.begin(), m_start_types.end(),type) 
      == m_start_types.end()) {
    // error, invalid type!
    return NULL;
  }
  switch (type) {
  case START_SECTOR:
    return new StartPoint(new FAISectorZone(wp.Location),
                          m_task.get_task_projection(),wp,m_behaviour);
    break;
  case START_LINE:
    return new StartPoint(new LineSectorZone(wp.Location),
                          m_task.get_task_projection(),wp,m_behaviour);
    break;
  case START_CYLINDER:
    return new StartPoint(new CylinderZone(wp.Location),
                          m_task.get_task_projection(),wp,m_behaviour);
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
  if (std::find(m_intermediate_types.begin(), m_intermediate_types.end(),type) 
      == m_intermediate_types.end()) {
    return NULL;
  }
  switch (type) {
  case FAI_SECTOR:
    return new ASTPoint(new FAISectorZone(wp.Location),
                        m_task.get_task_projection(),wp,m_behaviour);
    break;
  case AST_CYLINDER:
    return new ASTPoint(new CylinderZone(wp.Location),
                        m_task.get_task_projection(),wp,m_behaviour);
    break;
  case AAT_CYLINDER:
    return new AATPoint(new CylinderZone(wp.Location),
                        m_task.get_task_projection(),wp,m_behaviour);
    break;
  case AAT_SEGMENT:
    return new AATPoint(new SectorZone(wp.Location),
                        m_task.get_task_projection(),wp,m_behaviour);
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
  if (std::find(m_finish_types.begin(), m_finish_types.end(),type) 
      == m_finish_types.end()) {
    return NULL;
  }
  switch (type) {
  case FINISH_SECTOR:
    return new FinishPoint(new FAISectorZone(wp.Location),
                          m_task.get_task_projection(),wp,m_behaviour);
    break;
  case FINISH_LINE:
    return new FinishPoint(new LineSectorZone(wp.Location),
                          m_task.get_task_projection(),wp,m_behaviour);
    break;
  case FINISH_CYLINDER:
    return new FinishPoint(new CylinderZone(wp.Location),
                          m_task.get_task_projection(),wp,m_behaviour);
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
  } else if (position+1>=m_task.task_size()) {
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
    if (!m_task.task_size()) {
      // empty task, so add as a start point
      if (validType(new_tp, m_task.task_size())) {
        // candidate is ok, so add it
        return m_task.append(new_tp);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.append(sp);
      }
    }

    // non-empty task

    if (m_task.has_finish()) {
      // old finish must be mutated into an intermediate point
      IntermediatePoint* sp = createIntermediate(m_task.getTaskPoint(m_task.task_size()-1)
                                                 ->get_waypoint());
      m_task.replace(sp, m_task.task_size()-1);
    }

    if (validType(new_tp, m_task.task_size())) {
      // ok to append directly
      return m_task.append(new_tp);
    } else {
      // this point must be mutated into a finish
      FinishPoint* sp = createFinish(new_tp->get_waypoint());
      // delete original since we own it now
      delete new_tp;
      return m_task.append(sp);
    }
  } else {
    return m_task.append(new_tp);
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
      return m_task.replace(new_tp, position);
    } else {
      // will need to convert type of candidate

      if (position==0) {

        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.replace(sp, position);

      } else if (position+1==m_task.task_size()) {

        // this point must be mutated into a finish
        FinishPoint* sp = createFinish(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.replace(sp, position);

      } else {

        // this point must be mutated into an intermediate
        IntermediatePoint* sp = createIntermediate(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.replace(sp, position);
      }
    }
  } else {
    return m_task.replace(new_tp, position);
  }
}

bool 
AbstractTaskFactory::insert(OrderedTaskPoint *new_tp, const unsigned position,
                            const bool auto_mutate)
{
  if (!new_tp) return false;

  if (position>= m_task.task_size()) {
    return append(new_tp, auto_mutate);
  }

  if (auto_mutate) {

    if (position==0) {
      if (m_task.has_start()) {
        // old start must be mutated into an intermediate point
        IntermediatePoint* sp = createIntermediate(m_task.getTaskPoint(0)->get_waypoint());
        m_task.replace(sp, 0);
      }
      if (validType(new_tp, 0)) {
        return m_task.insert(new_tp, 0);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.insert(sp, 0);
      }
    } else {
      if (dynamic_cast<IntermediatePoint*>(new_tp) != NULL) {
        // candidate ok for direct insertion
        return m_task.insert(new_tp, position);
      } else {
        // candidate must be transformed into a intermediatepoint
        IntermediatePoint* sp = createIntermediate(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.insert(sp, position);
      }
    }
  } else {
    return m_task.insert(new_tp, position);
  }
}

bool 
AbstractTaskFactory::remove(const unsigned position, 
                            const bool auto_mutate)
{
  if (position>= m_task.task_size()) {
    return false;
  }

  if (auto_mutate) {

    if (position==0) {
      // special case, remove start point..
      if (m_task.task_size()==1) {
        return m_task.remove(0);
      } else {
        // create new start point from next point
        StartPoint* sp = createStart(m_task.getTaskPoint(1)->get_waypoint());
        return m_task.remove(0) && m_task.replace(sp,0);
      }
    } else if (position+1 == m_task.task_size()) {
      // create new finish from previous point
      FinishPoint* sp = createFinish(m_task.getTaskPoint(position-1)->get_waypoint());
      return m_task.remove(position) && m_task.replace(sp,position-1);
    } else {
      // intermediate point deleted, nothing special to do
      return m_task.remove(position);
    }
  } else {
    return m_task.remove(position);
  }

}

bool 
AbstractTaskFactory::has_entered(unsigned position) const
{
  if (m_task.getTaskPoint(position)) {
    return m_task.getTaskPoint(position)->has_entered();
  } else {
    return true;
  }
}
