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

#include "GlueMapWindow.hpp"
#include "Message.hpp"
#include "InputEvents.h"
#include "Screen/Layout.hpp"
#include "Appearance.hpp"
#include "Defines.h"
#include "Simulator.hpp"
#include "TaskClientUI.hpp"
#include "DeviceBlackboard.hpp"
#include "Math/Earth.hpp"
#include "Protection.hpp"
#include "Dialogs.h"
#include "UtilsSystem.hpp"

#include <algorithm>

using std::min;
using std::max;

bool
GlueMapWindow::on_setfocus()
{
  MapWindow::on_setfocus();

  if (InputEvents::getModeID() == InputEvents::MODE_INFOBOX)
    // the focus comes from the info box; restore the "default" mode
    InputEvents::setMode(InputEvents::MODE_DEFAULT);

  return true;
}

bool
GlueMapWindow::on_mouse_double(int x, int y)
{
  mouse_down_clock.update();
  InputEvents::ShowMenu();
  ignore_single_click = true;
  return true;
}

bool
GlueMapWindow::on_mouse_move(int x, int y, unsigned keys)
{
#ifdef OLD_TASK // target control
  if (task != NULL &&
      task->getSettings().AATEnabled &&
      SettingsMap().TargetPan &&
      (TargetDrag_State > 0)) {
    // target follows "finger" so easier to drop near edge of
    // sector
    if (TargetDrag_State == 1) {
      GEOPOINT mouseMove;
      Screen2LonLat((int)x, (int)y, mouseMove);
      unsigned index = SettingsMap().TargetPanIndex;
      if (task->InAATTurnSector(mouseMove, index)) {
        // update waypoints so if we drag out of the cylinder, it
        // will remain adjacent to the edge

        TASK_POINT tp = task->getTaskPoint(index);
        tp.AATTargetLocation = mouseMove;
        task->setTaskPoint(index, tp);
        TargetDrag_Location = mouseMove;

        MapGfx.hBmpTarget.draw(get_canvas(), get_bitmap_canvas(), x, y);
        return true;
      }
    }
  }
#endif

  return MapWindow::on_mouse_move(x, y, keys);
}

bool
GlueMapWindow::on_mouse_down(int x, int y)
{
  // Ignore single click event if double click detected
  if (ignore_single_click)
    return true;

  mouse_down_clock.update();

  set_focus();

  drag_start.x = x;
  drag_start.y = y;
  Screen2LonLat(x, y, drag_start_geopoint);

#ifdef OLD_TASK // target control
  if (task != NULL &&
      task->getSettings().AATEnabled &&
      SettingsMap().TargetPan) {
    if (task->ValidTaskPoint(SettingsMap().TargetPanIndex)) {
      POINT tscreen;
      LonLat2Screen(task->getTargetLocation(SettingsMap().TargetPanIndex),
                    tscreen);
      double distance = hypot(drag_start.x - tscreen.x,
                              drag_start.y - tscreen.y);
      distance /= Layout::scale;

      if (distance < 10)
        TargetDrag_State = 1;
    }
  }
#endif

  return true;
}

bool
GlueMapWindow::on_mouse_up(int x, int y)
{
  // Ignore single click event if double click detected
  if (ignore_single_click) {
    ignore_single_click = false;
    return true;
  }

  int click_time = mouse_down_clock.elapsed();
  mouse_down_clock.reset();

  double distance = hypot(drag_start.x - x, drag_start.y - y);

  if (SettingsMap().TargetPan) {
#ifdef OLD_TASK // target control
    if (task != NULL &&
        task->getSettings().AATEnabled &&
        TargetDrag_State > 0) {
      TargetDrag_State = 2;
      if (task->InAATTurnSector(G, SettingsMap().TargetPanIndex))
        // if release mouse out of sector, don't update w/ bad coords
        TargetDrag_Location = G;

      return true;
    }

    return false;
#endif
  }

  if (SettingsMap().EnablePan && (distance > Layout::Scale(36))) {
    GEOPOINT G;
    Screen2LonLat(x, y, G);

    // JMW broken!
    XCSoarInterface::SetSettingsMap().PanLocation.Longitude += drag_start_geopoint.Longitude - G.Longitude;
    XCSoarInterface::SetSettingsMap().PanLocation.Latitude += drag_start_geopoint.Latitude - G.Latitude;
    RefreshMap();
    return true;
  }

  if (is_simulator() && !Basic().gps.Replay
      && click_time > 50 && distance > Layout::Scale(36)) {
    GEOPOINT G;
    Screen2LonLat(x, y, G);

    // This drag moves the aircraft (changes speed and direction)
    const Angle oldbearing = Basic().TrackBearing;
    const fixed minspeed = fixed(1.1) * (task != NULL ?
                                  task->get_glide_polar() :
                                  GlidePolar(fixed_zero)).get_Vmin();
    const Angle newbearing = Bearing(drag_start_geopoint, G);
    if (((newbearing - oldbearing).as_delta().magnitude_degrees() < fixed(30)) ||
        (Basic().GroundSpeed < minspeed))
      device_blackboard.SetSpeed(min(fixed(100.0),
                                 max(minspeed,
                                     fixed(distance / (3 * Layout::scale)))));

    device_blackboard.SetTrackBearing(newbearing);
    // change bearing without changing speed if direction change > 30
    // 20080815 JMW prevent dragging to stop glider

    // JMW trigger recalcs immediately
    TriggerGPSUpdate();
    return true;
  }

  if(click_time < 1000) {
    // click less then one second -> open nearest waypoint details
    if (way_points != NULL &&
        PopupNearestWaypointDetails(*way_points, drag_start_geopoint,
                                    DistancePixelsToMeters(Layout::Scale(10)),
                                    false)) {
      return true;
    }
  } else {
    // click more then one second -> open nearest airspace details
    if (m_airspace != NULL &&
        AirspaceDetailsAtPoint(drag_start_geopoint))
      return true;
  }

  return false;
}

bool
GlueMapWindow::on_mouse_wheel(int delta)
{
  if (delta > 0)
    // zoom in
    InputEvents::sub_ScaleZoom(1);
  else if (delta < 0)
    // zoom out
    InputEvents::sub_ScaleZoom(-1);

  return true;
}

#if defined(GNAV) || defined(PNA)

bool
GlueMapWindow::on_key_down(unsigned key_code)
{
  return on_key_press(key_code) || MapWindow::on_key_down(key_code);
}

#else

bool
GlueMapWindow::on_key_up(unsigned key_code)
{
  return on_key_press(key_code) || MapWindow::on_key_up(key_code);
}

#endif

bool
GlueMapWindow::on_key_press(unsigned key_code)
{
  key_code = TranscodeKey(key_code);
  if (is_altair() && key_code == 0xF5) {
    XCSoarInterface::SignalShutdown(false);
    return true;
  }

  mouse_down_clock.reset();
  if (InputEvents::processKey(key_code)) {
    return true; // don't go to default handler
  }

  return false;
}
