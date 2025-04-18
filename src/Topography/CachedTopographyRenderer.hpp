// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TopographyRenderer.hpp"
#include "Renderer/TransparentRendererCache.hpp"

/**
 * Class used to manage and render vector topography layers
 */
class CachedTopographyRenderer {
  TopographyRenderer renderer;

#ifndef ENABLE_OPENGL
  TransparentRendererCache cache;

  unsigned last_serial = 0;
#endif

public:
  CachedTopographyRenderer(const TopographyStore &store,
                           const TopographyLook &look) noexcept
    :renderer(store, look)
  {}

  void Flush() noexcept {
#ifndef ENABLE_OPENGL
    cache.Invalidate();
#endif
  }

#ifdef ENABLE_OPENGL
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept {
    renderer.Draw(canvas, projection);
  }
#else
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept;
#endif

  void DrawLabels(Canvas &canvas, const WindowProjection &projection,
                  LabelBlock &label_block) noexcept {
    renderer.DrawLabels(canvas, projection, label_block);
  }
};
