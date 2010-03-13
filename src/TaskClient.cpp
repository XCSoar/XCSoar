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

