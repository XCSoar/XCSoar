// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "DrawThread.hpp"
#ifdef ENABLE_OPENGL
#include "OverlayBitmap.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"
#endif
#include "Blackboard/DeviceBlackboard.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "time/PeriodClock.hpp"
#include "ui/event/Idle.hpp"
#include "Topography/Thread.hpp"
#include "Terrain/Thread.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

GlueMapWindow::GlueMapWindow(const Look &look) noexcept
  :MapWindow(look.map, look.traffic),
   thermal_band_renderer(look.thermal_band, look.chart),
   final_glide_bar_renderer(look.final_glide_bar, look.map.task),
   vario_bar_renderer(look.vario_bar),
   gesture_look(look.gesture)
{
}

GlueMapWindow::~GlueMapWindow() noexcept
{
  Destroy();
}

void
GlueMapWindow::SetTopography(TopographyStore *_topography) noexcept
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
GlueMapWindow::SetTerrain(RasterTerrain *_terrain) noexcept
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

#ifdef ENABLE_OPENGL

void
GlueMapWindow::LoadOverlays() noexcept
{
  std::vector<std::unique_ptr<MapOverlayBitmap>> overlays;

  for (const auto &path : Profile::GetMultiplePaths(ProfileKeys::OverlayFileList,
                                                    GetFileTypePatterns(FileType::TIFF))) {
    try {
      overlays.emplace_back(new MapOverlayBitmap(path));
    } catch (...) {
      LogError(std::current_exception(), "Failed to load map overlay");
    }
  }

  SetImageOverlays(std::move(overlays));
}

#endif

void
GlueMapWindow::SetMapSettings(const MapSettings &new_value) noexcept
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
GlueMapWindow::SetComputerSettings(const ComputerSettings &new_value) noexcept
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
GlueMapWindow::SetUIState(const UIState &new_value) noexcept
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
GlueMapWindow::ExchangeBlackboard() noexcept
{
  /* copy device_blackboard to MapWindow */

  {
    auto &device_blackboard = *backend_components->device_blackboard;
    const std::lock_guard lock{device_blackboard.mutex};
    ReadBlackboard(device_blackboard.Basic(),
                   device_blackboard.Calculated());
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
GlueMapWindow::SuspendThreads() noexcept
{
#ifndef ENABLE_OPENGL
  if (draw_thread != nullptr)
    draw_thread->Suspend();
#endif
}

void
GlueMapWindow::ResumeThreads() noexcept
{
#ifndef ENABLE_OPENGL
  if (draw_thread != nullptr)
    draw_thread->Resume();
#endif
}

void
GlueMapWindow::InjectRedraw() noexcept
{
#ifdef ENABLE_OPENGL
  /* async tile/topography loads may arrive after the idle upgrade
     timer has already fired (common in simulator startup) */
  terrain_quantisation_idle_done = false;
#endif

  redraw_notify.SendNotification();
}

void
GlueMapWindow::FullRedraw() noexcept
{
  UpdateDisplayMode();
  UpdateScreenAngle();
  UpdateProjection();
  UpdateMapScale();
  UpdateScreenBounds();

  DeferRedraw();

#ifdef ENABLE_OPENGL
  NoteTerrainQuantisationUserActivity();
#endif
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
GlueMapWindow::QuickRedraw() noexcept
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
#else
  NoteTerrainQuantisationUserActivity();
#endif
}

#ifdef ENABLE_OPENGL

/** Must match the steps in RasterRenderer::GetQuantisation(). */
static constexpr auto TERRAIN_QUANTISATION_IDLE_STEP =
  std::chrono::milliseconds(750);

void
GlueMapWindow::NoteTerrainQuantisationUserActivity() noexcept
{
  terrain_quantisation_idle_done = false;
  terrain_quantisation_timer.Schedule(TERRAIN_QUANTISATION_IDLE_STEP);
}

void
GlueMapWindow::PollTerrainQuantisationIdle() noexcept
{
  if (!IsUserIdle(750)) {
    terrain_quantisation_idle_done = false;
    return;
  }

  if (terrain_quantisation_timer.IsPending())
    return;

  if (IsUserIdle(1500)) {
    if (!terrain_quantisation_idle_done) {
      terrain_quantisation_idle_done = true;
      PartialRedraw();
    }
    return;
  }

  /* idle past the first threshold but the one-shot timer was missed
     (e.g. simulator startup before the map was shown) */
  terrain_quantisation_idle_done = false;
  PartialRedraw();
  terrain_quantisation_timer.Schedule(TERRAIN_QUANTISATION_IDLE_STEP);
}

void
GlueMapWindow::OnTerrainQuantisationTimer() noexcept
{
  PartialRedraw();

  if (!IsUserIdle(1500))
    terrain_quantisation_timer.Schedule(TERRAIN_QUANTISATION_IDLE_STEP);
  else
    terrain_quantisation_idle_done = true;
}

#endif
