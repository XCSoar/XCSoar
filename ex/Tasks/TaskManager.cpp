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
}

void TaskManager::report(const GEOPOINT &location)
{
  switch(mode) {
  case (MODE_NULL):
    return;
  case (MODE_GOTO):
    return task_goto.report(location);
  case (MODE_ORDERED):
    return task_ordered.report(location);
  case (MODE_ABORT):
    return task_abort.report(location);
  };
}

bool TaskManager::update_sample(const AIRCRAFT_STATE &state, 
                                const AIRCRAFT_STATE& state_last)
{
  // TODO: always update ordered task so even if we are temporarily
  // in abort/goto mode, the task stats are still updated

  switch(mode) {
  case (MODE_NULL):
    return false;
  case (MODE_GOTO):
    return task_goto.update_sample(state, state_last);
  case (MODE_ORDERED):
    return task_ordered.update_sample(state, state_last);
  case (MODE_ABORT):
    return task_abort.update_sample(state, state_last);
  };
}

