#include "TaskClient.hpp"
#include "Task/TaskManager.hpp"

Mutex TaskClient::mutex;

TaskClient::TaskClient(TaskManager& the_manager):
  task_manager(the_manager)
{

}


GlidePolar 
TaskClient::get_glide_polar() const
{
  ScopeLock lock(mutex);
  return task_manager.get_glide_polar();
}

void 
TaskClient::set_glide_polar(const GlidePolar& glide_polar)
{
  ScopeLock lock(mutex);
  task_manager.set_glide_polar(glide_polar);
}


void
TaskClient::lock()
{
  mutex.Lock();
}

void
TaskClient::unlock()
{
  mutex.Unlock();
}

bool 
TaskClient::check_task() const
{
  ScopeLock lock(mutex);
  return task_manager.check_task();
}

TaskManager::TaskMode_t 
TaskClient::get_mode() const
{
  ScopeLock lock(mutex);
  return task_manager.get_mode();
}

TracePointVector 
TaskClient::find_trace_points(const GEOPOINT &loc, 
                              const fixed range,
                              const unsigned mintime, 
                              const fixed resolution) const
{
  ScopeLock lock(mutex);
  return task_manager.find_trace_points(loc, range, mintime, resolution);
}


void 
TaskClient::CAccept(TaskVisitor &visitor) const
{
  ScopeLock lock(mutex);
  task_manager.CAccept(visitor);
}

void 
TaskClient::ordered_CAccept(TaskVisitor &visitor) const
{
  ScopeLock lock(mutex);
  task_manager.ordered_CAccept(visitor);
}

const OrderedTaskBehaviour 
TaskClient::get_ordered_task_behaviour() const
{
  ScopeLock lock(mutex);
  return task_manager.get_ordered_task_behaviour();
}
