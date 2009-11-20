#include "TaskManager.hpp"
#include "Visitors/TaskVisitor.hpp"

// uses delegate pattern

TaskManager::TaskManager(const TaskEvents &te,
                         const TaskBehaviour &tb,
                         GlidePolar &gp,
                         const Waypoints &wps): 
    task_ordered(te,tb,task_advance,gp),
    task_goto(te,tb,task_advance,gp),
    task_abort(te,tb,task_advance,gp,wps),
    task_behaviour(tb),
    factory_fai(task_ordered,tb),
    factory_aat(task_ordered,tb),
    factory_mixed(task_ordered,tb)
{
    set_mode(MODE_ORDERED);
    set_factory(FACTORY_FAI);
}

TaskManager::TaskMode_t 
TaskManager::set_mode(const TaskMode_t the_mode)
{
  switch(the_mode) {
  case (MODE_ABORT):
    active_task = &task_abort;
    mode = MODE_ABORT;
    break;
  case (MODE_ORDERED):
    if (task_ordered.task_size()) {
      active_task = &task_ordered;
      mode = MODE_ORDERED;
      break;
    }
  case (MODE_GOTO):
    if (task_goto.getActiveTaskPoint()) {
      active_task = &task_goto;
      mode = MODE_GOTO;
      break;
    }
  case (MODE_NULL):
    active_task = NULL;
    mode = MODE_NULL;
    break;
  };
  return mode;
}

void TaskManager::setActiveTaskPoint(unsigned index)
{
  if (active_task) active_task->setActiveTaskPoint(index);
}

TaskPoint* TaskManager::getActiveTaskPoint()
{
  if (active_task) 
    return active_task->getActiveTaskPoint();
  else 
    return NULL;
}


bool TaskManager::update(const AIRCRAFT_STATE &state, 
                         const AIRCRAFT_STATE& state_last)
{
  // always update ordered task so even if we are temporarily
  // in abort/goto mode, the task stats are still updated

  bool retval = task_ordered.update(state, state_last);
  if (active_task && (active_task != &task_ordered)) {
    retval |= active_task->update(state, state_last);
  }
  return retval;
}

bool 
TaskManager::update_idle(const AIRCRAFT_STATE& state)
{
  if (active_task) {
    return active_task->update_idle(state);
  } else {
    return false;
  }
}


const TaskStats& TaskManager::get_stats() const
{
  if (active_task) {
    return active_task->get_stats();
  } else {
    return null_stats;
  }
}

void
TaskManager::abort()
{
  set_mode(MODE_ABORT);
}

void
TaskManager::resume()
{
  set_mode(MODE_ORDERED);
}

void
TaskManager::do_goto(const Waypoint & wp)
{
  task_goto.do_goto(wp);
  set_mode(MODE_GOTO);
}

bool 
TaskManager::append(OrderedTaskPoint *new_tp)
{
  return task_ordered.append(new_tp);
}

bool 
TaskManager::replace(OrderedTaskPoint *new_tp, const unsigned position)
{
  return task_ordered.replace(new_tp, position);
}

bool 
TaskManager::insert(OrderedTaskPoint *new_tp, const unsigned position)
{
  return task_ordered.insert(new_tp, position);
}

bool 
TaskManager::remove(const unsigned position)
{
  return task_ordered.remove(position);
}

bool 
TaskManager::check_task() const
{
  return task_ordered.check_task();
}

void
TaskManager::Accept(BaseVisitor& visitor) const
{
  if (active_task) active_task->Accept(visitor);
}

void
TaskManager::reset()
{
  task_ordered.reset();
}

TaskManager::Factory_t 
TaskManager::set_factory(const Factory_t the_factory)
{
  if ((the_factory != factory_mode) && (the_factory != FACTORY_MIXED)) {
    // can switch from anything to mixed, otherwise need reset
    task_ordered.reset();

    /// \todo call into task_events to ask if reset is desired on factory change
  }
  factory_mode = the_factory;
  switch (factory_mode) {
  case FACTORY_FAI:
    active_factory = &factory_fai;
    break;
  case FACTORY_AAT:
    active_factory = &factory_aat;
    break;
  case FACTORY_MIXED:
    active_factory = &factory_mixed;
    break;
  };
  return factory_mode;
}


unsigned 
TaskManager::get_task_size() const
{
  return task_ordered.task_size();
}

GEOPOINT 
TaskManager::random_point_in_task(const unsigned index) const
{
  
  if (index< get_task_size()) {
    return task_ordered.getTaskPoint(index)->randomPointInSector();
  }
  GEOPOINT null_location;
  return null_location;
}
