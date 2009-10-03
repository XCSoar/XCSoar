#include "Tasks/AbortTask.h"

AbortTask::AbortTask()
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


void AbortTask::report(const GEOPOINT &location)
{

}

bool AbortTask::update_sample(const AIRCRAFT_STATE &state, 
                              const AIRCRAFT_STATE& state_last)
{
  // TODO, update aborted task list
  return false; // nothing to do
}

