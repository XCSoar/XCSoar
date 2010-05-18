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

// Interface/touchscreen callbacks

/**
 * Virtual Key Manager
 * @param X
 * @param Y
 * @param keytime
 * @param vkmode
 * @return 0 if invalid virtual scan code, otherwise a valid transcoded keycode
 */
int
GlueMapWindow::ProcessVirtualKey(int X, int Y, long keytime, short vkmode)
{
  // 0 is always thermal mode, and does not account
#define MAXBOTTOMMODES 5
#define VKTIMELONG 1500

#ifdef DEBUG_PROCVK
  TCHAR buf[100];
  wsprintf(buf, _T("R=%d,%d,%d,%d, X=%d Y=%d kt=%ld"),
           MapRect.left, MapRect.top,
           MapRect.right, MapRect.bottom,
           X, Y, keytime);
  DoStatusMessage(buf);
#endif

  short sizeup = MapRect.bottom - MapRect.top;
  short yup = (sizeup / 3) + MapRect.top;
  short ydown = MapRect.bottom - (sizeup / 3);

  if (Y < yup) {
    if (keytime >= VKTIMELONG)
      return 0xc1;
    else
      return 38;
  }
  if (Y > ydown) {
    if (keytime >= VKTIMELONG)
      return 0xc2;
    else
      return 40;
  }

  /*
   * FIX ready: do not pass virtual ENTER while in Panmode.
   * Currently it is allowed, should be better tested.  VNT 090702
   if ( !MapWindow::EnablePan ) {
     DoStatusMessage(_T("Virtual ENTER"));
     return 13;
   }
   return 0; // ignore it
  */
  return 13;

  Message::AddMessage(_T("VirtualKey Error"));
  return 0;
}

bool
GlueMapWindow::on_setfocus()
{
  MapWindow::on_setfocus();

  if (InputEvents::getModeID() == InputEvents::MODE_INFOBOX)
    // the focus comes from the info box; restore the "default" mode
    InputEvents::setMode(InputEvents::MODE_DEFAULT);

  return true;
}

static GEOPOINT LLstart;
static int XstartScreen, YstartScreen;
static bool ignorenext = true;

bool
GlueMapWindow::on_mouse_double(int x, int y)
{
  // Added by ARH to show menu button when mapwindow is double clicked.
  //
  // VNT TODO: do not handle this event and remove CS_DBLCLKS in register class.
  // Only handle timed clicks in BUTTONDOWN with no proximity.
  //
  mouse_down_clock.update();
  InputEvents::ShowMenu();
  ignorenext = true;
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

        draw_masked_bitmap(get_canvas(), MapGfx.hBmpTarget, x, y, 10, 10, true);
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
  mouse_down_clock.update();
  if (ignorenext)
    return true;

  set_focus();

  // TODO VNT move Screen2LonLat in LBUTTONUP after making sure we
  // really need Xstart and Ystart so we save precious
  // milliseconds waiting for BUTTONUP GetTickCount
  Screen2LonLat(x, y, LLstart);
  XstartScreen = x;
  YstartScreen = y;

#ifdef OLD_TASK // target control
  if (task != NULL &&
      task->getSettings().AATEnabled &&
      SettingsMap().TargetPan) {
    if (task->ValidTaskPoint(SettingsMap().TargetPanIndex)) {
      POINT tscreen;
      LonLat2Screen(task->getTargetLocation(SettingsMap().TargetPanIndex),
                    tscreen);
      double distance = isqrt4((long)((XstartScreen - tscreen.x) *
                                      (XstartScreen - tscreen.x) +
                                      (YstartScreen - tscreen.y) *
                                      (YstartScreen - tscreen.y)));
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
  if (ignorenext) {
    ignorenext = false;
    return true;
  }

  int dwInterval = mouse_down_clock.elapsed();
  mouse_down_clock.reset();

  RECT rc = MapRect;
  bool my_target_pan = SettingsMap().TargetPan;

  double distance = isqrt4((long)((XstartScreen - x) *
                                  (XstartScreen - x) +
                                  (YstartScreen - y) *
                                  (YstartScreen - y)));
  distance /= Layout::scale;

#ifdef DEBUG_VIRTUALKEYS
  TCHAR buf[80];
  char sbuf[80];
  sprintf(sbuf,"%.0f",distance);
  _stprintf(buf, _T("XY=%d,%d dist=%S Up=%ld Down=%ld Int=%ld"),
            x, y, sbuf, dwUpTime, dwDownTime, dwInterval);
  Message::AddMessage(buf);
#endif

  // Caution, timed clicks from PC with a mouse are different
  // from real touchscreen devices

  if ((distance < 50) &&
      (CommonInterface::VirtualKeys==(VirtualKeys_t)vkEnabled) &&
      (dwInterval>= DOUBLECLICKINTERVAL)) {
    unsigned wParam = ProcessVirtualKey(x, y, dwInterval, 0);
    if (wParam == 0) {
#ifdef DEBUG_VIRTUALKEYS
      Message::AddMessage(_T("E02 INVALID Virtual Key!"));
#endif
      return true;
    }
    else {
      if (InputEvents::processKey(wParam)) {
        return true; // don't go to default handler
      }
    }
  }

  GEOPOINT G;
  Screen2LonLat(x, y, G);

#ifdef OLD_TASK // target control
  if (task != NULL &&
      task->getSettings().AATEnabled &&
      my_target_pan &&
      TargetDrag_State > 0) {
    TargetDrag_State = 2;
    if (task->InAATTurnSector(G, SettingsMap().TargetPanIndex))
      // if release mouse out of sector, don't update w/ bad coords
      TargetDrag_Location = G;

    return true;
  }
#endif

  if (!my_target_pan && SettingsMap().EnablePan && (distance > IBLSCALE(36))) {
    // JMW broken!
    XCSoarInterface::SetSettingsMap().PanLocation.Longitude += LLstart.Longitude - G.Longitude;
    XCSoarInterface::SetSettingsMap().PanLocation.Latitude += LLstart.Latitude - G.Latitude;
    RefreshMap();
    return true;
  }

  if (is_simulator() && (dwInterval > 50)) {
    if (!Basic().gps.Replay && !my_target_pan && (distance > IBLSCALE(36))) {
      // This drag moves the aircraft (changes speed and direction)
      const Angle oldbearing = Basic().TrackBearing;
      const fixed minspeed = fixed(1.1) * (task != NULL ?
                                    task->get_glide_polar() :
                                    GlidePolar(fixed_zero)).get_Vmin();
      const Angle newbearing = Bearing(LLstart, G);
      if (((newbearing - oldbearing).as_delta().magnitude_degrees() < fixed(30)) ||
          (Basic().GroundSpeed < minspeed))
        device_blackboard.SetSpeed(min(fixed(100.0),
                                   max(minspeed, fixed(distance / 3))));

      device_blackboard.SetTrackBearing(newbearing);
      // change bearing without changing speed if direction change > 30
      // 20080815 JMW prevent dragging to stop glider

      // JMW trigger recalcs immediately
      TriggerGPSUpdate();
      return true;
    }
  }

  if (!my_target_pan) {
    if (CommonInterface::VirtualKeys == (VirtualKeys_t)vkEnabled) {
      if (dwInterval < VKSHORTCLICK) {
        // 100ms is NOT enough for a short click since GetTickCount is OEM custom!
        if (task != NULL &&
            PopupNearestWaypointDetails(task->get_waypoints(), LLstart,
					DistancePixelsToMeters(IBLSCALE(10)), false)) {
          return true;
        }
      } else {
        if (m_airspace != NULL && AirspaceDetailsAtPoint(LLstart))
          return true;
      }
    } else {
      if(dwInterval < AIRSPACECLICK) { // original and untouched interval
        if (task != NULL &&
            PopupNearestWaypointDetails(task->get_waypoints(), LLstart,
					DistancePixelsToMeters(IBLSCALE(10)), false)) {
          return true;
        }
      } else {
        if (m_airspace != NULL &&
            AirspaceDetailsAtPoint(LLstart))
          return true;
      }
    } // VK enabled
  } // !TargetPan

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
  // VENTA-TODO careful here, keyup no more trapped for PNA.
  // Forbidden usage of keypress timing.

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
