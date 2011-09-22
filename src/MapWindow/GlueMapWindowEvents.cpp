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
#include "Simulator.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "DeviceBlackboard.hpp"
#include "Math/Earth.hpp"
#include "Protection.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/Task.hpp"
#include "Math/FastMath.h"
#include "Compiler.h"
#include "Interface.hpp"
#include "Screen/Fonts.hpp"

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
  if (map_item_timer) {
    kill_timer(map_item_timer);
    map_item_timer = NULL;
  }

  mouse_down_clock.update();

  if (IsPanning())
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
  if (drag_mode != DRAG_NONE && !dragOverMinDist &&
      (abs(drag_start.x - x) + abs(drag_start.y - y)) > Layout::Scale(threshold))
    dragOverMinDist = true;

  switch (drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_PAN:
    visible_projection.SetGeoLocation(drag_projection.GetGeoLocation()
                                      + drag_start_geopoint
                                      - drag_projection.ScreenToGeo(x, y));
    QuickRedraw();
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
  if (map_item_timer) {
    kill_timer(map_item_timer);
    map_item_timer = NULL;
  }

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

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    drag_mode = DRAG_PAN;
    drag_projection = visible_projection;
    break;
  }

  if (Basic().gps.simulator && drag_mode == DRAG_NONE)
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

  DragMode old_drag_mode = drag_mode;
  drag_mode = DRAG_NONE;

  switch (old_drag_mode) {
  case DRAG_NONE:
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
        SettingsComputer().glide_polar_task.GetVMin();
      const Angle newbearing = Bearing(drag_start_geopoint, G);
      if (((newbearing - oldbearing).as_delta().magnitude_degrees() < fixed(30)) ||
          (Basic().ground_speed < minspeed))
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

  if (!dragOverMinDist) {
    map_item_timer = set_timer(1002, 200);
    return true;
  }

  return false;
}

bool
GlueMapWindow::on_mouse_wheel(int x, int y, int delta)
{
  if (map_item_timer) {
    kill_timer(map_item_timer);
    map_item_timer = NULL;
  }

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
  if (map_item_timer) {
    kill_timer(map_item_timer);
    map_item_timer = NULL;
  }

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
  ExchangeBlackboard();

  /* update terrain, topography, ... */
  Idle();
#endif

  MapWindow::on_paint(canvas);

  // Draw center screen cross hair in pan mode
  if (IsPanning())
    DrawCrossHairs(canvas);
}

void
GlueMapWindow::on_paint_buffer(Canvas &canvas)
{
  MapWindow::on_paint_buffer(canvas);

  DrawMapScale(canvas, get_client_rect(), render_projection);
}

bool
GlueMapWindow::on_timer(timer_t id)
{
  if (id != map_item_timer)
    return false;

  kill_timer(map_item_timer);
  map_item_timer = NULL;
  ShowMapItems(drag_start_geopoint);
  return true;
}

void
GlueMapWindow::Render(Canvas &canvas, const PixelRect &rc)
{
  MapWindow::Render(canvas, rc);

  if (IsNearSelf()) {
    draw_sw.Mark(_T("DrawGlueMisc"));
    if (SettingsMap().EnableThermalProfile)
      DrawThermalBand(canvas, rc);
    DrawStallRatio(canvas, rc);
    DrawFlightMode(canvas, rc);
    DrawFinalGlide(canvas, rc);
    DrawGPSStatus(canvas, rc, Basic());
  }
}
