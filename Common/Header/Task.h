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

#if !defined(XCSOAR_TASK_H)
#define XCSOAR_TASK_H

#include "TaskImpl.hpp"
#include "Poco/RWLock.h"


class TaskSafe {
private:
  Task _task;
  Poco::RWLock lock;
public:

  void RefreshTask(const SETTINGS_COMPUTER &settings_computer);

  void ReplaceWaypoint(const int index,
                               const SETTINGS_COMPUTER &settings_computer);
  void InsertWaypoint(const int index,
                              const SETTINGS_COMPUTER &settings_computer,
                              bool append=false);
  void SwapWaypoint(const int index,
                            const SETTINGS_COMPUTER &settings_computer);
  void RemoveWaypoint(const int index,
                              const SETTINGS_COMPUTER &settings_computer);
  void RemoveTaskPoint(const int index,
                               const SETTINGS_COMPUTER &settings_computer);
  void FlyDirectTo(const int index,
                           const SETTINGS_COMPUTER &settings_computer);

  void advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer);
  void retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer);

  void ClearTask(void);
  void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer);
  void DefaultTask(const SETTINGS_COMPUTER &settings);
  void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                               const int set = 0);
  void CheckStartPointInTask(void);
  void ClearStartPoints(void);
  void SetStartPoint(const int pointnum, const int waypointnum);

  // AAT functions
  double AdjustAATTargets(double desired);
  double FindInsideAATSectorRange(const GEOPOINT &location,
                                  const int taskwaypoint,
                                  const double course_bearing,
                                  const double p_found);
  double FindInsideAATSectorDistance(const GEOPOINT &location,
                                     const int taskwaypoint,
                                     const double course_bearing,
                                     const double p_found=0.0);
  bool isTaskModified();
  void SetTaskModified(const bool set=true);
  bool isTargetModified();
  void SetTargetModified(const bool set=true);
  bool InAATTurnSector(const GEOPOINT &location,
                       const int the_turnpoint);

  // queries
  bool ValidTaskPoint(const unsigned i);
  bool Valid();
  double DoubleLegDistance(const int taskwaypoint,
                           const GEOPOINT &location);
  bool TaskIsTemporary(void);
  int  getFinalWaypoint(void);
  bool ActiveIsFinalWaypoint(void);
  bool isTaskAborted();
  const GEOPOINT& getTaskPointLocation(const unsigned i);
  const GEOPOINT& getActiveLocation();
  const GEOPOINT& getTargetLocation(const int v=-1);

  // file load/save
  void LoadNewTask(const TCHAR *FileName,
                   const SETTINGS_COMPUTER &settings_computer);
  void SaveTask(const TCHAR *FileName);
  void SaveDefaultTask(void);
  const TCHAR* getTaskFilename();
  void ClearTaskFileName();
  bool LoadTaskWaypoints(FILE *file);
  unsigned getActiveIndex();
  void setActiveIndex(unsigned i);
  const TASK_POINT& getTaskPoint(const int v=-1);
  void setTaskPoint(const unsigned index, const TASK_POINT& tp);

  const WAYPOINT& getWaypoint(const int v=-1);
  int getWaypointIndex(const int v=-1);

  // advance
  bool isAdvanceArmed();
  void setAdvanceArmed(const bool set);

  // settings
  const SETTINGS_TASK &getSettings();
  void setSettings(const SETTINGS_TASK& set);

  int getSelected();
  void setSelected(const int v=-1);

  // other

  void setTaskIndices(const int wpindex[MAXTASKPOINTS]);

  void scan_point_forward(RelativeTaskPointVisitor &visitor, const bool write=true);
  void scan_point_forward(AbsoluteTaskPointVisitor &visitor, const bool write=true);
  void scan_leg_forward(RelativeTaskLegVisitor &visitor, const bool write=true);
  void scan_leg_forward(AbsoluteTaskLegVisitor &visitor, const bool write=true);
  void scan_leg_reverse(RelativeTaskLegVisitor &visitor, const bool write=true);
  void scan_leg_reverse(AbsoluteTaskLegVisitor &visitor, const bool write=true);
};


extern TaskSafe task;

#endif
