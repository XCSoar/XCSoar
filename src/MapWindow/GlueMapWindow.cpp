/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Topography/Thread.hpp"

GlueMapWindow::GlueMapWindow(const Look &look)
  :MapWindow(look.map, look.traffic),
   topography_thread(nullptr),
   logger(nullptr),
#ifdef ENABLE_OPENGL
   data_timer(*this),
#endif
   drag_mode(DRAG_NONE),
   ignore_single_click(false),
   skip_idle(true),
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
GlueMapWindow::SetTopography(TopographyStore *_topography)
{
  if (topography_thread != nullptr) {
    topography_thread->LockStop();
    delete topography_thread;
    topography_thread = nullptr;
  }

  MapWindow::SetTopography(_topography);

  if (_topography != nullptr)
    topography_thread =
      new TopographyThread(*_topography,
                           [this](){
                             SendUser(unsigned(Command::INVALIDATE));
                           });
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
  if (draw_thread != nullptr)
    draw_thread->Suspend();
#endif
}

void
GlueMapWindow::ResumeThreads()
{
#ifndef ENABLE_OPENGL
  if (draw_thread != nullptr)
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

  if (skip_idle) {
    /* draw the first frame as quickly as possible, so the user can
       start interacting with XCSoar immediately */
    skip_idle = false;
    return true;
  }

  /* hack: update RASP weather maps as quickly as possible; they only
     ever need to be updated after the user has selected a new map, so
     this is not a UI latency problem (quite contrary, don't let the
     user wait until he sees the new map) */
  UpdateWeather();

  if (!IsUserIdle(2500))
    /* don't hold back the UI thread while the user is interacting */
    return true;

  PeriodClock clock;
  clock.Update();

  bool still_dirty;

  do {
    still_dirty = UpdateWeather() || UpdateTerrain();
  } while (!clock.Check(700) && /* stop after 700ms */
#ifndef ENABLE_OPENGL
           !draw_thread->IsTriggered() &&
#endif
           IsUserIdle(2500) &&
           still_dirty);

  return still_dirty;
}

bool
GlueMapWindow::OnUser(unsigned id)
{
  switch (Command(id)) {
  case Command::INVALIDATE:
#ifdef ENABLE_OPENGL
    Invalidate();
#else
    draw_thread->TriggerRedraw();
#endif
    return true;

  default:
    return MapWindow::OnUser(id);
  }
}
