/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_CACHED_TOPOGRAPHY_RENDERER_HPP
#define XCSOAR_CACHED_TOPOGRAPHY_RENDERER_HPP

#include "TopographyRenderer.hpp"
#include "Renderer/TransparentRendererCache.hpp"

/**
 * Class used to manage and render vector topography layers
 */
class CachedTopographyRenderer {
  TopographyRenderer renderer;

#ifndef ENABLE_OPENGL
  TransparentRendererCache cache;

  unsigned last_serial;
#endif

public:
  CachedTopographyRenderer(const TopographyStore &store,
                           const TopographyLook &look)
    :renderer(store, look)
#ifndef ENABLE_OPENGL
    , last_serial(0)
#endif
  {}

  void Flush() {
#ifndef ENABLE_OPENGL
    cache.Invalidate();
#endif
  }

#ifdef ENABLE_OPENGL
  void Draw(Canvas &canvas, const WindowProjection &projection) {
    renderer.Draw(canvas, projection);
  }
#else
  void Draw(Canvas &canvas, const WindowProjection &projection);
#endif

  void DrawLabels(Canvas &canvas, const WindowProjection &projection,
                  LabelBlock &label_block) const {
    renderer.DrawLabels(canvas, projection, label_block);
  }
};

#endif
