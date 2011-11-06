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
#include "Components.hpp"
#include "DrawThread.hpp"
#include "DeviceBlackboard.hpp"
#include "Look/Look.hpp"

GlueMapWindow::GlueMapWindow(const Look &look)
  :MapWindow(look.waypoint, look.airspace, look.trail, look.task,
             look.aircraft, look.traffic, look.marker),
   logger(NULL),
   idle_robin(2),
   drag_mode(DRAG_NONE),
   ignore_single_click(false),
   DisplayMode(DM_CRUISE),
   thermal_band_renderer(look.thermal_band, look.chart),
   final_glide_bar_renderer(look.task),
   map_item_timer(0)
{
}

void
GlueMapWindow::set(ContainerWindow &parent, const PixelRect &rc)
{
  MapWindow::set(parent, rc);

  LoadDisplayModeScales();
  visible_projection.SetScale(zoomclimb.CruiseScale);
}

void
GlueMapWindow::SetSettingsMap(const SETTINGS_MAP &new_value)
{
  AssertThreadOrUndefined();

#ifdef ENABLE_OPENGL
  ReadSettingsMap(new_value);
#else
  ScopeLock protect(next_mutex);
  next_settings_map = new_value;
#endif
}

void
GlueMapWindow::SetSettingsComputer(const SETTINGS_COMPUTER &new_value)
{
  AssertThreadOrUndefined();

#ifdef ENABLE_OPENGL
  ReadSettingsComputer(new_value);
#else
  ScopeLock protect(next_mutex);
  next_settings_computer = new_value;
#endif
}

void
GlueMapWindow::ExchangeBlackboard()
{
  /* copy device_blackboard to MapWindow */

  device_blackboard->mutex.Lock();
  ReadBlackboard(device_blackboard->Basic(), device_blackboard->Calculated());
  device_blackboard->mutex.Unlock();

#ifndef ENABLE_OPENGL
  next_mutex.Lock();
  ReadSettingsMap(next_settings_map);
  ReadSettingsComputer(next_settings_computer);
  next_mutex.Unlock();
#endif
}

void
GlueMapWindow::FullRedraw()
{
  UpdateDisplayMode();
  UpdateScreenAngle();
  UpdateProjection();
  UpdateMapScale();

#ifdef ENABLE_OPENGL
  invalidate();
#else
  draw_thread->TriggerRedraw();
#endif
}

void
GlueMapWindow::QuickRedraw()
{
  UpdateScreenAngle();
  UpdateProjection();
  UpdateMapScale();

#ifndef ENABLE_OPENGL
  /* update the Projection */

  ++ui_generation;

  /* quickly stretch the existing buffer into the window */

  scale_buffer = 2;
#endif

  invalidate();

#ifndef ENABLE_OPENGL
  /* we suppose that the operation will need a full redraw later, so
     trigger that now */
  draw_thread->TriggerRedraw();
#endif
}

/**
 * This idle function allows progressive scanning of visibility etc
 */
bool
GlueMapWindow::Idle()
{
  bool still_dirty;
  bool topography_dirty = true; /* scan topography in every Idle() call */
  bool terrain_dirty = true;
  bool weather_dirty = true;

  // StartTimer();

  do {
    idle_robin = (idle_robin + 1) % 3;
    switch (idle_robin) {
    case 0:
      topography_dirty = UpdateTopography(1) > 0;
      break;

    case 1:
      terrain_dirty = UpdateTerrain();
      break;

    case 2:
      weather_dirty = UpdateWeather();
      break;
    }

    still_dirty = terrain_dirty || topography_dirty || weather_dirty;
  } while (RenderTimeAvailable() &&
#ifndef ENABLE_OPENGL
           !draw_thread->IsTriggered() &&
#endif
           still_dirty);

  return still_dirty;
}
