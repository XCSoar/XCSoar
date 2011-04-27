/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "InputEvents.hpp"
#include "Screen/Key.h"
#include "Screen/Layout.hpp"
#include "Appearance.hpp"
#include "Defines.h"
#include "Simulator.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "DeviceBlackboard.hpp"
#include "Math/Earth.hpp"
#include "Protection.hpp"
#include "Dialogs/Dialogs.h"
#include "Math/FastMath.h"
#include "Compiler.h"
#include "Interface.hpp"
#include "Screen/Fonts.hpp"
#include "HorizonRenderer.hpp"

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

  if (settings_map.EnablePan)
    return true;

  InputEvents::ShowMenu();
  ignore_single_click = true;
  return true;
}

bool
GlueMapWindow::on_mouse_move(int x, int y, unsigned keys)
{
  /* allow a bigger threshold on touch screens */
  const int threshold = is_embedded() ? 50 : 10;
  if (!dragOverMinDist &&
      (abs(drag_start.x - x) + abs(drag_start.y - y)) > Layout::Scale(threshold))
    dragOverMinDist = true;

  switch (drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_TARGET:
    if (isInSector(x, y)) {
      drag_last.x = x;
      drag_last.y = y;
      invalidate();
    }
    return true;

  case DRAG_PAN:

    XCSoarInterface::SetSettingsMap().PanLocation =
      drag_projection.GetGeoLocation() + drag_start_geopoint
      - drag_projection.ScreenToGeo(x, y);

    QuickRedraw(XCSoarInterface::SettingsMap());
    ActionInterface::SendSettingsMap(true);
    return true;

  case DRAG_GESTURE:
    gestures.Update(x, y);
    return true;

  case DRAG_SIMULATOR:
    return true;
  }

  return MapWindow::on_mouse_move(x, y, keys);
}

bool
GlueMapWindow::on_mouse_down(int x, int y)
{
  // Ignore single click event if double click detected
  if (ignore_single_click || drag_mode != DRAG_NONE)
    return true;

  mouse_down_clock.update();
  dragOverMinDist = false;

  set_focus();

  drag_start.x = x;
  drag_start.y = y;
  drag_start_geopoint = visible_projection.ScreenToGeo(x, y);
  drag_last = drag_start;

  if (SettingsMap().TargetPan) {
    drag_mode = isClickOnTarget(drag_start) ? DRAG_TARGET : DRAG_NONE;
  } else if (SettingsMap().EnablePan) {
    drag_mode = DRAG_PAN;
    drag_projection = visible_projection;
  }

  if (Basic().gps.Simulator && drag_mode == DRAG_NONE)
    if (compare_squared(visible_projection.GetScreenOrigin().x - x,
                        visible_projection.GetScreenOrigin().y - y,
                        Layout::Scale(30)) != 1)
        drag_mode = DRAG_SIMULATOR;
  if (drag_mode == DRAG_NONE ) {
    gestures.Start(x, y, Layout::Scale(20));
    drag_mode = DRAG_GESTURE;
  }

  if (drag_mode != DRAG_NONE)
    set_capture();

  return true;
}

bool
GlueMapWindow::on_mouse_up(int x, int y)
{
  if (drag_mode != DRAG_NONE)
    release_capture();

  // Ignore single click event if double click detected
  if (ignore_single_click) {
    ignore_single_click = false;
    return true;
  }

  int click_time = mouse_down_clock.elapsed();
  mouse_down_clock.reset();

  enum drag_mode old_drag_mode = drag_mode;
  drag_mode = DRAG_NONE;

  switch (old_drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_TARGET:
    TargetDragged(drag_last.x, drag_last.y);
    break;

  case DRAG_PAN:
#ifndef ENABLE_OPENGL
    /* allow the use of the stretched last buffer for the next two
       redraws */
    scale_buffer = 2;
#endif
    break;

  case DRAG_SIMULATOR:
    if (click_time > 50 &&
        compare_squared(drag_start.x - x, drag_start.y - y,
                        Layout::Scale(36)) == 1) {
      GeoPoint G = visible_projection.ScreenToGeo(x, y);

      double distance = hypot(drag_start.x - x, drag_start.y - y);

      // This drag moves the aircraft (changes speed and direction)
      const Angle oldbearing = Basic().track;
      const fixed minspeed = fixed(1.1) *
        SettingsComputer().glide_polar_task.get_Vmin();
      const Angle newbearing = Bearing(drag_start_geopoint, G);
      if (((newbearing - oldbearing).as_delta().magnitude_degrees() < fixed(30)) ||
          (Basic().GroundSpeed < minspeed))
        device_blackboard.SetSpeed(min(fixed(100.0),
                                       max(minspeed,
                                           fixed(distance / (Layout::FastScale(3))))));

      device_blackboard.SetTrack(newbearing);
      // change bearing without changing speed if direction change > 30
      // 20080815 JMW prevent dragging to stop glider

      return true;
    }

    break;

  case DRAG_GESTURE:
    const TCHAR* gesture = gestures.Finish();
    if (gesture && on_mouse_gesture(gesture))
      return true;

    break;
  }

  if(!dragOverMinDist && !SettingsMap().TargetPan) {
    if (click_time < 1000) {
      // click less then one second -> open nearest waypoint details
      if (way_points != NULL &&
          PopupNearestWaypointDetails(*way_points, drag_start_geopoint,
                                      visible_projection.DistancePixelsToMeters(Layout::Scale(10)),
                                      true))
        return true;
    } else {
      // click more then one second -> open nearest airspace details
      if (airspace_database != NULL &&
          AirspaceDetailsAtPoint(drag_start_geopoint))
        return true;
      const int TargetPoint = isInAnyActiveSector(drag_start_geopoint);
      if (TargetPoint >= 0)
        dlgTargetShowModal(TargetPoint);
    }
  }

  return false;
}

bool
GlueMapWindow::on_mouse_wheel(int delta)
{
  if (drag_mode != DRAG_NONE)
    return true;

  if (delta > 0)
    // zoom in
    InputEvents::sub_ScaleZoom(1);
  else if (delta < 0)
    // zoom out
    InputEvents::sub_ScaleZoom(-1);

  return true;
}

bool
GlueMapWindow::on_mouse_gesture(const TCHAR* gesture)
{
  return InputEvents::processGesture(gesture);
}

bool
GlueMapWindow::on_key_down(unsigned key_code)
{
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

bool
GlueMapWindow::on_cancel_mode()
{
  MapWindow::on_cancel_mode();

  if (drag_mode != DRAG_NONE) {
    release_capture();
    drag_mode = DRAG_NONE;
  }

  return false;
}

void
GlueMapWindow::on_paint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* update terrain, topography, ... */
  Idle();
#endif

  MapWindow::on_paint(canvas);

  if (drag_mode == DRAG_TARGET)
    TargetPaintDrag(canvas, drag_last);

  // Draw center screen cross hair in pan mode
  if (SettingsMap().EnablePan && !SettingsMap().TargetPan)
    DrawCrossHairs(canvas);

#if 0
  /*
  FEATURE TEMPORARILY DISABLED DUE TO USE OF XCSOAR IN FAI COMPETITIONS

  This feature of having a backup artificial horizon based on inferred
  orientation from GPS and vario data is useful, and reasonably well
  tested, but has the issue of potentially invalidating use of XCSoar in
  FAI contests due to rule ref Annex A to Section 3 (2010 Edition) 4.1.2
  "No instruments permitting pilots to fly without visual reference to
  the ground may be carried on board, even if made unserviceable."  The
  quality of XCSoar's pseudo-AH is arguably good enough that this
  violates the rule.  We need to seek clarificat ion as to whether this
  is the case or not.
  */
  if (EnableAuxiliaryInfo)
    DrawHorizon(canvas, get_client_rect());
#endif
}

void
GlueMapWindow::on_paint_buffer(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  ExchangeBlackboard();
#endif

  MapWindow::on_paint_buffer(canvas);

  DrawMapScale(canvas, get_client_rect(), render_projection);
}

void
GlueMapWindow::Render(Canvas &canvas, const PixelRect &rc)
{
  UpdateScreenAngle();
  UpdateProjection();

  MapWindow::Render(canvas, rc);

  if (!settings_map.EnablePan) {
    DrawFlightMode(canvas, rc);
    DrawFinalGlide(canvas, rc);
    DrawGPSStatus(canvas, rc, Basic());
  }

#ifdef DRAWLOAD
  canvas.select(Fonts::Map);
  TCHAR load[80];
  _stprintf(load, _T("draw %d gps %d idle %d"),
            GetAverageTime(),
            Calculated().time_process_gps,
            Calculated().time_process_idle);

  canvas.text(rc.left, rc.top, load);
#endif
}
