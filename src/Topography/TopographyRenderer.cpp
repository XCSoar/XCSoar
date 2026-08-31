// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyRenderer.hpp"
#include "Topography/TopographyFileRenderer.hpp"
#include "TopographyStore.hpp"
#include "TopographyFile.hpp"
#include "Projection/WindowProjection.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <cmath>
#ifdef ENABLE_OPENGL
#include <chrono>
#endif

TopographyRenderer::TopographyRenderer(const TopographyStore &_store,
                                       const TopographyLook &look) noexcept
  :store(_store)
{
  auto previous = files.before_begin();
  for (const auto &file : store)
    previous = files.emplace_after(previous, file, look);
}

TopographyRenderer::~TopographyRenderer() noexcept = default;

void
TopographyRenderer::Draw(Canvas &canvas,
                         const WindowProjection &projection) noexcept
{
#ifdef ENABLE_OPENGL
  TopographyGpuStatsBeginDraw();
#endif
  for (auto &i : files)
    i.Paint(canvas, projection);
#ifdef ENABLE_OPENGL
  TopographyGpuStatsEndDraw(projection);
#endif

  const double map_scale = projection.GetMapScale();
  if (last_logged_map_scale > 0 &&
      std::fabs(map_scale - last_logged_map_scale) /
        std::max(map_scale, last_logged_map_scale) < 0.05)
    return;

  last_logged_map_scale = map_scale;

  unsigned n_layers = 0;
  std::size_t n_fills = 0, n_points = 0, n_labels = 0, n_cached = 0;
  for (const auto &i : files) {
    const auto &file = i.GetFile();
    const bool on = file.IsVisible(map_scale);
    const std::size_t fills = on ? i.GetVisibleShapeCount() : 0;
    const std::size_t points = on ? i.GetVisiblePointCount() : 0;
    const std::size_t labels = on ? i.GetVisibleLabelCount() : 0;
    const unsigned cached = file.GetCachedShapeCount();
    if (!on && cached == 0)
      continue;

    ++n_layers;
    n_fills += fills;
    n_points += points;
    n_labels += labels;
    n_cached += cached;

    LogFmt("Topography: scale={:.0f} m  {}  fills={}  points={}  "
           "labels={}  cached={}/{}",
           map_scale, file.GetName(), fills, points, labels,
           cached, file.GetFileShapeCount());
  }

  LogFmt("Topography: scale={:.0f} m  layers={}  fills={}  points={}  "
         "labels={}  cached={}",
         map_scale, n_layers, n_fills, n_points, n_labels, n_cached);
}

void
TopographyRenderer::DrawLabels(Canvas &canvas,
                               const WindowProjection &projection,
                               LabelBlock &label_block) noexcept
{
#ifdef ENABLE_OPENGL
  const auto t0 = std::chrono::steady_clock::now();
#endif
  for (auto &i : files)
    i.PaintLabels(canvas, projection, label_block);
#ifdef ENABLE_OPENGL
  const unsigned us = unsigned(
    std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - t0)
      .count());
  TopographyGpuStatsAddLabels(us);
#endif
}
