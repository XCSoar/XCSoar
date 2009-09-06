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

#include "MapWindow.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Task.h"
#include "SettingsTask.hpp"
#include "WayPoint.hpp"


//////////////////////// TARGET STUFF /////////

// JMW to be used for target preview
bool MapWindow::SetTargetPan(bool do_pan, int target_point) {
  static double old_latitude;
  static double old_longitude;
  static bool old_pan=false;
  static bool old_fullscreen=false;

  mutexTaskData.Lock(); // protect thread because some target stuff used in mapwindow thread

  if (!TargetPan || (TargetPanIndex != target_point)) {
    TargetDrag_State = 0;
  }

  TargetPanIndex = target_point;

  if (do_pan && !TargetPan) {
    old_latitude = PanLatitude;
    old_longitude = PanLongitude;
    old_pan = EnablePan;
    EnablePan = true;
    TargetPan = do_pan;

    /* JMW broken/illegal
    old_fullscreen = SettingsMap().FullScreen;
    if (askFullScreen) {
      askFullScreen = false;
    }
    */

    SwitchZoomClimb();
  }
  if (do_pan) {
    mutexTaskData.Lock();
    if (ValidTaskPoint(target_point)) {
      PanLongitude = WayPointList[Task[target_point].Index].Longitude;
      PanLatitude = WayPointList[Task[target_point].Index].Latitude;
      if (target_point==0) {
        TargetZoomDistance = max(2e3, StartRadius*2);
      } else if (!ValidTaskPoint(target_point+1)) {
        TargetZoomDistance = max(2e3, FinishRadius*2);
      } else if (AATEnabled) {
        if (Task[target_point].AATType == SECTOR) {
          TargetZoomDistance = max(2e3, Task[target_point].AATSectorRadius*2);
        } else {
          TargetZoomDistance = max(2e3, Task[target_point].AATCircleRadius*2);
        }
      } else {
        TargetZoomDistance = max(2e3, SectorRadius*2);
      }
    }
    mutexTaskData.Unlock();
  } else if (TargetPan) {
    PanLongitude = old_longitude;
    PanLatitude = old_latitude;
    EnablePan = old_pan;
    TargetPan = do_pan;
    /* JMW broken/illegal
    if (old_fullscreen) {
      askFullScreen = true;
    }
    */
    SwitchZoomClimb();
  }
  TargetPan = do_pan;

  mutexTaskData.Unlock();

  return old_pan;
}


bool MapWindow::TargetDragged(double *longitude, double *latitude) {
  bool retval = false;
  mutexTaskData.Lock();
  if (TargetDrag_State >0) { // always return true if we're dragging
			     // or just stopped dragging, so screen is
			     // updated
    *longitude = TargetDrag_Longitude;
    *latitude = TargetDrag_Latitude;
    if (TargetDrag_State == 2) {
        TargetDrag_State = 0; // mouse up/ stop dragging
    }
    retval = true;
  }
  mutexTaskData.Unlock();
  return retval;
}

