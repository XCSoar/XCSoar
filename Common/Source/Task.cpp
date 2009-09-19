/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Task.h"
#include "TaskImpl.hpp"

TaskSafe task;


void 
TaskSafe::RefreshTask(const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.RefreshTask(settings_computer);
};

void 
TaskSafe::ReplaceWaypoint(const int index, 
                     const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.ReplaceWaypoint(index, settings_computer);
}

void 
TaskSafe::InsertWaypoint(const int index, 
                    const SETTINGS_COMPUTER &settings_computer,
                    bool append)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.InsertWaypoint(index, settings_computer, append);
}

void 
TaskSafe::SwapWaypoint(const int index, 
                  const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.SwapWaypoint(index, settings_computer);
}

void 
TaskSafe::RemoveWaypoint(const int index, 
                    const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.RemoveWaypoint(index, settings_computer);
}

void 
TaskSafe::RemoveTaskPoint(const int index, 
                     const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.RemoveTaskPoint(index, settings_computer);
}

void 
TaskSafe::FlyDirectTo(const int index, 
                 const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.FlyDirectTo(index, settings_computer);
}

void 
TaskSafe::advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.advanceTaskPoint(settings_computer);
}

void 
TaskSafe::retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.retreatTaskPoint(settings_computer);
}


void 
TaskSafe::ClearTask(void)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.ClearTask();
}

void 
TaskSafe::RotateStartPoints(const SETTINGS_COMPUTER &settings_computer)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.RotateStartPoints(settings_computer);
}

void 
TaskSafe::DefaultTask(const SETTINGS_COMPUTER &settings)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.DefaultTask(settings);
}

void 
TaskSafe::ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                     const int set)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.ResumeAbortTask(settings_computer, set);
}

 
void 
TaskSafe::CheckStartPointInTask(void)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.CheckStartPointInTask();
}

void 
TaskSafe::ClearStartPoints(void)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.ClearStartPoints();
}

void 
TaskSafe::SetStartPoint(const int pointnum, const int waypointnum)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.SetStartPoint(pointnum, waypointnum);
}


// AAT functions
double 
TaskSafe::AdjustAATTargets(double desired) {
  // write
  Poco::ScopedRWLock protect(lock, true);
  return _task.AdjustAATTargets(desired);
}

//////
const double 
TaskSafe::FindInsideAATSectorRange(const GEOPOINT &location,
                                      const int taskwaypoint,
                                      const double course_bearing,
                                      const double p_found) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.FindInsideAATSectorRange(location, taskwaypoint,
                                        course_bearing, p_found);
}

const double 
TaskSafe::FindInsideAATSectorDistance(const GEOPOINT &location,
                                         const int taskwaypoint,
                                         const double course_bearing,
                                         const double p_found) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.FindInsideAATSectorDistance(location, taskwaypoint,
                                           course_bearing, p_found);
}


const bool 
TaskSafe::isAdvanceArmed() 
{
  // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.isAdvanceArmed();
}

void 
TaskSafe::setAdvanceArmed(const bool set) 
{
  Poco::ScopedRWLock protect(lock, true);
  _task.setAdvanceArmed(set);
}

const bool 
TaskSafe::isTaskModified()  
{
  // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.isTaskModified();
}

void 
TaskSafe::SetTaskModified(const bool set)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.SetTaskModified(set);
}

const bool 
TaskSafe::isTargetModified()  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.isTargetModified();
}

void 
TaskSafe::SetTargetModified(const bool set)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.SetTargetModified(set);
}

const bool 
TaskSafe::InAATTurnSector(const GEOPOINT &location, 
                           const int the_turnpoint) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.InAATTurnSector(location, the_turnpoint);
}

const unsigned 
TaskSafe::getActiveIndex()  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getActiveIndex();
}

void 
TaskSafe::setActiveIndex(unsigned i) 
{ // write
  Poco::ScopedRWLock protect(lock, true);
  return _task.setActiveIndex(i);
}

const TASK_POINT& 
TaskSafe::getTaskPoint(const int v) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getTaskPoint(v);
}

void 
TaskSafe::setTaskPoint(const unsigned index, const TASK_POINT& tp)
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.setTaskPoint(index, tp);
}

void 
TaskSafe::setTaskIndices(const int wpindex[MAXTASKPOINTS]) 
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.setTaskIndices(wpindex);
}

const int 
TaskSafe::getWaypointIndex(const int v) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getWaypointIndex(v);
}
const WAYPOINT& 
TaskSafe::getWaypoint(const int v) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getWaypoint(v);
}

const bool 
TaskSafe::ValidTaskPoint(const unsigned i) 
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.ValidTaskPoint(i);
}
const bool 
TaskSafe::Valid()  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.Valid();
}

const double 
TaskSafe::DoubleLegDistance(const int taskwaypoint, // read
                               const GEOPOINT &location)  
{
  Poco::ScopedRWLock protect(lock, false);
  return _task.DoubleLegDistance(taskwaypoint, location);
}

const bool 
TaskSafe::TaskIsTemporary(void)  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.TaskIsTemporary();
}

const int  
TaskSafe::getFinalWaypoint(void)  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getFinalWaypoint();
}

const bool 
TaskSafe::ActiveIsFinalWaypoint(void)  
{ //read
  Poco::ScopedRWLock protect(lock, false);
  return _task.ActiveIsFinalWaypoint();
}

const bool 
TaskSafe::isTaskAborted()  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.isTaskAborted();
}

const GEOPOINT &
TaskSafe::getTaskPointLocation(const unsigned i)  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getTaskPointLocation(i);
}

const GEOPOINT &
TaskSafe::getActiveLocation()  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getActiveLocation();
}

const GEOPOINT &
TaskSafe::getTargetLocation(const int v)  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getTargetLocation(v);
}

// file load/save
void 
TaskSafe::LoadNewTask(const TCHAR *FileName,
                 const SETTINGS_COMPUTER &settings_computer) // write
{
  Poco::ScopedRWLock protect(lock, true);
  _task.LoadNewTask(FileName, settings_computer);
}

void 
TaskSafe::SaveTask(const TCHAR *FileName) // write
{
  Poco::ScopedRWLock protect(lock, true);
  _task.SaveTask(FileName);
}

void 
TaskSafe::SaveDefaultTask(void) 
{ // write
  Poco::ScopedRWLock protect(lock, true);
  _task.SaveDefaultTask();
}

const TCHAR* 
TaskSafe::getTaskFilename()  
{ // read
  Poco::ScopedRWLock protect(lock, false);
  return _task.getTaskFilename();
}
void 
TaskSafe::ClearTaskFileName() // write
{
  Poco::ScopedRWLock protect(lock, true);
  _task.ClearTaskFileName();
}
 
// potentially write

void 
TaskSafe::scan_point_forward(RelativeTaskPointVisitor &visitor, const bool write) {
  Poco::ScopedRWLock protect(lock, write);
  _task.scan_point_forward(visitor);
};

void 
TaskSafe::scan_point_forward(AbsoluteTaskPointVisitor &visitor, const bool write) {
  Poco::ScopedRWLock protect(lock, write);
  _task.scan_point_forward(visitor);
};

void 
TaskSafe::scan_leg_forward(RelativeTaskLegVisitor &visitor, const bool write) {
  Poco::ScopedRWLock protect(lock, write);
  _task.scan_leg_forward(visitor);
};

void 
TaskSafe::scan_leg_forward(AbsoluteTaskLegVisitor &visitor, const bool write) {
  Poco::ScopedRWLock protect(lock, write);
  _task.scan_leg_forward(visitor);
};

void 
TaskSafe::scan_leg_reverse(RelativeTaskLegVisitor &visitor, const bool write) {
  Poco::ScopedRWLock protect(lock, write);
  _task.scan_leg_reverse(visitor);
};

void 
TaskSafe::scan_leg_reverse(AbsoluteTaskLegVisitor &visitor, const bool write) {
  Poco::ScopedRWLock protect(lock, write);
  _task.scan_leg_reverse(visitor);
};


const SETTINGS_TASK &
TaskSafe::getSettings() 
{
  Poco::ScopedRWLock protect(lock, false);
  return _task.getSettings();
}

void 
TaskSafe::setSettings(const SETTINGS_TASK& set)
{
  Poco::ScopedRWLock protect(lock, true);
  _task.setSettings(set);
}

