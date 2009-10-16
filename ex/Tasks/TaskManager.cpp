#include "Tasks/TaskManager.h"

// uses delegate pattern

void
TaskManager::set_mode(const TaskMode_t the_mode)
{
  mode = the_mode;
  switch(mode) {
  case (MODE_NULL):
    active_task = NULL;
    return;
  case (MODE_GOTO):
    active_task = &task_goto;
    return;
  case (MODE_ORDERED):
    active_task = &task_ordered;
    return;
  case (MODE_ABORT):
    active_task = &task_abort;
    return;
  };
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
