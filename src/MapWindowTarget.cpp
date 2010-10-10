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

#include "MapWindow.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Canvas.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Screen/Layout.hpp"
#include "SettingsMap.hpp"

// TARGET STUFF

// JMW to be used for target preview
  /* This whole thing needs rework now that stuff is being passed in
     via the SettingsMap

bool MapWindow::SetTargetPan(bool do_pan, int target_point) {
  static double old_latitude;
  static double old_longitude;
  static bool old_pan=false;
  static bool old_fullscreen=false;

  if (!SettingsMap().TargetPan
      || (SettingsMap().TargetPanIndex != target_point)) {
    TargetDrag_State = 0;
  }

  if (do_pan && !TargetPan) {
    old_latitude = PanLatitude;
    old_longitude = PanLongitude;
    old_pan = SettingsMap().EnablePan;
    //JMW broken    EnablePan = true;
    TargetPan = do_pan;

    old_fullscreen = SettingsMap().FullScreen;
    if (askFullScreen) {
      askFullScreen = false;
    }
    SwitchZoomClimb();
  }
  if (do_pan) {
    mutexTaskData.Lock();
    if (ValidTaskPoint(target_point)) {
      PanLongitude = WayPointList[task_points[target_point].Index].Longitude;
      PanLatitude = WayPointList[task_points[target_point].Index].Latitude;
      if (target_point==0) {
        TargetZoomDistance = max(2e3, StartRadius*2);
      } else if (!ValidTaskPoint(target_point+1)) {
        TargetZoomDistance = max(2e3, FinishRadius*2);
      } else if (AATEnabled) {
        if (task_points[target_point].AATType == SECTOR) {
          TargetZoomDistance = max(2e3, task_points[target_point].AATSectorRadius*2);
        } else {
          TargetZoomDistance = max(2e3, task_points[target_point].AATCircleRadius*2);
        }
      } else {
        TargetZoomDistance = max(2e3, SectorRadius*2);
      }
    }
    mutexTaskData.Unlock();
  } else if (TargetPan) {
    PanLongitude = old_longitude;
    PanLatitude = old_latitude;
    // JMW broken    EnablePan = old_pan;
    TargetPan = do_pan;
    if (old_fullscreen) {
      askFullScreen = true;
    }
    SwitchZoomClimb();
  }
  TargetPan = do_pan;

  mutexTaskData.Unlock();
  return old_pan;
}
  */


/*
* set to -1 to turn off
* returns old zoom value
*/

bool MapWindow::isClickOnTarget(const POINT pc)
{
  if (XCSoarInterface::SetSettingsMap().TargetPan) {

    if (!protected_task_manager.target_is_locked(XCSoarInterface::SetSettingsMap().TargetPanIndex))
      return false;
    GeoPoint gnull;
    const GeoPoint& t = protected_task_manager.get_location_target(
        XCSoarInterface::SetSettingsMap().TargetPanIndex,
        gnull);

    if (t == gnull)
      return false;

    const POINT pt = visible_projection.LonLat2Screen(t);
    const GeoPoint gp = visible_projection.Screen2LonLat(pc.x, pc.y);
    if (visible_projection.DistanceMetersToScreen(gp.distance((t))) < unsigned(Layout::Scale(10)) )
      return true;
  }
  return false;
}

bool MapWindow::isInSector(const int x, const int y)
{
  POINT dragPT;
  dragPT.x = x;
  dragPT.y = y;

  if (XCSoarInterface::SetSettingsMap().TargetPan) {
    GeoPoint gp = visible_projection.Screen2LonLat(dragPT.x, dragPT.y);
    AIRCRAFT_STATE a;
    a.Location = gp;
    return protected_task_manager.isInSector(XCSoarInterface::SetSettingsMap().TargetPanIndex, a);
  }
  return false;
}
void
MapWindow::TargetPaintDrag(Canvas &canvas, const POINT drag_last)
{
  Graphics::hBmpTarget.draw(canvas, get_bitmap_canvas(), drag_last.x, drag_last.y);
}

bool MapWindow::TargetDragged(const int x, const int y)
{
  GeoPoint gp = visible_projection.Screen2LonLat(x, y);
  if (protected_task_manager.target_is_locked(XCSoarInterface::SetSettingsMap().TargetPanIndex)) {
    protected_task_manager.set_target(XCSoarInterface::SetSettingsMap().TargetPanIndex, gp, true);
    return true;
  }
  return false;
}


/* JMW illegal
  mutexTaskData.Lock();
  if (TargetDrag_State >0) { // always return true if we're dragging
			     // or just stopped dragging, so screen is
			     // updated
    *longitude = TargetDrag_Location.Longitude;
    *latitude = TargetDrag_Location.Latitude;
    if (TargetDrag_State == 2) {
        TargetDrag_State = 0; // mouse up/ stop dragging
    }
    retval = true;
  }
  mutexTaskData.Unlock();
*/
