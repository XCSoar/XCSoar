/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
