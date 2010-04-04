#include "TaskClientUI.hpp"
#include "Util/Serialiser.hpp"
#include "Util/DataNodeXML.hpp"
#include "LocalPath.hpp"

#include "Task/TaskAdvance.hpp"

TaskAdvance::TaskAdvanceState_t 
TaskClientUI::get_advance_state() const
{
  ScopeLock lock(mutex);
  return task_manager.get_task_advance().get_advance_state();
}

/*
TaskAdvance::TaskAdvanceMode_t 
TaskClientUI::get_advance_mode() const
{
  ScopeLock lock(mutex);
  return task_manager.get_task_advance().get_mode();
}

void 
TaskClientUI::set_advance_mode(TaskAdvance::TaskAdvanceMode_t the_mode)
{
  ScopeLock lock(mutex);
  task_manager.get_task_advance().set_mode(the_mode);
}
*/

void 
TaskClientUI::set_advance_armed(const bool do_armed)
{
  ScopeLock lock(mutex);
  task_manager.get_task_advance().set_armed(do_armed);
}

bool 
TaskClientUI::is_advance_armed() const
{
  ScopeLock lock(mutex);
  return task_manager.get_task_advance().is_armed();
}

bool 
TaskClientUI::toggle_advance_armed()
{
  ScopeLock lock(mutex);
  return task_manager.get_task_advance().toggle_armed();
}


GlidePolar 
TaskClientUI::get_safety_polar() const
{
  ScopeLock lock(mutex);
  return task_manager.get_safety_polar();
}


const Waypoint* 
TaskClientUI::getActiveWaypoint() const
{
  ScopeLock lock(mutex);
  TaskPoint* tp = task_manager.getActiveTaskPoint();
  if (tp) {
    return &tp->get_waypoint();
  } else {
    return NULL;
  }
}


void 
TaskClientUI::incrementActiveTaskPoint(int offset)
{
  ScopeLock lock(mutex);
  task_manager.incrementActiveTaskPoint(offset);
}


TaskManager::TaskMode_t 
TaskClientUI::get_mode() const
{
  ScopeLock lock(mutex);
  return task_manager.get_mode();
}

bool 
TaskClientUI::do_goto(const Waypoint & wp)
{
  ScopeLock lock(mutex);
  return task_manager.do_goto(wp);
}

void 
TaskClientUI::abort()
{
  ScopeLock lock(mutex);
  task_manager.abort();
}

void 
TaskClientUI::resume()
{
  ScopeLock lock(mutex);
  task_manager.resume();
}


AIRCRAFT_STATE 
TaskClientUI::get_start_state() const
{
  ScopeLock lock(mutex);
  return task_manager.get_start_state();
}

fixed 
TaskClientUI::get_finish_height() const
{
  ScopeLock lock(mutex);
  return task_manager.get_finish_height();
}


const TracePointVector 
TaskClientUI::get_trace_points()
{
  ScopeLock lock(mutex);
  return task_manager.get_trace_points();
}

const TracePointVector 
TaskClientUI::get_olc_points()
{
  ScopeLock lock(mutex);
  return task_manager.get_olc_points();
}


bool 
TaskClientUI::check_ordered_task() const
{
  ScopeLock lock(mutex);
  return task_manager.check_ordered_task();
}

bool 
TaskClientUI::check_task() const
{
  ScopeLock lock(mutex);
  return task_manager.check_task();
}


GEOPOINT 
TaskClientUI::get_task_center(const GEOPOINT& fallback_location) const
{
  ScopeLock lock(mutex);
  return task_manager.get_task_center(fallback_location);
}

fixed 
TaskClientUI::get_task_radius(const GEOPOINT& fallback_location) const
{
  ScopeLock lock(mutex);
  return task_manager.get_task_radius(fallback_location);
}


void 
TaskClientUI::CAccept(BaseVisitor& visitor) const
{
  ScopeLock lock(mutex);
  task_manager.CAccept(visitor);
}

void 
TaskClientUI::ordered_CAccept(BaseVisitor& visitor) const
{
  ScopeLock lock(mutex);
  task_manager.ordered_CAccept(visitor);
}

TracePointVector 
TaskClientUI::find_trace_points(const GEOPOINT &loc, 
                                const fixed range,
                                const unsigned mintime, 
                                const fixed resolution) const
{
  ScopeLock lock(mutex);
  return task_manager.find_trace_points(loc, range, mintime, resolution);
}

OrderedTask*
TaskClientUI::task_clone()
{
  ScopeLock lock(mutex);
  glide_polar = task_manager.get_glide_polar();
  return task_manager.clone(task_events,
                            task_behaviour,
                            glide_polar);
}

OrderedTask* 
TaskClientUI::task_copy(const OrderedTask& that)
{
  ScopeLock lock(mutex);
  glide_polar = task_manager.get_glide_polar();
  return that.clone(task_events,
                    task_behaviour,
                    glide_polar);
}

OrderedTask* 
TaskClientUI::task_blank()
{
  ScopeLock lock(mutex);
  glide_polar = task_manager.get_glide_polar();
  return new OrderedTask(task_events,
                         task_behaviour,
                         glide_polar);
}


bool
TaskClientUI::task_commit(const OrderedTask& that)
{
  ScopeLock lock(mutex);
  return task_manager.commit(that);
}

const OrderedTaskBehaviour 
TaskClientUI::get_ordered_task_behaviour() const
{
  ScopeLock lock(mutex);
  return task_manager.get_ordered_task_behaviour();
}

bool 
TaskClientUI::task_save(const TCHAR* path, const OrderedTask& task)
{
  DataNodeXML* root = DataNodeXML::createRoot(_T("Task"));
  Serialiser tser(*root);
  tser.serialise(task);

  bool retval = false;
  if (!root->save(path)) {
//    printf("can't save\n");
  } else {
    retval = true;
  }
  delete root;  
  return retval;
}


bool 
TaskClientUI::task_save(const TCHAR* path)
{
  OrderedTask* task = task_clone();
  bool retval = task_save(path, *task);
  delete task;
  return retval;
}


OrderedTask* 
TaskClientUI::task_create(const TCHAR* path)
{
  DataNode* root = DataNodeXML::load(path);
  if (!root) {
    return NULL;
  }
  if (_tcscmp(root->get_name().c_str(),_T("Task"))==0) {
    OrderedTask* task = task_blank();
    Serialiser des(*root);
    des.deserialise(*task);
    if (task->check_task()) {
      delete root;
      return task;
    } else {
      delete task;
      delete root;
      return NULL;
    }
  }
  delete root;
  return NULL;
}
 
bool 
TaskClientUI::task_load(const TCHAR* path)
{
  OrderedTask* task = task_create(path);
  if (task != NULL) {
    task_commit(*task);
    resume();
    delete task;
    return true;
  }
  return false;
}

const TCHAR TaskClientUI::default_task_path[] = _T("Default.tsk");

bool 
TaskClientUI::task_load_default()
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return task_load(path);
}

bool 
TaskClientUI::task_save_default()
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return task_save(path);
}
