#include "Tasks/AbortTask.h"

AbortTask::AbortTask(const TaskEvents &te, TaskAdvance &ta):
  AbstractTask(te, ta)
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



double
AbortTask::calc_mc_best(const AIRCRAFT_STATE &, 
                        const double mc)
{
  // TODO
  return mc;
}

double
AbortTask::calc_cruise_efficiency(const AIRCRAFT_STATE &aircraft, 
                                  const double mc)
{
  return 1.0;
}

double
AbortTask::calc_min_target(const AIRCRAFT_STATE &, 
                           const double mc,
                           const double t_target)
{
  return 0.0;
}


bool 
AbortTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  return false; // nothing to do
}

double 
AbortTask::scan_distance_remaining(const GEOPOINT &location)
{
  return 0.0; // TODO
}
