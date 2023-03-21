// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "Components.hpp"
#include "DrawThread.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "time/PeriodClock.hpp"
#include "ui/event/Idle.hpp"
#include "Topography/Thread.hpp"
#include "Terrain/Thread.hpp"

GlueMapWindow::GlueMapWindow(const Look &look)
  :MapWindow(look.map, look.traffic),
   thermal_band_renderer(look.thermal_band, look.chart),
   final_glide_bar_renderer(look.final_glide_bar, look.map.task),
   vario_bar_renderer(look.vario_bar),
   gesture_look(look.gesture)
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
      new TopographyThread(*_topography, [this](){ InjectRedraw(); });
}

void
GlueMapWindow::SetTerrain(RasterTerrain *_terrain)
{
  if (terrain_thread != nullptr) {
    terrain_thread->LockStop();
    delete terrain_thread;
    terrain_thread = nullptr;
  }

  MapWindow::SetTerrain(_terrain);

  if (_terrain != nullptr)
    terrain_thread =
      new TerrainThread(*_terrain, [this](){ InjectRedraw(); });
}

void
GlueMapWindow::SetMapSettings(const MapSettings &new_value)
{
  AssertThreadOrUndefined();

#ifdef ENABLE_OPENGL
  ReadMapSettings(new_value);
#else
  const std::lock_guard lock{next_mutex};
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
  const std::lock_guard lock{next_mutex};
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
  const std::lock_guard lock{next_mutex};
  next_ui_state = new_value;
#endif
}

void
GlueMapWindow::ExchangeBlackboard()
{
  /* copy device_blackboard to MapWindow */

  {
    const std::lock_guard lock{device_blackboard->mutex};
    ReadBlackboard(device_blackboard->Basic(),
                   device_blackboard->Calculated());
  }

#ifndef ENABLE_OPENGL
  {
    const std::lock_guard lock{next_mutex};
    ReadMapSettings(next_settings_map);
    ReadComputerSettings(next_settings_computer);
    ReadUIState(next_ui_state);
  }
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
  UpdateScreenBounds();

  DeferRedraw();
}

void
GlueMapWindow::PartialRedraw() noexcept
{

#ifdef ENABLE_OPENGL
  Invalidate();
#else
  if (draw_thread != nullptr)
    draw_thread->TriggerRedraw();
#endif
}

void
GlueMapWindow::QuickRedraw()
{
  UpdateScreenAngle();
  UpdateProjection();
  UpdateMapScale();
  UpdateScreenBounds();

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
  DeferRedraw();
#endif
}
