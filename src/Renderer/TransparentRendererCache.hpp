// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef ENABLE_OPENGL
#include "Projection/CompareProjection.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#endif

#include <cstdint>

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

  constexpr bool Check([[maybe_unused]] const WindowProjection &projection) const {
    return false;
  }

  constexpr Canvas &Begin(Canvas &canvas,
                          [[maybe_unused]] const WindowProjection &projection) const {
    return canvas;
  }

  void Commit([[maybe_unused]] Canvas &canvas, [[maybe_unused]] const WindowProjection &projection) {
  }

  void CopyAndTo([[maybe_unused]] Canvas &canvas) const {
  }

  void CopyTransparentWhiteTo([[maybe_unused]] Canvas &canvas) const {
  }

  void AlphaBlendTo([[maybe_unused]] Canvas &canvas, [[maybe_unused]] const WindowProjection &projection,
                    [[maybe_unused]] uint8_t alpha) const {
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
  [[gnu::pure]]
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
