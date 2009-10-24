#include "Tasks/AbortTask.h"

AbortTask::AbortTask(const TaskEvents &te, 
                     TaskAdvance &ta,
                     GlidePolar &gp):
  AbstractTask(te, ta, gp)
{

}

AbortTask::~AbortTask()
{
  // TODO delete tps
}

void AbortTask::setActiveTaskPoint(unsigned index)
{
  if (index<tps.size()) {
    activeTaskPoint = index;
  }
}

TaskPoint* AbortTask::getActiveTaskPoint()
{
  if (activeTaskPoint<tps.size()) {
    return tps[activeTaskPoint];
  } else {
    return NULL;
  }
}


void AbortTask::report(const AIRCRAFT_STATE &state)
{

}

bool AbortTask::update_sample(const AIRCRAFT_STATE &state, 
                              const bool full_update)
{
  // TODO, update aborted task list
  return false; // nothing to do
}


bool 
AbortTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  return false; // nothing to do
}

