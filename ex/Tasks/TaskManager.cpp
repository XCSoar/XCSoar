#include "Tasks/TaskManager.h"

void TaskManager::setActiveTaskPoint(unsigned index)
{
  switch(mode) {
  case (MODE_NULL):
  case (MODE_GOTO):
    // nothing to do
    return;
  case (MODE_ORDERED):
    task_ordered.setActiveTaskPoint(index);
    return;
  case (MODE_ABORT):
    task_abort.setActiveTaskPoint(index);
    return;
  };
}

TaskPoint* TaskManager::getActiveTaskPoint()
{
  switch(mode) {
  case (MODE_NULL):
    return NULL;
  case (MODE_GOTO):
    return task_goto.getActiveTaskPoint();
  case (MODE_ORDERED):
    return task_ordered.getActiveTaskPoint();
  case (MODE_ABORT):
    return task_abort.getActiveTaskPoint();
  };
  return NULL;
  // should never get here
}

void TaskManager::report(const AIRCRAFT_STATE &state)
{
  switch(mode) {
  case (MODE_NULL):
    return;
  case (MODE_GOTO):
    return task_goto.report(state);
  case (MODE_ORDERED):
    return task_ordered.report(state);
  case (MODE_ABORT):
    return task_abort.report(state);
  };
}

bool TaskManager::update(const AIRCRAFT_STATE &state, 
                         const AIRCRAFT_STATE& state_last)
{
  // TODO: always update ordered task so even if we are temporarily
  // in abort/goto mode, the task stats are still updated

  switch(mode) {
  case (MODE_NULL):
    return false;
  case (MODE_GOTO):
    return task_goto.update(state, state_last);
  case (MODE_ORDERED):
    return task_ordered.update(state, state_last);
  case (MODE_ABORT):
    return task_abort.update(state, state_last);
  };
  // should never get here
  return false;
}

const TaskStats& TaskManager::get_stats() const
{
  switch(mode) {
  case (MODE_NULL):
    return null_stats;
  case (MODE_GOTO):
    return task_goto.get_stats();
  case (MODE_ORDERED):
    return task_ordered.get_stats();
  case (MODE_ABORT):
    return task_abort.get_stats();
  };
  // should never get here
  return null_stats;
}
