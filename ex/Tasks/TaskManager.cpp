#include "Tasks/TaskManager.h"

// uses delegate pattern

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

void TaskManager::report(const AIRCRAFT_STATE &state)
{
  // for testing only:
  task_abort.report(state);

  if (active_task) 
    return active_task->report(state);
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

  // for testing only:
  task_abort.update(state, state_last);

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
TaskManager::do_goto(const WAYPOINT & wp)
{
  task_goto.do_goto(wp);
  set_mode(MODE_GOTO);
}

