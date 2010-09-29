/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ProtectedTaskManager.hpp"
#include "Task/TaskManager.hpp"
#include "Util/Serialiser.hpp"
#include "Util/DataNodeXML.hpp"
#include "LocalPath.hpp"

#include <windef.h> // for MAX_PATH

GlidePolar 
ProtectedTaskManager::get_glide_polar() const
{
  Lease lease(*this);
  return lease->get_glide_polar();
}

void 
ProtectedTaskManager::set_glide_polar(const GlidePolar& glide_polar)
{
  ExclusiveLease lease(*this);
  lease->set_glide_polar(glide_polar);
}

TaskManager::TaskMode_t 
ProtectedTaskManager::get_mode() const
{
  Lease lease(*this);
  return lease->get_mode();
}

TracePointVector 
ProtectedTaskManager::find_trace_points(const GeoPoint &loc, 
                                        const fixed range,
                                        const unsigned mintime,
                                        const fixed resolution) const
{
  Lease lease(*this);
  return lease->find_trace_points(loc, range, mintime, resolution);
}

const OrderedTaskBehaviour 
ProtectedTaskManager::get_ordered_task_behaviour() const
{
  Lease lease(*this);
  return lease->get_ordered_task_behaviour();
}

GlidePolar 
ProtectedTaskManager::get_safety_polar() const
{
  Lease lease(*this);
  return lease->get_safety_polar();
}

const Waypoint* 
ProtectedTaskManager::getActiveWaypoint() const
{
  Lease lease(*this);
  const TaskPoint *tp = lease->getActiveTaskPoint();
  if (tp)
    return &tp->get_waypoint();

  return NULL;
}
//////////////////////////////
bool
ProtectedTaskManager::isInSector (const unsigned TPindex, const AIRCRAFT_STATE &ref) const
{
  Lease lease(*this);
  return lease->isInSector(TPindex, ref);
}

const GeoPoint&
ProtectedTaskManager::get_location_target(const unsigned TPindex, const GeoPoint& fallback_location) const
{
  Lease lease(*this);
  return lease->get_location_target(TPindex, fallback_location);
}

bool
ProtectedTaskManager::target_is_locked(const unsigned TPindex) const
{
  Lease lease(*this);
  return lease->target_is_locked(TPindex);
}

bool
ProtectedTaskManager::has_target(const unsigned TPindex) const
{
  Lease lease(*this);
  return lease->has_target(TPindex);
}

bool
ProtectedTaskManager::set_target(const unsigned TPindex, const GeoPoint &loc,
   const bool override_lock)
{
  ExclusiveLease lease(*this);
  return lease->set_target(TPindex, loc, override_lock);
}

bool
ProtectedTaskManager::set_target(const unsigned TPindex, const fixed range,
   const fixed radial)
{
  ExclusiveLease lease(*this);
  return lease->set_target(TPindex, range, radial);
}

bool
ProtectedTaskManager::get_target_range_radial(const unsigned TPindex, fixed &range,
   fixed &radial)
{
  Lease lease(*this);
  return lease->get_target_range_radial(TPindex, range, radial);
}

bool
ProtectedTaskManager::target_lock(const unsigned TPindex, bool do_lock)
{
  ExclusiveLease lease(*this);
  return lease->target_lock(TPindex, do_lock);
}

const GeoPoint&
ProtectedTaskManager::get_ordered_taskpoint_location(const unsigned TPindex,
   const GeoPoint& fallback_location) const
{
  Lease lease(*this);
  return lease->get_ordered_taskpoint_location(TPindex, fallback_location);
}

fixed
ProtectedTaskManager::get_ordered_taskpoint_radius(const unsigned TPindex) const
{
  Lease lease(*this);
  return lease->get_ordered_taskpoint_radius(TPindex);
}

const TCHAR*
ProtectedTaskManager::get_ordered_taskpoint_name(const unsigned TPindex)
{
 Lease lease(*this);
 return lease->get_ordered_taskpoint_name(TPindex);
}
/////////////////////////////////////
void 
ProtectedTaskManager::incrementActiveTaskPoint(int offset)
{
  ExclusiveLease lease(*this);
  lease->incrementActiveTaskPoint(offset);
}

bool 
ProtectedTaskManager::do_goto(const Waypoint & wp)
{
  ExclusiveLease lease(*this);
  return lease->do_goto(wp);
}

AIRCRAFT_STATE 
ProtectedTaskManager::get_start_state() const
{
  Lease lease(*this);
  return lease->get_start_state();
}

fixed 
ProtectedTaskManager::get_finish_height() const
{
  Lease lease(*this);
  return lease->get_finish_height();
}

OrderedTask*
ProtectedTaskManager::task_clone()
{
  ExclusiveLease lease(*this);
  glide_polar = lease->get_glide_polar();
  return lease->clone(task_events, task_behaviour, glide_polar);
}

OrderedTask* 
ProtectedTaskManager::task_copy(const OrderedTask& that)
{
  ExclusiveLease lease(*this);
  glide_polar = lease->get_glide_polar();
  return that.clone(task_events, task_behaviour, glide_polar);
}

OrderedTask* 
ProtectedTaskManager::task_blank()
{
  ExclusiveLease lease(*this);
  glide_polar = lease->get_glide_polar();
  return new OrderedTask(task_events, task_behaviour, glide_polar);
}

bool
ProtectedTaskManager::task_commit(const OrderedTask& that)
{
  ExclusiveLease lease(*this);
  return lease->commit(that);
}

bool 
ProtectedTaskManager::task_save(const TCHAR* path, const OrderedTask& task)
{
  DataNodeXML* root = DataNodeXML::createRoot(_T("Task"));
  Serialiser tser(*root);
  tser.serialise(task);

  bool retval = root->save(path);
  delete root;  
  return retval;
}

bool 
ProtectedTaskManager::task_save(const TCHAR* path)
{
  OrderedTask* task = task_clone();
  bool retval = task_save(path, *task);
  delete task;
  return retval;
}

OrderedTask* 
ProtectedTaskManager::task_create(const TCHAR* path)
{
  DataNode* root = DataNodeXML::load(path);
  if (!root)
    return NULL;

  if (_tcscmp(root->get_name().c_str(),_T("Task"))==0) {
    OrderedTask* task = task_blank();
    Serialiser des(*root);
    des.deserialise(*task);
    if (task->check_task()) {
      delete root;
      return task;
    }
    delete task;
    delete root;
    return NULL;
  }
  delete root;
  return NULL;
}
 
bool 
ProtectedTaskManager::task_load(const TCHAR* path)
{
  OrderedTask* task = task_create(path);
  if (task != NULL) {
    ExclusiveLease lease(*this);
    lease->commit(*task);
    lease->resume();
    delete task;
    return true;
  }
  return false;
}

const TCHAR ProtectedTaskManager::default_task_path[] = _T("Default.tsk");

bool 
ProtectedTaskManager::task_load_default()
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return task_load(path);
}

bool 
ProtectedTaskManager::task_save_default()
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return task_save(path);
}

void 
ProtectedTaskManager::reset()
{
  ExclusiveLease lease(*this);
  lease->reset();
}
