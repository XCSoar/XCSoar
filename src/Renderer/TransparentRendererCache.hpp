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

#ifndef XCSOAR_CACHED_RENDERER_HPP
#define XCSOAR_CACHED_RENDERER_HPP

#ifndef ENABLE_OPENGL
#include "Projection/CompareProjection.hpp"
#include "Screen/BufferCanvas.hpp"
#endif

#include <stdint.h>

class Canvas;
class WindowProjection;

/**
 * Helper base class for implementing renderers that cache their
 * output.  If supported by the platform, the real class renders into
 * a texture that is slightly bigger than the screen, and reuses that
 * texture instead of rendering again.
 */
class TransparentRendererCache {
#ifdef ENABLE_OPENGL
  /* this class is a no-op on OpenGL, because OpenGL doesn't support
     color keying */
public:
  void Invalidate() {
  }

  constexpr bool Check(const WindowProjection &projection) const {
    return false;
  }

  constexpr Canvas &Begin(Canvas &canvas,
                          const WindowProjection &projection) const {
    return canvas;
  }

  void Commit(Canvas &canvas, const WindowProjection &projection) {
  }

  void CopyAndTo(Canvas &canvas) const {
  }

  void CopyTransparentWhiteTo(Canvas &canvas) const {
  }

  void AlphaBlendTo(Canvas &canvas, const WindowProjection &projection,
                    uint8_t alpha) const {
  }
#else
  CompareProjection compare_projection;
  BufferCanvas buffer;
  bool empty;

public:

  /**
   * Finish drawing the cache. Indicate that no drawing is necessary
   * since nothing relevant was rendered.
   */
  inline void CommitEmpty() {
    empty = true;
  }

  void Invalidate() {
    compare_projection.Clear();
    CommitEmpty();
  }

  /**
   * Check if the cache can be used.
   *
   * @return true if the cache is valid for the given projection; the
   * caller may skip to CopyTo()
   */
  gcc_pure
  bool Check(const WindowProjection &projection) const;

  /**
   * Begin drawing to the cache.  Render to the returned Canvas.  Call
   * Commit() when you're done.
   */
  Canvas &Begin(Canvas &canvas, const WindowProjection &projection);

  /**
   * Finish drawing to the cache.  Call CopyTo().
   */
  void Commit(Canvas &canvas, const WindowProjection &projection);

  void CopyAndTo(Canvas &canvas,
                 const WindowProjection &projection) const;

  void CopyTransparentWhiteTo(Canvas &canvas,
                              const WindowProjection &projection) const;

#ifdef HAVE_ALPHA_BLEND
  /**
   * Copy the cache to the given Canvas.
   */
  void AlphaBlendTo(Canvas &canvas, const WindowProjection &projection,
                    uint8_t alpha) const;
#endif
#endif
};

#endif
