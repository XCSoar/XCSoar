/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Blackboard/DeviceBlackboard.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "Time/PeriodClock.hpp"
#include "Event/Idle.hpp"

GlueMapWindow::GlueMapWindow(const Look &look)
  :MapWindow(look.map, look.traffic),
   logger(NULL),
   idle_robin(-1),
#ifdef ENABLE_OPENGL
   data_timer(*this),
#endif
   drag_mode(DRAG_NONE),
   ignore_single_click(false),
#ifdef ENABLE_OPENGL
   kinetic_x(700),
   kinetic_y(700),
   kinetic_timer(*this),
#endif
   arm_mapitem_list(false),
   last_display_mode(DisplayMode::NONE),
   thermal_band_renderer(look.thermal_band, look.chart),
   final_glide_bar_renderer(look.final_glide_bar, look.map.task),
   vario_bar_renderer(look.vario_bar),
   gesture_look(look.gesture),
   map_item_timer(*this)
{
}

GlueMapWindow::~GlueMapWindow()
{
  Destroy();
}

void
GlueMapWindow::Create(ContainerWindow &parent, const PixelRect &rc)
{
  MapWindow::Create(parent, rc);

  visible_projection.SetScale(CommonInterface::GetMapSettings().cruise_scale);
}

void
GlueMapWindow::SetMapSettings(const MapSettings &new_value)
{
  AssertThreadOrUndefined();

#ifdef ENABLE_OPENGL
  ReadMapSettings(new_value);
#else
  ScopeLock protect(next_mutex);
  next_settings_map = new_value;
#endif
}

void
GlueMapWindow::SetComputerSettings(const ComputerSettings &new_value)
{
  AssertThreadOrUndefined();

#ifdef ENABLE_OPENGL
  ReadComputerSettings(new_value);
#else
  ScopeLock protect(next_mutex);
  next_settings_computer = new_value;
#endif
}

void
GlueMapWindow::SetUIState(const UIState &new_value)
{
  AssertThreadOrUndefined();

#ifdef ENABLE_OPENGL
  ReadUIState(new_value);
#else
  ScopeLock protect(next_mutex);
  next_ui_state = new_value;
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
  ReadMapSettings(next_settings_map);
  ReadComputerSettings(next_settings_computer);
  ReadUIState(next_ui_state);
  next_mutex.Unlock();
#endif
}

void
GlueMapWindow::SuspendThreads()
{
#ifndef ENABLE_OPENGL
  if (draw_thread != NULL)
    draw_thread->Suspend();
#endif
}

void
GlueMapWindow::ResumeThreads()
{
#ifndef ENABLE_OPENGL
  if (draw_thread != NULL)
    draw_thread->Resume();
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
  Invalidate();
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

  Invalidate();

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
  if (!render_projection.IsValid())
    return false;

  if (idle_robin == unsigned(-1)) {
    /* draw the first frame as quickly as possible, so the user can
       start interacting with XCSoar immediately */
    idle_robin = 2;
    return true;
  }

  if (!IsUserIdle(2500))
    /* don't hold back the UI thread while the user is interacting */
    return true;

  PeriodClock clock;
  clock.Update();

  bool still_dirty;
  bool topography_dirty = true; /* scan topography in every Idle() call */
  bool terrain_dirty = true;
  bool weather_dirty = true;

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
  } while (!clock.Check(700) && /* stop after 700ms */
#ifndef ENABLE_OPENGL
           !draw_thread->IsTriggered() &&
#endif
           IsUserIdle(2500) &&
           still_dirty);

  return still_dirty;
}
