#include "TaskClientCalc.hpp"
#include "Task/TaskManager.hpp"

void 
TaskClientCalc::reset()
{
  ScopeLock lock(mutex);
  task_manager.reset();
}

bool 
TaskClientCalc::update(const AIRCRAFT_STATE &state_now, 
                       const AIRCRAFT_STATE &state_last) 
{
  ScopeLock lock(mutex);
  return task_manager.update(state_now, state_last);
}

bool
TaskClientCalc::update_idle(const AIRCRAFT_STATE &state)
{
  ScopeLock lock(mutex);
  return task_manager.update_idle(state);
}

bool 
TaskClientCalc::update_auto_mc(const AIRCRAFT_STATE& state_now,
                               const fixed fallback_mc)
{
  ScopeLock lock(mutex);
  return task_manager.update_auto_mc(state_now, fallback_mc);
}


const TaskStats& 
TaskClientCalc::get_stats() const
{
  ScopeLock lock(mutex);
  return task_manager.get_stats();
}

const CommonStats& 
TaskClientCalc::get_common_stats() const
{
  ScopeLock lock(mutex);
  return task_manager.get_common_stats();
}

void
TaskClientCalc::set_task_behaviour(const TaskBehaviour& behaviour)
{
  ScopeLock lock(mutex);
  task_manager.set_task_behaviour(behaviour);
}
